/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "IAudioFlinger"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <stdint.h>
#include <sys/types.h>

#include <binder/IPCThreadState.h>
#include <binder/Parcel.h>
#include "IAudioFlinger.h"

namespace android {

#define MAX_ITEMS_PER_LIST 1024

ConversionResult<media::CreateTrackRequest> IAudioFlinger::CreateTrackInput::toAidl() const {
    media::CreateTrackRequest aidl;
    aidl.attr = VALUE_OR_RETURN(legacy2aidl_audio_attributes_t_AudioAttributesInternal(attr));
    aidl.config = VALUE_OR_RETURN(legacy2aidl_audio_config_t_AudioConfig(config));
    aidl.clientInfo = VALUE_OR_RETURN(legacy2aidl_AudioClient(clientInfo));
    aidl.sharedBuffer = VALUE_OR_RETURN(legacy2aidl_NullableIMemory_SharedFileRegion(sharedBuffer));
    aidl.notificationsPerBuffer = VALUE_OR_RETURN(convertIntegral<int32_t>(notificationsPerBuffer));
    aidl.speed = speed;
    aidl.audioTrackCallback = audioTrackCallback;
    aidl.opPackageName = opPackageName;
    aidl.flags = VALUE_OR_RETURN(legacy2aidl_audio_output_flags_mask(flags));
    aidl.frameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(frameCount));
    aidl.notificationFrameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(notificationFrameCount));
    aidl.selectedDeviceId = VALUE_OR_RETURN(
            legacy2aidl_audio_port_handle_t_int32_t(selectedDeviceId));
    aidl.sessionId = VALUE_OR_RETURN(legacy2aidl_audio_session_t_int32_t(sessionId));
    return aidl;
}

ConversionResult<IAudioFlinger::CreateTrackInput>
IAudioFlinger::CreateTrackInput::fromAidl(const media::CreateTrackRequest& aidl) {
    IAudioFlinger::CreateTrackInput legacy;
    legacy.attr = VALUE_OR_RETURN(aidl2legacy_AudioAttributesInternal_audio_attributes_t(aidl.attr));
    legacy.config = VALUE_OR_RETURN(aidl2legacy_AudioConfig_audio_config_t(aidl.config));
    legacy.clientInfo = VALUE_OR_RETURN(aidl2legacy_AudioClient(aidl.clientInfo));
    legacy.sharedBuffer = VALUE_OR_RETURN(aidl2legacy_NullableSharedFileRegion_IMemory(aidl.sharedBuffer));
    legacy.notificationsPerBuffer = VALUE_OR_RETURN(
            convertIntegral<uint32_t>(aidl.notificationsPerBuffer));
    legacy.speed = aidl.speed;
    legacy.audioTrackCallback = aidl.audioTrackCallback;
    legacy.opPackageName = aidl.opPackageName;
    legacy.flags = VALUE_OR_RETURN(aidl2legacy_audio_output_flags_mask(aidl.flags));
    legacy.frameCount = VALUE_OR_RETURN(convertIntegral<size_t>(aidl.frameCount));
    legacy.notificationFrameCount = VALUE_OR_RETURN(
            convertIntegral<size_t>(aidl.notificationFrameCount));
    legacy.selectedDeviceId = VALUE_OR_RETURN(
            aidl2legacy_int32_t_audio_port_handle_t(aidl.selectedDeviceId));
    legacy.sessionId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_session_t(aidl.sessionId));
    return legacy;
}

ConversionResult<media::CreateTrackResponse>
IAudioFlinger::CreateTrackOutput::toAidl() const {
    media::CreateTrackResponse aidl;
    aidl.flags = VALUE_OR_RETURN(legacy2aidl_audio_output_flags_mask(flags));
    aidl.frameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(frameCount));
    aidl.notificationFrameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(notificationFrameCount));
    aidl.selectedDeviceId = VALUE_OR_RETURN(
            legacy2aidl_audio_port_handle_t_int32_t(selectedDeviceId));
    aidl.sessionId = VALUE_OR_RETURN(legacy2aidl_audio_session_t_int32_t(sessionId));
    aidl.sampleRate = VALUE_OR_RETURN(convertIntegral<int32_t>(sampleRate));
    aidl.afFrameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(afFrameCount));
    aidl.afSampleRate = VALUE_OR_RETURN(convertIntegral<int32_t>(afSampleRate));
    aidl.afLatencyMs = VALUE_OR_RETURN(convertIntegral<int32_t>(afLatencyMs));
    aidl.outputId = VALUE_OR_RETURN(legacy2aidl_audio_io_handle_t_int32_t(outputId));
    aidl.portId = VALUE_OR_RETURN(legacy2aidl_audio_port_handle_t_int32_t(portId));
    aidl.audioTrack = audioTrack;
    return aidl;
}

ConversionResult<IAudioFlinger::CreateTrackOutput>
IAudioFlinger::CreateTrackOutput::fromAidl(
        const media::CreateTrackResponse& aidl) {
    IAudioFlinger::CreateTrackOutput legacy;
    legacy.flags = VALUE_OR_RETURN(aidl2legacy_audio_output_flags_mask(aidl.flags));
    legacy.frameCount = VALUE_OR_RETURN(convertIntegral<size_t>(aidl.frameCount));
    legacy.notificationFrameCount = VALUE_OR_RETURN(
            convertIntegral<size_t>(aidl.notificationFrameCount));
    legacy.selectedDeviceId = VALUE_OR_RETURN(
            aidl2legacy_int32_t_audio_port_handle_t(aidl.selectedDeviceId));
    legacy.sessionId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_session_t(aidl.sessionId));
    legacy.sampleRate = VALUE_OR_RETURN(convertIntegral<uint32_t>(aidl.sampleRate));
    legacy.afFrameCount = VALUE_OR_RETURN(convertIntegral<size_t>(aidl.afFrameCount));
    legacy.afSampleRate = VALUE_OR_RETURN(convertIntegral<uint32_t>(aidl.afSampleRate));
    legacy.afLatencyMs = VALUE_OR_RETURN(convertIntegral<uint32_t>(aidl.afLatencyMs));
    legacy.outputId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_io_handle_t(aidl.outputId));
    legacy.portId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_port_handle_t(aidl.portId));
    legacy.audioTrack = aidl.audioTrack;
    return legacy;
}

