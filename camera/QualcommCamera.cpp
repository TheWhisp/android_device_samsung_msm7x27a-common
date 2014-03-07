/*
 * Copyright (C) 2012 Raviprasad V Mummidi.
 * Copyright (c) 2011 Code Aurora Forum. All rights reserved.
 *
 * Modified by Andrew Sutherland <dr3wsuth3rland@gmail.com>
 *              for The Evervolv Project's qsd8k lineup
 *
 * Modified by Conn O'Griofa <connogriofa@gmail.com>
 *              for The AndroidARMv6 Project's msm7x27 lineup
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

#define LOG_TAG "QcomCamera"

#include <hardware/hardware.h>
#include <binder/IMemory.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/msm_mdp.h>
#include <gralloc_priv.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <dlfcn.h>

#include "CameraHardwareInterface.h"
/* include QCamera Hardware Interface Header*/
#include "QcomCamera.h"
//#include "QualcommCameraHardware.h"

extern "C" {
#include <sys/time.h>
}

#ifdef PREVIEW_MSM7K
#define GRALLOC_USAGE_PMEM_PRIVATE_ADSP GRALLOC_USAGE_PRIVATE_0
#endif

/* HAL function implementation goes here*/

/**
 * The functions need to be provided by the camera HAL.
 *
 * If getNumberOfCameras() returns N, the valid cameraId for getCameraInfo()
 * and openCameraHardware() is 0 to N-1.
 */

/* Prototypes and extern functions. */
#if DLOPEN_LIBCAMERA
android::sp<android::CameraHardwareInterface> (*LINK_openCameraHardware)(int id);
int (*LINK_getNumberofCameras)(void);
void (*LINK_getCameraInfo)(int cameraId, struct camera_info *info);
#else
using android::HAL_getCameraInfo;
using android::HAL_getNumberOfCameras;
using android::HAL_openCameraHardware;
#endif

/* Global variables. */
camera_notify_callback         origNotify_cb    = NULL;
camera_data_callback           origData_cb      = NULL;
camera_data_timestamp_callback origDataTS_cb    = NULL;
camera_request_memory          origCamReqMemory = NULL;

android::CameraParameters camSettings;
preview_stream_ops_t      *mWindow = NULL;
android::sp<android::CameraHardwareInterface> qCamera;


static hw_module_methods_t camera_module_methods = {
   open: camera_device_open
};

static hw_module_t camera_common  = {
  tag: HARDWARE_MODULE_TAG,
  version_major: 1,
  version_minor: 1,
  id: CAMERA_HARDWARE_MODULE_ID,
  name: "Jellybean Camera Hal",
  author: "Raviprasad V Mummidi",
  methods: &camera_module_methods,
  dso: NULL,
  reserved: {0},
};

camera_module_t HAL_MODULE_INFO_SYM = {
  common: camera_common,
  get_number_of_cameras: get_number_of_cameras,
  get_camera_info: get_camera_info,
};

camera_device_ops_t camera_ops = {
  set_preview_window:         android::set_preview_window,
  set_callbacks:              android::set_callbacks,
  enable_msg_type:            android::enable_msg_type,
  disable_msg_type:           android::disable_msg_type,
  msg_type_enabled:           android::msg_type_enabled,

  start_preview:              android::start_preview,
  stop_preview:               android::stop_preview,
  preview_enabled:            android::preview_enabled,
#if 0
  store_meta_data_in_buffers: android::store_meta_data_in_buffers,
#else
  store_meta_data_in_buffers: NULL,
#endif
  start_recording:            android::start_recording,
  stop_recording:             android::stop_recording,
  recording_enabled:          android::recording_enabled,
  release_recording_frame:    android::release_recording_frame,

  auto_focus:                 android::auto_focus,
  cancel_auto_focus:          android::cancel_auto_focus,

  take_picture:               android::take_picture,
  cancel_picture:             android::cancel_picture,

  set_parameters:             android::set_parameters,
  get_parameters:             android::get_parameters,
  put_parameters:             android::put_parameters,
  send_command:               android::send_command,

  release:                    android::release,
  dump:                       android::dump,
};

