// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android-qemu2-glue/qemu-setup.h"

// We need to include this first due to msvc-posix defining some magical macros
#ifdef ANDROID_WEBRTC
#include "android/emulation/control/WebRtcBridge.h"  // for WebRtcBridge
#endif

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>

#include "android-qemu2-glue/android_qemud.h"
#include "android-qemu2-glue/audio-capturer.h"
#include "android-qemu2-glue/audio-output.h"
#include "android-qemu2-glue/base/async/CpuLooper.h"
#include "android-qemu2-glue/base/async/Looper.h"
#include "android-qemu2-glue/emulation/DmaMap.h"
#include "android-qemu2-glue/emulation/VmLock.h"
#include "android-qemu2-glue/emulation/android_address_space_device.h"
#include "android-qemu2-glue/emulation/android_pipe_device.h"
#include "android-qemu2-glue/emulation/charpipe.h"
#include "android-qemu2-glue/emulation/goldfish_sync.h"
#include "android-qemu2-glue/looper-qemu.h"
#include "android-qemu2-glue/net-android.h"
#include "android-qemu2-glue/proxy/slirp_proxy.h"
#include "android-qemu2-glue/qemu-control-impl.h"
#include "android-qemu2-glue/snapshot_compression.h"
#include "android/android.h"
#include "android/avd/info.h"
#include "android/base/CpuUsage.h"
#include "android/base/Log.h"
#include "android/base/files/MemStream.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/HangDetector.h"
#include "android/crashreport/crash-handler.h"
#include "android/crashreport/detectors/CrashDetectors.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/emulation/AudioOutputEngine.h"
#include "android/emulation/DmaMap.h"
#include "android/emulation/QemuMiscPipe.h"
#include "android/emulation/VmLock.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/control/EmulatorAdvertisement.h"
#include "android/emulation/control/EmulatorService.h"
#include "android/emulation/control/GrpcServices.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/globals.h"
#include "android/skin/LibuiAgent.h"
#include "android/skin/winsys.h"
#include "android/snapshot/interface.h"
#include "android/utils/debug.h"

extern "C" {

#include "qemu/abort.h"
#include "qemu/main-loop.h"
#include "qemu/osdep.h"
#include "qemu/thread.h"
#include "sysemu/device_tree.h"
#include "sysemu/sysemu.h"

namespace android {
namespace emulation {
namespace control {
class RtcBridge;
}  // namespace control
}  // namespace emulation
}  // namespace android

extern char* android_op_ports;

// TODO: Remove op_http_proxy global variable.
extern char* op_http_proxy;

}  // extern "C"

using android::DmaMap;
using android::VmLock;
using android::emulation::control::EmulatorAdvertisement;
using android::emulation::control::EmulatorProperties;

extern "C" void ranchu_device_tree_setup(void* fdt) {
    /* fstab */
    qemu_fdt_add_subnode(fdt, "/firmware/android/fstab");
    qemu_fdt_setprop_string(fdt, "/firmware/android/fstab", "compatible",
                            "android,fstab");

    char* system_path =
            avdInfo_getSystemImageDevicePathInGuest(android_avdInfo);
    if (system_path) {
        qemu_fdt_add_subnode(fdt, "/firmware/android/fstab/system");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system",
                                "compatible", "android,system");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "dev",
                                system_path);
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system",
                                "fsmgr_flags", "wait");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system",
                                "mnt_flags", "ro");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/system", "type",
                                "ext4");
        free(system_path);
    }

    char* vendor_path =
            avdInfo_getVendorImageDevicePathInGuest(android_avdInfo);
    if (vendor_path) {
        qemu_fdt_add_subnode(fdt, "/firmware/android/fstab/vendor");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor",
                                "compatible", "android,vendor");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "dev",
                                vendor_path);
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor",
                                "fsmgr_flags", "wait");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor",
                                "mnt_flags", "ro");
        qemu_fdt_setprop_string(fdt, "/firmware/android/fstab/vendor", "type",
                                "ext4");
        free(vendor_path);
    }
}