ConversionResult<media::CreateRecordRequest>
IAudioFlinger::CreateRecordInput::toAidl() const {
    media::CreateRecordRequest aidl;
    aidl.attr = VALUE_OR_RETURN(legacy2aidl_audio_attributes_t_AudioAttributesInternal(attr));
    aidl.config = VALUE_OR_RETURN(legacy2aidl_audio_config_base_t_AudioConfigBase(config));
    aidl.clientInfo = VALUE_OR_RETURN(legacy2aidl_AudioClient(clientInfo));
    aidl.opPackageName = VALUE_OR_RETURN(legacy2aidl_String16_string(opPackageName));
    aidl.riid = VALUE_OR_RETURN(legacy2aidl_audio_unique_id_t_int32_t(riid));
    aidl.flags = VALUE_OR_RETURN(legacy2aidl_audio_input_flags_mask(flags));
    aidl.frameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(frameCount));
    aidl.notificationFrameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(notificationFrameCount));
    aidl.selectedDeviceId = VALUE_OR_RETURN(
            legacy2aidl_audio_port_handle_t_int32_t(selectedDeviceId));
    aidl.sessionId = VALUE_OR_RETURN(legacy2aidl_audio_session_t_int32_t(sessionId));
    return aidl;
}

ConversionResult<IAudioFlinger::CreateRecordInput>
IAudioFlinger::CreateRecordInput::fromAidl(
        const media::CreateRecordRequest& aidl) {
    IAudioFlinger::CreateRecordInput legacy;
    legacy.attr = VALUE_OR_RETURN(aidl2legacy_AudioAttributesInternal_audio_attributes_t(aidl.attr));
    legacy.config = VALUE_OR_RETURN(aidl2legacy_AudioConfigBase_audio_config_base_t(aidl.config));
    legacy.clientInfo = VALUE_OR_RETURN(aidl2legacy_AudioClient(aidl.clientInfo));
    legacy.opPackageName = VALUE_OR_RETURN(aidl2legacy_string_view_String16(aidl.opPackageName));
    legacy.riid = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_unique_id_t(aidl.riid));
    legacy.flags = VALUE_OR_RETURN(aidl2legacy_audio_input_flags_mask(aidl.flags));
    legacy.frameCount = VALUE_OR_RETURN(convertIntegral<size_t>(aidl.frameCount));
    legacy.notificationFrameCount = VALUE_OR_RETURN(
            convertIntegral<size_t>(aidl.notificationFrameCount));
    legacy.selectedDeviceId = VALUE_OR_RETURN(
            aidl2legacy_int32_t_audio_port_handle_t(aidl.selectedDeviceId));
    legacy.sessionId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_session_t(aidl.sessionId));
    return legacy;
}

ConversionResult<media::CreateRecordResponse>
IAudioFlinger::CreateRecordOutput::toAidl() const {
    media::CreateRecordResponse aidl;
    aidl.flags = VALUE_OR_RETURN(legacy2aidl_audio_input_flags_mask(flags));
    aidl.frameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(frameCount));
    aidl.notificationFrameCount = VALUE_OR_RETURN(convertIntegral<int64_t>(notificationFrameCount));
    aidl.selectedDeviceId = VALUE_OR_RETURN(
            legacy2aidl_audio_port_handle_t_int32_t(selectedDeviceId));
    aidl.sessionId = VALUE_OR_RETURN(legacy2aidl_audio_session_t_int32_t(sessionId));
    aidl.sampleRate = VALUE_OR_RETURN(convertIntegral<int32_t>(sampleRate));
    aidl.inputId = VALUE_OR_RETURN(legacy2aidl_audio_io_handle_t_int32_t(inputId));
    aidl.cblk = VALUE_OR_RETURN(legacy2aidl_NullableIMemory_SharedFileRegion(cblk));
    aidl.buffers = VALUE_OR_RETURN(legacy2aidl_NullableIMemory_SharedFileRegion(buffers));
    aidl.portId = VALUE_OR_RETURN(legacy2aidl_audio_port_handle_t_int32_t(portId));
    aidl.audioRecord = audioRecord;
    return aidl;
}

ConversionResult<IAudioFlinger::CreateRecordOutput>
IAudioFlinger::CreateRecordOutput::fromAidl(
        const media::CreateRecordResponse& aidl) {
    IAudioFlinger::CreateRecordOutput legacy;
    legacy.flags = VALUE_OR_RETURN(aidl2legacy_audio_input_flags_mask(aidl.flags));
    legacy.frameCount = VALUE_OR_RETURN(convertIntegral<size_t>(aidl.frameCount));
    legacy.notificationFrameCount = VALUE_OR_RETURN(
            convertIntegral<size_t>(aidl.notificationFrameCount));
    legacy.selectedDeviceId = VALUE_OR_RETURN(
            aidl2legacy_int32_t_audio_port_handle_t(aidl.selectedDeviceId));
    legacy.sessionId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_session_t(aidl.sessionId));
    legacy.sampleRate = VALUE_OR_RETURN(convertIntegral<uint32_t>(aidl.sampleRate));
    legacy.inputId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_io_handle_t(aidl.inputId));
    legacy.cblk = VALUE_OR_RETURN(aidl2legacy_NullableSharedFileRegion_IMemory(aidl.cblk));
    legacy.buffers = VALUE_OR_RETURN(aidl2legacy_NullableSharedFileRegion_IMemory(aidl.buffers));
    legacy.portId = VALUE_OR_RETURN(aidl2legacy_int32_t_audio_port_handle_t(aidl.portId));
    legacy.audioRecord = aidl.audioRecord;
    return legacy;
}

