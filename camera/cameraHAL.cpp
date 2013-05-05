/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2012 Zhibin Wu, Simon Davie, Nico Kaiser
 * Copyright (C) 2012 QiSS ME Project Team
 * Copyright (C) 2012 Twisted, Sean Neeley
 * Copyright (C) 2012 GalaxyICS
 * Copyright (C) 2012 Pavel Kirpichyov aka WaylandACE

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

#define LOG_TAG "CameraHAL"

#define GRALLOC_USAGE_PMEM_PRIVATE_ADSP GRALLOC_USAGE_PRIVATE_0

#include <camera/CameraParameters.h>
#include <hardware/camera.h>
#include <binder/IMemory.h>
#include <gralloc_priv.h>
#include "CameraHardwareInterface.h"

using android::sp;
using android::Overlay;
using android::String8;
using android::IMemory;
using android::IMemoryHeap;
using android::CameraParameters;

using android::CameraInfo;
using android::HAL_getCameraInfo;
using android::HAL_getNumberOfCameras;
using android::HAL_openCameraHardware;
using android::CameraHardwareInterface;

static sp<CameraHardwareInterface> qCamera;

static int camera_device_open(const hw_module_t* module, const char* name,
                              hw_device_t** device);
static int camera_device_close(hw_device_t* device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);

static struct hw_module_methods_t camera_module_methods = {
    open: camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: CAMERA_DEVICE_API_VERSION_1_0,
        hal_api_version: 0,
        id: CAMERA_HARDWARE_MODULE_ID,
        name: "Camera HAL",
        author: "Zhibin Wu & Marcin Chojnacki & Pavel Kirpichyov",
        methods: &camera_module_methods,
        dso: NULL, /* remove compilation warnings */
        reserved: {0}, /* remove compilation warnings */
    },
    get_number_of_cameras: camera_get_number_of_cameras,
    get_camera_info: camera_get_camera_info,
};

typedef struct priv_camera_device {
    camera_device_t base;
    /* specific "private" data can go here (base.priv) */
    int cameraid;
    /* new world */
    preview_stream_ops *window;
    camera_notify_callback notify_callback;
    camera_data_callback data_callback;
    camera_data_timestamp_callback data_timestamp_callback;
    camera_request_memory request_memory;
    void *user;
    int preview_started;
    /* old world*/
    int preview_width;
    int preview_height;
    sp<Overlay> overlay;
    gralloc_module_t const *gralloc;
} priv_camera_device_t;

static camera_memory_t *wrap_memory_data(priv_camera_device_t *dev, const sp<IMemory>& dataPtr) {
    if (!dev->request_memory)
        return NULL;

    size_t size;
    ssize_t offset;

    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
    void *data = (void *)((char *)(heap->base()) + offset);

    camera_memory_t *mem = dev->request_memory(-1, size, 1, dev->user);

    memcpy(mem->data, data, size);

    return mem;
}

static void wrap_notify_callback(int32_t msg_type, int32_t ext1, int32_t ext2, void* user) {
    if(!user)
        return;

    priv_camera_device_t* dev = (priv_camera_device_t*) user;

    if (dev->notify_callback)
        dev->notify_callback(msg_type, ext1, ext2, dev->user);
}

void CameraHAL_HandlePreviewData(priv_camera_device_t* dev, const sp<IMemory>& dataPtr) {
    preview_stream_ops_t *mWindow = dev->window;
    if(mWindow == NULL) return;

    size_t size;
    ssize_t offset;

    sp<IMemoryHeap> heap = dataPtr->getMemory(&offset, &size);
    if(heap == 0) {
        ALOGE("%s: heap allocation failed", __FUNCTION__);
        return;
    }

    char *frame = (char *)(heap->base()) + offset;

    ALOGV("%s: base:%p offset:%i frame:%p", __FUNCTION__, heap->base(), offset, frame);

    int stride;
    void *vaddr;
    buffer_handle_t *buf_handle;

    if (0 != mWindow->dequeue_buffer(mWindow, &buf_handle, &stride)) {
        ALOGE("%s: could not dequeue gralloc buffer", __FUNCTION__);
        goto skipframe;
    }
    if (dev->gralloc == NULL) {
        goto skipframe;
    }
    if (0 == dev->gralloc->lock(dev->gralloc, *buf_handle,
                                GRALLOC_USAGE_SW_WRITE_MASK,
                                0, 0, dev->preview_width, dev->preview_height, &vaddr)) {
        memcpy(vaddr, frame, dev->preview_width * dev->preview_height * 3 / 2);
        ALOGV("%s: copy frame to gralloc buffer", __FUNCTION__);
    } else {
        ALOGE("%s: could not lock gralloc buffer", __FUNCTION__);
        goto skipframe;
    }

    dev->gralloc->unlock(dev->gralloc, *buf_handle);

    if (0 != mWindow->enqueue_buffer(mWindow, buf_handle)) {
        ALOGE("%s: could not dequeue gralloc buffer", __FUNCTION__);
        goto skipframe;
    }

skipframe:
    ALOGV("%s---: ", __FUNCTION__);

    return;
}

