/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "SpatializerPoseController.h"

#define LOG_TAG "VirtualizerStageController"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <utils/SystemClock.h>

namespace android {

using media::createHeadTrackingProcessor;
using media::HeadTrackingMode;
using media::HeadTrackingProcessor;
using media::Pose3f;
using media::SensorPoseProvider;
using media::Twist3f;

using namespace std::chrono_literals;

namespace {

// This is how fast, in m/s, we allow position to shift during rate-limiting.
constexpr auto kMaxTranslationalVelocity = 2 ;

// This is how fast, in rad/s, we allow rotation angle to shift during rate-limiting.
constexpr auto kMaxRotationalVelocity = 4 * M_PI ;

// This should be set to the typical time scale that the translation sensors used drift in. This
// means, loosely, for how long we can trust the reading to be "accurate enough". This would
// determine the time constants used for high-pass filtering those readings. If the value is set
// too high, we may experience drift. If it is set too low, we may experience poses tending toward
// identity too fast.
constexpr auto kTranslationalDriftTimeConstant = 20s;

// This should be set to the typical time scale that the rotation sensors used drift in. This
// means, loosely, for how long we can trust the reading to be "accurate enough". This would
// determine the time constants used for high-pass filtering those readings. If the value is set
// too high, we may experience drift. If it is set too low, we may experience poses tending toward
// identity too fast.
constexpr auto kRotationalDriftTimeConstant = 20s;

// This is how far into the future we predict the head pose, using linear extrapolation based on
// twist (velocity). It should be set to a value that matches the characteristic durations of moving
// one's head. The higher we set this, the more latency we are able to reduce, but setting this too
// high will result in high prediction errors whenever the head accelerates (changes velocity).
constexpr auto kPredictionDuration = 10ms;

// After losing this many consecutive samples from either sensor, we would treat the measurement as
// stale;
constexpr auto kMaxLostSamples = 4;

// Time units for system clock ticks. This is what the Sensor Framework timestamps represent and
// what we use for pose filtering.
using Ticks = std::chrono::nanoseconds;

// How many ticks in a second.
constexpr auto kTicksPerSecond = Ticks::period::den;

}  // namespace

SpatializerPoseController::SpatializerPoseController(Listener* listener,
                                                       std::chrono::microseconds sensorPeriod,
                                                       std::chrono::microseconds maxUpdatePeriod)
    : mListener(listener),
      mSensorPeriod(sensorPeriod),
      mPoseProvider(SensorPoseProvider::create("headtracker", this)),
      mProcessor(createHeadTrackingProcessor(HeadTrackingProcessor::Options{
              .maxTranslationalVelocity = kMaxTranslationalVelocity / kTicksPerSecond,
              .maxRotationalVelocity = kMaxRotationalVelocity / kTicksPerSecond,
              .translationalDriftTimeConstant = Ticks(kTranslationalDriftTimeConstant).count(),
              .rotationalDriftTimeConstant = Ticks(kRotationalDriftTimeConstant).count(),
              .freshnessTimeout = Ticks(sensorPeriod * kMaxLostSamples).count(),
              .predictionDuration = Ticks(kPredictionDuration).count(),
      })),
      mThread([this, maxUpdatePeriod] {
          while (true) {
              {
                  std::unique_lock lock(mMutex);
                  mCondVar.wait_for(lock, maxUpdatePeriod,
                                    [this] { return mShouldExit || mShouldCalculate; });
                  if (mShouldExit) {
                      ALOGV("Exiting thread");
                      return;
                  }
                  calculate_l();
                  if (!mCalculated) {
                      mCalculated = true;
                      mCondVar.notify_all();
                  }
                  mShouldCalculate = false;
              }
          }
      }) {}

SpatializerPoseController::~SpatializerPoseController() {
    {
        std::unique_lock lock(mMutex);
        mShouldExit = true;
        mCondVar.notify_all();
    }
    mThread.join();
}

void SpatializerPoseController::setHeadSensor(const ASensor* sensor) {
    std::lock_guard lock(mMutex);
    // Stop current sensor, if valid.
    if (mHeadSensor != SensorPoseProvider::INVALID_HANDLE) {
        mPoseProvider->stopSensor(mHeadSensor);
    }
    // Start new sensor, if valid.
    mHeadSensor = sensor != nullptr ? mPoseProvider->startSensor(sensor, mSensorPeriod)
                                    : SensorPoseProvider::INVALID_HANDLE;
    mProcessor->recenter(true, false);
}

void SpatializerPoseController::setScreenSensor(const ASensor* sensor) {
    std::lock_guard lock(mMutex);
    // Stop current sensor, if valid.
    if (mScreenSensor != SensorPoseProvider::INVALID_HANDLE) {
        mPoseProvider->stopSensor(mScreenSensor);
    }
    // Start new sensor, if valid.
    mScreenSensor = sensor != nullptr ? mPoseProvider->startSensor(sensor, mSensorPeriod)
                                      : SensorPoseProvider::INVALID_HANDLE;
    mProcessor->recenter(false, true);
}

void SpatializerPoseController::setDesiredMode(HeadTrackingMode mode) {
    std::lock_guard lock(mMutex);
    mProcessor->setDesiredMode(mode);
}

void SpatializerPoseController::setScreenToStagePose(const Pose3f& screenToStage) {
    std::lock_guard lock(mMutex);
    mProcessor->setScreenToStagePose(screenToStage);
}

void SpatializerPoseController::setDisplayOrientation(float physicalToLogicalAngle) {
    std::lock_guard lock(mMutex);
    mProcessor->setDisplayOrientation(physicalToLogicalAngle);
}

void SpatializerPoseController::calculateAsync() {
    std::lock_guard lock(mMutex);
    mShouldCalculate = true;
    mCondVar.notify_all();
}

void SpatializerPoseController::waitUntilCalculated() {
    std::unique_lock lock(mMutex);
    mCondVar.wait(lock, [this] { return mCalculated; });
}

void SpatializerPoseController::calculate_l() {
    Pose3f headToStage;
    HeadTrackingMode mode;
    mProcessor->calculate(elapsedRealtimeNano());
    headToStage = mProcessor->getHeadToStagePose();
    mode = mProcessor->getActualMode();
    mListener->onHeadToStagePose(headToStage);
    if (!mActualMode.has_value() || mActualMode.value() != mode) {
        mActualMode = mode;
        mListener->onActualModeChange(mode);
    }
}

void SpatializerPoseController::recenter() {
    std::lock_guard lock(mMutex);
    mProcessor->recenter();
}

void SpatializerPoseController::onPose(int64_t timestamp, int32_t sensor, const Pose3f& pose,
                                        const std::optional<Twist3f>& twist) {
    std::lock_guard lock(mMutex);
    if (sensor == mHeadSensor) {
        mProcessor->setWorldToHeadPose(timestamp, pose, twist.value_or(Twist3f()));
    } else if (sensor == mScreenSensor) {
        mProcessor->setWorldToScreenPose(timestamp, pose);
    }
}

}  // namespace android
