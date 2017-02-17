/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "WGraphicBufferSource.h"
#include "Conversion.h"
#include "WOmxNode.h"
#include <stagefright/foundation/ColorUtils.h>

namespace android {
namespace hardware {
namespace media {
namespace omx {
namespace V1_0 {
namespace utils {

using android::ColorUtils;

// LWGraphicBufferSource
LWGraphicBufferSource::LWGraphicBufferSource(
        sp<TGraphicBufferSource> const& base) : mBase(base) {
}

::android::binder::Status LWGraphicBufferSource::configure(
        const sp<IOMXNode>& omxNode, int32_t dataSpace) {
    return toBinderStatus(mBase->configure(
            new TWOmxNode(omxNode), toHardwareDataspace(dataSpace)));
}

::android::binder::Status LWGraphicBufferSource::setSuspend(bool suspend) {
    return toBinderStatus(mBase->setSuspend(suspend));
}

::android::binder::Status LWGraphicBufferSource::setRepeatPreviousFrameDelayUs(
        int64_t repeatAfterUs) {
    return toBinderStatus(mBase->setRepeatPreviousFrameDelayUs(repeatAfterUs));
}

::android::binder::Status LWGraphicBufferSource::setMaxFps(float maxFps) {
    return toBinderStatus(mBase->setMaxFps(maxFps));
}

::android::binder::Status LWGraphicBufferSource::setTimeLapseConfig(
        int64_t timePerFrameUs, int64_t timePerCaptureUs) {
    return toBinderStatus(mBase->setTimeLapseConfig(
            timePerFrameUs, timePerCaptureUs));
}

::android::binder::Status LWGraphicBufferSource::setStartTimeUs(
        int64_t startTimeUs) {
    return toBinderStatus(mBase->setStartTimeUs(startTimeUs));
}

::android::binder::Status LWGraphicBufferSource::setColorAspects(
        int32_t aspects) {
    return toBinderStatus(mBase->setColorAspects(
            toHardwareColorAspects(aspects)));
}

::android::binder::Status LWGraphicBufferSource::setTimeOffsetUs(
        int64_t timeOffsetsUs) {
    return toBinderStatus(mBase->setTimeOffsetUs(timeOffsetsUs));
}

::android::binder::Status LWGraphicBufferSource::signalEndOfInputStream() {
    return toBinderStatus(mBase->signalEndOfInputStream());
}

// TWGraphicBufferSource
TWGraphicBufferSource::TWGraphicBufferSource(
        sp<LGraphicBufferSource> const& base) : mBase(base) {
}

Return<void> TWGraphicBufferSource::configure(
        const sp<IOmxNode>& omxNode, Dataspace dataspace) {
    mBase->configure(new LWOmxNode(omxNode), toRawDataspace(dataspace));
    return Void();
}

Return<void> TWGraphicBufferSource::setSuspend(bool suspend) {
    mBase->setSuspend(suspend);
    return Void();
}

Return<void> TWGraphicBufferSource::setRepeatPreviousFrameDelayUs(
        int64_t repeatAfterUs) {
    mBase->setRepeatPreviousFrameDelayUs(repeatAfterUs);
    return Void();
}

Return<void> TWGraphicBufferSource::setMaxFps(float maxFps) {
    mBase->setMaxFps(maxFps);
    return Void();
}

Return<void> TWGraphicBufferSource::setTimeLapseConfig(
        int64_t timePerFrameUs, int64_t timePerCaptureUs) {
    mBase->setTimeLapseConfig(timePerFrameUs, timePerCaptureUs);
    return Void();
}

Return<void> TWGraphicBufferSource::setStartTimeUs(int64_t startTimeUs) {
    mBase->setStartTimeUs(startTimeUs);
    return Void();
}

Return<void> TWGraphicBufferSource::setColorAspects(
        const ColorAspects& aspects) {
    mBase->setColorAspects(toCompactColorAspects(aspects));
    return Void();
}

Return<void> TWGraphicBufferSource::setTimeOffsetUs(int64_t timeOffsetUs) {
    mBase->setTimeOffsetUs(timeOffsetUs);
    return Void();
}

Return<void> TWGraphicBufferSource::signalEndOfInputStream() {
    mBase->signalEndOfInputStream();
    return Void();
}

}  // namespace utils
}  // namespace V1_0
}  // namespace omx
}  // namespace media
}  // namespace hardware
}  // namespace android
