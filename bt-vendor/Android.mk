#
# Copyright (C) 2013  Rudolf Tammekivi <rtammekivi@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see [http://www.gnu.org/licenses/].
#

BDROID_DIR:= external/bluetooth/bluedroid

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	bt_vendor_qcom.c \
	bt_hardware.c \
	bt_helper.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := libc libcutils liblog

include $(BUILD_SHARED_LIBRARY)