class BpAudioFlinger : public BpInterface<IAudioFlinger>
{
public:
    explicit BpAudioFlinger(const sp<IBinder>& impl)
        : BpInterface<IAudioFlinger>(impl)
    {
    }

    virtual status_t createTrack(const media::CreateTrackRequest& input,
                                 media::CreateTrackResponse& output)
    {
        Parcel data, reply;
        status_t status;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeParcelable(input);

        status_t lStatus = remote()->transact(CREATE_TRACK, data, &reply);
        if (lStatus != NO_ERROR) {
            ALOGE("createTrack transaction error %d", lStatus);
            return DEAD_OBJECT;
        }
        status = reply.readInt32();
        if (status != NO_ERROR) {
            ALOGE("createTrack returned error %d", status);
            return status;
        }
        output.readFromParcel(&reply);
        if (output.audioTrack == 0) {
            ALOGE("createTrack returned an NULL IAudioTrack with status OK");
            return DEAD_OBJECT;
        }
        return OK;
    }

    virtual status_t createRecord(const media::CreateRecordRequest& input,
                                  media::CreateRecordResponse& output)
    {
        Parcel data, reply;
        status_t status;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());

        data.writeParcelable(input);

        status_t lStatus = remote()->transact(CREATE_RECORD, data, &reply);
        if (lStatus != NO_ERROR) {
            ALOGE("createRecord transaction error %d", lStatus);
            return DEAD_OBJECT;
        }
        status = reply.readInt32();
        if (status != NO_ERROR) {
            ALOGE("createRecord returned error %d", status);
            return status;
        }