namespace android {

/* XXX: this _should_ be done with the copybit module
        TODO: figure out how */
bool internal_hw_blit(int srcFd, int destFd,
                         size_t srcOffset, size_t destOffset,
                         int srcFormat, int destFormat,
                         int x, int y, int w, int h)
{
    bool success = true;

    int fb_fd = open("/dev/graphics/fb0", O_RDWR);

    if (fb_fd < 0) {
        ALOGE("%s: Error opening frame buffer errno=%d (%s)",
              __FUNCTION__, errno, strerror(errno));
        return false;
    }

    ALOGV("%s: srcFD:%d destFD:%d srcOffset:%#x destOffset:%#x x:%d y:%d w:%d h:%d",
          __FUNCTION__, srcFd, destFd, srcOffset, destOffset, x, y, w, h);

    struct {
        uint32_t count;
        struct mdp_blit_req req[1];
    } list;

    memset(&list, 0, sizeof(list));

    list.count = 1;

    list.req[0].flags       = 0;
    list.req[0].alpha       = MDP_ALPHA_NOP;
    list.req[0].transp_mask = MDP_TRANSP_NOP;

    list.req[0].src.width     = w;
    list.req[0].src.height    = h;
    list.req[0].src.offset    = srcOffset;
    list.req[0].src.memory_id = srcFd;
    list.req[0].src.format    = srcFormat;

    list.req[0].dst.width     = w;
    list.req[0].dst.height    = h;
    list.req[0].dst.offset    = destOffset;
    list.req[0].dst.memory_id = destFd;
    list.req[0].dst.format    = destFormat;

    list.req[0].src_rect.x = list.req[0].dst_rect.x = x;
    list.req[0].src_rect.y = list.req[0].dst_rect.y = y;
    list.req[0].src_rect.w = list.req[0].dst_rect.w = w;
    list.req[0].src_rect.h = list.req[0].dst_rect.h = h;

    if (ioctl(fb_fd, MSMFB_BLIT, &list)) {
       ALOGE("%s: MSMFB_BLIT failed = %d %s",
            __FUNCTION__, errno, strerror(errno));
       success = false;
    }
    close(fb_fd);
    return success;
}

void internal_decode_sw(unsigned int* rgb, char* yuv420sp, int width, int height)
{
   int frameSize = width * height;

   if (!qCamera->previewEnabled()) return;

   for (int j = 0, yp = 0; j < height; j++) {
      int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
      for (int i = 0; i < width; i++, yp++) {
         int y = (0xff & ((int) yuv420sp[yp])) - 16;
         if (y < 0) y = 0;
         if ((i & 1) == 0) {
            v = (0xff & yuv420sp[uvp++]) - 128;
            u = (0xff & yuv420sp[uvp++]) - 128;
         }

         int y1192 = 1192 * y;
         int r = (y1192 + 1634 * v);
         int g = (y1192 - 833 * v - 400 * u);
         int b = (y1192 + 2066 * u);

         if (r < 0) r = 0; else if (r > 262143) r = 262143;
         if (g < 0) g = 0; else if (g > 262143) g = 262143;
         if (b < 0) b = 0; else if (b > 262143) b = 262143;

         rgb[yp] = 0xff000000 | ((b << 6) & 0xff0000) |
                   ((g >> 2) & 0xff00) | ((r >> 10) & 0xff);
      }
   }
}

void internal_copybuffers_sw(char *dest, char *src, int size)
{
   int       i;
   int       numWords  = size / sizeof(unsigned);
   unsigned *srcWords  = (unsigned *)src;
   unsigned *destWords = (unsigned *)dest;

   for (i = 0; i < numWords; i++) {
      if ((i % 8) == 0 && (i + 8) < numWords) {
         __builtin_prefetch(srcWords  + 8, 0, 0);
         __builtin_prefetch(destWords + 8, 1, 0);
      }
      *destWords++ = *srcWords++;
   }
   if (__builtin_expect((size - (numWords * sizeof(unsigned))) > 0, 0)) {
      int numBytes = size - (numWords * sizeof(unsigned));
      char *destBytes = (char *)destWords;
      char *srcBytes  = (char *)srcWords;
      for (i = 0; i < numBytes; i++) {
         *destBytes++ = *srcBytes++;
      }
   }
}

void internal_handle_preview(const sp<IMemory>& dataPtr,
                            preview_stream_ops_t *mWindow,
                            camera_request_memory getMemory,
                            int32_t previewWidth, int32_t previewHeight)
{
   if (mWindow != NULL && getMemory != NULL) {
      ssize_t  offset;
      size_t   size;
      int32_t  previewFormat = MDP_Y_CBCR_H2V2;
      int32_t  destFormat    = MDP_RGBX_8888;

      status_t retVal;
      sp<IMemoryHeap> mHeap = dataPtr->getMemory(&offset,
                                                                   &size);

      ALOGV("%s: previewWidth:%d previewHeight:%d offset:%#x size:%#x base:%p",
            __FUNCTION__, previewWidth, previewHeight,
           (unsigned)offset, size, mHeap != NULL ? mHeap->base() : 0);

      mWindow->set_usage(mWindow,
#ifdef PREVIEW_MSM7K
                         GRALLOC_USAGE_PMEM_PRIVATE_ADSP |
#endif
                         GRALLOC_USAGE_SW_READ_OFTEN);

      retVal = mWindow->set_buffers_geometry(mWindow,
                                             previewWidth, previewHeight,
                                             HAL_PIXEL_FORMAT_RGBX_8888);
      if (retVal == NO_ERROR) {
         int32_t          stride;
         buffer_handle_t *bufHandle = NULL;

         ALOGV("%s: dequeueing buffer",__FUNCTION__);
         retVal = mWindow->dequeue_buffer(mWindow, &bufHandle, &stride);
         if (retVal == NO_ERROR) {
            retVal = mWindow->lock_buffer(mWindow, bufHandle);
            if (retVal == NO_ERROR) {
               private_handle_t const *privHandle =
                  reinterpret_cast<private_handle_t const *>(*bufHandle);
               if (!internal_hw_blit(mHeap->getHeapID(), privHandle->fd,
                                             offset, privHandle->offset,
                                             previewFormat, destFormat,
                                             0, 0, previewWidth,
                                             previewHeight)) {
                  void *bits;
                  Rect bounds;
                  GraphicBufferMapper &mapper = GraphicBufferMapper::get();

                  bounds.left   = 0;
                  bounds.top    = 0;
                  bounds.right  = previewWidth;
                  bounds.bottom = previewHeight;

                  mapper.lock(*bufHandle, GRALLOC_USAGE_SW_READ_OFTEN, bounds,
                              &bits);
                  ALOGV("CameraHAL_HPD: w:%d h:%d bits:%p",
                       previewWidth, previewHeight, bits);
                  internal_decode_sw((unsigned int *)bits, (char *)mHeap->base() + offset,
                                      previewWidth, previewHeight);

                  // unlock buffer before sending to display
                  mapper.unlock(*bufHandle);
               }

               mWindow->enqueue_buffer(mWindow, bufHandle);
               ALOGV("%s: enqueued buffer",__FUNCTION__);
            } else {
               ALOGE("%s: ERROR locking the buffer",__FUNCTION__);
               mWindow->cancel_buffer(mWindow, bufHandle);
            }
         } else {
            ALOGE("%s: ERROR dequeueing the buffer",__FUNCTION__);
         }
      }
   }
}

camera_memory_t * internal_generate_client_data(const sp<IMemory> &dataPtr,
                        camera_request_memory reqClientMemory,
                        void *user)
{
   ssize_t          offset;
   size_t           size;
   camera_memory_t *clientData = NULL;
   sp<IMemoryHeap> mHeap = dataPtr->getMemory(&offset, &size);

   ALOGV("%s: offset:%#x size:%#x base:%p", __FUNCTION__
        (unsigned)offset, size, mHeap != NULL ? mHeap->base() : 0);

   clientData = reqClientMemory(-1, size, 1, user);
   if (clientData != NULL) {
      internal_copybuffers_sw((char *)clientData->data,
                               (char *)(mHeap->base()) + offset, size);
   } else {
      ALOGE("%s: ERROR allocating memory from client",__FUNCTION__);
   }
   return clientData;
}

void internal_fixup_settings(CameraParameters &settings)
{
   const char *preview_sizes =
      "1280x720,800x480,768x432,720x480,640x480,576x432,480x320,384x288,352x288,320x240,240x160,176x144";
   const char *video_sizes =
      "1280x720,800x480,768x432,720x480,640x480,576x432,480x320,384x288,352x288,320x240,240x160,176x144";
#if (SENSOR_SIZE > 3)
   const char *preferred_size       = "640x480";
#elif (SENSOR_SIZE > 2)
   const char *preferred_size       = "480x320";
#else /* SENSOR_SIZE=2 */
   const char *preferred_size       = "320x240";
#endif
   const char *preview_frame_rates  = "25,24,15";
   const char *preferred_frame_rate = "15";
   const char *frame_rate_range     = "(10,25)";
   const char *preferred_horizontal_viewing_angle = "51.2";
   const char *preferred_vertical_viewing_angle = "39.4";

   settings.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,
                CameraParameters::PIXEL_FORMAT_YUV420SP);

