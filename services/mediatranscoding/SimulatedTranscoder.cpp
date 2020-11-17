/*
 * Copyright (C) 2020 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "SimulatedTranscoder"
#include "SimulatedTranscoder.h"

#include <utils/Log.h>

#include <thread>

namespace android {

//static
const char* SimulatedTranscoder::toString(Event::Type type) {
    switch (type) {
    case Event::Start:
        return "Start";
    case Event::Pause:
        return "Pause";
    case Event::Resume:
        return "Resume";
    default:
        break;
    }
    return "(unknown)";
}

SimulatedTranscoder::SimulatedTranscoder() {
    std::thread(&SimulatedTranscoder::threadLoop, this).detach();
}

void SimulatedTranscoder::setCallback(const std::shared_ptr<TranscoderCallbackInterface>& cb) {
    mCallback = cb;
}

void SimulatedTranscoder::start(
        ClientIdType clientId, SessionIdType sessionId, const TranscodingRequestParcel& request,
        const std::shared_ptr<ITranscodingClientCallback>& /*clientCallback*/) {
    if (request.testConfig.has_value() && request.testConfig->processingTotalTimeMs > 0) {
        mSessionProcessingTimeMs = request.testConfig->processingTotalTimeMs;
    }
    ALOGV("%s: session {%d}: processingTime: %lld", __FUNCTION__, sessionId,
          (long long)mSessionProcessingTimeMs);
    queueEvent(Event::Start, clientId, sessionId, [=] {
        auto callback = mCallback.lock();
        if (callback != nullptr) {
            callback->onStarted(clientId, sessionId);
        }
    });
}

void SimulatedTranscoder::pause(ClientIdType clientId, SessionIdType sessionId) {
    queueEvent(Event::Pause, clientId, sessionId, [=] {
        auto callback = mCallback.lock();
        if (callback != nullptr) {
            callback->onPaused(clientId, sessionId);
        }
    });
}

void SimulatedTranscoder::resume(
        ClientIdType clientId, SessionIdType sessionId, const TranscodingRequestParcel& /*request*/,
        const std::shared_ptr<ITranscodingClientCallback>& /*clientCallback*/) {
    queueEvent(Event::Resume, clientId, sessionId, [=] {
        auto callback = mCallback.lock();
        if (callback != nullptr) {
            callback->onResumed(clientId, sessionId);
        }
    });
}

void SimulatedTranscoder::stop(ClientIdType clientId, SessionIdType sessionId) {
    queueEvent(Event::Stop, clientId, sessionId, nullptr);
}

void SimulatedTranscoder::queueEvent(Event::Type type, ClientIdType clientId,
                                     SessionIdType sessionId, std::function<void()> runnable) {
    ALOGV("%s: session {%lld, %d}: %s", __FUNCTION__, (long long)clientId, sessionId,
          toString(type));

    auto lock = std::scoped_lock(mLock);

    mQueue.push_back({type, clientId, sessionId, runnable});
    mCondition.notify_one();
}

void SimulatedTranscoder::threadLoop() {
    bool running = false;
    std::chrono::microseconds remainingUs(kSessionDurationUs);
    std::chrono::system_clock::time_point lastRunningTime;
    Event lastRunningEvent;

    std::unique_lock<std::mutex> lock(mLock);
    // SimulatedTranscoder currently lives in the transcoding service, as long as
    // MediaTranscodingService itself.
    while (true) {
        // Wait for the next event.
        while (mQueue.empty()) {
            if (!running) {
                mCondition.wait(lock);
                continue;
            }
            // If running, wait for the remaining life of this session. Report finish if timed out.
            std::cv_status status = mCondition.wait_for(lock, remainingUs);
            if (status == std::cv_status::timeout) {
                running = false;

                auto callback = mCallback.lock();
                if (callback != nullptr) {
                    lock.unlock();
                    callback->onFinish(lastRunningEvent.clientId, lastRunningEvent.sessionId);
                    lock.lock();
                }
            } else {
                // Advance last running time and remaining time. This is needed to guard
                // against bad events (which will be ignored) or spurious wakeups, in that
                // case we don't want to wait for the same time again.
                auto now = std::chrono::system_clock::now();
                remainingUs -= (now - lastRunningTime);
                lastRunningTime = now;
            }
        }

        // Handle the events, adjust state and send updates to client accordingly.
        while (!mQueue.empty()) {
            Event event = *mQueue.begin();
            mQueue.pop_front();

            ALOGV("%s: session {%lld, %d}: %s", __FUNCTION__, (long long)event.clientId,
                  event.sessionId, toString(event.type));

            if (!running && (event.type == Event::Start || event.type == Event::Resume)) {
                running = true;
                lastRunningTime = std::chrono::system_clock::now();
                lastRunningEvent = event;
                if (event.type == Event::Start) {
                    remainingUs = std::chrono::milliseconds(mSessionProcessingTimeMs);
                }
            } else if (running && (event.type == Event::Pause || event.type == Event::Stop)) {
                running = false;
                remainingUs -= (std::chrono::system_clock::now() - lastRunningTime);
            } else {
                ALOGW("%s: discarding bad event: session {%lld, %d}: %s", __FUNCTION__,
                      (long long)event.clientId, event.sessionId, toString(event.type));
                continue;
            }

            if (event.runnable != nullptr) {
                lock.unlock();
                event.runnable();
                lock.lock();
            }
        }
    }
}

}  // namespace android
