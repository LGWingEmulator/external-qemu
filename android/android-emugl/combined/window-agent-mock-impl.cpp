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

#include <stdio.h>
#include "android/emulation/control/window_agent.h"

static bool sIsFolded = false;
static bool sIsSwiveled = false;
static bool sIsDualed = false;

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow =
                [](void) {
                    printf("window-agent-mock-impl: .getEmulatorWindow\n");
                    return (EmulatorWindow*)nullptr;
                },
        .rotate90Clockwise =
                [](void) {
                    printf("window-agent-mock-impl: .rotate90Clockwise\n");
                    return true;
                },
        .rotate =
                [](SkinRotation rotation) {
                    printf("window-agent-mock-impl: .rotate90Clockwise\n");
                    return true;
                },
        .getRotation =
                [](void) {
                    printf("window-agent-mock-impl: .getRotation\n");
                    return SKIN_ROTATION_0;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    printf("window-agent-mock-impl: .showMessage %s\n", message);
                },
        .showMessageWithDismissCallback =
                [](const char* message, WindowMessageType type,
                   const char* dismissText, void* context,
                   void (*func)(void*), int timeoutMs) {
                    printf("window-agent-mock-impl: .showMessageWithDismissCallback %s\n", message);
                },
        .fold =
                [](bool is_fold) {
                    printf("window-agent-mock-impl: .fold %d\n", is_fold);
                    sIsFolded = is_fold;
                    return true;
                },
        .isFolded =
                [](void) -> bool {
                    printf("window-agent-mock-impl: .isFolded ? %d\n", sIsFolded);
                    return sIsFolded;
                },
        .swivel=
                [](bool is_swivel) {
                    printf("window-agent-mock-impl: .swivel %d\n", is_swivel);
                    sIsSwiveled = is_swivel;
                    return true;
                },
        .isSwiveled =
                [](void) -> bool {
                    printf("window-agent-mock-impl: .isSwiveled ? %d\n", sIsSwiveled);
                    return sIsSwiveled;
                },
        .dual=
                [](bool is_dual) {
                    printf("window-agent-mock-impl: .dual %d\n", is_dual);
                    sIsDualed = is_dual;
                    return true;
                },
        .isDualed =
                [](void) -> bool {
                    printf("window-agent-mock-impl: .isDualed ? %d\n", sIsDualed);
                    return sIsDualed;
                },
        /**********/
        .setUIDisplayRegion =
                [](int x_offset, int y_offset, int w, int h) {
                    printf("window-agent-mock-impl: .setUIDisplayRegion %d %d %dx%d\n",
                           x_offset, y_offset, w, h);
                },
        .setUIMultiDisplay =
                [](uint32_t id, int32_t x, int32_t y, uint32_t w, uint32_t h, bool add, uint32_t dpi = 0) {
                    printf("window-agent-mock-impl: .setUIMultiDisplay id %d %d %d %dx%d %s\n",
                           id, x, y, w, h, add ? "add" : "del");
                },
        .getMultiDisplay = 0,
        .getDualSize =
                [](int* w,int* h,int* s_w,int* s_h,int* gap, int* rot) { return true; },
	.getOrientation = 
		[] (void) { return 0; },
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) {
                    if (w) *w = 2500;
                    if (h) *h = 1600;
                    return true;
                },
        .setNoSkin = [](void){
                },
        .restoreSkin = [](void){
                },
        .switchMultiDisplay =
                [](bool add, uint32_t id, int32_t x, int32_t y, uint32_t w, uint32_t h,
                   uint32_t dpi, uint32_t flag)->bool {
                    printf("window-agent-mock-impl: .switchMultiDisplay id %d %d %d %dx%d "
                           "dpi %d flag %d %s\n",
                           id, x, y, w, h, dpi, flag, add ? "add" : "del");
                },
        .updateUIMultiDisplayPage =
                [](uint32_t id) {
                    printf("updateMultiDisplayPage\n");
                }
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
