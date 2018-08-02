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

#include <memory>
#include <set>
#include <sys/wait.h>
#include <unistd.h>

#include <gtest/gtest.h>

#define LOG_TAG "APM_Test"
#include <log/log.h>
#include <media/PatchBuilder.h>

#include "AudioPolicyTestClient.h"
#include "AudioPolicyTestManager.h"

using namespace android;

TEST(AudioPolicyManagerTestInit, Failure) {
    AudioPolicyTestClient client;
    AudioPolicyTestManager manager(&client);
    manager.getConfig().setDefault();
    // Since the default client fails to open anything,
    // APM should indicate that the initialization didn't succeed.
    ASSERT_EQ(NO_INIT, manager.initialize());
    ASSERT_EQ(NO_INIT, manager.initCheck());
}


class AudioPolicyManagerTestClient : public AudioPolicyTestClient {
  public:
    // AudioPolicyClientInterface implementation
    audio_module_handle_t loadHwModule(const char* /*name*/) override {
        return mNextModuleHandle++;
    }

    status_t openOutput(audio_module_handle_t module,
                        audio_io_handle_t* output,
                        audio_config_t* /*config*/,
                        audio_devices_t* /*devices*/,
                        const String8& /*address*/,
                        uint32_t* /*latencyMs*/,
                        audio_output_flags_t /*flags*/) override {
        if (module >= mNextModuleHandle) {
            ALOGE("%s: Module handle %d has not been allocated yet (next is %d)",
                    __func__, module, mNextModuleHandle);
            return BAD_VALUE;
        }
        *output = mNextIoHandle++;
        return NO_ERROR;
    }

    status_t openInput(audio_module_handle_t module,
                       audio_io_handle_t* input,
                       audio_config_t* /*config*/,
                       audio_devices_t* /*device*/,
                       const String8& /*address*/,
                       audio_source_t /*source*/,
                       audio_input_flags_t /*flags*/) override {
        if (module >= mNextModuleHandle) {
            ALOGE("%s: Module handle %d has not been allocated yet (next is %d)",
                    __func__, module, mNextModuleHandle);
            return BAD_VALUE;
        }
        *input = mNextIoHandle++;
        return NO_ERROR;
    }

    status_t createAudioPatch(const struct audio_patch* /*patch*/,
                              audio_patch_handle_t* handle,
                              int /*delayMs*/) override {
        *handle = mNextPatchHandle++;
        mActivePatches.insert(*handle);
        return NO_ERROR;
    }

    status_t releaseAudioPatch(audio_patch_handle_t handle,
                               int /*delayMs*/) override {
        if (mActivePatches.erase(handle) != 1) {
            if (handle >= mNextPatchHandle) {
                ALOGE("%s: Patch handle %d has not been allocated yet (next is %d)",
                        __func__, handle, mNextPatchHandle);
            } else {
                ALOGE("%s: Attempt to release patch %d twice", __func__, handle);
            }
            return BAD_VALUE;
        }
        return NO_ERROR;
    }

    // Helper methods for tests
    size_t getActivePatchesCount() const { return mActivePatches.size(); }

  private:
    audio_module_handle_t mNextModuleHandle = AUDIO_MODULE_HANDLE_NONE + 1;
    audio_io_handle_t mNextIoHandle = AUDIO_IO_HANDLE_NONE + 1;
    audio_patch_handle_t mNextPatchHandle = AUDIO_PATCH_HANDLE_NONE + 1;
    std::set<audio_patch_handle_t> mActivePatches;
};

class AudioPolicyManagerTest : public testing::Test {
  protected:
    virtual void SetUp();
    virtual void TearDown();

    std::unique_ptr<AudioPolicyManagerTestClient> mClient;
    std::unique_ptr<AudioPolicyTestManager> mManager;
};

void AudioPolicyManagerTest::SetUp() {
    mClient.reset(new AudioPolicyManagerTestClient);
    mManager.reset(new AudioPolicyTestManager(mClient.get()));
    mManager->getConfig().setDefault();
    ASSERT_EQ(NO_ERROR, mManager->initialize());
    ASSERT_EQ(NO_ERROR, mManager->initCheck());
}

void AudioPolicyManagerTest::TearDown() {
    mManager.reset();
    mClient.reset();
}