        output.readFromParcel(&reply);
        if (output.audioRecord == 0) {
            ALOGE("createRecord returned a NULL IAudioRecord with status OK");
            return DEAD_OBJECT;
        }
        return OK;
    }

    virtual uint32_t sampleRate(audio_io_handle_t ioHandle) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) ioHandle);
        remote()->transact(SAMPLE_RATE, data, &reply);
        return reply.readInt32();
    }

    // RESERVED for channelCount()

    virtual audio_format_t format(audio_io_handle_t output) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output);
        remote()->transact(FORMAT, data, &reply);
        return (audio_format_t) reply.readInt32();
    }

    virtual size_t frameCount(audio_io_handle_t ioHandle) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) ioHandle);
        remote()->transact(FRAME_COUNT, data, &reply);
        return reply.readInt64();
    }

    virtual uint32_t latency(audio_io_handle_t output) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output);
        remote()->transact(LATENCY, data, &reply);
        return reply.readInt32();
    }

    virtual status_t setMasterVolume(float value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeFloat(value);
        remote()->transact(SET_MASTER_VOLUME, data, &reply);
        return reply.readInt32();
    }

    virtual status_t setMasterMute(bool muted)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(muted);
        remote()->transact(SET_MASTER_MUTE, data, &reply);
        return reply.readInt32();
    }

    virtual float masterVolume() const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        remote()->transact(MASTER_VOLUME, data, &reply);
        return reply.readFloat();
    }

    virtual bool masterMute() const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        remote()->transact(MASTER_MUTE, data, &reply);
        return reply.readInt32();
    }

    status_t setMasterBalance(float balance) override
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeFloat(balance);
        status_t status = remote()->transact(SET_MASTER_BALANCE, data, &reply);
        if (status != NO_ERROR) {
            return status;
        }
        return reply.readInt32();
    }

    status_t getMasterBalance(float *balance) const override
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        status_t status = remote()->transact(GET_MASTER_BALANCE, data, &reply);
        if (status != NO_ERROR) {
            return status;
        }
        status = (status_t)reply.readInt32();
        if (status != NO_ERROR) {
            return status;
        }
        *balance = reply.readFloat();
        return NO_ERROR;
    }

    virtual status_t setStreamVolume(audio_stream_type_t stream, float value,
            audio_io_handle_t output)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        data.writeFloat(value);
        data.writeInt32((int32_t) output);
        remote()->transact(SET_STREAM_VOLUME, data, &reply);
        return reply.readInt32();
    }

    virtual status_t setStreamMute(audio_stream_type_t stream, bool muted)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        data.writeInt32(muted);
        remote()->transact(SET_STREAM_MUTE, data, &reply);
        return reply.readInt32();
    }

    virtual float streamVolume(audio_stream_type_t stream, audio_io_handle_t output) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        data.writeInt32((int32_t) output);
        remote()->transact(STREAM_VOLUME, data, &reply);
        return reply.readFloat();
    }

    virtual bool streamMute(audio_stream_type_t stream) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        remote()->transact(STREAM_MUTE, data, &reply);
        return reply.readInt32();
    }

    virtual status_t setMode(audio_mode_t mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(mode);
        remote()->transact(SET_MODE, data, &reply);
        return reply.readInt32();
    }

    virtual status_t setMicMute(bool state)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(state);
        remote()->transact(SET_MIC_MUTE, data, &reply);
        return reply.readInt32();
    }

    virtual bool getMicMute() const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        remote()->transact(GET_MIC_MUTE, data, &reply);
        return reply.readInt32();
    }

    virtual void setRecordSilenced(audio_port_handle_t portId, bool silenced)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(portId);
        data.writeInt32(silenced ? 1 : 0);
        remote()->transact(SET_RECORD_SILENCED, data, &reply);
    }

    virtual status_t setParameters(audio_io_handle_t ioHandle, const String8& keyValuePairs)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) ioHandle);
        data.writeString8(keyValuePairs);
        remote()->transact(SET_PARAMETERS, data, &reply);
        return reply.readInt32();
    }

    virtual String8 getParameters(audio_io_handle_t ioHandle, const String8& keys) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) ioHandle);
        data.writeString8(keys);
        remote()->transact(GET_PARAMETERS, data, &reply);
        return reply.readString8();
    }

    virtual void registerClient(const sp<media::IAudioFlingerClient>& client)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(client));
        remote()->transact(REGISTER_CLIENT, data, &reply);
    }

    virtual size_t getInputBufferSize(uint32_t sampleRate, audio_format_t format,
            audio_channel_mask_t channelMask) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(sampleRate);
        data.writeInt32(format);
        data.writeInt32(channelMask);
        remote()->transact(GET_INPUTBUFFERSIZE, data, &reply);
        return reply.readInt64();
    }

    virtual status_t openOutput(const media::OpenOutputRequest& request,
                                media::OpenOutputResponse* response)
    {
        status_t status;
        Parcel data, reply;
        return data.writeParcelable(request)
                ?: remote()->transact(OPEN_OUTPUT, data, &reply)
                ?: data.readInt32(&status)
                ?: status
                ?: data.readParcelable(response);
    }

    virtual audio_io_handle_t openDuplicateOutput(audio_io_handle_t output1,
            audio_io_handle_t output2)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output1);
        data.writeInt32((int32_t) output2);
        remote()->transact(OPEN_DUPLICATE_OUTPUT, data, &reply);
        return (audio_io_handle_t) reply.readInt32();
    }

    virtual status_t closeOutput(audio_io_handle_t output)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output);
        remote()->transact(CLOSE_OUTPUT, data, &reply);
        return reply.readInt32();
    }

    virtual status_t suspendOutput(audio_io_handle_t output)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output);
        remote()->transact(SUSPEND_OUTPUT, data, &reply);
        return reply.readInt32();
    }

    virtual status_t restoreOutput(audio_io_handle_t output)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output);
        remote()->transact(RESTORE_OUTPUT, data, &reply);
        return reply.readInt32();
    }

    virtual status_t openInput(const media::OpenInputRequest& request,
                               media::OpenInputResponse* response)
    {
        Parcel data, reply;
        status_t status;
        return data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor())
            ?: data.writeParcelable(request)
            ?: remote()->transact(OPEN_INPUT, data, &reply)
            ?: reply.readInt32(&status)
            ?: status
            ?: reply.readParcelable(response);
    }

    virtual status_t closeInput(int input)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(input);
        remote()->transact(CLOSE_INPUT, data, &reply);
        return reply.readInt32();
    }

    virtual status_t invalidateStream(audio_stream_type_t stream)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) stream);
        remote()->transact(INVALIDATE_STREAM, data, &reply);
        return reply.readInt32();
    }

    virtual status_t setVoiceVolume(float volume)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeFloat(volume);
        remote()->transact(SET_VOICE_VOLUME, data, &reply);
        return reply.readInt32();
    }

    virtual status_t getRenderPosition(uint32_t *halFrames, uint32_t *dspFrames,
            audio_io_handle_t output) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) output);
        remote()->transact(GET_RENDER_POSITION, data, &reply);
        status_t status = reply.readInt32();
        if (status == NO_ERROR) {
            uint32_t tmp = reply.readInt32();
            if (halFrames != NULL) {
                *halFrames = tmp;
            }
            tmp = reply.readInt32();
            if (dspFrames != NULL) {
                *dspFrames = tmp;
            }
        }
        return status;
    }

    virtual uint32_t getInputFramesLost(audio_io_handle_t ioHandle) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) ioHandle);
        status_t status = remote()->transact(GET_INPUT_FRAMES_LOST, data, &reply);
        if (status != NO_ERROR) {
            return 0;
        }
        return (uint32_t) reply.readInt32();
    }

    virtual audio_unique_id_t newAudioUniqueId(audio_unique_id_use_t use)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) use);
        status_t status = remote()->transact(NEW_AUDIO_UNIQUE_ID, data, &reply);
        audio_unique_id_t id = AUDIO_UNIQUE_ID_ALLOCATE;
        if (status == NO_ERROR) {
            id = reply.readInt32();
        }
        return id;
    }

    void acquireAudioSessionId(audio_session_t audioSession, pid_t pid, uid_t uid) override
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(audioSession);
        data.writeInt32((int32_t)pid);
        data.writeInt32((int32_t)uid);
        remote()->transact(ACQUIRE_AUDIO_SESSION_ID, data, &reply);
    }

    virtual void releaseAudioSessionId(audio_session_t audioSession, int pid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(audioSession);
        data.writeInt32(pid);
        remote()->transact(RELEASE_AUDIO_SESSION_ID, data, &reply);
    }

    virtual status_t queryNumberEffects(uint32_t *numEffects) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        status_t status = remote()->transact(QUERY_NUM_EFFECTS, data, &reply);
        if (status != NO_ERROR) {
            return status;
        }
        status = reply.readInt32();
        if (status != NO_ERROR) {
            return status;
        }
        if (numEffects != NULL) {
            *numEffects = (uint32_t)reply.readInt32();
        }
        return NO_ERROR;
    }

    virtual status_t queryEffect(uint32_t index, effect_descriptor_t *pDescriptor) const
    {
        if (pDescriptor == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(index);
        status_t status = remote()->transact(QUERY_EFFECT, data, &reply);
        if (status != NO_ERROR) {
            return status;
        }
        status = reply.readInt32();
        if (status != NO_ERROR) {
            return status;
        }
        reply.read(pDescriptor, sizeof(effect_descriptor_t));
        return NO_ERROR;
    }

    virtual status_t getEffectDescriptor(const effect_uuid_t *pUuid,
                                         const effect_uuid_t *pType,
                                         uint32_t preferredTypeFlag,
                                         effect_descriptor_t *pDescriptor) const
    {
        if (pUuid == NULL || pType == NULL || pDescriptor == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.write(pUuid, sizeof(effect_uuid_t));
        data.write(pType, sizeof(effect_uuid_t));
        data.writeUint32(preferredTypeFlag);
        status_t status = remote()->transact(GET_EFFECT_DESCRIPTOR, data, &reply);
        if (status != NO_ERROR) {
            return status;
        }
        status = reply.readInt32();
        if (status != NO_ERROR) {
            return status;
        }
        reply.read(pDescriptor, sizeof(effect_descriptor_t));
        return NO_ERROR;
    }

    virtual status_t createEffect(const media::CreateEffectRequest& request,
                                  media::CreateEffectResponse* response)
    {
        Parcel data, reply;
        sp<media::IEffect> effect;
        if (response == nullptr) {
            return BAD_VALUE;
        }
        status_t status;
        status_t lStatus = data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor())
                           ?: data.writeParcelable(request)
                           ?: remote()->transact(CREATE_EFFECT, data, &reply)
                           ?: reply.readInt32(&status)
                           ?: reply.readParcelable(response)
                           ?: status;
        if (lStatus != NO_ERROR) {
            ALOGE("createEffect error: %s", strerror(-lStatus));
        }
        return lStatus;
    }

    virtual status_t moveEffects(audio_session_t session, audio_io_handle_t srcOutput,
            audio_io_handle_t dstOutput)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(session);
        data.writeInt32((int32_t) srcOutput);
        data.writeInt32((int32_t) dstOutput);
        remote()->transact(MOVE_EFFECTS, data, &reply);
        return reply.readInt32();
    }

    virtual void setEffectSuspended(int effectId,
                                    audio_session_t sessionId,
                                    bool suspended)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(effectId);
        data.writeInt32(sessionId);
        data.writeInt32(suspended ? 1 : 0);
        remote()->transact(SET_EFFECT_SUSPENDED, data, &reply);
    }

    virtual audio_module_handle_t loadHwModule(const char *name)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeCString(name);
        remote()->transact(LOAD_HW_MODULE, data, &reply);
        return (audio_module_handle_t) reply.readInt32();
    }

    virtual uint32_t getPrimaryOutputSamplingRate()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        remote()->transact(GET_PRIMARY_OUTPUT_SAMPLING_RATE, data, &reply);
        return reply.readInt32();
    }

    virtual size_t getPrimaryOutputFrameCount()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        remote()->transact(GET_PRIMARY_OUTPUT_FRAME_COUNT, data, &reply);
        return reply.readInt64();
    }

    virtual status_t setLowRamDevice(bool isLowRamDevice, int64_t totalMemory) override
    {
        Parcel data, reply;

        static_assert(NO_ERROR == 0, "NO_ERROR must be 0");
        return data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor())
                ?: data.writeInt32((int) isLowRamDevice)
                ?: data.writeInt64(totalMemory)
                ?: remote()->transact(SET_LOW_RAM_DEVICE, data, &reply)
                ?: reply.readInt32();
    }

    virtual status_t listAudioPorts(unsigned int *num_ports,
                                    struct audio_port *ports)
    {
        if (num_ports == NULL || *num_ports == 0 || ports == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(*num_ports);
        status_t status = remote()->transact(LIST_AUDIO_PORTS, data, &reply);
        if (status != NO_ERROR ||
                (status = (status_t)reply.readInt32()) != NO_ERROR) {
            return status;
        }
        *num_ports = (unsigned int)reply.readInt32();
        reply.read(ports, *num_ports * sizeof(struct audio_port));
        return status;
    }
    virtual status_t getAudioPort(struct audio_port_v7 *port)
    {
        if (port == nullptr) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.write(port, sizeof(struct audio_port_v7));
        status_t status = remote()->transact(GET_AUDIO_PORT, data, &reply);
        if (status != NO_ERROR ||
                (status = (status_t)reply.readInt32()) != NO_ERROR) {
            return status;
        }
        reply.read(port, sizeof(struct audio_port));
        return status;
    }
    virtual status_t createAudioPatch(const struct audio_patch *patch,
                                       audio_patch_handle_t *handle)
    {
        if (patch == NULL || handle == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.write(patch, sizeof(struct audio_patch));
        data.write(handle, sizeof(audio_patch_handle_t));
        status_t status = remote()->transact(CREATE_AUDIO_PATCH, data, &reply);
        if (status != NO_ERROR ||
                (status = (status_t)reply.readInt32()) != NO_ERROR) {
            return status;
        }
        reply.read(handle, sizeof(audio_patch_handle_t));
        return status;
    }
    virtual status_t releaseAudioPatch(audio_patch_handle_t handle)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.write(&handle, sizeof(audio_patch_handle_t));
        status_t status = remote()->transact(RELEASE_AUDIO_PATCH, data, &reply);
        if (status != NO_ERROR) {
            status = (status_t)reply.readInt32();
        }
        return status;
    }
    virtual status_t listAudioPatches(unsigned int *num_patches,
                                      struct audio_patch *patches)
    {
        if (num_patches == NULL || *num_patches == 0 || patches == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(*num_patches);
        status_t status = remote()->transact(LIST_AUDIO_PATCHES, data, &reply);
        if (status != NO_ERROR ||
                (status = (status_t)reply.readInt32()) != NO_ERROR) {
            return status;
        }
        *num_patches = (unsigned int)reply.readInt32();
        reply.read(patches, *num_patches * sizeof(struct audio_patch));
        return status;
    }
    virtual status_t setAudioPortConfig(const struct audio_port_config *config)
    {
        if (config == NULL) {
            return BAD_VALUE;
        }
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.write(config, sizeof(struct audio_port_config));
        status_t status = remote()->transact(SET_AUDIO_PORT_CONFIG, data, &reply);
        if (status != NO_ERROR) {
            status = (status_t)reply.readInt32();
        }
        return status;
    }
    virtual audio_hw_sync_t getAudioHwSyncForSession(audio_session_t sessionId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(sessionId);
        status_t status = remote()->transact(GET_AUDIO_HW_SYNC_FOR_SESSION, data, &reply);
        if (status != NO_ERROR) {
            return AUDIO_HW_SYNC_INVALID;
        }
        return (audio_hw_sync_t)reply.readInt32();
    }
    virtual status_t systemReady()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        return remote()->transact(SYSTEM_READY, data, &reply, IBinder::FLAG_ONEWAY);
    }
    virtual size_t frameCountHAL(audio_io_handle_t ioHandle) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32((int32_t) ioHandle);
        status_t status = remote()->transact(FRAME_COUNT_HAL, data, &reply);
        if (status != NO_ERROR) {
            return 0;
        }
        return reply.readInt64();
    }
    virtual status_t getMicrophones(std::vector<media::MicrophoneInfo> *microphones)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        status_t status = remote()->transact(GET_MICROPHONES, data, &reply);
        if (status != NO_ERROR ||
                (status = (status_t)reply.readInt32()) != NO_ERROR) {
            return status;
        }
        status = reply.readParcelableVector(microphones);
        return status;
    }
    virtual status_t setAudioHalPids(const std::vector<pid_t>& pids)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IAudioFlinger::getInterfaceDescriptor());
        data.writeInt32(pids.size());
        for (auto pid : pids) {
            data.writeInt32(pid);
        }
        status_t status = remote()->transact(SET_AUDIO_HAL_PIDS, data, &reply);
        if (status != NO_ERROR) {
            return status;
        }
        return static_cast <status_t> (reply.readInt32());
    }
};