   if (!settings.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES)) {
      settings.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                   preview_sizes);
   }
#if 0
   if (!settings.get(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES)) {
      settings.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
                   video_sizes);
   }
#endif
   if (!settings.get(CameraParameters::KEY_VIDEO_SIZE)) {
      settings.set(CameraParameters::KEY_VIDEO_SIZE, preferred_size);
   }

   if (!settings.get(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO)) {
      settings.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,
                   preferred_size);
   }

   if (!settings.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES)) {
      settings.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
                   preview_frame_rates);
   }

   if (!settings.get(CameraParameters::KEY_PREVIEW_FRAME_RATE)) {
      settings.set(CameraParameters::KEY_PREVIEW_FRAME_RATE,
                   preferred_frame_rate);
   }

   if (!settings.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE)) {
      settings.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
                   frame_rate_range);
   }

   if (!settings.get(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE)) {
      settings.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE,
                   preferred_horizontal_viewing_angle);
   }

   if (!settings.get(CameraParameters::KEY_VERTICAL_VIEW_ANGLE)) {
      settings.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE,
                   preferred_vertical_viewing_angle);
   }

   if (settings.get(android::CameraParameters::KEY_MAX_CONTRAST)) {
      settings.set("max-contrast",
                  settings.get(android::CameraParameters::KEY_MAX_CONTRAST));
   } else {
      settings.set("max-contrast",
                  -1);
   }

   if (settings.get(android::CameraParameters::KEY_MAX_SATURATION)) {
      settings.set("max-saturation",
                  settings.get(android::CameraParameters::KEY_MAX_SATURATION));
   } else {
      settings.set("max-saturation",
                  -1);
   }

   if (settings.get(android::CameraParameters::KEY_MAX_SHARPNESS)) {
      settings.set("max-sharpness",
                  settings.get(android::CameraParameters::KEY_MAX_SHARPNESS));
   } else {
      settings.set("max-sharpness",
                  -1);
   }
}

