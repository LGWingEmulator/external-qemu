// Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include "android/utils/compiler.h"
#include "android/utils/looper.h"

ANDROID_BEGIN_HEADER

typedef void (*on_post_callback_t)(void*, int, int, const void*);

// Initialize state to ensure that new GPU frame data is passed to the caller
// in the appropriate thread. |looper| is a Looper instance, |context| is an
// opaque handle passed to |callback| at runtime, which is a function called
// from the looper's thread whenever a new frame is available.
void gpu_frame_set_post_callback(Looper* looper,
                                 void* context,
                                 on_post_callback_t callback);

// Recording mode can only be enabled in host gpu mode. Any other configuration
// will not work. Turning record mode on will initialize the gpu frame state for
// recording, and turning off will detach and deallocate resources that were
// being used, if any. Returns false if gpu mode is not supported, true
// otherwise.
bool gpu_frame_set_record_mode(bool on);

// Use in recording mode only. Make sure to turn on recording mode first with
// gpu_frame_set_record_mode() before using this. May return NULL if no data is
// available.
void* gpu_frame_get_record_frame();

typedef void (*FrameAvailableCallback)(void* opaque);

// Used by the VideoFrameSharer to obtain a new frame when one is available.
// Do not do any expensive calculations on this callback, it should be as fast
// as possible.
void gpu_register_shared_memory_callback(FrameAvailableCallback frameAvailable,
                                         void* opaque);
void gpu_unregister_shared_memory_callback(void* opaque);

ANDROID_END_HEADER
