prebuilt(WEBRTC)
android_add_executable(
  TARGET goldfish-webrtc-bridge
  LICENSE Apache-2.0 INSTALL .
  SRC # cmake-format: sortable
      emulator/main/GoldfishBridge.cpp
      emulator/net/EmulatorConnection.cpp
      emulator/net/RtcAsyncSocketAdapter.cpp
      emulator/webrtc/Participant.cpp
      emulator/webrtc/Switchboard.cpp
      emulator/webrtc/capture/VideoShareCapture.cpp
      emulator/webrtc/capture/VideoShareFactory.cpp
      emulator/webrtc/capture/VideoShareInfo.cpp)

android_target_compile_options(goldfish-webrtc-bridge darwin-x86_64
                               PRIVATE "-fvisibility=hidden")
android_target_compile_definitions(goldfish-webrtc-bridge linux-x86_64 PRIVATE
                                   "-DWEBRTC_POSIX")
android_target_compile_definitions(goldfish-webrtc-bridge windows_msvc-x86_64
                                   PRIVATE "-DWEBRTC_WIN")
android_target_compile_definitions(
  goldfish-webrtc-bridge darwin-x86_64 PRIVATE "-DWEBRTC_MAC" "-DWEBRTC_POSIX"
  "-D_GLIBCXX_USE_CXX11_ABI=0")

# WARNING! We are linking against a large webrtc prebuilt. This archive has
# everything needed for webrtc support This includes SSL, LIBYUV, etc.. Be very
# careful linking any of the android emulator libraries against it
target_link_libraries(
  goldfish-webrtc-bridge PRIVATE WebRTC::WebRTC Threads::Threads android-net
                                 nlohmann_json::nlohmann_json android-emu-base)
target_include_directories(goldfish-webrtc-bridge PRIVATE .)

# The linker dependencies, mainly needed to keep the encoders/decoders happy..
android_target_link_libraries(goldfish-webrtc-bridge linux-x86_64
                              PRIVATE dl X11 rt)
android_target_link_libraries(
  goldfish-webrtc-bridge darwin-x86_64
  PRIVATE "-framework AudioToolbox"
          "-framework CoreAudio"
          "-framework CoreFoundation"
          "-framework CoreGraphics"
          "-framework Foundation"
          "-framework AVFoundation"
          "-framework CoreVideo"
          "-framework CoreMedia")
