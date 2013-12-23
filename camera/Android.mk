$(shell mkdir -p $(OUT)/obj/SHARED_LIBRARIES/libcamera_intermediates/)
$(shell touch $(OUT)/obj/SHARED_LIBRARIES/libcamera_intermediates/export_includes)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_FLAGS          += -O3
LOCAL_MODULE_TAGS      := optional
LOCAL_MODULE_PATH      := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE           := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_SRC_FILES        := QcomCamera.cpp

LOCAL_SHARED_LIBRARIES := liblog libdl libutils libcamera_client libbinder \
                          libcutils libhardware libui libcamera

LOCAL_C_INCLUDES       := frameworks/base/services \
                          frameworks/base/include \
                          hardware/libhardware/include

ifeq ($(TARGET_BOARD_PLATFORM),msm7k)
LOCAL_CFLAGS           := -DPREVIEW_MSM7K
else
	ifeq ($(TARGET_QCOM_DISPLAY_VARIANT),caf)
		LOCAL_C_INCLUDES += hardware/qcom/display-caf/libgralloc
	else
		LOCAL_C_INCLUDES += hardware/qcom/display-legacy/libgralloc
	endif
endif

LOCAL_PRELINK_MODULE   := false

include $(BUILD_SHARED_LIBRARY)
