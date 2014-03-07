/*
** Copyright (c) 2011 The Linux Foundation. All rights reserved.
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

#define LOG_TAG "QualcommCamera"

#include "QualcommCamera.h"
#include "QualcommCameraHardware.h"

static hw_module_methods_t camera_module_methods = {
	.open = qcamera_open,
};

static hw_module_t qcamera_common = {
	.tag			= HARDWARE_MODULE_TAG,
	.module_api_version	= CAMERA_MODULE_API_VERSION_1_0,
	.hal_api_version	= HARDWARE_HAL_API_VERSION,
	.id			= CAMERA_HARDWARE_MODULE_ID,
	.name			= "QCamera",
	.author			= "The Linux Foundation",
	.methods		= &camera_module_methods,
	.dso			= NULL,
	.reserved		= {0},
};

camera_module_t HAL_MODULE_INFO_SYM = {
	.common			= qcamera_common,
	.get_number_of_cameras	= qcamera_get_number_of_cameras,
	.get_camera_info	= qcamera_get_camera_info,
	.set_callbacks		= NULL,
	.get_vendor_tag_ops	= NULL,
	.reserved		= {0},
};

static camera_device_ops_t camera_ops = {
	.set_preview_window		= android::set_preview_window,
	.set_callbacks			= android::set_callbacks,
	.enable_msg_type		= android::enable_msg_type,
	.disable_msg_type		= android::disable_msg_type,
	.msg_type_enabled		= android::msg_type_enabled,
	.start_preview			= android::start_preview,
	.stop_preview			= android::stop_preview,
	.preview_enabled		= android::preview_enabled,
	.store_meta_data_in_buffers	= android::store_meta_data_in_buffers,
	.start_recording		= android::start_recording,
	.stop_recording			= android::stop_recording,
	.recording_enabled		= android::recording_enabled,
	.release_recording_frame	= android::release_recording_frame,
	.auto_focus			= android::auto_focus,
	.cancel_auto_focus		= android::cancel_auto_focus,
	.take_picture			= android::take_picture,
	.cancel_picture			= android::cancel_picture,
	.set_parameters			= android::set_parameters,
	.get_parameters			= android::get_parameters,
	.put_parameters			= android::put_parameters,
	.send_command			= android::send_command,
	.release			= android::release,
	.dump				= android::dump,
};

namespace android {

typedef struct {
	QualcommCameraHardware *hardware;
	int camera_released;
	CameraParameters parameters;
	camera_notify_callback notify_cb;
	camera_data_callback data_cb;
	camera_data_timestamp_callback data_cb_timestamp;
	camera_request_memory get_memory;
	void *user_data;
} camera_hardware_t;

static QualcommCameraHardware *qcamera_get_hardware(
	struct camera_device *device)
{
	if (!device || !device->priv)
		return NULL;

	camera_hardware_t *camHal = (camera_hardware_t *)device->priv;
	return camHal->hardware;
}

extern "C" int qcamera_get_number_of_cameras(void)
{
	ALOGV("%s", __FUNCTION__);
	return HAL_getNumberOfCameras();
}

extern "C" int qcamera_get_camera_info(int camera_id, struct camera_info *info)
{
	ALOGV("%s", __FUNCTION__);

	if (!info) {
		ALOGE("Invalid arguments");
		return -EINVAL;
	}

	struct CameraInfo camInfo;
	memset(&camInfo, -1, sizeof(camInfo));

	HAL_getCameraInfo(camera_id, &camInfo);
	if (camInfo.facing >= 0) {
		info->facing = camInfo.facing;
		info->orientation = camInfo.orientation;
		return 0;
	}

	return -1;
}

extern "C" int qcamera_open(const struct hw_module_t* module, const char* id,
	struct hw_device_t** device)
{
	ALOGD("%s", __FUNCTION__);

	if (!module || !id || !device) {
		ALOGE("Invalid arguments");
		return -EINVAL;
	}

	camera_device *cam_device = new camera_device();
	camera_hardware_t *camHal = new camera_hardware_t();

	camHal->hardware = HAL_openCameraHardware(atoi(id));
	if (camHal->hardware) {
		cam_device->common.close = qcamera_close;
		cam_device->ops = &camera_ops;
		cam_device->priv = camHal;
	} else {
		delete camHal;
		delete cam_device;
	}

	*device = (hw_device_t *)cam_device;
	return 0;
}

extern "C" int qcamera_close(struct hw_device_t* device)
{
	ALOGD("%s", __FUNCTION__);
	camera_device_t *cam_device = (camera_device_t *)device;

	if (!device) {
		ALOGE("Invalid arguments");
		return -EINVAL;
	}

	camera_hardware_t *camHal = (camera_hardware_t *)cam_device->priv;
	QualcommCameraHardware *hardware = qcamera_get_hardware(cam_device);
	if (hardware) {
		if (!camHal->camera_released)
			hardware->release();
	}

	delete hardware;
	delete camHal;
	delete cam_device;

	return 0;
}

int set_preview_window(struct camera_device * device,
	struct preview_stream_ops *window)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->set_PreviewWindow(window);

	return -1;
}

void set_callbacks(struct camera_device * device,
	camera_notify_callback notify_cb,
	camera_data_callback data_cb,
	camera_data_timestamp_callback data_cb_timestamp,
	camera_request_memory get_memory,
	void *user)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware) {
		camera_hardware_t *camHal = (camera_hardware_t *)device->priv;
		if (camHal) {
			camHal->notify_cb = notify_cb;
			camHal->data_cb = data_cb;
			camHal->data_cb_timestamp = data_cb_timestamp;
			camHal->get_memory = get_memory;
			camHal->user_data = user;

			hardware->setCallbacks(notify_cb, data_cb,
				data_cb_timestamp, get_memory, user);
		}
	}
}

void enable_msg_type(struct camera_device * device, int32_t msg_type)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		hardware->enableMsgType(msg_type);
}

void disable_msg_type(struct camera_device * device, int32_t msg_type)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		hardware->disableMsgType(msg_type);
}

int msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->msgTypeEnabled(msg_type);

	return -1;
}

int start_preview(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->startPreview();

	return -1;
}

void stop_preview(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		hardware->stopPreview();
}

int preview_enabled(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->previewEnabled();

	return -1;
}

int store_meta_data_in_buffers(struct camera_device * device, int enable)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->storeMetaDataInBuffers(enable);

	return -1;
}

int start_recording(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->startRecording();

	return -1;
}

void stop_recording(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		hardware->stopRecording();
}

int recording_enabled(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->recordingEnabled();

	return -1;
}

void release_recording_frame(struct camera_device * device, const void *opaque)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware != NULL) {
		hardware->releaseRecordingFrame(opaque);
	}
}

int auto_focus(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->autoFocus();

	return -1;
}

int cancel_auto_focus(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->cancelAutoFocus();

	return -1;
}

int take_picture(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->takePicture();

	return -1;
}

int cancel_picture(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->cancelPicture();

	return -1;
}

static CameraParameters g_param;
static String8 g_str;

int set_parameters(struct camera_device * device, const char *parms)
{
	ALOGV("%s", __FUNCTION__);

	if (!parms) {
		ALOGE("Invalid arguments");
		return -EINVAL;
	}

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware) {
		g_str = String8(parms);
		g_param.unflatten(g_str);
		return hardware->setParameters(g_param);
	}

	return -1;
}

char *get_parameters(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	CameraParameters param;
	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware) {
		g_param = hardware->getParameters();
		g_str = g_param.flatten();
		return (char *)g_str.string();
	}

	return NULL;
}

void put_parameters(struct camera_device * device, char *parm)
{
	ALOGV("%s", __FUNCTION__);
}

int send_command(struct camera_device * device,
	int32_t cmd, int32_t arg1, int32_t arg2)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware)
		return hardware->sendCommand(cmd, arg1, arg2);

	return -1;
}

void release(struct camera_device * device)
{
	ALOGV("%s", __FUNCTION__);

	QualcommCameraHardware *hardware = qcamera_get_hardware(device);
	if (hardware) {
		camera_hardware_t *camHal = (camera_hardware_t *)device->priv;
		hardware->release();
		camHal->camera_released = true;
	}
}

int dump(struct camera_device * device, int fd)
{
	/* dummy */
	return 0;
}

}; // namespace android
