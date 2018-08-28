/*
 * Copyright (C) 2013 The Android Open Source Project
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
 *
 *
 */

#define LOG_TAG "NBLog"

#include <algorithm>
#include <climits>
#include <math.h>
#include <unordered_set>
#include <vector>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <time.h>
#include <new>
#include <audio_utils/roundup.h>
#include <media/nblog/NBLog.h>
#include <media/nblog/PerformanceAnalysis.h>
#include <media/nblog/ReportPerformance.h>
#include <utils/CallStack.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include <queue>
#include <utility>

namespace android {

int NBLog::Entry::copyEntryDataAt(size_t offset) const
{
    // FIXME This is too slow
    if (offset == 0)
        return mEvent;
    else if (offset == 1)
        return mLength;
    else if (offset < (size_t) (mLength + 2))
        return ((char *) mData)[offset - 2];
    else if (offset == (size_t) (mLength + 2))
        return mLength;
    else
        return 0;
}

// ---------------------------------------------------------------------------

/*static*/
std::unique_ptr<NBLog::AbstractEntry> NBLog::AbstractEntry::buildEntry(const uint8_t *ptr)
{
    if (ptr == nullptr) {
        return nullptr;
    }
    const uint8_t type = EntryIterator(ptr)->type;
    switch (type) {
    case EVENT_START_FMT:
        return std::make_unique<FormatEntry>(FormatEntry(ptr));
    case EVENT_AUDIO_STATE:
    case EVENT_HISTOGRAM_ENTRY_TS:
        return std::make_unique<HistogramEntry>(HistogramEntry(ptr));
    default:
        ALOGW("Tried to create AbstractEntry of type %d", type);
        return nullptr;
    }
}

NBLog::AbstractEntry::AbstractEntry(const uint8_t *entry) : mEntry(entry)
{
}

// ---------------------------------------------------------------------------

NBLog::EntryIterator NBLog::FormatEntry::begin() const
{
    return EntryIterator(mEntry);
}

const char *NBLog::FormatEntry::formatString() const
{
    return (const char*) mEntry + offsetof(entry, data);
}

size_t NBLog::FormatEntry::formatStringLength() const
{
    return mEntry[offsetof(entry, length)];
}

NBLog::EntryIterator NBLog::FormatEntry::args() const
{
    auto it = begin();
    ++it; // skip start fmt
    ++it; // skip timestamp
    ++it; // skip hash
    // Skip author if present
    if (it->type == EVENT_AUTHOR) {
        ++it;
    }
    return it;
}

int64_t NBLog::FormatEntry::timestamp() const
{
    auto it = begin();
    ++it; // skip start fmt
    return it.payload<int64_t>();
}

NBLog::log_hash_t NBLog::FormatEntry::hash() const
{
    auto it = begin();
    ++it; // skip start fmt
    ++it; // skip timestamp
    // unaligned 64-bit read not supported
    log_hash_t hash;
    memcpy(&hash, it->data, sizeof(hash));
    return hash;
}

int NBLog::FormatEntry::author() const
{
    auto it = begin();
    ++it; // skip start fmt
    ++it; // skip timestamp
    ++it; // skip hash
    // if there is an author entry, return it, return -1 otherwise
    return it->type == EVENT_AUTHOR ? it.payload<int>() : -1;
}

NBLog::EntryIterator NBLog::FormatEntry::copyWithAuthor(
        std::unique_ptr<audio_utils_fifo_writer> &dst, int author) const
{
    auto it = begin();
    it.copyTo(dst);     // copy fmt start entry
    (++it).copyTo(dst); // copy timestamp
    (++it).copyTo(dst); // copy hash
    // insert author entry
    size_t authorEntrySize = Entry::kOverhead + sizeof(author);
    uint8_t authorEntry[authorEntrySize];
    authorEntry[offsetof(entry, type)] = EVENT_AUTHOR;
    authorEntry[offsetof(entry, length)] =
        authorEntry[authorEntrySize + Entry::kPreviousLengthOffset] =
        sizeof(author);
    *(int*) (&authorEntry[offsetof(entry, data)]) = author;
    dst->write(authorEntry, authorEntrySize);
    // copy rest of entries
    while ((++it)->type != EVENT_END_FMT) {
        it.copyTo(dst);
    }
    it.copyTo(dst);
    ++it;
    return it;
}

void NBLog::EntryIterator::copyTo(std::unique_ptr<audio_utils_fifo_writer> &dst) const
{
    size_t length = mPtr[offsetof(entry, length)] + Entry::kOverhead;
    dst->write(mPtr, length);
}

void NBLog::EntryIterator::copyData(uint8_t *dst) const
{
    memcpy((void*) dst, mPtr + offsetof(entry, data), mPtr[offsetof(entry, length)]);
}

NBLog::EntryIterator::EntryIterator()   // Dummy initialization.
    : mPtr(nullptr)
{
}

NBLog::EntryIterator::EntryIterator(const uint8_t *entry)
    : mPtr(entry)
{
}

NBLog::EntryIterator::EntryIterator(const NBLog::EntryIterator &other)
    : mPtr(other.mPtr)
{
}

const NBLog::entry& NBLog::EntryIterator::operator*() const
{
    return *(entry*) mPtr;
}

const NBLog::entry* NBLog::EntryIterator::operator->() const
{
    return (entry*) mPtr;
}

NBLog::EntryIterator& NBLog::EntryIterator::operator++()
{
    mPtr += mPtr[offsetof(entry, length)] + Entry::kOverhead;
    return *this;
}

NBLog::EntryIterator& NBLog::EntryIterator::operator--()
{
    mPtr -= mPtr[Entry::kPreviousLengthOffset] + Entry::kOverhead;
    return *this;
}

NBLog::EntryIterator NBLog::EntryIterator::next() const
{
    EntryIterator aux(*this);
    return ++aux;
}

NBLog::EntryIterator NBLog::EntryIterator::prev() const
{
    EntryIterator aux(*this);
    return --aux;
}

int NBLog::EntryIterator::operator-(const NBLog::EntryIterator &other) const
{
    return mPtr - other.mPtr;
}

bool NBLog::EntryIterator::operator!=(const EntryIterator &other) const
{
    return mPtr != other.mPtr;
}

bool NBLog::EntryIterator::hasConsistentLength() const
{
    return mPtr[offsetof(entry, length)] == mPtr[mPtr[offsetof(entry, length)] +
        Entry::kOverhead + Entry::kPreviousLengthOffset];
}

// ---------------------------------------------------------------------------

int64_t NBLog::HistogramEntry::timestamp() const
{
    return EntryIterator(mEntry).payload<HistTsEntry>().ts;
}

NBLog::log_hash_t NBLog::HistogramEntry::hash() const
{
    return EntryIterator(mEntry).payload<HistTsEntry>().hash;
}

int NBLog::HistogramEntry::author() const
{
    EntryIterator it(mEntry);
    return it->length == sizeof(HistTsEntryWithAuthor)
            ? it.payload<HistTsEntryWithAuthor>().author : -1;
}

NBLog::EntryIterator NBLog::HistogramEntry::copyWithAuthor(
        std::unique_ptr<audio_utils_fifo_writer> &dst, int author) const
{
    // Current histogram entry has {type, length, struct HistTsEntry, length}.
    // We now want {type, length, struct HistTsEntryWithAuthor, length}
    uint8_t buffer[Entry::kOverhead + sizeof(HistTsEntryWithAuthor)];
    // Copy content until the point we want to add the author
    memcpy(buffer, mEntry, sizeof(entry) + sizeof(HistTsEntry));
    // Copy the author
    *(int*) (buffer + sizeof(entry) + sizeof(HistTsEntry)) = author;
    // Update lengths
    buffer[offsetof(entry, length)] = sizeof(HistTsEntryWithAuthor);
    buffer[offsetof(entry, data) + sizeof(HistTsEntryWithAuthor) + offsetof(ending, length)]
        = sizeof(HistTsEntryWithAuthor);
    // Write new buffer into FIFO
    dst->write(buffer, sizeof(buffer));
    return EntryIterator(mEntry).next();
}

// ---------------------------------------------------------------------------

#if 0   // FIXME see note in NBLog.h
NBLog::Timeline::Timeline(size_t size, void *shared)
    : mSize(roundup(size)), mOwn(shared == NULL),
      mShared((Shared *) (mOwn ? new char[sharedSize(size)] : shared))
{
    new (mShared) Shared;
}

NBLog::Timeline::~Timeline()
{
    mShared->~Shared();
    if (mOwn) {
        delete[] (char *) mShared;
    }
}
#endif

/*static*/
size_t NBLog::Timeline::sharedSize(size_t size)
{
    // TODO fifo now supports non-power-of-2 buffer sizes, so could remove the roundup
    return sizeof(Shared) + roundup(size);
}

// ---------------------------------------------------------------------------

NBLog::Writer::Writer()
    : mShared(NULL), mFifo(NULL), mFifoWriter(NULL), mEnabled(false), mPidTag(NULL), mPidTagSize(0)
{
}

NBLog::Writer::Writer(void *shared, size_t size)
    : mShared((Shared *) shared),
      mFifo(mShared != NULL ?
        new audio_utils_fifo(size, sizeof(uint8_t),
            mShared->mBuffer, mShared->mRear, NULL /*throttlesFront*/) : NULL),
      mFifoWriter(mFifo != NULL ? new audio_utils_fifo_writer(*mFifo) : NULL),
      mEnabled(mFifoWriter != NULL)
{
    // caching pid and process name
    pid_t id = ::getpid();
    char procName[16];
    int status = prctl(PR_GET_NAME, procName);
    if (status) {  // error getting process name
        procName[0] = '\0';
    }
    size_t length = strlen(procName);
    mPidTagSize = length + sizeof(pid_t);
    mPidTag = new char[mPidTagSize];
    memcpy(mPidTag, &id, sizeof(pid_t));
    memcpy(mPidTag + sizeof(pid_t), procName, length);
}

NBLog::Writer::Writer(const sp<IMemory>& iMemory, size_t size)
    : Writer(iMemory != 0 ? (Shared *) iMemory->pointer() : NULL, size)
{
    mIMemory = iMemory;
}

NBLog::Writer::~Writer()
{
    delete mFifoWriter;
    delete mFifo;
    delete[] mPidTag;
}

void NBLog::Writer::log(const char *string)
{
    if (!mEnabled) {
        return;
    }
    LOG_ALWAYS_FATAL_IF(string == NULL, "Attempted to log NULL string");
    size_t length = strlen(string);
    if (length > Entry::kMaxLength) {
        length = Entry::kMaxLength;
    }
    log(EVENT_STRING, string, length);
}

void NBLog::Writer::logf(const char *fmt, ...)
{
    if (!mEnabled) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    Writer::logvf(fmt, ap);     // the Writer:: is needed to avoid virtual dispatch for LockedWriter
    va_end(ap);
}

void NBLog::Writer::logvf(const char *fmt, va_list ap)
{
    if (!mEnabled) {
        return;
    }
    char buffer[Entry::kMaxLength + 1 /*NUL*/];
    int length = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    if (length >= (int) sizeof(buffer)) {
        length = sizeof(buffer) - 1;
        // NUL termination is not required
        // buffer[length] = '\0';
    }
    if (length >= 0) {
        log(EVENT_STRING, buffer, length);
    }
}

void NBLog::Writer::logTimestamp()
{
    if (!mEnabled) {
        return;
    }
    int64_t ts = get_monotonic_ns();
    if (ts > 0) {
        log(EVENT_TIMESTAMP, &ts, sizeof(ts));
    } else {
        ALOGE("Failed to get timestamp");
    }
}

void NBLog::Writer::logTimestamp(const int64_t ts)
{
    if (!mEnabled) {
        return;
    }
    log(EVENT_TIMESTAMP, &ts, sizeof(ts));
}

void NBLog::Writer::logInteger(const int x)
{
    if (!mEnabled) {
        return;
    }
    log(EVENT_INTEGER, &x, sizeof(x));
}

void NBLog::Writer::logFloat(const float x)
{
    if (!mEnabled) {
        return;
    }
    log(EVENT_FLOAT, &x, sizeof(x));
}

void NBLog::Writer::logPID()
{
    if (!mEnabled) {
        return;
    }
    log(EVENT_PID, mPidTag, mPidTagSize);
}

void NBLog::Writer::logStart(const char *fmt)
{
    if (!mEnabled) {
        return;
    }
    size_t length = strlen(fmt);
    if (length > Entry::kMaxLength) {
        length = Entry::kMaxLength;
    }
    log(EVENT_START_FMT, fmt, length);
}

void NBLog::Writer::logEnd()
{
    if (!mEnabled) {
        return;
    }
    Entry entry = Entry(EVENT_END_FMT, NULL, 0);
    log(entry, true);
}

void NBLog::Writer::logHash(log_hash_t hash)
{
    if (!mEnabled) {
        return;
    }
    log(EVENT_HASH, &hash, sizeof(hash));
}

void NBLog::Writer::logEventHistTs(Event event, log_hash_t hash)
{
    if (!mEnabled) {
        return;
    }
    HistTsEntry data;
    data.hash = hash;
    data.ts = get_monotonic_ns();
    if (data.ts > 0) {
        log(event, &data, sizeof(data));
    } else {
        ALOGE("Failed to get timestamp");
    }
}

void NBLog::Writer::logMonotonicCycleTime(uint32_t monotonicNs)
{
    if (!mEnabled) {
        return;
    }
    log(EVENT_MONOTONIC_CYCLE_TIME, &monotonicNs, sizeof(&monotonicNs));
}

void NBLog::Writer::logFormat(const char *fmt, log_hash_t hash, ...)
{
    if (!mEnabled) {
        return;
    }
    va_list ap;
    va_start(ap, hash);
    Writer::logVFormat(fmt, hash, ap);
    va_end(ap);
}

void NBLog::Writer::logVFormat(const char *fmt, log_hash_t hash, va_list argp)
{
    if (!mEnabled) {
        return;
    }
    Writer::logStart(fmt);
    int i;
    double f;
    char* s;
    int64_t t;
    Writer::logTimestamp();
    Writer::logHash(hash);
    for (const char *p = fmt; *p != '\0'; p++) {
        // TODO: implement more complex formatting such as %.3f
        if (*p != '%') {
            continue;
        }
        switch(*++p) {
        case 's': // string
            s = va_arg(argp, char *);
            Writer::log(s);
            break;

        case 't': // timestamp
            t = va_arg(argp, int64_t);
            Writer::logTimestamp(t);
            break;

        case 'd': // integer
            i = va_arg(argp, int);
            Writer::logInteger(i);
            break;

        case 'f': // float
            f = va_arg(argp, double); // float arguments are promoted to double in vararg lists
            Writer::logFloat((float)f);
            break;

        case 'p': // pid
            Writer::logPID();
            break;

        // the "%\0" case finishes parsing
        case '\0':
            --p;
            break;

        case '%':
            break;

        default:
            ALOGW("NBLog Writer parsed invalid format specifier: %c", *p);
            break;
        }
    }
    Writer::logEnd();
}

void NBLog::Writer::log(Event event, const void *data, size_t length)
{
    if (!mEnabled) {
        return;
    }
    if (data == NULL || length > Entry::kMaxLength) {
        // TODO Perhaps it makes sense to display truncated data or at least a
        //      message that the data is too long?  The current behavior can create
        //      a confusion for a programmer debugging their code.
        return;
    }
    // Ignore if invalid event
    if (event == EVENT_RESERVED || event >= EVENT_UPPER_BOUND) {
        return;
    }
    Entry etr(event, data, length);
    log(etr, true /*trusted*/);
}

void NBLog::Writer::log(const NBLog::Entry &etr, bool trusted)
{
    if (!mEnabled) {
        return;
    }
    if (!trusted) {
        log(etr.mEvent, etr.mData, etr.mLength);
        return;
    }
    const size_t need = etr.mLength + Entry::kOverhead; // mEvent, mLength, data[mLength], mLength
                                                        // need = number of bytes written to FIFO

    // FIXME optimize this using memcpy for the data part of the Entry.
    // The Entry could have a method copyTo(ptr, offset, size) to optimize the copy.
    // checks size of a single log Entry: type, length, data pointer and ending
    uint8_t temp[Entry::kMaxLength + Entry::kOverhead];
    // write this data to temp array
    for (size_t i = 0; i < need; i++) {
        temp[i] = etr.copyEntryDataAt(i);
    }
    // write to circular buffer
    mFifoWriter->write(temp, need);
}

bool NBLog::Writer::isEnabled() const
{
    return mEnabled;
}

bool NBLog::Writer::setEnabled(bool enabled)
{
    bool old = mEnabled;
    mEnabled = enabled && mShared != NULL;
    return old;
}

// ---------------------------------------------------------------------------

NBLog::LockedWriter::LockedWriter()
    : Writer()
{
}

NBLog::LockedWriter::LockedWriter(void *shared, size_t size)
    : Writer(shared, size)
{
}

void NBLog::LockedWriter::log(const char *string)
{
    Mutex::Autolock _l(mLock);
    Writer::log(string);
}

void NBLog::LockedWriter::logf(const char *fmt, ...)
{
    // FIXME should not take the lock until after formatting is done
    Mutex::Autolock _l(mLock);
    va_list ap;
    va_start(ap, fmt);
    Writer::logvf(fmt, ap);
    va_end(ap);
}

void NBLog::LockedWriter::logvf(const char *fmt, va_list ap)
{
    // FIXME should not take the lock until after formatting is done
    Mutex::Autolock _l(mLock);
    Writer::logvf(fmt, ap);
}

void NBLog::LockedWriter::logTimestamp()
{
    // FIXME should not take the lock until after the clock_gettime() syscall
    Mutex::Autolock _l(mLock);
    Writer::logTimestamp();
}

void NBLog::LockedWriter::logTimestamp(const int64_t ts)
{
    Mutex::Autolock _l(mLock);
    Writer::logTimestamp(ts);
}

void NBLog::LockedWriter::logInteger(const int x)
{
    Mutex::Autolock _l(mLock);
    Writer::logInteger(x);
}

void NBLog::LockedWriter::logFloat(const float x)
{
    Mutex::Autolock _l(mLock);
    Writer::logFloat(x);
}

void NBLog::LockedWriter::logPID()
{
    Mutex::Autolock _l(mLock);
    Writer::logPID();
}

void NBLog::LockedWriter::logStart(const char *fmt)
{
    Mutex::Autolock _l(mLock);
    Writer::logStart(fmt);
}


void NBLog::LockedWriter::logEnd()
{
    Mutex::Autolock _l(mLock);
    Writer::logEnd();
}

void NBLog::LockedWriter::logHash(log_hash_t hash)
{
    Mutex::Autolock _l(mLock);
    Writer::logHash(hash);
}

bool NBLog::LockedWriter::isEnabled() const
{
    Mutex::Autolock _l(mLock);
    return Writer::isEnabled();
}

bool NBLog::LockedWriter::setEnabled(bool enabled)
{
    Mutex::Autolock _l(mLock);
    return Writer::setEnabled(enabled);
}

// ---------------------------------------------------------------------------

const std::unordered_set<NBLog::Event> NBLog::Reader::startingTypes {
        NBLog::Event::EVENT_START_FMT,
        NBLog::Event::EVENT_HISTOGRAM_ENTRY_TS,
        NBLog::Event::EVENT_AUDIO_STATE,
        NBLog::Event::EVENT_MONOTONIC_CYCLE_TIME
};
const std::unordered_set<NBLog::Event> NBLog::Reader::endingTypes   {
        NBLog::Event::EVENT_END_FMT,
        NBLog::Event::EVENT_HISTOGRAM_ENTRY_TS,
        NBLog::Event::EVENT_AUDIO_STATE,
        NBLog::Event::EVENT_MONOTONIC_CYCLE_TIME
};

NBLog::Reader::Reader(const void *shared, size_t size, const std::string &name)
    : mName(name),
      mShared((/*const*/ Shared *) shared), /*mIMemory*/
      mFifo(mShared != NULL ?
        new audio_utils_fifo(size, sizeof(uint8_t),
            mShared->mBuffer, mShared->mRear, NULL /*throttlesFront*/) : NULL),
      mFifoReader(mFifo != NULL ? new audio_utils_fifo_reader(*mFifo) : NULL)
{
}

NBLog::Reader::Reader(const sp<IMemory>& iMemory, size_t size, const std::string &name)
    : Reader(iMemory != 0 ? (Shared *) iMemory->pointer() : NULL, size, name)
{
    mIMemory = iMemory;
}

NBLog::Reader::~Reader()
{
    delete mFifoReader;
    delete mFifo;
}

const uint8_t *NBLog::Reader::findLastEntryOfTypes(const uint8_t *front, const uint8_t *back,
                                            const std::unordered_set<Event> &types) {
    while (back + Entry::kPreviousLengthOffset >= front) {
        const uint8_t *prev = back - back[Entry::kPreviousLengthOffset] - Entry::kOverhead;
        if (prev < front || prev + prev[offsetof(entry, length)] +
                            Entry::kOverhead != back) {

            // prev points to an out of limits or inconsistent entry
            return nullptr;
        }
        if (types.find((const Event) prev[offsetof(entry, type)]) != types.end()) {
            return prev;
        }
        back = prev;
    }
    return nullptr; // no entry found
}

// Copies content of a Reader FIFO into its Snapshot
// The Snapshot has the same raw data, but represented as a sequence of entries
// and an EntryIterator making it possible to process the data.
std::unique_ptr<NBLog::Snapshot> NBLog::Reader::getSnapshot()
{
    if (mFifoReader == NULL) {
        return std::make_unique<Snapshot>();
    }

    // This emulates the behaviour of audio_utils_fifo_reader::read, but without incrementing the
    // reader index. The index is incremented after handling corruption, to after the last complete
    // entry of the buffer
    size_t lost = 0;
    audio_utils_iovec iovec[2];
    const size_t capacity = mFifo->capacity();
    ssize_t availToRead;
    // A call to audio_utils_fifo_reader::obtain() places the read pointer one buffer length
    // before the writer's pointer (since mFifoReader was constructed with flush=false). The
    // do while loop is an attempt to read all of the FIFO's contents regardless of how behind
    // the reader is with respect to the writer. However, the following scheduling sequence is
    // possible and can lead to a starvation situation:
    // - Writer T1 writes, overrun with respect to Reader T2
    // - T2 calls obtain() and gets EOVERFLOW, T2 ptr placed one buffer size behind T1 ptr
    // - T1 write, overrun
    // - T2 obtain(), EOVERFLOW (and so on...)
    // To address this issue, we limit the number of tries for the reader to catch up with
    // the writer.
    int tries = 0;
    size_t lostTemp;
    do {
        availToRead = mFifoReader->obtain(iovec, capacity, NULL /*timeout*/, &lostTemp);
        lost += lostTemp;
    } while (availToRead < 0 || ++tries <= kMaxObtainTries);

    if (availToRead <= 0) {
        ALOGW_IF(availToRead < 0, "NBLog Reader %s failed to catch up with Writer", mName.c_str());
        return std::make_unique<Snapshot>();
    }

    std::unique_ptr<Snapshot> snapshot(new Snapshot(availToRead));
    memcpy(snapshot->mData, (const char *) mFifo->buffer() + iovec[0].mOffset, iovec[0].mLength);
    if (iovec[1].mLength > 0) {
        memcpy(snapshot->mData + (iovec[0].mLength),
                (const char *) mFifo->buffer() + iovec[1].mOffset, iovec[1].mLength);
    }

    // Handle corrupted buffer
    // Potentially, a buffer has corrupted data on both beginning (due to overflow) and end
    // (due to incomplete format entry). But even if the end format entry is incomplete,
    // it ends in a complete entry (which is not an END_FMT). So is safe to traverse backwards.
    // TODO: handle client corruption (in the middle of a buffer)

    const uint8_t *back = snapshot->mData + availToRead;
    const uint8_t *front = snapshot->mData;

    // Find last END_FMT. <back> is sitting on an entry which might be the middle of a FormatEntry.
    // We go backwards until we find an EVENT_END_FMT.
    const uint8_t *lastEnd = findLastEntryOfTypes(front, back, endingTypes);
    if (lastEnd == nullptr) {
        snapshot->mEnd = snapshot->mBegin = EntryIterator(front);
    } else {
        // end of snapshot points to after last END_FMT entry
        snapshot->mEnd = EntryIterator(lastEnd).next();
        // find first START_FMT
        const uint8_t *firstStart = nullptr;
        const uint8_t *firstStartTmp = snapshot->mEnd;
        while ((firstStartTmp = findLastEntryOfTypes(front, firstStartTmp, startingTypes))
                != nullptr) {
            firstStart = firstStartTmp;
        }
        // firstStart is null if no START_FMT entry was found before lastEnd
        if (firstStart == nullptr) {
            snapshot->mBegin = snapshot->mEnd;
        } else {
            snapshot->mBegin = EntryIterator(firstStart);
        }
    }

    // advance fifo reader index to after last entry read.
    mFifoReader->release(snapshot->mEnd - front);

    snapshot->mLost = lost;
    return snapshot;
}

// Takes raw content of the local merger FIFO, processes log entries, and
// writes the data to a map of class PerformanceAnalysis, based on their thread ID.
void NBLog::MergeReader::getAndProcessSnapshot(NBLog::Snapshot &snapshot, int author)
{
    for (const entry &etr : snapshot) {
        switch (etr.type) {
        case EVENT_HISTOGRAM_ENTRY_TS: {
            HistTsEntry *data = (HistTsEntry *) (etr.data);
            // TODO This memcpies are here to avoid unaligned memory access crash.
            // There's probably a more efficient way to do it
            log_hash_t hash;
            memcpy(&hash, &(data->hash), sizeof(hash));
            int64_t ts;
            memcpy(&ts, &data->ts, sizeof(ts));
            // TODO: hash for histogram ts and audio state need to match
            // and correspond to audio production source file location
            mThreadPerformanceAnalysis[author][0 /*hash*/].logTsEntry(ts);
        } break;
        case EVENT_AUDIO_STATE: {
            HistTsEntry *data = (HistTsEntry *) (etr.data);
            // TODO This memcpies are here to avoid unaligned memory access crash.
            // There's probably a more efficient way to do it
            log_hash_t hash;
            memcpy(&hash, &(data->hash), sizeof(hash));
            mThreadPerformanceAnalysis[author][0 /*hash*/].handleStateChange();
        } break;
        case EVENT_END_FMT:
        case EVENT_RESERVED:
        case EVENT_UPPER_BOUND:
            ALOGW("warning: unexpected event %d", etr.type);
        default:
            break;
        }
    }
}

void NBLog::MergeReader::getAndProcessSnapshot()
{
    // get a snapshot of each reader and process them
    // TODO insert lock here
    const size_t nLogs = mReaders.size();
    std::vector<std::unique_ptr<Snapshot>> snapshots(nLogs);
    for (size_t i = 0; i < nLogs; i++) {
        snapshots[i] = mReaders[i]->getSnapshot();
    }
    // TODO unlock lock here
    for (size_t i = 0; i < nLogs; i++) {
        if (snapshots[i] != nullptr) {
            getAndProcessSnapshot(*(snapshots[i]), i);
        }
    }
}

void NBLog::MergeReader::dump(int fd, int indent)
{
    // TODO: add a mutex around media.log dump
    ReportPerformance::dump(fd, indent, mThreadPerformanceAnalysis);
}

// TODO for future compatibility, would prefer to have a dump() go to string, and then go
// to fd only when invoked through binder.
void NBLog::DumpReader::dump(int fd, size_t indent)
{
    if (fd < 0) return;
    std::unique_ptr<Snapshot> snapshot = getSnapshot();
    if (snapshot == nullptr) {
        return;
    }
    String8 timestamp, body;

    // TODO all logged types should have a printable format.
    for (auto it = snapshot->begin(); it != snapshot->end(); ++it) {
        switch (it->type) {
        case EVENT_START_FMT:
            it = handleFormat(FormatEntry(it), &timestamp, &body);
            break;
        case EVENT_MONOTONIC_CYCLE_TIME: {
            uint32_t monotonicNs;
            memcpy(&monotonicNs, it->data, sizeof(monotonicNs));
            body.appendFormat("Thread cycle took %u ns", monotonicNs);
        } break;
        case EVENT_END_FMT:
        case EVENT_RESERVED:
        case EVENT_UPPER_BOUND:
            body.appendFormat("warning: unexpected event %d", it->type);
        default:
            break;
        }
        if (!body.isEmpty()) {
            dprintf(fd, "%.*s%s %s\n", (int)indent, "", timestamp.string(), body.string());
            body.clear();
        }
        timestamp.clear();
    }
}

bool NBLog::Reader::isIMemory(const sp<IMemory>& iMemory) const
{
    return iMemory != 0 && mIMemory != 0 && iMemory->pointer() == mIMemory->pointer();
}

void NBLog::DumpReader::appendTimestamp(String8 *body, const void *data)
{
    if (body == nullptr || data == nullptr) {
        return;
    }
    int64_t ts;
    memcpy(&ts, data, sizeof(ts));
    body->appendFormat("[%d.%03d]", (int) (ts / (1000 * 1000 * 1000)),
                    (int) ((ts / (1000 * 1000)) % 1000));
}

void NBLog::DumpReader::appendInt(String8 *body, const void *data)
{
    if (body == nullptr || data == nullptr) {
        return;
    }
    int x = *((int*) data);
    body->appendFormat("<%d>", x);
}

void NBLog::DumpReader::appendFloat(String8 *body, const void *data)
{
    if (body == nullptr || data == nullptr) {
        return;
    }
    float f;
    memcpy(&f, data, sizeof(f));
    body->appendFormat("<%f>", f);
}

void NBLog::DumpReader::appendPID(String8 *body, const void* data, size_t length)
{
    if (body == nullptr || data == nullptr) {
        return;
    }
    pid_t id = *((pid_t*) data);
    char * name = &((char*) data)[sizeof(pid_t)];
    body->appendFormat("<PID: %d, name: %.*s>", id, (int) (length - sizeof(pid_t)), name);
}

String8 NBLog::DumpReader::bufferDump(const uint8_t *buffer, size_t size)
{
    String8 str;
    if (buffer == nullptr) {
        return str;
    }
    str.append("[ ");
    for(size_t i = 0; i < size; i++) {
        str.appendFormat("%d ", buffer[i]);
    }
    str.append("]");
    return str;
}

String8 NBLog::DumpReader::bufferDump(const EntryIterator &it)
{
    return bufferDump(it, it->length + Entry::kOverhead);
}

NBLog::EntryIterator NBLog::DumpReader::handleFormat(const FormatEntry &fmtEntry,
                                                           String8 *timestamp,
                                                           String8 *body)
{
    // log timestamp
    int64_t ts = fmtEntry.timestamp();
    timestamp->clear();
    timestamp->appendFormat("[%d.%03d]", (int) (ts / (1000 * 1000 * 1000)),
                    (int) ((ts / (1000 * 1000)) % 1000));

    // log unique hash
    log_hash_t hash = fmtEntry.hash();
    // print only lower 16bit of hash as hex and line as int to reduce spam in the log
    body->appendFormat("%.4X-%d ", (int)(hash >> 16) & 0xFFFF, (int) hash & 0xFFFF);

    // log author (if present)
    handleAuthor(fmtEntry, body);

    // log string
    EntryIterator arg = fmtEntry.args();

    const char* fmt = fmtEntry.formatString();
    size_t fmt_length = fmtEntry.formatStringLength();

    for (size_t fmt_offset = 0; fmt_offset < fmt_length; ++fmt_offset) {
        if (fmt[fmt_offset] != '%') {
            body->append(&fmt[fmt_offset], 1); // TODO optimize to write consecutive strings at once
            continue;
        }
        // case "%%""
        if (fmt[++fmt_offset] == '%') {
            body->append("%");
            continue;
        }
        // case "%\0"
        if (fmt_offset == fmt_length) {
            continue;
        }

        NBLog::Event event = (NBLog::Event) arg->type;
        size_t length = arg->length;

        // TODO check length for event type is correct

        if (event == EVENT_END_FMT) {
            break;
        }

        // TODO: implement more complex formatting such as %.3f
        const uint8_t *datum = arg->data; // pointer to the current event args
        switch(fmt[fmt_offset])
        {
        case 's': // string
            ALOGW_IF(event != EVENT_STRING,
                "NBLog Reader incompatible event for string specifier: %d", event);
            body->append((const char*) datum, length);
            break;

        case 't': // timestamp
            ALOGW_IF(event != EVENT_TIMESTAMP,
                "NBLog Reader incompatible event for timestamp specifier: %d", event);
            appendTimestamp(body, datum);
            break;

        case 'd': // integer
            ALOGW_IF(event != EVENT_INTEGER,
                "NBLog Reader incompatible event for integer specifier: %d", event);
            appendInt(body, datum);
            break;

        case 'f': // float
            ALOGW_IF(event != EVENT_FLOAT,
                "NBLog Reader incompatible event for float specifier: %d", event);
            appendFloat(body, datum);
            break;

        case 'p': // pid
            ALOGW_IF(event != EVENT_PID,
                "NBLog Reader incompatible event for pid specifier: %d", event);
            appendPID(body, datum, length);
            break;

        default:
            ALOGW("NBLog Reader encountered unknown character %c", fmt[fmt_offset]);
        }
        ++arg;
    }
    ALOGW_IF(arg->type != EVENT_END_FMT, "Expected end of format, got %d", arg->type);
    return arg;
}

NBLog::Merger::Merger(const void *shared, size_t size):
      mShared((Shared *) shared),
      mFifo(mShared != NULL ?
        new audio_utils_fifo(size, sizeof(uint8_t),
            mShared->mBuffer, mShared->mRear, NULL /*throttlesFront*/) : NULL),
      mFifoWriter(mFifo != NULL ? new audio_utils_fifo_writer(*mFifo) : NULL)
{
}

void NBLog::Merger::addReader(const sp<NBLog::Reader> &reader)
{
    // FIXME This is called by binder thread in MediaLogService::registerWriter
    //       but the access to shared variable mReaders is not yet protected by a lock.
    mReaders.push_back(reader);
}

// items placed in priority queue during merge
// composed by a timestamp and the index of the snapshot where the timestamp came from
struct MergeItem
{
    int64_t ts;
    int index;
    MergeItem(int64_t ts, int index): ts(ts), index(index) {}
};

bool operator>(const struct MergeItem &i1, const struct MergeItem &i2)
{
    return i1.ts > i2.ts || (i1.ts == i2.ts && i1.index > i2.index);
}

// Merge registered readers, sorted by timestamp, and write data to a single FIFO in local memory
void NBLog::Merger::merge()
{
    if (true) return; // Merging is not necessary at the moment, so this is to disable it
                      // and bypass compiler warnings about member variables not being used.
    const int nLogs = mReaders.size();
    std::vector<std::unique_ptr<Snapshot>> snapshots(nLogs);
    std::vector<EntryIterator> offsets;
    offsets.reserve(nLogs);
    for (int i = 0; i < nLogs; ++i) {
        snapshots[i] = mReaders[i]->getSnapshot();
        offsets.push_back(snapshots[i]->begin());
    }
    // initialize offsets
    // TODO custom heap implementation could allow to update top, improving performance
    // for bursty buffers
    std::priority_queue<MergeItem, std::vector<MergeItem>, std::greater<MergeItem>> timestamps;
    for (int i = 0; i < nLogs; ++i)
    {
        if (offsets[i] != snapshots[i]->end()) {
            std::unique_ptr<AbstractEntry> abstractEntry = AbstractEntry::buildEntry(offsets[i]);
            if (abstractEntry == nullptr) {
                continue;
            }
            timestamps.emplace(abstractEntry->timestamp(), i);
        }
    }

    while (!timestamps.empty()) {
        int index = timestamps.top().index;     // find minimum timestamp
        // copy it to the log, increasing offset
        offsets[index] = AbstractEntry::buildEntry(offsets[index])->
            copyWithAuthor(mFifoWriter, index);
        // update data structures
        timestamps.pop();
        if (offsets[index] != snapshots[index]->end()) {
            int64_t ts = AbstractEntry::buildEntry(offsets[index])->timestamp();
            timestamps.emplace(ts, index);
        }
    }
}

const std::vector<sp<NBLog::Reader>>& NBLog::Merger::getReaders() const
{
    //AutoMutex _l(mLock);
    return mReaders;
}

// ---------------------------------------------------------------------------

NBLog::MergeReader::MergeReader(const void *shared, size_t size, Merger &merger)
    : Reader(shared, size, "MergeReader"), mReaders(merger.getReaders())
{
}

void NBLog::MergeReader::handleAuthor(const NBLog::AbstractEntry &entry, String8 *body)
{
    int author = entry.author();
    if (author == -1) {
        return;
    }
    // FIXME Needs a lock
    const char* name = mReaders[author]->name().c_str();
    body->appendFormat("%s: ", name);
}

// ---------------------------------------------------------------------------

NBLog::MergeThread::MergeThread(NBLog::Merger &merger, NBLog::MergeReader &mergeReader)
    : mMerger(merger),
      mMergeReader(mergeReader),
      mTimeoutUs(0)
{
}

NBLog::MergeThread::~MergeThread()
{
    // set exit flag, set timeout to 0 to force threadLoop to exit and wait for the thread to join
    requestExit();
    setTimeoutUs(0);
    join();
}

bool NBLog::MergeThread::threadLoop()
{
    bool doMerge;
    {
        AutoMutex _l(mMutex);
        // If mTimeoutUs is negative, wait on the condition variable until it's positive.
        // If it's positive, merge. The minimum period between waking the condition variable
        // is handled in AudioFlinger::MediaLogNotifier::threadLoop().
        mCond.wait(mMutex);
        doMerge = mTimeoutUs > 0;
        mTimeoutUs -= kThreadSleepPeriodUs;
    }
    if (doMerge) {
        // Merge data from all the readers
        mMerger.merge();
        // Process the data collected by mMerger and write it to PerformanceAnalysis
        // FIXME: decide whether to call getAndProcessSnapshot every time
        // or whether to have a separate thread that calls it with a lower frequency
        mMergeReader.getAndProcessSnapshot();
    }
    return true;
}

void NBLog::MergeThread::wakeup()
{
    setTimeoutUs(kThreadWakeupPeriodUs);
}

void NBLog::MergeThread::setTimeoutUs(int time)
{
    AutoMutex _l(mMutex);
    mTimeoutUs = time;
    mCond.signal();
}

}   // namespace android
