/*
** Copyright 2008, The Android Open-Source Project
** Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#ifndef ANDROID_AUDIO_HARDWARE_H
#define ANDROID_AUDIO_HARDWARE_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/List.h>

#include <utils/threads.h>
#include <sys/prctl.h>
#include <utils/SortedVector.h>

#include <hardware_legacy/AudioHardwareBase.h>

extern "C" {
#include <linux/msm_audio.h>
#include <linux/ion.h>
#include <linux/msm_audio_voicememo.h>
#ifdef QCOM_VOIP_ENABLED
#include <linux/msm_audio_mvs.h>
#endif
}

namespace android_audio_legacy {
using android::List;
using android::SortedVector;
using android::Mutex;
using android::Condition;

// ----------------------------------------------------------------------------
// Kernel driver interface
//

#define SAMP_RATE_INDX_8000 0
#define SAMP_RATE_INDX_11025 1
#define SAMP_RATE_INDX_12000 2
#define SAMP_RATE_INDX_16000 3
#define SAMP_RATE_INDX_22050 4
#define SAMP_RATE_INDX_24000 5
#define SAMP_RATE_INDX_32000 6
#define SAMP_RATE_INDX_44100 7
#define SAMP_RATE_INDX_48000 8

#define EQ_MAX_BAND_NUM 12

#define ADRC_ENABLE  0x0001
#define ADRC_DISABLE 0xFFFE
#define EQ_ENABLE    0x0002
#define EQ_DISABLE   0xFFFD
#define RX_IIR_ENABLE  0x0004
#define RX_IIR_DISABLE 0xFFFB
#define MBADRC_ENABLE  0x0010
#define MBADRC_DISABLE 0xFFEF
#define SRS_ENABLE 0x0020
#define SRS_DISABLE 0xFFDF
#define LPA_BUFFER_SIZE 512*1024
#define BUFFER_COUNT 2

#define AGC_ENABLE     0x0001
#define NS_ENABLE      0x0002
#define TX_IIR_ENABLE  0x0004

struct eq_filter_type {
    int16_t gain;
    uint16_t freq;
    uint16_t type;
    uint16_t qf;
};

struct eqalizer {
    uint16_t bands;
    uint16_t params[132];
};

struct rx_iir_filter {
    uint16_t num_bands;
    uint16_t iir_params[48];
};

struct adrc_filter {
    uint16_t adrc_params[8];
};

struct msm_audio_stats {
    uint32_t byte_count;
    uint32_t sample_count;
    uint32_t unused[2];
};

struct tx_iir {
        uint16_t  cmd_id;
        uint16_t  active_flag;
        uint16_t  num_bands;
        uint16_t iir_params[48];
};

struct ns {
        uint16_t  cmd_id;
        uint16_t  ec_mode_new;
        uint16_t  dens_gamma_n;
        uint16_t  dens_nfe_block_size;
        uint16_t  dens_limit_ns;
        uint16_t  dens_limit_ns_d;
        uint16_t  wb_gamma_e;
        uint16_t  wb_gamma_n;
};

struct tx_agc {
        uint16_t  cmd_id;
        uint16_t  tx_agc_param_mask;
        uint16_t  tx_agc_enable_flag;
        uint16_t  static_gain;
        int16_t   adaptive_gain_flag;
        uint16_t  agc_params[19];
};

struct adrc_config {
    uint16_t adrc_band_params[10];
};

struct adrc_ext_buf {
    int16_t buff[196];
};

struct mbadrc_filter {
    uint16_t num_bands;
    uint16_t down_samp_level;
    uint16_t adrc_delay;
    uint16_t ext_buf_size;
    uint16_t ext_partition;
    uint16_t ext_buf_msw;
    uint16_t ext_buf_lsw;
    struct adrc_config adrc_band[5];
    struct adrc_ext_buf  ext_buf;
};

enum tty_modes {
    TTY_OFF = 0,
    TTY_VCO = 1,
    TTY_HCO = 2,
    TTY_FULL = 3
};

#define CODEC_TYPE_PCM 0
#define AUDIO_HW_NUM_OUT_BUF 2  // Number of buffers in audio driver for output
// TODO: determine actual audio DSP and hardware latency
#define AUDIO_HW_OUT_LATENCY_MS 0  // Additionnal latency introduced by audio DSP and hardware in ms

#define AUDIO_HW_IN_SAMPLERATE 8000                 // Default audio input sample rate
#define AUDIO_HW_IN_CHANNELS (AudioSystem::CHANNEL_IN_MONO) // Default audio input channel mask
#define AUDIO_HW_IN_BUFFERSIZE 2048                 // Default audio input buffer size
#define AUDIO_HW_IN_FORMAT (AudioSystem::PCM_16_BIT)  // Default audio input sample format
#ifdef QCOM_VOIP_ENABLED
#define AUDIO_HW_VOIP_BUFFERSIZE_8K 320
#define AUDIO_HW_VOIP_BUFFERSIZE_16K 640
#define AUDIO_HW_VOIP_SAMPLERATE_8K 8000
#define AUDIO_HW_VOIP_SAMPLERATE_16K 16000
#endif

// ----------------------------------------------------------------------------
using android_audio_legacy::AudioHardwareBase;
using android_audio_legacy::AudioStreamOut;
using android_audio_legacy::AudioStreamIn;
using android_audio_legacy::AudioSystem;
using android_audio_legacy::AudioHardwareInterface;
class AudioHardware : public  AudioHardwareBase
{
    class AudioStreamOutMSM72xx;
    class AudioSessionOutLPA;
    class AudioStreamInMSM72xx;
#ifdef QCOM_VOIP_ENABLED
    class AudioStreamOutDirect;
    class AudioStreamInVoip;
#endif
public:
                        AudioHardware();
    virtual             ~AudioHardware();
    virtual status_t    initCheck();

    virtual status_t    setVoiceVolume(float volume);
#ifdef QCOM_FM_ENABLED
    virtual status_t    setFmVolume(float volume);
#endif
    virtual status_t    setMasterVolume(float volume);

    virtual status_t    setMode(int mode);

    // mic mute
    virtual status_t    setMicMute(bool state);
    virtual status_t    getMicMute(bool* state);

    virtual status_t    setParameters(const String8& keyValuePairs);
    virtual String8     getParameters(const String8& keys);

    // create I/O streams
    virtual AudioStreamOut* openOutputStream(
                                uint32_t devices,
                                int *format=0,
                                uint32_t *channels=0,
                                uint32_t *sampleRate=0,
                                status_t *status=0);
    virtual AudioStreamIn* openInputStream(

                                uint32_t devices,
                                int *format,
                                uint32_t *channels,
                                uint32_t *sampleRate,
                                status_t *status,
                                AudioSystem::audio_in_acoustics acoustics);

    virtual    void        closeOutputStream(AudioStreamOut* out);
    virtual    void        closeInputStream(AudioStreamIn* in);

    virtual    size_t      getInputBufferSize(uint32_t sampleRate, int format, int channelCount);
               void        clearCurDevice() { mCurSndDevice = -1; }
#ifdef QCOM_FM_ENABLED
                int IsFmon() const { return (mFmFd != -1); }
                int IsFmA2dpOn() const { return FmA2dpStatus; }
                void SwitchOffFmA2dp() { FmA2dpStatus = false; }
                bool isFMAnalog();
#endif
protected:
    virtual status_t    dump(int fd, const Vector<String16>& args);
    uint32_t getMvsMode(int format, int rate);
    uint32_t getMvsRateType(uint32_t MvsMode, uint32_t *rateType);
    status_t setupDeviceforVoipCall(bool value);

private:

    status_t    doAudioRouteOrMute(uint32_t device);
    status_t    setMicMute_nosync(bool state);
    status_t    checkMicMute();
    status_t    dumpInternals(int fd, const Vector<String16>& args);
    uint32_t    getInputSampleRate(uint32_t sampleRate);
    bool        checkOutputStandby();
    status_t    doRouting(AudioStreamInMSM72xx *input, int outputDevice = 0);
#ifdef QCOM_FM_ENABLED
    status_t    enableFM();
    status_t    disableFM();
#endif
    AudioStreamInMSM72xx*   getActiveInput_l();
#ifdef QCOM_VOIP_ENABLED
    AudioStreamInVoip* getActiveVoipInput_l();
#endif

    class AudioStreamOutMSM72xx : public AudioStreamOut {
    public:
                            AudioStreamOutMSM72xx();
        virtual             ~AudioStreamOutMSM72xx();
                status_t    set(AudioHardware* mHardware,
                                uint32_t devices,
                                int *pFormat,
                                uint32_t *pChannels,
                                uint32_t *pRate);
        virtual uint32_t sampleRate() const {
            char af_quality[PROPERTY_VALUE_MAX];
            property_get("af.resampler.quality",af_quality,"0");
            if(strcmp("255",af_quality) == 0) {
                ALOGD("SampleRate 48k");
                return 48000;
            } else {
                ALOGD("SampleRate 44.1k");
                return 44100;
            }
        }
        virtual size_t bufferSize() const {
            char af_quality[PROPERTY_VALUE_MAX];
            property_get("af.resampler.quality",af_quality,"0");
            if(strcmp("255",af_quality) == 0) {
                ALOGD("Bufsize 5248");
                return 5248;
            } else {
                ALOGD("Bufsize 4800");
                return 4800;
            }
        }
        virtual uint32_t    channels() const { return AudioSystem::CHANNEL_OUT_STEREO; }
        virtual int         format() const { return AudioSystem::PCM_16_BIT; }
        virtual uint32_t    latency() const { return (1000*AUDIO_HW_NUM_OUT_BUF*(bufferSize()/frameSize()))/sampleRate()+AUDIO_HW_OUT_LATENCY_MS; }
        virtual status_t    setVolume(float left, float right) { return INVALID_OPERATION; }
        virtual ssize_t     write(const void* buffer, size_t bytes);
        virtual status_t    standby();
        virtual status_t    dump(int fd, const Vector<String16>& args);
                bool        checkStandby();
        virtual status_t    setParameters(const String8& keyValuePairs);
        virtual String8     getParameters(const String8& keys);
                uint32_t    devices() { return mDevices; }
        virtual status_t    getRenderPosition(uint32_t *dspFrames);

    private:
                AudioHardware* mHardware;
                int         mFd;
                int         mStartCount;
                int         mRetryCount;
                bool        mStandby;
                uint32_t    mDevices;
    };
#ifdef QCOM_VOIP_ENABLED
    class AudioStreamOutDirect : public AudioStreamOut {
    public:
                            AudioStreamOutDirect();
        virtual             ~AudioStreamOutDirect();
                status_t    set(AudioHardware* mHardware,
                                uint32_t devices,
                                int *pFormat,
                                uint32_t *pChannels,
                                uint32_t *pRate);
        virtual uint32_t    sampleRate() const { ALOGD(" AudioStreamOutDirect: sampleRate\n"); return 8000; }
        // must be 32-bit aligned - driver only seems to like 4800
        virtual size_t      bufferSize() const { ALOGD(" AudioStreamOutDirect: bufferSize\n"); return 320; }
        virtual uint32_t    channels() const { ALOGD(" AudioStreamOutDirect: channels\n"); return mChannels; }
        virtual int         format() const { ALOGD(" AudioStreamOutDirect: format\n"); return AudioSystem::PCM_16_BIT; }
        virtual uint32_t    latency() const { return (1000*AUDIO_HW_NUM_OUT_BUF*(bufferSize()/frameSize()))/sampleRate()+AUDIO_HW_OUT_LATENCY_MS; }
        virtual status_t    setVolume(float left, float right) { return INVALID_OPERATION; }
        virtual ssize_t     write(const void* buffer, size_t bytes);
        virtual status_t    standby();
        virtual status_t    dump(int fd, const Vector<String16>& args);
                bool        checkStandby();
        virtual status_t    setParameters(const String8& keyValuePairs);
        virtual String8     getParameters(const String8& keys);
                uint32_t    devices() { return mDevices; }
        virtual status_t    getRenderPosition(uint32_t *dspFrames);

    private:
                AudioHardware* mHardware;
                int         mFd;
                int         mRetryCount;
                int         mStartCount;
                bool        mStandby;
                uint32_t    mDevices;
                uint32_t    mChannels;
                uint32_t    mSampleRate;
                size_t      mBufferSize;
                int         mFormat;
    };
#endif
// ----------------------------------------------------------------------------

class AudioSessionOutLPA : public AudioStreamOut
{
public:
    AudioSessionOutLPA(AudioHardware* mHardware,
                        uint32_t   devices,
                        int        format,
                        uint32_t   channels,
                        uint32_t   samplingRate,
                        int        type,
                        status_t   *status);
    virtual            ~AudioSessionOutLPA();

    virtual uint32_t    sampleRate() const
    {
        return mSampleRate;
    }

    virtual size_t      bufferSize() const
    {
        return mBufferSize;
    }

    virtual uint32_t    channels() const
    {
        return mChannels;
    }

    virtual int         format() const
    {
        return mFormat;
    }

    virtual uint32_t    latency() const;

    virtual ssize_t     write(const void *buffer, size_t bytes);

    virtual status_t    start( );
    virtual status_t    pause();
    virtual status_t    flush();
    virtual status_t    stop();

    virtual status_t    dump(int fd, const Vector<String16>& args);

    status_t            setVolume(float left, float right);

    virtual status_t    standby();

    virtual status_t    setParameters(const String8& keyValuePairs);

    virtual String8     getParameters(const String8& keys);


    // return the number of audio frames written by the audio dsp to DAC since
    // the output has exited standby
    virtual status_t    getRenderPosition(uint32_t *dspFrames);

    virtual status_t    getNextWriteTimestamp(int64_t *timestamp);
    virtual status_t    setObserver(void *observer);
    virtual status_t    getBufferInfo(buf_info **buf);
    virtual status_t    isBufferAvailable(int *isAvail);

	void* memBufferAlloc(int nSize, int32_t *ion_fd);

private:
    Mutex               mLock;
    uint32_t            mFrameCount;
    uint32_t            mSampleRate;
    uint32_t            mChannels;
    size_t              mBufferSize;
    int                 mFormat;
    uint32_t            mStreamVol;

    bool                mPaused;
    bool                mIsDriverStarted;
    bool                mGenerateEOS;
    bool                mSeeking;
    bool                mReachedEOS;
    bool                mSkipWrite;
    bool                mEosEventReceived;
    uint32_t    mDevices;
    AudioHardware* mHardware;
    AudioEventObserver *mObserver;

    void                createEventThread();
    void                bufferAlloc();
    void                bufferDeAlloc();
    bool                isReadyToPostEOS(int errPoll, void *fd);
    status_t            drain();
	status_t            openAudioSessionDevice();
    // make sure the event thread also exited
    void                requestAndWaitForEventThreadExit();
    int32_t             writeToDriver(char *buffer, int bytes);
    static void *       eventThreadWrapper(void *me);
    void                eventThreadEntry();
//??    status_t            pause_l();
//??    status_t            resume_l();
    void                reset();

    //Structure to hold ion buffer information
    class BuffersAllocated {
    /* overload BuffersAllocated constructor to support both ion and pmem memory allocation */
    public:
        BuffersAllocated(void *buf1, void *buf2, int32_t nSize, int32_t fd) :
        localBuf(buf1), memBuf(buf2), memBufsize(nSize), memFd(fd)
        {}
        BuffersAllocated(void *buf1, void *buf2, int32_t nSize, int32_t share_fd, struct ion_handle *handle) :
        ion_handle(handle), localBuf(buf1), memBuf(buf2), memBufsize(nSize), memFd(share_fd)
        {}
        struct ion_handle *ion_handle;
        void* localBuf;
        void* memBuf;
        int32_t memBufsize;
        int32_t memFd;
        uint32_t bytesToWrite;
    };
    List<BuffersAllocated> mEmptyQueue;
    List<BuffersAllocated> mFilledQueue;
    List<BuffersAllocated> mBufPool;

    //Declare all the threads
    pthread_t mEventThread;

    //Declare the condition Variables and Mutex
    Mutex mEmptyQueueMutex;
    Mutex mFilledQueueMutex;

    Condition mWriteCv;
    Condition mEventCv;
	pthread_mutex_t event_mutex;
    bool mKillEventThread;
    bool mEventThreadAlive;
    int mInputBufferSize;
    int mInputBufferCount;
    int64_t timePlayed;
    int64_t timeStarted;

    //event fd to signal the EOS and Kill from the userspace
    int efd;
	int afd;
	int ionfd;
};

    class AudioStreamInMSM72xx : public AudioStreamIn {
    public:
        enum input_state {
            AUDIO_INPUT_CLOSED,
            AUDIO_INPUT_OPENED,
            AUDIO_INPUT_STARTED
        };

                            AudioStreamInMSM72xx();
        virtual             ~AudioStreamInMSM72xx();
                status_t    set(AudioHardware* mHardware,
                                uint32_t devices,
                                int *pFormat,
                                uint32_t *pChannels,
                                uint32_t *pRate,
                                AudioSystem::audio_in_acoustics acoustics);
        virtual size_t      bufferSize() const { return mBufferSize; }
        virtual uint32_t    channels() const { return mChannels; }
        virtual int         format() const { return mFormat; }
        virtual uint32_t    sampleRate() const { return mSampleRate; }
        virtual status_t    setGain(float gain) { return INVALID_OPERATION; }
        virtual ssize_t     read(void* buffer, ssize_t bytes);
        virtual status_t    dump(int fd, const Vector<String16>& args);
        virtual status_t    standby();
        virtual status_t    setParameters(const String8& keyValuePairs);
        virtual String8     getParameters(const String8& keys);
        virtual unsigned int  getInputFramesLost() const { return 0; }
                uint32_t    devices() { return mDevices; }
                int         state() const { return mState; }
        virtual status_t    addAudioEffect(effect_interface_s**) { return 0;}
        virtual status_t    removeAudioEffect(effect_interface_s**) { return 0;}

    private:
                AudioHardware* mHardware;
                int         mFd;
                int         mState;
                int         mRetryCount;
                int         mFormat;
                uint32_t    mChannels;
                uint32_t    mSampleRate;
                size_t      mBufferSize;
                AudioSystem::audio_in_acoustics mAcoustics;
                uint32_t    mDevices;
                bool        mFirstread;
                static int InstanceCount;
    };