TEST_F(AudioPolicyManagerTest, InitSuccess) {
    // SetUp must finish with no assertions.
}

TEST_F(AudioPolicyManagerTest, Dump) {
    int pipefd[2];
    ASSERT_NE(-1, pipe(pipefd));
    pid_t cpid = fork();
    ASSERT_NE(-1, cpid);
    if (cpid == 0) {
        // Child process reads from the pipe and logs.
        close(pipefd[1]);
        std::string line;
        char buf;
        while (read(pipefd[0], &buf, sizeof(buf)) > 0) {
            if (buf != '\n') {
                line += buf;
            } else {
                ALOGI("%s", line.c_str());
                line = "";
            }
        }
        if (!line.empty()) ALOGI("%s", line.c_str());
        close(pipefd[0]);
        _exit(EXIT_SUCCESS);
    } else {
        // Parent does the dump and checks the status code.
        close(pipefd[0]);
        ASSERT_EQ(NO_ERROR, mManager->dump(pipefd[1]));
        close(pipefd[1]);
        wait(NULL);  // Wait for the child to exit.
    }
}

TEST_F(AudioPolicyManagerTest, CreateAudioPatchFailure) {
    audio_patch patch{};
    audio_patch_handle_t handle = AUDIO_PATCH_HANDLE_NONE;
    const size_t patchCountBefore = mClient->getActivePatchesCount();
    ASSERT_EQ(BAD_VALUE, mManager->createAudioPatch(nullptr, &handle, 0));
    ASSERT_EQ(BAD_VALUE, mManager->createAudioPatch(&patch, nullptr, 0));
    ASSERT_EQ(BAD_VALUE, mManager->createAudioPatch(&patch, &handle, 0));
    patch.num_sources = AUDIO_PATCH_PORTS_MAX + 1;
    patch.num_sinks = 1;
    ASSERT_EQ(BAD_VALUE, mManager->createAudioPatch(&patch, &handle, 0));
    patch.num_sources = 1;
    patch.num_sinks = AUDIO_PATCH_PORTS_MAX + 1;
    ASSERT_EQ(BAD_VALUE, mManager->createAudioPatch(&patch, &handle, 0));
    patch.num_sources = 2;
    patch.num_sinks = 1;
    ASSERT_EQ(INVALID_OPERATION, mManager->createAudioPatch(&patch, &handle, 0));
    patch = {};
    patch.num_sources = 1;
    patch.sources[0].role = AUDIO_PORT_ROLE_SINK;
    patch.num_sinks = 1;
    patch.sinks[0].role = AUDIO_PORT_ROLE_SINK;
    ASSERT_EQ(INVALID_OPERATION, mManager->createAudioPatch(&patch, &handle, 0));
    patch = {};
    patch.num_sources = 1;
    patch.sources[0].role = AUDIO_PORT_ROLE_SOURCE;
    patch.num_sinks = 1;
    patch.sinks[0].role = AUDIO_PORT_ROLE_SOURCE;
    ASSERT_EQ(INVALID_OPERATION, mManager->createAudioPatch(&patch, &handle, 0));
    // Verify that the handle is left unchanged.
    ASSERT_EQ(AUDIO_PATCH_HANDLE_NONE, handle);
    ASSERT_EQ(patchCountBefore, mClient->getActivePatchesCount());
}

TEST_F(AudioPolicyManagerTest, CreateAudioPatchFromMix) {
    audio_patch_handle_t handle = AUDIO_PATCH_HANDLE_NONE;
    uid_t uid = 42;
    const size_t patchCountBefore = mClient->getActivePatchesCount();
    ASSERT_FALSE(mManager->getConfig().getAvailableInputDevices().isEmpty());
    PatchBuilder patchBuilder;
    patchBuilder.addSource(mManager->getConfig().getAvailableInputDevices()[0]).
            addSink(mManager->getConfig().getDefaultOutputDevice());
    ASSERT_EQ(NO_ERROR, mManager->createAudioPatch(patchBuilder.patch(), &handle, uid));
    ASSERT_NE(AUDIO_PATCH_HANDLE_NONE, handle);
    ASSERT_EQ(patchCountBefore + 1, mClient->getActivePatchesCount());
}

// TODO: Add patch creation tests that involve already existing patch