static void camera_release_memory(struct camera_memory *mem) { }

void cam_notify_callback(int32_t msg_type, int32_t ext1,
                   int32_t ext2, void *user)
{
   ALOGV("cam_notify_callback: msg_type:%d ext1:%d ext2:%d user:%p",
        msg_type, ext1, ext2, user);
   if (origNotify_cb != NULL) {
      origNotify_cb(msg_type, ext1, ext2, user);
   }
}

static void cam_data_callback(int32_t msgType,
                              const sp<IMemory>& dataPtr,
                              void* user)
{
   ALOGV("cam_data_callback: msgType:%d user:%p", msgType, user);
   if (msgType == CAMERA_MSG_PREVIEW_FRAME) {
      int32_t previewWidth, previewHeight;
      CameraParameters hwParameters = qCamera->getParameters();
      hwParameters.getPreviewSize(&previewWidth, &previewHeight);
      internal_handle_preview(dataPtr, mWindow, origCamReqMemory,
                                  previewWidth, previewHeight);
   }
   if (origData_cb != NULL && origCamReqMemory != NULL) {
      camera_memory_t *clientData = internal_generate_client_data(dataPtr,
                                       origCamReqMemory, user);
      if (clientData != NULL) {
         ALOGV("cam_data_callback: Posting data to client");
         origData_cb(msgType, clientData, 0, NULL, user);
         clientData->release(clientData);
      }
   }
}