#ifdef QCOM_VOIP_ENABLED
    class AudioStreamInVoip : public AudioStreamInMSM72xx {
    public:
        enum input_state {
            AUDIO_INPUT_CLOSED,
            AUDIO_INPUT_OPENED,
            AUDIO_INPUT_STARTED
        };

                            AudioStreamInVoip();
        virtual             ~AudioStreamInVoip();
                status_t    set(AudioHardware* mHardware,
                                uint32_t devices,
                                int *pFormat,
                                uint32_t *pChannels,
                                uint32_t *pRate,
                                AudioSystem::audio_in_acoustics acoustics);
        virtual size_t      bufferSize() const { return mBufferSize; }
        virtual uint32_t    channels() const {ALOGD(" AudioStreamInVoip: channels %d \n",mChannels); return mChannels; }
        virtual int         format() const { return AUDIO_HW_IN_FORMAT; }
        virtual uint32_t    sampleRate() const { return mSampleRate; }
        virtual status_t    setGain(float gain) { return INVALID_OPERATION; }
        virtual ssize_t     read(void* buffer, ssize_t bytes);
        virtual status_t    dump(int fd, const Vector<String16>& args);
        virtual status_t    standby();
        virtual status_t    setParameters(const String8& keyValuePairs);
        virtual String8     getParameters(const String8& keys);
        virtual unsigned int  getInputFramesLost() const { return 0; }
                uint32_t    devices() { return mDevices; }
                int         state() const { return mState; }
                bool        mSetupDevice;

    private:
                AudioHardware* mHardware;
                int         mFd;
                int         mState;
                int         mRetryCount;
                int         mFormat;
                uint32_t    mChannels;
                uint32_t    mSampleRate;
                size_t      mBufferSize;
                AudioSystem::audio_in_acoustics mAcoustics;
                uint32_t    mDevices;
                bool        mFirstread;
#ifdef QCOM_FM_ENABLED
                uint32_t    mFmRec;
#endif
    };
