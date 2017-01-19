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

#ifndef OBOE_OBOEDEFINITIONS_H
#define OBOE_OBOEDEFINITIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t  oboe_handle_t; // negative handles are error codes
typedef int32_t  oboe_result_t;
/**
 * A platform specific identifier for a device.
 */
typedef int32_t  oboe_device_id_t;
typedef int32_t  oboe_sample_rate_t;
/** This is used for small quantities such as the number of frames in a buffer. */
typedef int32_t  oboe_size_frames_t;
/** This is used for small quantities such as the number of bytes in a frame. */
typedef int32_t  oboe_size_bytes_t;
/**
 * This is used for large quantities, such as the number of frames that have
 * been played since a stream was started.
 * At 48000 Hz, a 32-bit integer would wrap around in just over 12 hours.
 */
typedef int64_t  oboe_position_frames_t;

typedef int64_t  oboe_nanoseconds_t;

/**
 * This is used to represent a value that has not been specified.
 * For example, an application could use OBOE_UNSPECIFIED to indicate
 * that is did not not care what the specific value of a parameter was
 * and would accept whatever it was given.
 */
#define OBOE_UNSPECIFIED           0
#define OBOE_DEVICE_UNSPECIFIED    ((oboe_device_id_t) -1)
#define OBOE_NANOS_PER_MICROSECOND ((int64_t)1000)
#define OBOE_NANOS_PER_MILLISECOND (OBOE_NANOS_PER_MICROSECOND * 1000)
#define OBOE_MILLIS_PER_SECOND     1000
#define OBOE_NANOS_PER_SECOND      (OBOE_NANOS_PER_MILLISECOND * OBOE_MILLIS_PER_SECOND)

#define OBOE_HANDLE_INVALID     ((oboe_handle_t)-1)

enum oboe_direction_t {
    OBOE_DIRECTION_OUTPUT,
    OBOE_DIRECTION_INPUT,
    OBOE_DIRECTION_COUNT // This should always be last.
};

enum oboe_audio_format_t {
    OBOE_AUDIO_FORMAT_INVALID = -1,
    OBOE_AUDIO_FORMAT_UNSPECIFIED = 0,
    OBOE_AUDIO_FORMAT_PCM16, // TODO rename to _PCM_I16
    OBOE_AUDIO_FORMAT_PCM_FLOAT,
    OBOE_AUDIO_FORMAT_PCM824, // TODO rename to _PCM_I8_24
    OBOE_AUDIO_FORMAT_PCM32  // TODO rename to _PCM_I32
};

enum {
    OBOE_OK,
    OBOE_ERROR_BASE = -900, // TODO review
    OBOE_ERROR_DISCONNECTED,
    OBOE_ERROR_ILLEGAL_ARGUMENT,
    OBOE_ERROR_INCOMPATIBLE,
    OBOE_ERROR_INTERNAL, // an underlying API returned an error code
    OBOE_ERROR_INVALID_STATE,
    OBOE_ERROR_UNEXPECTED_STATE,
    OBOE_ERROR_UNEXPECTED_VALUE,
    OBOE_ERROR_INVALID_HANDLE,
    OBOE_ERROR_INVALID_QUERY,
    OBOE_ERROR_UNIMPLEMENTED,
    OBOE_ERROR_UNAVAILABLE,
    OBOE_ERROR_NO_FREE_HANDLES,
    OBOE_ERROR_NO_MEMORY,
    OBOE_ERROR_NULL,
    OBOE_ERROR_TIMEOUT,
    OBOE_ERROR_WOULD_BLOCK,
    OBOE_ERROR_INVALID_ORDER,
    OBOE_ERROR_OUT_OF_RANGE
};

typedef enum {
    OBOE_CLOCK_MONOTONIC, // Clock since booted, pauses when CPU is sleeping.
    OBOE_CLOCK_BOOTTIME,  // Clock since booted, runs all the time.
    OBOE_CLOCK_COUNT // This should always be last.
} oboe_clockid_t;

typedef enum
{
    OBOE_STREAM_STATE_UNINITIALIZED = 0,
    OBOE_STREAM_STATE_OPEN,
    OBOE_STREAM_STATE_STARTING,
    OBOE_STREAM_STATE_STARTED,
    OBOE_STREAM_STATE_PAUSING,
    OBOE_STREAM_STATE_PAUSED,
    OBOE_STREAM_STATE_FLUSHING,
    OBOE_STREAM_STATE_FLUSHED,
    OBOE_STREAM_STATE_STOPPING,
    OBOE_STREAM_STATE_STOPPED,
    OBOE_STREAM_STATE_CLOSING,
    OBOE_STREAM_STATE_CLOSED,
} oboe_stream_state_t;

// TODO review API
typedef enum {
    /**
     * This will use an AudioTrack object for playing audio
     * and an AudioRecord for recording data.
     */
    OBOE_SHARING_MODE_LEGACY,
    /**
     * This will be the only stream using a particular source or sink.
     * This mode will provide the lowest possible latency.
     * You should close EXCLUSIVE streams immediately when you are not using them.
     */
    OBOE_SHARING_MODE_EXCLUSIVE,
    /**
     * Multiple applications will be mixed by the Oboe Server.
     * This will have higher latency than the EXCLUSIVE mode.
     */
    OBOE_SHARING_MODE_SHARED,
    /**
     * Multiple applications will do their own mixing into a memory mapped buffer.
     * It may be possible for malicious applications to read the data produced by
     * other apps. So do not use this for private data such as telephony or messaging.
     */
    OBOE_SHARING_MODE_PUBLIC_MIX,
    OBOE_SHARING_MODE_COUNT // This should always be last.
} oboe_sharing_mode_t;

#ifdef __cplusplus
}
#endif

#endif // OBOE_OBOEDEFINITIONS_H