static void wrap_data_callback(int32_t msg_type, const sp<IMemory>& dataPtr, void* user) {
    if(!user) return;

    priv_camera_device_t* dev = (priv_camera_device_t*) user;
    if (msg_type == CAMERA_MSG_RAW_IMAGE) {
        qCamera->disableMsgType(CAMERA_MSG_RAW_IMAGE);
        return;
    }

    if (dataPtr == NULL) {
        ALOGE("%s+++: received null data", __FUNCTION__);
        return;
    }

    if(msg_type == CAMERA_MSG_PREVIEW_FRAME) {
        CameraHAL_HandlePreviewData(dev, dataPtr);
    }

    camera_memory_t *data = wrap_memory_data(dev, dataPtr);

    if (dev->data_callback)
        dev->data_callback(msg_type, data, 0, NULL, dev->user);

    if (data != NULL) {
        data->release(data);
    }
}

static void wrap_data_callback_timestamp(nsecs_t timestamp, int32_t msg_type, const sp<IMemory>& dataPtr, void* user) {
    if(!user)
        return;

    priv_camera_device_t* dev = (priv_camera_device_t*) user;

    camera_memory_t *data = wrap_memory_data(dev, dataPtr);

    if (dev->data_timestamp_callback)
        dev->data_timestamp_callback(timestamp, msg_type, data, 0, dev->user);

    qCamera->releaseRecordingFrame(dataPtr);

    if (data != NULL) {
        data->release(data);
    }
}

void CameraHAL_FixupParams(android::CameraParameters &camParams) {
    const char *video_sizes            = "640x480,384x288,352x288,320x240,240x160,176x144";
    const char *preferred_size         = "320x240";
    const char *preview_frame_rates    = "25,24,15";

    camParams.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    camParams.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,  preferred_size);

    camParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, preview_frame_rates);

    if (!camParams.get(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES)) {
         camParams.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, video_sizes);
    }

    if (!camParams.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS)) {
        camParams.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 1);
    }

    if (!camParams.get(CameraParameters::KEY_VIDEO_SIZE)) {
         camParams.set(CameraParameters::KEY_VIDEO_SIZE, preferred_size);
    }
    camParams.set("orientation", "landscape");
}

int camera_set_preview_window(struct camera_device * device, struct preview_stream_ops *window) {
    int min_bufs     = -1;
    int kBufferCount = 4;

    ALOGV("camera_set_preview_window : Window :%p\n", window);
    if (device == NULL) {
        ALOGE("camera_set_preview_window : Invalid device.\n");
        return -EINVAL;
    }
    ALOGV("camera_set_preview_window : window :%p\n", window);

    priv_camera_device_t* dev = (priv_camera_device_t*) device;
    dev->window = window;
    if(window == NULL) {
        return 0;
    }
    if (!dev->gralloc) {
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&(dev->gralloc))) {
            ALOGE("%s: Fail on loading gralloc HAL", __FUNCTION__);
        }
    }
    if (dev->window->get_min_undequeued_buffer_count(dev->window, &min_bufs)) {
        ALOGE("%s---: could not retrieve min undequeued buffer count", __FUNCTION__);
        return -1;
    }

    ALOGV("%s: bufs: %i", __FUNCTION__, min_bufs);

    if (min_bufs >= kBufferCount) {
        ALOGE("%s: min undequeued buffer count %i is too high (expecting at most %i)", __FUNCTION__, min_bufs, kBufferCount - 1);
    }

    ALOGV("%s: setting buffer count to %i", __FUNCTION__, kBufferCount);
    if (dev->window->set_buffer_count(dev->window, kBufferCount)) {
        ALOGE("%s---: could not set buffer count", __FUNCTION__);
        return -1;
    }

    CameraParameters params = qCamera->getParameters();
    params.getPreviewSize(&dev->preview_width, &dev->preview_height);

    const char *str_preview_format = params.getPreviewFormat();

    ALOGV("%s: preview format %s", __FUNCTION__, str_preview_format);

    dev->window->set_usage(dev->window, GRALLOC_USAGE_PMEM_PRIVATE_ADSP | GRALLOC_USAGE_SW_READ_OFTEN);

    if (dev->window->set_buffers_geometry(dev->window, dev->preview_width,
                                     dev->preview_height, HAL_PIXEL_FORMAT_YCrCb_420_SP)) {
        ALOGE("%s---: could not set buffers geometry to %s", __FUNCTION__, str_preview_format);
        return -1;
    }
    return 0;
}