#endif /*QCOM_VOIP_ENABLED*/
            static const uint32_t inputSamplingRates[];
            bool        mInit;
            bool        mMicMute;
#ifdef QCOM_FM_ENABLED
            int         mFmFd;
            int         FmA2dpStatus;
#endif
            bool        mBluetoothNrec;
            bool        mBluetoothVGS;
            uint32_t    mBluetoothId;
            AudioStreamOutMSM72xx*  mOutput;
#ifdef QCOM_VOIP_ENABLED
            AudioStreamOutDirect*  mDirectOutput;
#endif
            AudioSessionOutLPA* mOutputLPA;
            SortedVector <AudioStreamInMSM72xx*>   mInputs;
#ifdef QCOM_VOIP_ENABLED
            SortedVector <AudioStreamInVoip*>   mVoipInputs;
#endif /*QCOM_VOIP_ENABLED*/
            uint32_t mVoipBitRate;
            msm_snd_endpoint *mSndEndpoints;
            int mNumSndEndpoints;

            int mCurSndDevice;
            int m7xsnddriverfd;
            bool        mDualMicEnabled;
            int         mTtyMode;
#ifdef QCOM_VOIP_ENABLED
            int mVoipFd;
            bool mVoipInActive;
            bool mVoipOutActive;
            Mutex       mVoipLock;
            int         mDirectOutrefCnt;
#endif /*QCOM_VOIP_ENABLED*/
     friend class AudioStreamInMSM72xx;
            Mutex       mLock;
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_AUDIO_HARDWARE_MSM72XX_H