static void cam_data_callback_timestamp(nsecs_t timestamp,
                                        int32_t msgType,
                                        const sp<IMemory>& dataPtr,
                                        void* user)

{
   ALOGV("cam_data_callback_timestamp: timestamp:%lld msgType:%d user:%p",
        timestamp /1000, msgType, user);

   if (origDataTS_cb != NULL && origCamReqMemory != NULL) {
      camera_memory_t *clientData = internal_generate_client_data(dataPtr,
                                       origCamReqMemory, user);
      if (clientData != NULL) {
         ALOGV("cam_data_callback_timestamp: Posting data to client timestamp:%lld",
              systemTime());
         origDataTS_cb(timestamp, msgType, clientData, 0, user);
         qCamera->releaseRecordingFrame(dataPtr);
         clientData->release(clientData);
      } else {
         ALOGE("cam_data_callback_timestamp: ERROR allocating memory from client");
      }
   }
}

extern "C" int get_number_of_cameras(void)
{
   int numCameras = 1;

   ALOGV("get_number_of_cameras:");
#if DLOPEN_LIBCAMERA
   void *libcameraHandle = ::dlopen("libcamera.so", RTLD_NOW);
   ALOGD("HAL_get_number_of_cameras: loading libcamera at %p", libcameraHandle);
   if (!libcameraHandle) {
       ALOGE("FATAL ERROR: could not dlopen libcamera.so: %s", dlerror());
   } else {
      if (::dlsym(libcameraHandle, "HAL_getNumberOfCameras") != NULL) {
         *(void**)&LINK_getNumberofCameras =
                  ::dlsym(libcameraHandle, "HAL_getNumberOfCameras");
         numCameras = LINK_getNumberofCameras();
         ALOGD("HAL_get_number_of_cameras: numCameras:%d", numCameras);
      }
      dlclose(libcameraHandle);
   }
#else
   numCameras = HAL_getNumberOfCameras();
#endif
   return numCameras;
}

extern "C" int get_camera_info(int camera_id, struct camera_info *info)
{
#if DLOPEN_LIBCAMERA
   bool dynamic = false;
   ALOGV("get_camera_info:");
   void *libcameraHandle = ::dlopen("libcamera.so", RTLD_NOW);
   ALOGD("HAL_get_camera_info: loading libcamera at %p", libcameraHandle);
   if (!libcameraHandle) {
       ALOGE("FATAL ERROR: could not dlopen libcamera.so: %s", dlerror());
       return EINVAL;
   } else {
      if (::dlsym(libcameraHandle, "HAL_getCameraInfo") != NULL) {
         *(void**)&LINK_getCameraInfo =
                  ::dlsym(libcameraHandle, "HAL_getCameraInfo");
         LINK_getCameraInfo(camera_id, info);
         dynamic = true;
      }
      dlclose(libcameraHandle);
   }
   if (!dynamic) {
      info->facing      = CAMERA_FACING_BACK;
      info->orientation = 90;
   }
#else
   CameraInfo cameraInfo;

   ALOGV("get_camera_info:");

   HAL_getCameraInfo(camera_id, &cameraInfo);

   info->facing = cameraInfo.facing;
   info->orientation = info->facing == 1 ? 270 : 90;
#endif

   return NO_ERROR;
}

void sighandle(int s){
  //abort();
}