void camera_set_callbacks(struct camera_device * device,
                          camera_notify_callback notify_cb,
                          camera_data_callback data_cb,
                          camera_data_timestamp_callback data_cb_timestamp,
                          camera_request_memory get_memory,
                          void *user) {
    ALOGV("%s+++, device %p", __FUNCTION__,device);

    if(!device) return;

    priv_camera_device_t* dev    = (priv_camera_device_t*) device;

    dev->notify_callback         = notify_cb;
    dev->data_callback           = data_cb;
    dev->data_timestamp_callback = data_cb_timestamp;
    dev->request_memory          = get_memory;
    dev->user                    = user;

    qCamera->setCallbacks(wrap_notify_callback, wrap_data_callback, wrap_data_callback_timestamp, (void *)dev);
}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type) {
    if (msg_type & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
        msg_type &= ~CAMERA_MSG_RAW_IMAGE_NOTIFY;
        msg_type |= CAMERA_MSG_RAW_IMAGE;
    }

    qCamera->enableMsgType(msg_type);
}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type) {
    /* The camera app disables the shutter too early which leads to crash.
     * Leaving it enabled. */
    if (msg_type == CAMERA_MSG_SHUTTER) return;

    qCamera->disableMsgType(msg_type);
}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type) {
    return qCamera->msgTypeEnabled(msg_type);
}

int camera_start_preview(struct camera_device * device) {
    if (!qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        qCamera->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
    }
    return qCamera->startPreview();
}

void camera_stop_preview(struct camera_device * device) {
    if (qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        qCamera->disableMsgType(CAMERA_MSG_PREVIEW_FRAME);
    }
    qCamera->stopPreview();
}

int camera_preview_enabled(struct camera_device * device) {
    return qCamera->previewEnabled();
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable) {
    return 0;
}

int camera_start_recording(struct camera_device * device) {
    return qCamera->startRecording();
}

void camera_stop_recording(struct camera_device * device) {
    qCamera->stopRecording();
}

int camera_recording_enabled(struct camera_device * device) {
    return qCamera->recordingEnabled();
}

void camera_release_recording_frame(struct camera_device * device, const void *opaque) {
    //qCamera->releaseRecordingFrame(opaque);
}

int camera_auto_focus(struct camera_device * device) {
    return qCamera->autoFocus();
}

int camera_cancel_auto_focus(struct camera_device * device) {
    return qCamera->cancelAutoFocus();
}

int camera_take_picture(struct camera_device * device) {
    return qCamera->takePicture();
}

int camera_cancel_picture(struct camera_device * device) {
    return qCamera->cancelPicture();
}

int camera_set_parameters(struct camera_device * device, const char *params) {
    CameraParameters camParams;

    String8 params_str8(params);
    camParams.unflatten(params_str8);

    return qCamera->setParameters(camParams);
}

char* camera_get_parameters(struct camera_device * device) {
    CameraParameters camParams = qCamera->getParameters();
    CameraHAL_FixupParams(camParams);

    String8 params_str8 = camParams.flatten();
    char* params = (char*) malloc(sizeof(char) * (params_str8.length()+1));
    strcpy(params, params_str8.string());

    return params;
}

static void camera_put_parameters(struct camera_device *device, char *parms) {
    free(parms);
}

int camera_send_command(struct camera_device * device, int32_t cmd, int32_t arg1, int32_t arg2) {
    return qCamera->sendCommand(cmd, arg1, arg2);
}

void camera_release(struct camera_device * device) {
    qCamera->release();
}

int camera_dump(struct camera_device * device, int fd) {
    android::Vector<android::String16> args;
    return qCamera->dump(fd, args);
}

