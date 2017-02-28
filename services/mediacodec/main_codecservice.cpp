/*
**
** Copyright 2016, The Android Open Source Project
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

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>

#include <string>

#include <android-base/logging.h>

// from LOCAL_C_INCLUDES
#include "MediaCodecService.h"
#include "minijail.h"

#include <android/hardware/media/omx/1.0/IOmx.h>
#include <hidl/HidlTransportSupport.h>
#include <omx/hal/1.0/impl/Omx.h>

using namespace android;

// Must match location in Android.mk.
static const char kSeccompPolicyPath[] = "/system/etc/seccomp_policy/mediacodec-seccomp.policy";

int main(int argc __unused, char** argv)
{
    LOG(INFO) << "mediacodecservice starting";
    signal(SIGPIPE, SIG_IGN);
    SetUpMinijail(kSeccompPolicyPath, std::string());

    strcpy(argv[0], "media.codec");

    ::android::hardware::configureRpcThreadpool(64, false);
    sp<ProcessState> proc(ProcessState::self());

    int32_t trebleOmx = property_get_int32("persist.media.treble_omx", -1);
    if ((trebleOmx == 1) || ((trebleOmx == -1) &&
            property_get_bool("omx.binderization", 0))) {
        using namespace ::android::hardware::media::omx::V1_0;
        sp<IOmx> omx = new implementation::Omx();
        if (omx == nullptr) {
            LOG(ERROR) << "Cannot create a Treble IOmx service.";
        } else if (omx->registerAsService() != OK) {
            LOG(ERROR) << "Cannot register a Treble IOmx service.";
        } else {
            LOG(INFO) << "Treble IOmx service created.";
        }
    } else {
        MediaCodecService::instantiate();
        LOG(INFO) << "Non-Treble IOMX service created.";
    }

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