extern "C" int camera_device_open(const hw_module_t* module, const char* id,
                   hw_device_t** hw_device)
{

    ALOGD("%s:++",__FUNCTION__);
    int rc = -1;
    camera_device *device = NULL;

    if(module && id && hw_device) {
        int cameraId = atoi(id);
        signal(SIGFPE,(*sighandle)); //@nAa: Bad boy doing hacks
#if LIBCAMERA_DLOPEN
        void * libcameraHandle = ::dlopen("libcamera.so", RTLD_NOW);

        if (libcameraHandle) {
            ALOGD("%s: loaded libcamera at %p", __FUNCTION__, libcameraHandle);

            if (::dlsym(libcameraHandle, "openCameraHardware") != NULL) {
                *(void**)&LINK_openCameraHardware =
                    ::dlsym(libcameraHandle, "openCameraHardware");
            } else if (::dlsym(libcameraHandle, "HAL_openCameraHardware") != NULL) {
                *(void**)&LINK_openCameraHardware =
                    ::dlsym(libcameraHandle, "HAL_openCameraHardware");
            } else {
                ALOGE("FATAL ERROR: Could not find openCameraHardware");
                dlclose(libcameraHandle);
                return rc;
            }

            qCamera = LINK_openCameraHardware(cameraId);

            ::dlclose(libcameraHandle);

            device = (camera_device *)malloc(sizeof (struct camera_device));

            if(device) {
                //memset(device, 0, sizeof(*device));
                // Dont think these are needed
                //device->common.tag              = HARDWARE_DEVICE_TAG;
                //device->common.version          = 0;
                //device->common.module           = (hw_module_t *)(module);
                device->common.close            = close_camera_device;
                device->ops                     = &camera_ops;
                rc = 0;
            }
        }
#else
        qCamera = HAL_openCameraHardware(cameraId);

        device = (camera_device *)malloc(sizeof (struct camera_device));

        if(device) {
            //memset(device, 0, sizeof(*device));
            // Dont think these are needed
            //device->common.tag              = HARDWARE_DEVICE_TAG;
            //device->common.version          = 0;
            //device->common.module           = (hw_module_t *)(module);
            device->common.close            = close_camera_device;
            device->ops                     = &camera_ops;
            rc = 0;
        }
#endif
    }
    *hw_device = (hw_device_t*)device;
    ALOGD("%s:--",__FUNCTION__);
    return rc;
}

extern "C" int close_camera_device(hw_device_t* hw_dev)
{
    int rc = -1;
    ALOGD("%s:++",__FUNCTION__);
    camera_device_t *device = (camera_device_t *)hw_dev;
    if (device) {
        if (qCamera != NULL)
            qCamera.clear();
        free(device);
        rc = 0;
    }
    ALOGD("%s:--",__FUNCTION__);
    return rc;
}

int set_preview_window(struct camera_device * device,
                           struct preview_stream_ops *window)
{
   ALOGV("set_preview_window : Window :%p", window);
   if (device == NULL) {
      ALOGE("set_preview_window : Invalid device.");
      return -EINVAL;
   } else {
      ALOGV("set_preview_window : window :%p", window);
      mWindow = window;
      return 0;
   }
}

void set_callbacks(struct camera_device * device,
                      camera_notify_callback notify_cb,
                      camera_data_callback data_cb,
                      camera_data_timestamp_callback data_cb_timestamp,
                      camera_request_memory get_memory, void *user)
{
   ALOGV("set_callbacks: notify_cb: %p, data_cb: %p "
        "data_cb_timestamp: %p, get_memory: %p, user :%p",
        notify_cb, data_cb, data_cb_timestamp, get_memory, user);

   origNotify_cb    = notify_cb;
   origData_cb      = data_cb;
   origDataTS_cb    = data_cb_timestamp;
   origCamReqMemory = get_memory;
   qCamera->setCallbacks(cam_notify_callback, cam_data_callback,
                         cam_data_callback_timestamp, user);
}

void enable_msg_type(struct camera_device * device, int32_t msg_type)
{
   ALOGV("enable_msg_type: msg_type:%#x", msg_type);
   if (msg_type == 0xfff) {
      msg_type = 0x1ff;
   } else {
      msg_type &= ~(CAMERA_MSG_PREVIEW_METADATA | CAMERA_MSG_RAW_IMAGE_NOTIFY);
   }
   qCamera->enableMsgType(msg_type);
}

void disable_msg_type(struct camera_device * device, int32_t msg_type)
{
   ALOGV("disable_msg_type: msg_type:%#x", msg_type);
   if (msg_type == 0xfff) {
      msg_type = 0x1ff;
   }

   /* The camera app disables the shutter too early which leads to crash.
    * Leaving it enabled. */
   if (msg_type == CAMERA_MSG_SHUTTER)
       return;

   qCamera->disableMsgType(msg_type);
}

int msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
   ALOGV("msg_type_enabled: msg_type:%d", msg_type);
   return qCamera->msgTypeEnabled(msg_type);
}

int start_preview(struct camera_device * device)
{
   ALOGV("start_preview: Enabling CAMERA_MSG_PREVIEW_FRAME");

   /* TODO: Remove hack. */
   ALOGV("qcamera_start_preview: Preview enabled:%d msg enabled:%d",
        qCamera->previewEnabled(),
        qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME));
   if (!qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
      qCamera->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
   }
   return qCamera->startPreview();
}

