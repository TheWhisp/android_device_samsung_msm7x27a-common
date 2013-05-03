# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

DEVICE_PACKAGE_OVERLAYS += device/samsung/msm7x27a-common/overlay

## Video
PRODUCT_PACKAGES += \
    libstagefrighthw \
    libmm-omxcore \
    libOmxCore

## Graphics
PRODUCT_PACKAGES += \
    copybit.msm7x27a \
    gralloc.msm7x27a \
    hwcomposer.msm7x27a

## Misc.
PRODUCT_PACKAGES += \
    make_ext4fs \
    setup_fs \
    com.android.future.usb.accessory

## Bluetooth
PRODUCT_PACKAGES += \
    hciconfig \
    hcitool

## Audio
PRODUCT_PACKAGES += \
    audio.primary.msm7x27a \
    audio_policy.msm7x27a \
    audio.a2dp.default \
    audio_policy.conf \
    libaudioutils

## Other hardware
PRODUCT_PACKAGES += \
    camera.msm7x27a \
    lights.msm7x27a \
    gps.msm7x27a \
    power.msm7x27a

## Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml

## Media
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/etc/media_profiles.xml:system/etc/media_profiles.xml \
    device/samsung/msm7x27a-common/prebuilt/etc/media_codecs.xml:system/etc/media_codecs.xml

## rootdir
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/rootdir/init.qcom.rc:root/init.qcom.rc \
    device/samsung/msm7x27a-common/rootdir/init.qcom.usb.rc:root/init.qcom.usb.rc \
    device/samsung/msm7x27a-common/rootdir/ueventd.qcom.rc:root/ueventd.qcom.rc \
    device/samsung/msm7x27a-common/rootdir/fstab.qcom:root/fstab.qcom
   
## Bluetooth
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/etc/init.qcom.bt.sh:/system/etc/init.qcom.bt.sh \
    system/bluetooth/data/main.le.conf:system/etc/bluetooth/main.conf

## Network
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/etc/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    device/samsung/msm7x27a-common/prebuilt/etc/wifi/hostapd.conf:system/etc/wifi/hostapd.conf \
    device/samsung/msm7x27a-common/prebuilt/bin/get_macaddrs:system/bin/get_macaddrs

## Vold config
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/etc/vold.fstab:system/etc/vold.fstab

## Audio
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/etc/audio_policy.conf:system/etc/audio_policy.conf \
    device/samsung/msm7x27a-common/prebuilt/etc/AutoVolumeControl.txt:system/etc/AutoVolumeControl.txt \
    device/samsung/msm7x27a-common/prebuilt/etc/AudioFilter.csv:system/etc/AudioFilter.csv
	
## Keychar
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/usr/keychars/7x27a_kp.kcm.bin:system/usr/keychars/7x27a_kp.kcm.bin \
    device/samsung/msm7x27a-common/prebuilt/usr/keychars/surf_keypad.kcm.bin:system/usr/keychars/surf_keypad.kcm.bin \

## Keylayout
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/usr/keylayout/7x27a_kp.kl:system/usr/keylayout/7x27a_kp.kl \
    device/samsung/msm7x27a-common/prebuilt/usr/keylayout/sec_jack.kl:system/usr/keylayout/sec_jack.kl \
    device/samsung/msm7x27a-common/prebuilt/usr/keylayout/sec_key.kl:system/usr/keylayout/sec_key.kl \
    device/samsung/msm7x27a-common/prebuilt/usr/keylayout/sec_powerkey.kl:system/usr/keylayout/sec_powerkey.kl \
    device/samsung/msm7x27a-common/prebuilt/usr/keylayout/surf_keypad.kl:system/usr/keylayout/surf_keypad.kl \
    device/samsung/msm7x27a-common/prebuilt/usr/keylayout/sec_touchscreen.kl:system/usr/keylayout/sec_touchscreen.kl

## Touchscreen
PRODUCT_COPY_FILES += \
    device/samsung/msm7x27a-common/prebuilt/usr/idc/sec_touchscreen.idc:system/usr/idc/sec_touchscreen.idc

## Enable repeatable keys in CWM
PRODUCT_PROPERTY_OVERRIDES += \
    ro.cwm.enable_key_repeat=true

## This is an MDPI device
PRODUCT_AAPT_CONFIG := normal mdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi
PRODUCT_LOCALES += mdpi

## Other
PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=2
PRODUCT_TAGS += dalvik.gc.type-precise

$(call inherit-product, build/target/product/full.mk)
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)
$(call inherit-product, vendor/samsung/msm7x27a-common/blobs.mk)
$(call inherit-product, device/common/gps/gps_eu_supl.mk)
