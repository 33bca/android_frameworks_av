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

#ifndef ANDROID_MEDIA_TRANSCODING_SESSION_CONTROLLER_H
#define ANDROID_MEDIA_TRANSCODING_SESSION_CONTROLLER_H

#include <aidl/android/media/TranscodingSessionPriority.h>
#include <media/ControllerClientInterface.h>
#include <media/ResourcePolicyInterface.h>
#include <media/TranscoderInterface.h>
#include <media/TranscodingRequest.h>
#include <media/UidPolicyInterface.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include <list>
#include <map>
#include <mutex>

namespace android {
using ::aidl::android::media::TranscodingResultParcel;
using ::aidl::android::media::TranscodingSessionPriority;

class TranscodingSessionController : public UidPolicyCallbackInterface,
                                     public ControllerClientInterface,
                                     public TranscoderCallbackInterface,
                                     public ResourcePolicyCallbackInterface {
public:
    virtual ~TranscodingSessionController();

    // ControllerClientInterface
    bool submit(ClientIdType clientId, SessionIdType sessionId, uid_t uid,
                const TranscodingRequestParcel& request,
                const std::weak_ptr<ITranscodingClientCallback>& clientCallback) override;
    bool cancel(ClientIdType clientId, SessionIdType sessionId) override;
    bool getSession(ClientIdType clientId, SessionIdType sessionId,
                    TranscodingRequestParcel* request) override;
    // ~ControllerClientInterface

    // TranscoderCallbackInterface
    void onStarted(ClientIdType clientId, SessionIdType sessionId) override;
    void onPaused(ClientIdType clientId, SessionIdType sessionId) override;
    void onResumed(ClientIdType clientId, SessionIdType sessionId) override;
    void onFinish(ClientIdType clientId, SessionIdType sessionId) override;
    void onError(ClientIdType clientId, SessionIdType sessionId, TranscodingErrorCode err) override;
    void onProgressUpdate(ClientIdType clientId, SessionIdType sessionId,
                          int32_t progress) override;
    void onResourceLost(ClientIdType clientId, SessionIdType sessionId) override;
    // ~TranscoderCallbackInterface

    // UidPolicyCallbackInterface
    void onTopUidsChanged(const std::unordered_set<uid_t>& uids) override;
    // ~UidPolicyCallbackInterface

    // ResourcePolicyCallbackInterface
    void onResourceAvailable() override;
    // ~ResourcePolicyCallbackInterface

    /**
     * Dump all the session information to the fd.
     */
    void dumpAllSessions(int fd, const Vector<String16>& args);

private:
    friend class MediaTranscodingService;
    friend class TranscodingSessionControllerTest;

    using SessionKeyType = std::pair<ClientIdType, SessionIdType>;
    using SessionQueueType = std::list<SessionKeyType>;

    struct Session {
        SessionKeyType key;
        uid_t uid;
        enum State {
            NOT_STARTED,
            RUNNING,
            PAUSED,
        } state;
        int32_t lastProgress;
        TranscodingRequest request;
        std::weak_ptr<ITranscodingClientCallback> callback;
    };

    // TODO(chz): call transcoder without global lock.
    // Use mLock for all entrypoints for now.
    mutable std::mutex mLock;

    std::map<SessionKeyType, Session> mSessionMap;

    // uid->SessionQueue map (uid == -1: offline queue)
    std::map<uid_t, SessionQueueType> mSessionQueues;

    // uids, with the head being the most-recently-top app, 2nd item is the
    // previous top app, etc.
    std::list<uid_t> mUidSortedList;
    std::list<uid_t>::iterator mOfflineUidIterator;
    std::map<uid_t, std::string> mUidPackageNames;

    std::shared_ptr<TranscoderInterface> mTranscoder;
    std::shared_ptr<UidPolicyInterface> mUidPolicy;
    std::shared_ptr<ResourcePolicyInterface> mResourcePolicy;

    Session* mCurrentSession;
    bool mResourceLost;

    // Only allow MediaTranscodingService and unit tests to instantiate.
    TranscodingSessionController(const std::shared_ptr<TranscoderInterface>& transcoder,
                                 const std::shared_ptr<UidPolicyInterface>& uidPolicy,
                                 const std::shared_ptr<ResourcePolicyInterface>& resourcePolicy);

    Session* getTopSession_l();
    void updateCurrentSession_l();
    void removeSession_l(const SessionKeyType& sessionKey);
    void moveUidsToTop_l(const std::unordered_set<uid_t>& uids, bool preserveTopUid);
    void notifyClient(ClientIdType clientId, SessionIdType sessionId, const char* reason,
                      std::function<void(const SessionKeyType&)> func);
    // Internal state verifier (debug only)
    void validateState_l();

    static String8 sessionToString(const SessionKeyType& sessionKey);
    static const char* sessionStateToString(const Session::State sessionState);
};

}  // namespace android
#endif  // ANDROID_MEDIA_TRANSCODING_SESSION_CONTROLLER_H