void stop_preview(struct camera_device * device)
{
   ALOGV("stop_preview: msgenabled:%d",
         qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME));

   /* TODO: Remove hack. */
   if (qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
      qCamera->disableMsgType(CAMERA_MSG_PREVIEW_FRAME);
   }
   return qCamera->stopPreview();
}

int preview_enabled(struct camera_device * device)
{
   ALOGV("preview_enabled:");
   return qCamera->previewEnabled() ? 1 : 0;
}
#if 0
int store_meta_data_in_buffers(struct camera_device * device, int enable)
{
   ALOGV("store_meta_data_in_buffers:");
   return NO_ERROR;
}
#endif
int start_recording(struct camera_device * device)
{
   ALOGV("start_recording");

   /* TODO: Remove hack. */
   qCamera->enableMsgType(CAMERA_MSG_VIDEO_FRAME);
   qCamera->startRecording();
   return NO_ERROR;
}

void stop_recording(struct camera_device * device)
{
   ALOGV("stop_recording:");

   /* TODO: Remove hack. */
   qCamera->disableMsgType(CAMERA_MSG_VIDEO_FRAME);
   qCamera->stopRecording();
}

int recording_enabled(struct camera_device * device)
{
   ALOGV("recording_enabled:");
   return (int)qCamera->recordingEnabled();
}

void release_recording_frame(struct camera_device * device,
                                const void *opaque)
{
   /*
    * We release the frame immediately in cam_data_callback_timestamp after making a
    * copy. So, this is just a NOP.
    */
   ALOGV("release_recording_frame: opaque:%p", opaque);
}

int auto_focus(struct camera_device * device)
{
   ALOGV("auto_focus:");
   qCamera->autoFocus();
   return NO_ERROR;
}

int cancel_auto_focus(struct camera_device * device)
{
   ALOGV("cancel_auto_focus:");
   qCamera->cancelAutoFocus();
   return NO_ERROR;
}

int take_picture(struct camera_device * device)
{
   ALOGV("take_picture:");

   /* TODO: Remove hack. */
   qCamera->enableMsgType(CAMERA_MSG_SHUTTER |
                         CAMERA_MSG_POSTVIEW_FRAME |
                         CAMERA_MSG_RAW_IMAGE |
                         CAMERA_MSG_COMPRESSED_IMAGE);

   qCamera->takePicture();
   return NO_ERROR;
}

int cancel_picture(struct camera_device * device)
{
   ALOGV("cancel_picture:");
   qCamera->cancelPicture();
   return NO_ERROR;
}

//CameraParameters g_param;
String8 g_str;
int set_parameters(struct camera_device * device, const char *params)
{
   ALOGV("set_parameters: %s", params);
   g_str = String8(params);
   camSettings.unflatten(g_str);
   qCamera->setParameters(camSettings);
   return NO_ERROR;
}

char * get_parameters(struct camera_device * device)
{
   char *rc = NULL;
   ALOGV("get_parameters");
   camSettings = qCamera->getParameters();
   ALOGV("get_parameters: after calling qCamera->getParameters()");
   internal_fixup_settings(camSettings);
   g_str = camSettings.flatten();
   rc = strdup((char *)g_str.string());
   ALOGV("get_parameters: returning rc:%p :%s",
        rc, (rc != NULL) ? rc : "EMPTY STRING");
   return rc;
}

void put_parameters(struct camera_device *device, char *params)
{
   ALOGV("put_parameters: params:%p %s", params, params);
   free(params);
}


int send_command(struct camera_device * device, int32_t cmd,
                        int32_t arg0, int32_t arg1)
{
   ALOGV("send_command: cmd:%d arg0:%d arg1:%d",
        cmd, arg0, arg1);
   return qCamera->sendCommand(cmd, arg0, arg1);
}

void release(struct camera_device * device)
{
   ALOGV("release:");
   qCamera->release();
}

int dump(struct camera_device * device, int fd)
{
   ALOGV("dump:");
   Vector<String16> args;
   return qCamera->dump(fd, args);
}

}; // namespace android
