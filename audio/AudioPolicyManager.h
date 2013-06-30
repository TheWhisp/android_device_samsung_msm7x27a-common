/*
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (C) 2012, Code Aurora Forum. All rights reserved.
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


#include <stdint.h>
#include <sys/types.h>
#include <cutils/config_utils.h>
#include <cutils/misc.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <hardware_legacy/AudioPolicyManagerBase.h>


namespace android_audio_legacy {

enum fm_modes{
   FM_DIGITAL=1,
   FM_ANALOG,
   FM_NONE
};

class AudioPolicyManager: public AudioPolicyManagerBase
{

public:
                AudioPolicyManager(AudioPolicyClientInterface *clientInterface)
                : AudioPolicyManagerBase(clientInterface),fmMode(FM_NONE){}

        virtual ~AudioPolicyManager() {}

        virtual status_t setDeviceConnectionState(audio_devices_t device,
                                                           AudioSystem::device_connection_state state,
                                                           const char *device_address);
        virtual AudioSystem::device_connection_state getDeviceConnectionState(audio_devices_t device,
                                                                              const char *device_address);
        virtual audio_io_handle_t getInput(int inputSource,
                                            uint32_t samplingRate,
                                            uint32_t format,
                                            uint32_t channels,
                                            AudioSystem::audio_in_acoustics acoustics);

        virtual audio_devices_t getDeviceForVolume(audio_devices_t device);

        virtual uint32_t  checkDeviceMuteStrategies(AudioOutputDescriptor *outputDesc,
                                            audio_devices_t prevDevice,
                                            uint32_t delayMs);
        virtual void setForceUse(AudioSystem::force_use usage, AudioSystem::forced_config config);
protected:
        virtual audio_devices_t getDeviceForStrategy(routing_strategy strategy, bool fromCache = true);

        fm_modes fmMode;

#ifdef WITH_A2DP
        // true is current platform supports suplication of notifications and ringtones over A2DP output
        //virtual bool a2dpUsedForSonification() const { return true; }
#endif
        // when a device is connected, checks if an open output can be routed
        // to this device. If none is open, tries to open one of the available outputs.
        // Returns an output suitable to this device or 0.
        // when a device is disconnected, checks if an output is not used any more and
        // returns its handle if any.
        // transfers the audio tracks and effects from one output thread to another accordingly.
        status_t checkOutputsForDevice(audio_devices_t device,
                                       AudioSystem::device_connection_state state,
                                       SortedVector<audio_io_handle_t>& outputs);

        virtual AudioPolicyManagerBase::IOProfile* getProfileForDirectOutput(
                                                     audio_devices_t device,
                                                     uint32_t samplingRate,
                                                     uint32_t format,
                                                     uint32_t channelMask,
                                                     audio_output_flags_t flags);


        bool    isCompatibleProfile(AudioPolicyManagerBase::IOProfile *profile,
                                    audio_devices_t device,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channelMask,
                                    audio_output_flags_t flags);
        // check that volume change is permitted, compute and send new volume to audio hardware
        status_t checkAndSetVolume(int stream, int index, audio_io_handle_t output, audio_devices_t device, int delayMs = 0, bool force = false);
        // select input device corresponding to requested audio source
        virtual audio_devices_t getDeviceForInputSource(int inputSource);

        virtual uint32_t setOutputDevice(audio_io_handle_t output,
                        audio_devices_t device,
                        bool force = false,
                        int delayMs = 0);
        virtual status_t startOutput(audio_io_handle_t output,
                                AudioSystem::stream_type stream,
                                int session = 0);
        virtual status_t stopOutput(audio_io_handle_t output,
                               AudioSystem::stream_type stream,
                               int session = 0);
        virtual void setFmMode(fm_modes mode) {  fmMode = mode; }
        virtual fm_modes getFMMode() const {  return fmMode; }

private:
        // updates device caching and output for streams that can influence the
        //    routing of notifications
        void handleNotificationRoutingForStream(AudioSystem::stream_type stream);
};
};