extern "C" void rng_random_generic_read_random_bytes(void* buf, int size) {
    if (size <= 0)
        return;

    android::base::MemStream stream;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(0, 255);  // fill a byte

    for (int i = 0; i < size; ++i) {
        stream.putByte(uniform_dist(gen));
    }
    stream.read(buf, size);
}

bool qemu_android_emulation_early_setup() {
    // Ensure that the looper is set for the main thread and for any
    // future thread created by QEMU.
    qemu_looper_setForThread();
    qemu_thread_register_setup_callback(qemu_looper_setForThread);

    qemu_abort_set_handler((QemuAbortHandler)&crashhandler_die_format_v);
    qemu_crash_dump_message_func_set(
            (QemuCrashDumpMessageFunc)&crashhandler_append_message_format_v);

    // Make sure we override the ctrl-C handler as soon as possible.
    qemu_set_ctrlc_handler(&skin_winsys_quit_request);

    // Ensure charpipes i/o are handled properly.
    main_loop_register_poll_callback(qemu_charpipe_poll);

    if (android_qemu_mode) {
        // Register qemud-related snapshot callbacks.
        android_qemu2_qemud_init();
    }

    // Ensure the VmLock implementation is setup.
    VmLock* vmLock = new qemu2::VmLock();
    VmLock* prevVmLock = VmLock::set(vmLock);
    CHECK(prevVmLock == nullptr) << "Another VmLock was already installed!";

    // Ensure the DmaMap implementation is setup.
    DmaMap* dmaMap = new qemu2::DmaMap();
    DmaMap* prevDmaMap = DmaMap::set(dmaMap);
    CHECK(prevDmaMap == nullptr) << "Another DmaMap was already installed!";

    // Initialize host pipe service.
    if (!qemu_android_pipe_init(vmLock)) {
        return false;
    }

    // Initialize host sync service.
    if (!qemu_android_sync_init(vmLock)) {
        return false;
    }

    android::emulation::goldfish_address_space_set_vm_operations(
            gQAndroidVmOperations);

    qemu_set_address_space_device_control_ops(
            (struct qemu_address_space_device_control_ops*)
                    get_address_space_device_control_ops());

    qemu_android_address_space_device_init();

    androidSnapshot_initialize(gQAndroidVmOperations,
                               gQAndroidEmulatorWindowAgent);
    qemu_snapshot_compression_setup();

    android::emulation::AudioCaptureEngine::set(
            new android::qemu::QemuAudioCaptureEngine());

    android::emulation::AudioOutputEngine::set(
            new android::qemu::QemuAudioOutputEngine());
    return true;
}

static std::unique_ptr<EmulatorAdvertisement> advertiser;

const AndroidConsoleAgents* getConsoleAgents() {
    // Initialize UI/console agents.
    static const AndroidConsoleAgents consoleAgents = {
            gQAndroidBatteryAgent,
            gQAndroidDisplayAgent,
            gQAndroidEmulatorWindowAgent,
            gQAndroidFingerAgent,
            gQAndroidLocationAgent,
            gQAndroidHttpProxyAgent,
            gQAndroidRecordScreenAgent,
            gQAndroidTelephonyAgent,
            gQAndroidUserEventAgent,
            gQAndroidVmOperations,
            gQAndroidNetAgent,
            gQAndroidLibuiAgent,
            gQCarDataAgent,
            gQGrpcAgent,
            gQAndroidSensorsAgent};
    return &consoleAgents;
}

bool qemu_android_ports_setup() {
    return android_ports_setup(getConsoleAgents(), true);
}

static android::emulation::control::RtcBridge* qemu_setup_rtc_bridge() {
#ifdef ANDROID_WEBRTC
    std::string turn;
    if (android_cmdLineOptions->turncfg) {
        turn = android_cmdLineOptions->turncfg;
    }
    return android::emulation::control::WebRtcBridge::create(
            0, getConsoleAgents(), turn);
#else
    return new android::emulation::control::NopRtcBridge();
#endif
}

