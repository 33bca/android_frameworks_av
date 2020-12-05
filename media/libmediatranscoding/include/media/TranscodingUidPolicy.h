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

#ifndef ANDROID_MEDIA_TRANSCODING_UID_POLICY_H
#define ANDROID_MEDIA_TRANSCODING_UID_POLICY_H

#include <aidl/android/media/ITranscodingClient.h>
#include <aidl/android/media/ITranscodingClientCallback.h>
#include <media/UidPolicyInterface.h>
#include <sys/types.h>
#include <utils/Condition.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace android {

class ActivityManager;
// Observer for UID lifecycle and provide information about the uid's app
// priority used by the session controller.
class TranscodingUidPolicy : public UidPolicyInterface {
public:
    explicit TranscodingUidPolicy();
    ~TranscodingUidPolicy();

    // UidPolicyInterface
    bool isUidOnTop(uid_t uid) override;
    void registerMonitorUid(uid_t uid) override;
    void unregisterMonitorUid(uid_t uid) override;
    std::unordered_set<uid_t> getTopUids() const override;
    void setCallback(const std::shared_ptr<UidPolicyCallbackInterface>& cb) override;
    // ~UidPolicyInterface

private:
    void onUidStateChanged(uid_t uid, int32_t procState);
    void setUidObserverRegistered(bool registerd);
    void registerSelf();
    void unregisterSelf();
    void setProcessInfoOverride();
    int32_t getProcState_l(uid_t uid) NO_THREAD_SAFETY_ANALYSIS;
    void updateTopUid_l() NO_THREAD_SAFETY_ANALYSIS;

    struct UidObserver;
    struct ResourceManagerClient;
    mutable Mutex mUidLock;
    std::shared_ptr<ActivityManager> mAm;
    sp<UidObserver> mUidObserver;
    bool mRegistered GUARDED_BY(mUidLock);
    int32_t mTopUidState GUARDED_BY(mUidLock);
    std::unordered_map<uid_t, int32_t> mUidStateMap GUARDED_BY(mUidLock);
    std::map<int32_t, std::unordered_set<uid_t>> mStateUidMap GUARDED_BY(mUidLock);
    std::weak_ptr<UidPolicyCallbackInterface> mUidPolicyCallback;
    std::shared_ptr<ResourceManagerClient> mProcInfoOverrideClient;
};  // class TranscodingUidPolicy

}  // namespace android
#endif  // ANDROID_MEDIA_TRANSCODING_SERVICE_H
