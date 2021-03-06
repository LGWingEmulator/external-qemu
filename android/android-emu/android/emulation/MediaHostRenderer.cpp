// Copyright (C) 2020 The Android Open Source Project
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

#include "android/emulation/MediaHostRenderer.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt, ...)                                         \
    fprintf(stderr, "media-host-renderer: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

MediaHostRenderer::MediaHostRenderer() {
    mVirtioGpuOps = android_getVirtioGpuOps();
    if (mVirtioGpuOps == nullptr) {
        H264_DPRINT("Error, cannot get mVirtioGpuOps");
    }
}

MediaHostRenderer::~MediaHostRenderer() {
    cleanUpTextures();
}

const uint32_t kGlUnsignedByte = 0x1401;

constexpr uint32_t kGL_RGBA8 = 0x8058;
constexpr uint32_t kGL_RGBA = 0x1908;
constexpr uint32_t kFRAME_POOL_SIZE = 8;
constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;

MediaHostRenderer::TextureFrame MediaHostRenderer::getTextureFrame(int w,
                                                                   int h) {
    H264_DPRINT("calling %s %d", __func__, __LINE__);
    if (mFramePool.empty()) {
        std::vector<uint32_t> textures(2 * kFRAME_POOL_SIZE);
        mVirtioGpuOps->create_yuv_textures(kFRAMEWORK_FORMAT_NV12,
                                           kFRAME_POOL_SIZE, w, h,
                                           textures.data());
        for (uint32_t i = 0; i < kFRAME_POOL_SIZE; ++i) {
            TextureFrame frame{textures[2 * i], textures[2 * i + 1]};
            H264_DPRINT("allocated Y %d UV %d", frame.Ytex, frame.UVtex);
            mFramePool.push_back(std::move(frame));
        }
    }
    TextureFrame frame = mFramePool.front();
    mFramePool.pop_front();
    H264_DPRINT("done %s %d ret Y %d UV %d", __func__, __LINE__, frame.Ytex,
                frame.UVtex);
    return frame;
}

void MediaHostRenderer::saveDecodedFrameToTexture(TextureFrame frame,
                                                  void* privData,
                                                  void* func) {
    if (mVirtioGpuOps) {
        uint32_t textures[2] = {frame.Ytex, frame.UVtex};
        mVirtioGpuOps->update_yuv_textures(kFRAMEWORK_FORMAT_NV12, textures,
                                           privData, func);
    }
}

void MediaHostRenderer::cleanUpTextures() {
    if (mFramePool.empty()) {
        return;
    }
    std::vector<uint32_t> textures;
    for (auto& frame : mFramePool) {
        textures.push_back(frame.Ytex);
        textures.push_back(frame.UVtex);
        H264_DPRINT("delete Y %d UV %d", frame.Ytex, frame.UVtex);
    }
    mVirtioGpuOps->destroy_yuv_textures(kFRAMEWORK_FORMAT_NV12,
                                        mFramePool.size(), textures.data());
    mFramePool.clear();
}

void MediaHostRenderer::renderToHostColorBuffer(int hostColorBufferId,
                                                unsigned int outputWidth,
                                                unsigned int outputHeight,
                                                uint8_t* decodedFrame) {
    H264_DPRINT("Calling %s at %d buffer id %d", __func__, __LINE__,
                hostColorBufferId);
    if (hostColorBufferId < 0) {
        H264_DPRINT("ERROR: negative buffer id %d", hostColorBufferId);
        return;
    }
    if (mVirtioGpuOps) {
        mVirtioGpuOps->update_color_buffer(hostColorBufferId, 0, 0, outputWidth,
                                           outputHeight, kGL_RGBA,
                                           kGlUnsignedByte, decodedFrame);
    } else {
        H264_DPRINT("ERROR: there is no virtio Gpu Ops is not setup");
    }
}

void MediaHostRenderer::renderToHostColorBufferWithTextures(
        int hostColorBufferId,
        unsigned int outputWidth,
        unsigned int outputHeight,
        TextureFrame frame) {
    H264_DPRINT("Calling %s at %d buffer id %d", __func__, __LINE__,
                hostColorBufferId);
    if (hostColorBufferId < 0) {
        H264_DPRINT("ERROR: negative buffer id %d", hostColorBufferId);
        return;
    }
    if (mVirtioGpuOps) {
        uint32_t textures[2] = {frame.Ytex, frame.UVtex};
        mVirtioGpuOps->swap_textures_and_update_color_buffer(
                hostColorBufferId, 0, 0, outputWidth, outputHeight, kGL_RGBA,
                kGlUnsignedByte, kFRAMEWORK_FORMAT_NV12, textures);
        if (textures[0] > 0 && textures[1] > 0) {
            frame.Ytex = textures[0];
            frame.UVtex = textures[1];
            putTextureFrame(frame);
        }
    } else {
        H264_DPRINT("ERROR: there is no virtio Gpu Ops is not setup");
    }
}

}  // namespace emulation
}  // namespace android
