// Copyright 2015-2017 The Android Open Source Project
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

#include "android/emulation/control/window_agent.h"

#include "android/emulator-window.h"
#include "android/skin/qt/emulator-no-qt-no-window.h"
#include "android/utils/debug.h"

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = emulator_window_get,
        .rotate90Clockwise = [] { return emulator_window_rotate_90(true); },
        .rotate = emulator_window_rotate,
        .getRotation =
                [] {
                    EmulatorWindow* window = emulator_window_get();
                    if (!window)
                        return SKIN_ROTATION_0;
                    SkinLayout* layout = emulator_window_get_layout(window);
                    if (!layout)
                        return SKIN_ROTATION_0;
                    return layout->orientation;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    const auto printer =
                            (type == WINDOW_MESSAGE_ERROR)
                                    ? &derror
                                    : (type == WINDOW_MESSAGE_WARNING)
                                              ? &dwarning
                                              : &dprint;
                    printer("%s", message);
                },
        .showMessageWithDismissCallback =
                [](const char* message,
                   WindowMessageType type,
                   const char* dismissText,
                   void* context,
                   void (*func)(void*),
                   int timeoutMs) {
                    const auto printer =
                            (type == WINDOW_MESSAGE_ERROR)
                                    ? &derror
                                    : (type == WINDOW_MESSAGE_WARNING)
                                              ? &dwarning
                                              : &dprint;
                    printer("%s", message);
                    // Don't necessarily perform the func since the
                    // user doesn't get a chance to dismiss.
                },
        .fold =
                [](bool is_fold) {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        if (is_fold)
                            win->fold();
                        else
                            win->unFold();
                        return false;
                    }
                    return true;
                },
        .isFolded =
                [] {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        return win->isFolded();
                    }
                    return false;
                },
        .swivel =
                [](bool is_swivel) {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        if (is_swivel)
                            win->swivel();
                        else
                            win->unSwivel();
                        return false;
                    }
                    return true;
                },
        .isSwiveled =
                [] {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        return win->isSwiveled();
                    }
                    return false;
                },
        .dual=
                [](bool is_dual) {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        if (is_dual)
                            win->dual();
                        else
                            win->unDual();
                        return false;
                    }
                    return true;
                },
        .isDualed=
                [] {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        return win->isDualed();
                    }
                    return false;
                },
        /***************/
        .setUIDisplayRegion = [](int x, int y, int w, int h) {},
        .setUIMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              bool add,
                              uint32_t dpi = 0) {},
        .getMultiDisplay = [](uint32_t id,
                              int32_t* x,
                              int32_t* y,
                              uint32_t* w,
                              uint32_t* h,
                              uint32_t* dpi,
                              uint32_t* flag,
                              bool* enabled) { return false; },
        .getDualSize = [](int* w, int* h, int* s_w, int* s_h,int* gap, int* rot) {return true; },
	.getOrientation = []() { return 0;},
        .getMonitorRect = [](uint32_t* w, uint32_t* h) { return false; },
        .setNoSkin = []() {},
        .restoreSkin = []() {},
        .switchDual = [](int32_t opt0) {return true; },
        .switchOption = [](int32_t opt0,
                                 int32_t opt1,
                                 int32_t opt2,
                                 int32_t opt3,
                                 int32_t opt4,
                                 int32_t opt5,
                                 int32_t opt6,
                                 int32_t opt7,
                                 int32_t opt8,
                                 int32_t opt9) { return true; },
        .switchMultiDisplay = [](bool add,
                                 uint32_t id,
                                 int32_t x,
                                 int32_t y,
                                 uint32_t w,
                                 uint32_t h,
                                 uint32_t dpi,
                                 uint32_t flag)->bool { return true; },
        .updateUIMultiDisplayPage = [](uint32_t id) { },
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