extern "C" void heaptracker_free_leaked_memory(void);

int camera_device_close(hw_device_t* device) {
    int rc = -EINVAL;
    priv_camera_device_t* dev = (priv_camera_device_t*) device;
    if (dev) {
        qCamera = NULL;

        if (dev->base.ops) {
            free(dev->base.ops);
        }
        free(dev);
        rc = 0;
    }

    return rc;
}

void sighandle(int s) {
    ALOGW("Segfault handled, ignoring");
}

int camera_device_open(const hw_module_t* module, const char* name, hw_device_t** device) {
    ALOGI("CameraHAL v0.3");
    int rv = 0;
    int cameraid;
    int num_cameras = 0;
    priv_camera_device_t* priv_camera_device = NULL;
    camera_device_ops_t* camera_ops = NULL;
    signal(SIGFPE,(*sighandle));

    if (name != NULL) {
        cameraid = atoi(name);

        num_cameras = HAL_getNumberOfCameras();

        if(cameraid > num_cameras) {
            ALOGE("camera service provided cameraid out of bounds, cameraid = %d, num supported = %d", cameraid, num_cameras);
            rv = -EINVAL;
            goto fail;
        }

        priv_camera_device = (priv_camera_device_t*)malloc(sizeof(*priv_camera_device));
        if(!priv_camera_device) {
            ALOGE("camera_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        camera_ops = (camera_device_ops_t*)malloc(sizeof(*camera_ops));
        if(!camera_ops) {
            ALOGE("camera_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(priv_camera_device, 0, sizeof(*priv_camera_device));
        memset(camera_ops, 0, sizeof(*camera_ops));

        priv_camera_device->base.common.tag     = HARDWARE_DEVICE_TAG;
        priv_camera_device->base.common.version = 0;
        priv_camera_device->base.common.module  = (hw_module_t *)(module);
        priv_camera_device->base.common.close   = camera_device_close;
        priv_camera_device->base.ops            = camera_ops;

        camera_ops->set_preview_window         = camera_set_preview_window;
        camera_ops->set_callbacks              = camera_set_callbacks;
        camera_ops->enable_msg_type            = camera_enable_msg_type;
        camera_ops->disable_msg_type           = camera_disable_msg_type;
        camera_ops->msg_type_enabled           = camera_msg_type_enabled;
        camera_ops->start_preview              = camera_start_preview;
        camera_ops->stop_preview               = camera_stop_preview;
        camera_ops->preview_enabled            = camera_preview_enabled;
        camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
        camera_ops->start_recording            = camera_start_recording;
        camera_ops->stop_recording             = camera_stop_recording;
        camera_ops->recording_enabled          = camera_recording_enabled;
        camera_ops->release_recording_frame    = camera_release_recording_frame;
        camera_ops->auto_focus                 = camera_auto_focus;
        camera_ops->cancel_auto_focus          = camera_cancel_auto_focus;
        camera_ops->take_picture               = camera_take_picture;
        camera_ops->cancel_picture             = camera_cancel_picture;
        camera_ops->set_parameters             = camera_set_parameters;
        camera_ops->get_parameters             = camera_get_parameters;
        camera_ops->put_parameters             = camera_put_parameters;
        camera_ops->send_command               = camera_send_command;
        camera_ops->release                    = camera_release;
        camera_ops->dump                       = camera_dump;

        *device = &priv_camera_device->base.common;

        priv_camera_device->cameraid = cameraid;

        sp<CameraHardwareInterface> camera = HAL_openCameraHardware(cameraid);

        if(camera == NULL) {
            ALOGE("Couldn't create instance of CameraHal class");
            rv = -ENOMEM;
            goto fail;
        }

        qCamera = camera;
    }

    return rv;

fail:
    if(priv_camera_device) {
        free(priv_camera_device);
        priv_camera_device = NULL;
    }
    if(camera_ops) {
        free(camera_ops);
        camera_ops = NULL;
    }
    *device = NULL;

    return rv;
}

int camera_get_number_of_cameras(void) {
    return HAL_getNumberOfCameras();
}

int camera_get_camera_info(int camera_id, struct camera_info *info) {
    CameraInfo cameraInfo;

    HAL_getCameraInfo(camera_id, &cameraInfo);

    info->facing = cameraInfo.facing;
    info->orientation = info->facing == 1 ? 270 : 90;

    return 0;
}