static void qemu_setup_grpc() {
    EmulatorProperties props{
            {"port.serial", std::to_string(android_serial_number_port)},
            {"port.adb", std::to_string(android_adb_port)},
            {"avd.name", avdInfo_getName(android_avdInfo)},
            {"avd.id", avdInfo_getId(android_avdInfo)}};

    int grpc_start = android_serial_number_port + 3000;
    int grpc_end = grpc_start + 1000;
    std::string address = "127.0.0.1";

    android::emulation::control::RtcBridge* bridge;
    if (android_cmdLineOptions->grpc &&
        sscanf(android_cmdLineOptions->grpc, "%d", &grpc_start) == 1) {
        grpc_end = grpc_start + 1;
        address = "0.0.0.0";
        bridge = qemu_setup_rtc_bridge();
    } else {
        bridge = new android::emulation::control::NopRtcBridge();
    }

    auto service = android::emulation::control::GrpcServices::setup(
            grpc_start, grpc_end, address, getConsoleAgents(), bridge,
            android_cmdLineOptions->waterfall);

    if (service) {
        props["grpc.port"] = std::to_string(service->port());
        props["grpc.certificate"] = service->publicCert();
    }

    advertiser = std::make_unique<EmulatorAdvertisement>(std::move(props));
    advertiser->garbageCollect();
    advertiser->write();
}

bool qemu_android_emulation_setup() {
    android_qemu_init_slirp_shapers();

    if (!qemu_android_setup_http_proxy(op_http_proxy)) {
        return false;
    }

    if (!android_emulation_setup(getConsoleAgents(), true)) {
        return false;
    }

    // Only proceed if flag is there
    int grpc;
    if (android_cmdLineOptions->grpc &&
        sscanf(android_cmdLineOptions->grpc, "%d", &grpc) == 1) {
        qemu_setup_grpc();
    }

    // We are sharing video, time to launch the shared memory recorder.
    // Note, the webrtc module could have started the shared memory module
    // with a differrent fps suggestion. (Fps is mainly used by clients as a
    // suggestion on how often to check for new frames)
    if (android_cmdLineOptions->share_vid) {
        if (!getConsoleAgents()->record->startSharedMemoryModule(60)) {
            dwarning(
                    "Unable to setup a shared memory handler, last errno: "
                    "%d",
                    errno);
        }
    }

    if (!android_qemu_mode)
        return true;

    android::base::ScopedCPtr<const char> arch(
            avdInfo_getTargetCpuArch(android_avdInfo));
    const bool isX86 =
            arch && (strstr(arch.get(), "x86") || strstr(arch.get(), "i386"));
    const int nCores = isX86 ? android_hw->hw_cpu_ncore : 1;
    for (int i = 0; i < nCores; ++i) {
        auto cpuLooper = new android::qemu::CpuLooper(i);
        android::crashreport::CrashReporter::get()
                ->hangDetector()
                .addWatchedLooper(cpuLooper);
        android::base::CpuUsage::get()->addLooper(
                (int)android::base::CpuUsage::UsageArea::Vcpu + i, cpuLooper);
    }

    if (android_cmdLineOptions && android_cmdLineOptions->detect_image_hang) {
        dwarning("Enabled event overflow detection.\n");
        const int kMaxEventsDropped = 1024;
        android::crashreport::CrashReporter::get()
                ->hangDetector()
                .addPredicateCheck(
                        new android::crashreport::TimedHangDetector(
                                60 * 1000,
                                new android::crashreport::HeartBeatDetector(
                                        get_guest_heart_beat_count,
                                        &guest_boot_completed)),
                        "The guest is not sending heartbeat update, "
                        "probably "
                        "stalled.")
                .addPredicateCheck(
                        [] {
                            return gQAndroidUserEventAgent->eventsDropped() >
                                   kMaxEventsDropped;
                        },
                        "The goldfish event queue is overflowing, the "
                        "system "
                        "is no longer responding.");
    }

    return true;
}

void qemu_android_emulation_teardown() {
    android::qemu::skipTimerOps();
    androidSnapshot_finalize();
    android_emulation_teardown();
    android::emulation::control::GrpcServices::teardown();
    if (advertiser)
        advertiser->remove();
}
