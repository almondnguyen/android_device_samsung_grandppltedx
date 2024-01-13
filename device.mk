#
# Copyright (C) 2018 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

BLOCK_BASED_OTA := false

# Define Path
DEVICE_PATH := device/samsung/grandppltedx

# Vendor
$(call inherit-product-if-exists, vendor/samsung/grandppltedx/grandppltedx-vendor.mk)

# Overlay
DEVICE_PACKAGE_OVERLAYS += $(DEVICE_PATH)/overlay

# Display
#-- This device is hdpi.
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_AAPT_PREBUILT_DPI := hdpi

PRODUCT_PACKAGES += \
	libion

# Audio
PRODUCT_PACKAGES += \
	audio.usb.default \
	audio.r_submix.default \
	audio_policy.default \
	libaudiopolicymanager \
	libaudiopolicyservice \
	libaudiopolicyenginedefault \
	libaudioroute \
	libaudiospdif \
	libeffects \
	libaudio-resampler \
	libaudioutils \
	libtinyalsa \
	libtinycompress \
	libtinymix \
	libtinyxml

PRODUCT_COPY_FILES += $(call find-copy-subdir-files,*,frameworks/av/services/audiopolicy/config,system/etc)
PRODUCT_COPY_FILES += \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
	frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
	frameworks/native/data/etc/android.software.midi.xml:system/etc/permissions/android.software.midi.xml \
	$(DEVICE_PATH)/configs/audio/audio_policy.conf:system/etc/audio_policy.conf \
	$(DEVICE_PATH)/configs/audio/audio_device.xml:system/etc/audio_device.xml \
	$(DEVICE_PATH)/configs/audio/audio_param/AudioParamOptions.xml:system/etc/audio_param/AudioParamOptions.xml

#al: test
#PRODUCT_PACKAGES += libaudiosetting

# Media
PRODUCT_COPY_FILES += \
	frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video_le.xml:system/etc/media_codecs_google_video_le.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_performance.xml:system/etc/media_codecs_performance.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_mediatek_audio.xml:system/etc/media_codecs_mediatek_audio.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_mediatek_video.xml:system/etc/media_codecs_mediatek_video.xml \
	$(DEVICE_PATH)/configs/media/media_codecs.xml:system/etc/media_codecs.xml \
	$(DEVICE_PATH)/configs/media/media_profiles.xml:system/etc/media_profiles.xml

# BT
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml

# Wifi
PRODUCT_PACKAGES += \
	dhcpcd.conf \
	hostapd \
	libwpa_client \
	wpa_supplicant \
	wpa_supplicant.conf \
	lib_driver_cmd_mt66xx

PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/hostapd/hostapd.accept:system/etc/hostapd/hostapd.accept \
	$(DEVICE_PATH)/configs/hostapd/hostapd.deny:system/etc/hostapd/hostapd.deny \
	$(DEVICE_PATH)/configs/hostapd/hostapd_default.conf:system/etc/hostapd/hostapd_default.conf \
	$(DEVICE_PATH)/configs/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
	$(DEVICE_PATH)/configs/wifi/wpa_supplicant_overlay.conf:system/etc/wifi/wpa_supplicant_overlay.conf \
	$(DEVICE_PATH)/configs/wifi/p2p_supplicant_overlay.conf:system/etc/wifi/p2p_supplicant_overlay.conf

# Carrier
PRODUCT_COPY_FILES += $(DEVICE_PATH)/configs/carrier/old-apns-conf.xml:system/etc/old-apns-conf.xml

# RIL
SIM_COUNT := 2

PRODUCT_PACKAGES += \
	libxml2 \
	libprotobuf-cpp-full \
	libstlport

PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml

PRODUCT_PROPERTY_OVERRIDES += \
	ro.kernel.android.checkjni=0 \
	ro.telephony.ril_class=grandpplteRIL

# Disable mobile data on first boot
PRODUCT_PROPERTY_OVERRIDES += ro.com.android.mobiledata=false

# Disable Data Roaming
PRODUCT_PROPERTY_OVERRIDES += ro.com.android.dataroaming=false

# Disable SIM keyguard
PRODUCT_PROPERTY_OVERRIDES += keyguard.no_require_sim=true

# FM
MTK_FM_SUPPORT := true

PRODUCT_PACKAGES += \
	libfmjni \
	FMRadio \
	libfmcust

# shim / symbols
PRODUCT_PACKAGES += \
	mtk_symbols \
	libshim_thermal \
	libshim_ssl \
	libshim_camera \
	libshim_agpsd \
	libshim_bionic \
	libshim_xlog

# Platform
PRODUCT_PACKAGES += \
	libem_sensor_jni \
	libstlport

PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.sensor.stepcounter.xml:system/etc/permissions/android.hardware.sensor.stepcounter.xml \
	frameworks/native/data/etc/android.hardware.sensor.stepdetector.xml:system/etc/permissions/android.hardware.sensor.stepdetector.xml \
	frameworks/native/data/etc/android.hardware.faketouch.xml:system/etc/permissions/android.hardware.faketouch.xml \
	frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
	frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
	frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.xml:system/etc/permissions/android.hardware.touchscreen.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
	frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
	frameworks/native/data/etc/android.software.sip.xml:system/etc/permissions/android.software.sip.xml \
	frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

