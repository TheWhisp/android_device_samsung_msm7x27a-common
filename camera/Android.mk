LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),trebon)
  LOCAL_CFLAGS += -DENABLE_FLASH_AND_AUTOFOCUS
endif

LOCAL_CFLAGS           += -O3
LOCAL_MODULE_PATH      := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE           := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS      := optional

LOCAL_SRC_FILES        := cameraHAL.cpp
LOCAL_C_INCLUDES       := $(TOP)/frameworks/base/include
LOCAL_C_INCLUDES       += hardware/qcom/display-legacy/libgralloc

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils
LOCAL_SHARED_LIBRARIES += libui libhardware libcamera_client

LOCAL_LDFLAGS          += -Lvendor/samsung/msm7x27a-common/proprietary/lib -lcamera

include $(BUILD_SHARED_LIBRARY)
