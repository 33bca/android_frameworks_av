/*
 * Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <mutex>
#include <unordered_map>

// IMediaAnalyticsService must include Vector, String16, Errors
#include <media/IMediaAnalyticsService.h>
#include <utils/String8.h>

namespace android {

class MediaAnalyticsService : public BnMediaAnalyticsService
{
public:
    MediaAnalyticsService();
    ~MediaAnalyticsService() override;

    /**
     * Submits the indicated record to the mediaanalytics service, where
     * it will be merged (if appropriate) with incomplete records that
     * share the same key and sessionid.
     *
     * \param item the item to submit.
     * \param forcenew marks any matching incomplete record as complete before
     *                 inserting this new record.
     *
     * \return the sessionID associated with that item or
     *         MediaAnalyticsItem::SessionIDInvalid on failure.
     *
     * BEWARE: When called directly on the service (not from the binder interface),
     * the caller surrenders ownership of item, MediaAnalyticsService will delete
     * even on error.  The binder interface does not take ownership.
     * TODO: fix this inconsistency with the binder RPC interface.
     */
    MediaAnalyticsItem::SessionID_t submit(MediaAnalyticsItem *item, bool forcenew) override;

    status_t dump(int fd, const Vector<String16>& args) override;

    static constexpr const char * const kServiceName = "media.metrics";

private:
    void processExpirations();
    MediaAnalyticsItem::SessionID_t generateUniqueSessionID();
    // input validation after arrival from client
    static bool isContentValid(const MediaAnalyticsItem *item, bool isTrusted);
    bool isRateLimited(MediaAnalyticsItem *) const;
    void saveItem(MediaAnalyticsItem *);

    // The following methods are GUARDED_BY(mLock)
    bool expirations_l(MediaAnalyticsItem *);

    // support for generating output
    void dumpQueue_l(String8 &result, int dumpProto);
    void dumpQueue_l(String8 &result, int dumpProto, nsecs_t, const char *only);
    void dumpHeaders_l(String8 &result, int dumpProto, nsecs_t ts_since);
    void dumpSummaries_l(String8 &result, int dumpProto, nsecs_t ts_since, const char * only);
    void dumpRecent_l(String8 &result, int dumpProto, nsecs_t ts_since, const char * only);

    // The following variables accessed without mLock

    // limit how many records we'll retain
    // by count (in each queue (open, finalized))
    const size_t mMaxRecords;
    // by time (none older than this)
    const nsecs_t mMaxRecordAgeNs;
    // max to expire per expirations_l() invocation
    const size_t mMaxRecordsExpiredAtOnce;
    const int mDumpProtoDefault;

    std::atomic<MediaAnalyticsItem::SessionID_t> mLastSessionID{};

    class UidInfo {
    public:
        void setPkgInfo(MediaAnalyticsItem *item, uid_t uid, bool setName, bool setVersion);

    private:
        std::mutex mUidInfoLock;

        struct UidToPkgInfo {
            uid_t uid = -1;
            std::string pkg;
            std::string installer;
            int64_t versionCode = 0;
            nsecs_t expiration = 0;  // TODO: remove expiration.
        };

        // TODO: use concurrent hashmap with striped lock.
        std::unordered_map<uid_t, struct UidToPkgInfo> mPkgMappings; // GUARDED_BY(mUidInfoLock)
    } mUidInfo;  // mUidInfo can be accessed without lock (locked internally)

    std::atomic<int64_t> mItemsSubmitted{}; // accessed outside of lock.

    std::mutex mLock;
    // statistics about our analytics
    int64_t mItemsFinalized = 0;        // GUARDED_BY(mLock)
    int64_t mItemsDiscarded = 0;        // GUARDED_BY(mLock)
    int64_t mItemsDiscardedExpire = 0;  // GUARDED_BY(mLock)
    int64_t mItemsDiscardedCount = 0;   // GUARDED_BY(mLock)

    // If we have a worker thread to garbage collect
    std::future<void> mExpireFuture;    // GUARDED_BY(mLock)

    // Our item queue, generally (oldest at front)
    // TODO: Make separate class, use segmented queue, write lock only end.
    // Note: Another analytics module might have ownership of an item longer than the log.
    std::deque<std::shared_ptr<const MediaAnalyticsItem>> mItems; // GUARDED_BY(mLock)
};

} // namespace android
