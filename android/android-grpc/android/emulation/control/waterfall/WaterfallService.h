
// Copyright (C) 2018 The Android Open Source Project
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
#ifdef _MSC_VER
namespace waterfall {
namespace Waterfall {
class Service {};
}  // namespace Waterfall
}  // namespace waterfall
#else
#include "waterfall.grpc.pb.h"
#endif

namespace android {
namespace emulation {
namespace control {

enum class WaterfallProvider { none, adb, forward };

waterfall::Waterfall::Service* getAdbWaterfallService();
waterfall::Waterfall::Service* getWaterfallService();
waterfall::Waterfall::Service* getWaterfallService(WaterfallProvider variant);

}  // namespace control
}  // namespace emulation
}  // namespace android