# GPS
PRODUCT_PACKAGES += \
	wifi2agps \
	libepos \
	libcurl
	
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
	$(DEVICE_PATH)/configs/gps/agps_profiles_conf2.xml:system/etc/agps_profiles_conf2.xml

$(call inherit-product, device/common/gps/gps_us_supl.mk)

# Keylayout config
KEYLAYOUTS := \
	$(DEVICE_PATH)/configs/sec_touchscreen.kl

PRODUCT_COPY_FILES += \
	$(foreach f,$(KEYLAYOUTS),$(f):system/usr/keylayout/$(notdir $(f)))

# Camera
PRODUCT_PACKAGES += \
	Snap

#-- camera sensor
CAMERA_SENSOR_TYPE_BACK := "imx219_mipi_raw"
CAMERA_SENSOR_TYPE_FRONT := "s5k5e3yx_mipi_raw"

CAMERA_SUPPORT_SIZE := 8M
FRONT_CAMERA_SUPPORT_SIZE := 5M

#-- samsung camera
BOARD_USE_SAMSUNG_CAMERAFORMAT_YUV420SP := true
BOARD_USE_SAMSUNG_COLORFORMAT_NV21 := true
TARGET_HAS_LEGACY_CAMERA_HAL1 := true
TARGET_NEEDS_LEGACY_CAMERA_HAL1_DYN_NATIVE_HANDLE := true

BOARD_GLOBAL_CFLAGS += -DMETADATA_CAMERA_SOURCE
TARGET_GLOBAL_CFLAGS += -DSAMSUNG_CAMERA_HARDWARE

#-- perm
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
	frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
	frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml

# Graphics
PRODUCT_PACKAGES += \
	libgui_ext \
	libui_ext \
	libGLES_android

# Init
PRODUCT_PACKAGES += \
	libinit_grandpplte

# Power
PRODUCT_PACKAGES += \
	power.default \
	power.mt6735

# Lights
PRODUCT_PACKAGES += \
	lights.mt6735

# Sensor
PRODUCT_PACKAGES += \
	libnvram

# memtrack
PRODUCT_PACKAGES += \
	memtrack.mt6735

# Rootdir
PRODUCT_PACKAGES += \
	enableswap.sh \
	fstab.mt6735 \
	init.modem.rc \
	init.mt6735.rc \
	init.mt6735.usb.rc \
	init.project.rc \
	init.recovery.mt6735.rc \
	init.samsung.rc \
	init.wifi.rc \
	ueventd.mt6735.rc \
	log.sh

#-- sbin
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/rootdir/sbin/busybox:root/sbin/busybox

#-- etc/init/
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/init/audioserver.rc:system/etc/init/audioserver.rc \
	$(DEVICE_PATH)/configs/init/mediaserver.rc:system/etc/init/mediaserver.rc \
	$(DEVICE_PATH)/configs/init/mediacodec.rc:system/etc/init/mediacodec.rc \
	$(DEVICE_PATH)/configs/init/init.grandpplte.rild.rc:system/etc/init/init.grandpplte.rild.rc	

#-- custom logging
BUILD_INCLUDE_CUSTOM_LOG := false
ifeq ($(BUILD_INCLUDE_CUSTOM_LOG), true)
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/init/al-cust-logcat.rc:system/etc/init/cust-log.rc \
	$(DEVICE_PATH)/configs/init/log.sh:system/etc/init/log.sh

endif

#-- default.prop
ADDITIONAL_DEFAULT_PROPERTIES += \
	ro.zygote=zygote32 \
	ro.mount.fs=EXT4 \
	ro.adb.secure=0 \
	ro.secure=0 \
	ro.allow.mock.location=0 \
	ro.debuggable=1 \
	persist.sys.dun.override=0 \
	persist.service.acm.enable=0 \
	persist.sys.usb.config=mtp,adb \
	persist.sys.display.clearMotion=0

PRODUCT_PROPERTY_OVERRIDES += \
    media.stagefright.legacyencoder=true \
    media.stagefright.less-secure=true

# Misc
PRODUCT_PACKAGES += \
	librs_jni \
	libnl_2

PRODUCT_PACKAGES += SamsungServiceMode
PRODUCT_PACKAGES += RemovePackages

# Filesystem management tools
PRODUCT_PACKAGES += \
	libfs_mgr \
	f2fstat \
	fibmap.f2fs \
	e2fsck \
	fsck.f2fs \
	mkfs.f2fs \
	setup_fs \
	make_ext4fs

PRODUCT_PACKAGES += \
	fsck.exfat \
	mkfs.exfat

PRODUCT_PACKAGES += \
	fsck.ntfs \
	mkfs.ntfs \
	mount.ntfs