IMPLEMENT_META_INTERFACE(AudioFlinger, "android.media.IAudioFlinger");

// ----------------------------------------------------------------------

status_t BnAudioFlinger::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case CREATE_TRACK: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);

            media::CreateTrackRequest input;
            if (data.readParcelable(&input) != NO_ERROR) {
                reply->writeInt32(DEAD_OBJECT);
                return NO_ERROR;
            }

            status_t status;
            media::CreateTrackResponse output;

            status = createTrack(input, output);

            LOG_ALWAYS_FATAL_IF((output.audioTrack != 0) != (status == NO_ERROR));
            reply->writeInt32(status);
            if (status != NO_ERROR) {
                return NO_ERROR;
            }
            output.writeToParcel(reply);
            return NO_ERROR;
        } break;
        case CREATE_RECORD: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);

            media::CreateRecordRequest input;
            if (data.readParcelable(&input) != NO_ERROR) {
                reply->writeInt32(DEAD_OBJECT);
                return NO_ERROR;
            }

            status_t status;
            media::CreateRecordResponse output;

            status = createRecord(input, output);

            LOG_ALWAYS_FATAL_IF((output.audioRecord != 0) != (status == NO_ERROR));
            reply->writeInt32(status);
            if (status != NO_ERROR) {
                return NO_ERROR;
            }
            output.writeToParcel(reply);
            return NO_ERROR;
        } break;
        case SAMPLE_RATE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( sampleRate((audio_io_handle_t) data.readInt32()) );
            return NO_ERROR;
        } break;

        // RESERVED for channelCount()

        case FORMAT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( format((audio_io_handle_t) data.readInt32()) );
            return NO_ERROR;
        } break;
        case FRAME_COUNT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt64( frameCount((audio_io_handle_t) data.readInt32()) );
            return NO_ERROR;
        } break;
        case LATENCY: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( latency((audio_io_handle_t) data.readInt32()) );
            return NO_ERROR;
        } break;
        case SET_MASTER_VOLUME: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( setMasterVolume(data.readFloat()) );
            return NO_ERROR;
        } break;
        case SET_MASTER_MUTE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( setMasterMute(data.readInt32()) );
            return NO_ERROR;
        } break;
        case MASTER_VOLUME: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeFloat( masterVolume() );
            return NO_ERROR;
        } break;
        case MASTER_MUTE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( masterMute() );
            return NO_ERROR;
        } break;
        case SET_MASTER_BALANCE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( setMasterBalance(data.readFloat()) );
            return NO_ERROR;
        } break;
        case GET_MASTER_BALANCE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            float f;
            const status_t status = getMasterBalance(&f);
            reply->writeInt32((int32_t)status);
            if (status == NO_ERROR) {
                (void)reply->writeFloat(f);
            }
            return NO_ERROR;
        } break;
        case SET_STREAM_VOLUME: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int stream = data.readInt32();
            float volume = data.readFloat();
            audio_io_handle_t output = (audio_io_handle_t) data.readInt32();
            reply->writeInt32( setStreamVolume((audio_stream_type_t) stream, volume, output) );
            return NO_ERROR;
        } break;
        case SET_STREAM_MUTE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int stream = data.readInt32();
            reply->writeInt32( setStreamMute((audio_stream_type_t) stream, data.readInt32()) );
            return NO_ERROR;
        } break;
        case STREAM_VOLUME: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int stream = data.readInt32();
            int output = data.readInt32();
            reply->writeFloat( streamVolume((audio_stream_type_t) stream, output) );
            return NO_ERROR;
        } break;
        case STREAM_MUTE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int stream = data.readInt32();
            reply->writeInt32( streamMute((audio_stream_type_t) stream) );
            return NO_ERROR;
        } break;
        case SET_MODE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_mode_t mode = (audio_mode_t) data.readInt32();
            reply->writeInt32( setMode(mode) );
            return NO_ERROR;
        } break;
        case SET_MIC_MUTE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int state = data.readInt32();
            reply->writeInt32( setMicMute(state) );
            return NO_ERROR;
        } break;
        case GET_MIC_MUTE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32( getMicMute() );
            return NO_ERROR;
        } break;
        case SET_RECORD_SILENCED: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_port_handle_t portId = data.readInt32();
            bool silenced = data.readInt32() == 1;
            setRecordSilenced(portId, silenced);
            return NO_ERROR;
        } break;
        case SET_PARAMETERS: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_io_handle_t ioHandle = (audio_io_handle_t) data.readInt32();
            String8 keyValuePairs(data.readString8());
            reply->writeInt32(setParameters(ioHandle, keyValuePairs));
            return NO_ERROR;
        } break;
        case GET_PARAMETERS: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_io_handle_t ioHandle = (audio_io_handle_t) data.readInt32();
            String8 keys(data.readString8());
            reply->writeString8(getParameters(ioHandle, keys));
            return NO_ERROR;
        } break;

        case REGISTER_CLIENT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            sp<media::IAudioFlingerClient> client = interface_cast<media::IAudioFlingerClient>(
                    data.readStrongBinder());
            registerClient(client);
            return NO_ERROR;
        } break;
        case GET_INPUTBUFFERSIZE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            uint32_t sampleRate = data.readInt32();
            audio_format_t format = (audio_format_t) data.readInt32();
            audio_channel_mask_t channelMask = (audio_channel_mask_t) data.readInt32();
            reply->writeInt64( getInputBufferSize(sampleRate, format, channelMask) );
            return NO_ERROR;
        } break;
        case OPEN_OUTPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            status_t status;
            media::OpenOutputRequest request;
            media::OpenOutputResponse response;
            return data.readParcelable(&request)
                ?: (status = openOutput(request, &response), OK)
                ?: reply->writeInt32(status)
                ?: reply->writeParcelable(response);
        } break;
        case OPEN_DUPLICATE_OUTPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_io_handle_t output1 = (audio_io_handle_t) data.readInt32();
            audio_io_handle_t output2 = (audio_io_handle_t) data.readInt32();
            reply->writeInt32((int32_t) openDuplicateOutput(output1, output2));
            return NO_ERROR;
        } break;
        case CLOSE_OUTPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(closeOutput((audio_io_handle_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case SUSPEND_OUTPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(suspendOutput((audio_io_handle_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case RESTORE_OUTPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(restoreOutput((audio_io_handle_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case OPEN_INPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            media::OpenInputRequest request;
            media::OpenInputResponse response;
            status_t status;
            return data.readParcelable(&request)
                ?: (status = openInput(request, &response), OK)
                ?: reply->writeInt32(status)
                ?: reply->writeParcelable(response);
        } break;
        case CLOSE_INPUT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(closeInput((audio_io_handle_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case INVALIDATE_STREAM: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_stream_type_t stream = (audio_stream_type_t) data.readInt32();
            reply->writeInt32(invalidateStream(stream));
            return NO_ERROR;
        } break;
        case SET_VOICE_VOLUME: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            float volume = data.readFloat();
            reply->writeInt32( setVoiceVolume(volume) );
            return NO_ERROR;
        } break;
        case GET_RENDER_POSITION: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_io_handle_t output = (audio_io_handle_t) data.readInt32();
            uint32_t halFrames = 0;
            uint32_t dspFrames = 0;
            status_t status = getRenderPosition(&halFrames, &dspFrames, output);
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->writeInt32(halFrames);
                reply->writeInt32(dspFrames);
            }
            return NO_ERROR;
        }
        case GET_INPUT_FRAMES_LOST: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_io_handle_t ioHandle = (audio_io_handle_t) data.readInt32();
            reply->writeInt32((int32_t) getInputFramesLost(ioHandle));
            return NO_ERROR;
        } break;
        case NEW_AUDIO_UNIQUE_ID: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(newAudioUniqueId((audio_unique_id_use_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case ACQUIRE_AUDIO_SESSION_ID: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_session_t audioSession = (audio_session_t) data.readInt32();
            const pid_t pid = (pid_t)data.readInt32();
            const uid_t uid = (uid_t)data.readInt32();
            acquireAudioSessionId(audioSession, pid, uid);
            return NO_ERROR;
        } break;
        case RELEASE_AUDIO_SESSION_ID: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_session_t audioSession = (audio_session_t) data.readInt32();
            int pid = data.readInt32();
            releaseAudioSessionId(audioSession, pid);
            return NO_ERROR;
        } break;
        case QUERY_NUM_EFFECTS: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            uint32_t numEffects = 0;
            status_t status = queryNumberEffects(&numEffects);
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->writeInt32((int32_t)numEffects);
            }
            return NO_ERROR;
        }
        case QUERY_EFFECT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            effect_descriptor_t desc = {};
            status_t status = queryEffect(data.readInt32(), &desc);
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->write(&desc, sizeof(effect_descriptor_t));
            }
            return NO_ERROR;
        }
        case GET_EFFECT_DESCRIPTOR: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            effect_uuid_t uuid = {};
            if (data.read(&uuid, sizeof(effect_uuid_t)) != NO_ERROR) {
                android_errorWriteLog(0x534e4554, "139417189");
            }
            effect_uuid_t type = {};
            if (data.read(&type, sizeof(effect_uuid_t)) != NO_ERROR) {
                android_errorWriteLog(0x534e4554, "139417189");
            }
            uint32_t preferredTypeFlag = data.readUint32();
            effect_descriptor_t desc = {};
            status_t status = getEffectDescriptor(&uuid, &type, preferredTypeFlag, &desc);
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->write(&desc, sizeof(effect_descriptor_t));
            }
            return NO_ERROR;
        }
        case CREATE_EFFECT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);

            media::CreateEffectRequest request;
            media::CreateEffectResponse response;

            return data.readParcelable(&request)
                ?: reply->writeInt32(createEffect(request, &response))
                ?: reply->writeParcelable(response);
        } break;
        case MOVE_EFFECTS: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_session_t session = (audio_session_t) data.readInt32();
            audio_io_handle_t srcOutput = (audio_io_handle_t) data.readInt32();
            audio_io_handle_t dstOutput = (audio_io_handle_t) data.readInt32();
            reply->writeInt32(moveEffects(session, srcOutput, dstOutput));
            return NO_ERROR;
        } break;
        case SET_EFFECT_SUSPENDED: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int effectId = data.readInt32();
            audio_session_t sessionId = (audio_session_t) data.readInt32();
            bool suspended = data.readInt32() == 1;
            setEffectSuspended(effectId, sessionId, suspended);
            return NO_ERROR;
        } break;
        case LOAD_HW_MODULE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(loadHwModule(data.readCString()));
            return NO_ERROR;
        } break;
        case GET_PRIMARY_OUTPUT_SAMPLING_RATE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(getPrimaryOutputSamplingRate());
            return NO_ERROR;
        } break;
        case GET_PRIMARY_OUTPUT_FRAME_COUNT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt64(getPrimaryOutputFrameCount());
            return NO_ERROR;
        } break;
        case SET_LOW_RAM_DEVICE: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            int32_t isLowRamDevice;
            int64_t totalMemory;
            const status_t status =
                    data.readInt32(&isLowRamDevice) ?:
                    data.readInt64(&totalMemory) ?:
                    setLowRamDevice(isLowRamDevice != 0, totalMemory);
            (void)reply->writeInt32(status);
            return NO_ERROR;
        } break;
        case LIST_AUDIO_PORTS: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            unsigned int numPortsReq = data.readInt32();
            if (numPortsReq > MAX_ITEMS_PER_LIST) {
                numPortsReq = MAX_ITEMS_PER_LIST;
            }
            unsigned int numPorts = numPortsReq;
            struct audio_port *ports =
                    (struct audio_port *)calloc(numPortsReq,
                                                           sizeof(struct audio_port));
            if (ports == NULL) {
                reply->writeInt32(NO_MEMORY);
                reply->writeInt32(0);
                return NO_ERROR;
            }
            status_t status = listAudioPorts(&numPorts, ports);
            reply->writeInt32(status);
            reply->writeInt32(numPorts);
            if (status == NO_ERROR) {
                if (numPortsReq > numPorts) {
                    numPortsReq = numPorts;
                }
                reply->write(ports, numPortsReq * sizeof(struct audio_port));
            }
            free(ports);
            return NO_ERROR;
        } break;
        case GET_AUDIO_PORT: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            struct audio_port_v7 port = {};
            status_t status = data.read(&port, sizeof(struct audio_port));
            if (status != NO_ERROR) {
                ALOGE("b/23905951");
                return status;
            }
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->write(&port, sizeof(struct audio_port_v7));
            }
            return NO_ERROR;
        } break;
        case CREATE_AUDIO_PATCH: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            struct audio_patch patch;
            status_t status = data.read(&patch, sizeof(struct audio_patch));
            if (status != NO_ERROR) {
                return status;
            }
            audio_patch_handle_t handle = AUDIO_PATCH_HANDLE_NONE;
            status = data.read(&handle, sizeof(audio_patch_handle_t));
            if (status != NO_ERROR) {
                ALOGE("b/23905951");
                return status;
            }
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->write(&handle, sizeof(audio_patch_handle_t));
            }
            return NO_ERROR;
        } break;
        case RELEASE_AUDIO_PATCH: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            audio_patch_handle_t handle;
            data.read(&handle, sizeof(audio_patch_handle_t));
            status_t status = releaseAudioPatch(handle);
            reply->writeInt32(status);
            return NO_ERROR;
        } break;
        case LIST_AUDIO_PATCHES: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            unsigned int numPatchesReq = data.readInt32();
            if (numPatchesReq > MAX_ITEMS_PER_LIST) {
                numPatchesReq = MAX_ITEMS_PER_LIST;
            }
            unsigned int numPatches = numPatchesReq;
            struct audio_patch *patches =
                    (struct audio_patch *)calloc(numPatchesReq,
                                                 sizeof(struct audio_patch));
            if (patches == NULL) {
                reply->writeInt32(NO_MEMORY);
                reply->writeInt32(0);
                return NO_ERROR;
            }
            status_t status = listAudioPatches(&numPatches, patches);
            reply->writeInt32(status);
            reply->writeInt32(numPatches);
            if (status == NO_ERROR) {
                if (numPatchesReq > numPatches) {
                    numPatchesReq = numPatches;
                }
                reply->write(patches, numPatchesReq * sizeof(struct audio_patch));
            }
            free(patches);
            return NO_ERROR;
        } break;
        case SET_AUDIO_PORT_CONFIG: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            struct audio_port_config config;
            status_t status = data.read(&config, sizeof(struct audio_port_config));
            if (status != NO_ERROR) {
                return status;
            }
            reply->writeInt32(status);
            return NO_ERROR;
        } break;
        case GET_AUDIO_HW_SYNC_FOR_SESSION: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt32(getAudioHwSyncForSession((audio_session_t) data.readInt32()));
            return NO_ERROR;
        } break;
        case SYSTEM_READY: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            systemReady();
            return NO_ERROR;
        } break;
        case FRAME_COUNT_HAL: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            reply->writeInt64( frameCountHAL((audio_io_handle_t) data.readInt32()) );
            return NO_ERROR;
        } break;
        case GET_MICROPHONES: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            std::vector<media::MicrophoneInfo> microphones;
            status_t status = getMicrophones(&microphones);
            reply->writeInt32(status);
            if (status == NO_ERROR) {
                reply->writeParcelableVector(microphones);
            }
            return NO_ERROR;
        }
        case SET_AUDIO_HAL_PIDS: {
            CHECK_INTERFACE(IAudioFlinger, data, reply);
            std::vector<pid_t> pids;
            int32_t size;
            status_t status = data.readInt32(&size);
            if (status != NO_ERROR) {
                return status;
            }
            if (size < 0) {
                return BAD_VALUE;
            }
            if (size > MAX_ITEMS_PER_LIST) {
                size = MAX_ITEMS_PER_LIST;
            }
            for (int32_t i = 0; i < size; i++) {
                int32_t pid;
                status =  data.readInt32(&pid);
                if (status != NO_ERROR) {
                    return status;
                }
                pids.push_back(pid);
            }
            reply->writeInt32(setAudioHalPids(pids));
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

} // namespace android
