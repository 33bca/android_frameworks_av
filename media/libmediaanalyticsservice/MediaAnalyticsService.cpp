/*
 * Copyright (C) 2016 The Android Open Source Project
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

// Proxy for media player implementations

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaAnalyticsService"
#include <utils/Log.h>

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>

#include <string.h>

#include <cutils/atomic.h>
#include <cutils/properties.h> // for property_get

#include <utils/misc.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <gui/Surface.h>
#include <utils/Errors.h>  // for status_t
#include <utils/List.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <utils/Timers.h>
#include <utils/Vector.h>

#include <media/AudioPolicyHelper.h>
#include <media/IMediaHTTPService.h>
#include <media/IRemoteDisplay.h>
#include <media/IRemoteDisplayClient.h>
#include <media/MediaPlayerInterface.h>
#include <media/mediarecorder.h>
#include <media/MediaMetadataRetrieverInterface.h>
#include <media/Metadata.h>
#include <media/AudioTrack.h>
#include <media/MemoryLeakTrackUtil.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooperRoster.h>
#include <mediautils/BatteryNotifier.h>

//#include <memunreachable/memunreachable.h>
#include <system/audio.h>

#include <private/android_filesystem_config.h>

#include "MediaAnalyticsService.h"


namespace android {


#define DEBUG_QUEUE     0

//using android::status_t;
//using android::OK;
//using android::BAD_VALUE;
//using android::NOT_ENOUGH_DATA;
//using android::Parcel;


void MediaAnalyticsService::instantiate() {
    defaultServiceManager()->addService(
            String16("media.analytics"), new MediaAnalyticsService());
}

// XXX: add dynamic controls for mMaxRecords
MediaAnalyticsService::MediaAnalyticsService()
        : mMaxRecords(100) {

    ALOGD("MediaAnalyticsService created");
    // clear our queues
    mOpen = new List<MediaAnalyticsItem *>();
    mFinalized = new List<MediaAnalyticsItem *>();

    mItemsSubmitted = 0;
    mItemsFinalized = 0;
    mItemsDiscarded = 0;

    mLastSessionID = 0;
    // recover any persistency we set up
    // etc
}

MediaAnalyticsService::~MediaAnalyticsService() {
        ALOGD("MediaAnalyticsService destroyed");

    // XXX: clean out mOpen and mFinalized
}


MediaAnalyticsItem::SessionID_t MediaAnalyticsService::generateUniqueSessionID() {
    // generate a new sessionid

    Mutex::Autolock _l(mLock_ids);
    return (++mLastSessionID);
}

// caller surrenders ownership of 'item'
MediaAnalyticsItem::SessionID_t MediaAnalyticsService::submit(MediaAnalyticsItem *item, bool forcenew) {

    MediaAnalyticsItem::SessionID_t id = MediaAnalyticsItem::SessionIDInvalid;

    // we control these, not using whatever the user might have sent
    nsecs_t now = systemTime(SYSTEM_TIME_REALTIME);
    item->setTimestamp(now);
    int pid = IPCThreadState::self()->getCallingPid();
    item->setPid(pid);
    int uid = IPCThreadState::self()->getCallingUid();
    item->setUid(uid);

    mItemsSubmitted++;

    // validate the record; we discard if we don't like it
    if (contentValid(item) == false) {
        delete item;
        return MediaAnalyticsItem::SessionIDInvalid;
    }


    // if we have a sesisonid in the new record, look to make
    // sure it doesn't appear in the finalized list.
    // XXX: this is for security / DOS prevention.
    // may also require that we persist the unique sessionIDs
    // across boots [instead of within a single boot]


    // match this new record up against records in the open
    // list...
    // if there's a match, merge them together
    // deal with moving the old / merged record into the finalized que

    bool finalizing = item->getFinalized();

    // if finalizing, we'll remove it
    MediaAnalyticsItem *oitem = findItem(mOpen, item, finalizing | forcenew);
    if (oitem != NULL) {
        if (forcenew) {
            // old one gets finalized, then we insert the new one
            // so we'll have 2 records at the end of this.
            // but don't finalize an empty record
            if (oitem->count() == 0) {
                // we're responsible for disposing of the dead record
                delete oitem;
                oitem = NULL;
            } else {
                oitem->setFinalized(true);
                saveItem(mFinalized, oitem, 0);
            }
            // new record could itself be marked finalized...
            if (finalizing) {
                saveItem(mFinalized, item, 0);
                mItemsFinalized++;
            } else {
                saveItem(mOpen, item, 1);
            }
            id = item->getSessionID();
        } else {
            // combine the records, send it to finalized if appropriate
            oitem->merge(item);
            if (finalizing) {
                saveItem(mFinalized, oitem, 0);
                mItemsFinalized++;
            }
            id = oitem->getSessionID();

            // we're responsible for disposing of the dead record
            delete item;
            item = NULL;
        }
    } else {
        // nothing to merge, save the new record
        id = item->getSessionID();
        if (finalizing) {
            if (item->count() == 0) {
                // drop empty records
                delete item;
                item = NULL;
            } else {
                saveItem(mFinalized, item, 0);
                mItemsFinalized++;
            }
        } else {
            saveItem(mOpen, item, 1);
        }
    }
    return id;
}

List<MediaAnalyticsItem *> *MediaAnalyticsService::getMediaAnalyticsItemList(bool finished, nsecs_t ts) {
    // this might never get called; the binder interface maps to the full parm list
    // on the client side before making the binder call.
    // but this lets us be sure...
    List<MediaAnalyticsItem*> *list;
    list = getMediaAnalyticsItemList(finished, ts, MediaAnalyticsItem::kKeyAny);
    return list;
}

List<MediaAnalyticsItem *> *MediaAnalyticsService::getMediaAnalyticsItemList(bool , nsecs_t , MediaAnalyticsItem::Key ) {

    // XXX: implement the get-item-list semantics

    List<MediaAnalyticsItem *> *list = NULL;
    // set up our query on the persistent data
    // slurp in all of the pieces
    // return that
    return list;
}

status_t MediaAnalyticsService::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 512;
    char buffer[SIZE];
    String8 result;

    if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump MediaAnalyticsService from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
        write(fd, result.string(), result.size());
        return NO_ERROR;
    }

    // crack any parameters
    bool clear = false;
    nsecs_t ts_since = 0;
    String16 clearOption("-clear");
    String16 sinceOption("-since");
    int n = args.size();
    for (int i = 0; i < n; i++) {
        String8 myarg(args[i]);
        if (args[i] == clearOption) {
            clear = true;
        } else if (args[i] == sinceOption) {
            i++;
            if (i < n) {
                String8 value(args[i]);
                char *endp;
                const char *p = value.string();
                ts_since = strtoll(p, &endp, 10);
                if (endp == p || *endp != '\0') {
                    ts_since = 0;
                }
            } else {
                ts_since = 0;
            }
        }
    }

    Mutex::Autolock _l(mLock);

    snprintf(buffer, SIZE, "Dump of the mediaanalytics process:\n");
    result.append(buffer);

    int enabled = MediaAnalyticsItem::isEnabled();
    if (enabled) {
        snprintf(buffer, SIZE, "Analytics gathering: enabled\n");
    } else {
        snprintf(buffer, SIZE, "Analytics gathering: DISABLED via property\n");
    }
    result.append(buffer);

    snprintf(buffer, SIZE,
        "Since Boot: Submissions: %" PRId64
            " Finalizations: %" PRId64
        " Discarded: %" PRId64 "\n",
        mItemsSubmitted, mItemsFinalized, mItemsDiscarded);
    result.append(buffer);
    if (ts_since != 0) {
        snprintf(buffer, SIZE,
            "Dumping Queue entries more recent than: %" PRId64 "\n",
            (int64_t) ts_since);
        result.append(buffer);
    }

    // show the recently recorded records
    snprintf(buffer, sizeof(buffer), "\nFinalized Analytics (oldest first):\n");
    result.append(buffer);
    result.append(this->dumpQueue(mFinalized, ts_since));

    snprintf(buffer, sizeof(buffer), "\nIn-Progress Analytics (newest first):\n");
    result.append(buffer);
    result.append(this->dumpQueue(mOpen, ts_since));

    // show who is connected and injecting records?
    // talk about # records fed to the 'readers'
    // talk about # records we discarded, perhaps "discarded w/o reading" too

    if (clear) {
        // remove everything from the finalized queue
        while (mFinalized->size() > 0) {
            MediaAnalyticsItem * oitem = *(mFinalized->begin());
            if (DEBUG_QUEUE) {
                ALOGD("zap old record: key %s sessionID %" PRId64 " ts %" PRId64 "",
                    oitem->getKey().c_str(), oitem->getSessionID(),
                    oitem->getTimestamp());
            }
            mFinalized->erase(mFinalized->begin());
            mItemsDiscarded++;
        }
    }

    write(fd, result.string(), result.size());
    return NO_ERROR;
}

// caller has locked mLock...
String8 MediaAnalyticsService::dumpQueue(List<MediaAnalyticsItem *> *theList) {
    return dumpQueue(theList, (nsecs_t) 0);
}

String8 MediaAnalyticsService::dumpQueue(List<MediaAnalyticsItem *> *theList, nsecs_t ts_since) {
    const size_t SIZE = 512;
    char buffer[SIZE];
    String8 result;
    int slot = 0;

    if (theList->empty()) {
            result.append("empty\n");
    } else {
        List<MediaAnalyticsItem *>::iterator it = theList->begin();
        for (; it != theList->end(); it++) {
            nsecs_t when = (*it)->getTimestamp();
            if (when < ts_since) {
                continue;
            }
            AString entry = (*it)->toString();
            snprintf(buffer, sizeof(buffer), "%4d: %s",
                        slot, entry.c_str());
            result.append(buffer);
            buffer[0] = '\n';
            buffer[1] = '\0';
            result.append(buffer);
            slot++;
        }
    }

    return result;
}

//
// Our Cheap in-core, non-persistent records management.
// XXX: rewrite this to manage persistence, etc.

// insert appropriately into queue
void MediaAnalyticsService::saveItem(List<MediaAnalyticsItem *> *l, MediaAnalyticsItem * item, int front) {

    Mutex::Autolock _l(mLock);

    if (DEBUG_QUEUE) {
        ALOGD("Inject a record: session %" PRId64 " ts %" PRId64 "",
            item->getSessionID(), item->getTimestamp());
        String8 before = dumpQueue(l);
        ALOGD("Q before insert: %s", before.string());
    }

    // adding at back of queue (fifo order)
    if (front)  {
        l->push_front(item);
    } else {
        l->push_back(item);
    }

    if (DEBUG_QUEUE) {
        String8 after = dumpQueue(l);
        ALOGD("Q after insert: %s", after.string());
    }

    // keep removing old records the front until we're in-bounds
    if (mMaxRecords > 0) {
        while (l->size() > (size_t) mMaxRecords) {
            MediaAnalyticsItem * oitem = *(l->begin());
            if (DEBUG_QUEUE) {
                ALOGD("zap old record: key %s sessionID %" PRId64 " ts %" PRId64 "",
                    oitem->getKey().c_str(), oitem->getSessionID(),
                    oitem->getTimestamp());
            }
            l->erase(l->begin());
        ALOGD("drop record at %s:%d", __FILE__, __LINE__);
            delete oitem;
        ALOGD("[done] drop record at %s:%d", __FILE__, __LINE__);
            mItemsDiscarded++;
        }
    }

    if (DEBUG_QUEUE) {
        String8 after = dumpQueue(l);
        ALOGD("Q after cleanup: %s", after.string());
    }
}

// are they alike enough that nitem can be folded into oitem?
static bool compatibleItems(MediaAnalyticsItem * oitem, MediaAnalyticsItem * nitem) {

    if (0) {
        ALOGD("Compare: o %s n %s",
              oitem->toString().c_str(), nitem->toString().c_str());
    }

    // general safety
    if (nitem->getUid() != oitem->getUid()) {
        return false;
    }
    if (nitem->getPid() != oitem->getPid()) {
        return false;
    }

    // key -- needs to match
    if (nitem->getKey() == oitem->getKey()) {
        // still in the game.
    } else {
        return false;
    }

    // session id -- empty field in new is allowed
    MediaAnalyticsItem::SessionID_t osession = oitem->getSessionID();
    MediaAnalyticsItem::SessionID_t nsession = nitem->getSessionID();
    if (nsession != osession) {
        // incoming '0' matches value in osession
        if (nsession != 0) {
            return false;
        }
    }

    return true;
}

// find the incomplete record that this will overlay
MediaAnalyticsItem *MediaAnalyticsService::findItem(List<MediaAnalyticsItem*> *theList, MediaAnalyticsItem *nitem, bool removeit) {
    if (nitem == NULL) {
        return NULL;
    }

    MediaAnalyticsItem *item = NULL;

    Mutex::Autolock _l(mLock);

    for (List<MediaAnalyticsItem *>::iterator it = theList->begin();
        it != theList->end(); it++) {
        MediaAnalyticsItem *tmp = (*it);

        if (!compatibleItems(tmp, nitem)) {
            continue;
        }

        // we match! this is the one I want.
        if (removeit) {
            theList->erase(it);
        }
        item = tmp;
        break;
    }
    return item;
}


// delete the indicated record
void MediaAnalyticsService::deleteItem(List<MediaAnalyticsItem *> *l, MediaAnalyticsItem *item) {

    Mutex::Autolock _l(mLock);

    if(DEBUG_QUEUE) {
        String8 before = dumpQueue(l);
        ALOGD("Q before delete: %s", before.string());
    }

    for (List<MediaAnalyticsItem *>::iterator it = l->begin();
        it != l->end(); it++) {
        if ((*it)->getSessionID() != item->getSessionID())
            continue;

        if (DEBUG_QUEUE) {
            ALOGD(" --- removing record for SessionID %" PRId64 "", item->getSessionID());
            ALOGD("drop record at %s:%d", __FILE__, __LINE__);
        }
        delete *it;
        l->erase(it);
        break;
    }

    if (DEBUG_QUEUE) {
        String8 after = dumpQueue(l);
        ALOGD("Q after delete: %s", after.string());
    }
}

// are the contents good
bool MediaAnalyticsService::contentValid(MediaAnalyticsItem *) {

    // certain keys require certain uids
    // internal consistency

    return true;
}

// are we rate limited, normally false
bool MediaAnalyticsService::rateLimited(MediaAnalyticsItem *) {

    return false;
}


} // namespace android
