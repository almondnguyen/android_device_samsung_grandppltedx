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

$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/core.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
BLOCK_BASED_OTA := true

# Device is a phone
PRODUCT_CHARACTERISTICS := phone

# Define Path
DEVICE_PATH := device/samsung/grandppltedx
VENDOR_PATH := vendor/samsung/grandppltedx

# Vendor
$(call inherit-product-if-exists, vendor/samsung/grandppltedx/grandppltedx-vendor.mk)

# Overlay
DEVICE_PACKAGE_OVERLAYS += $(DEVICE_PATH)/overlay

# Display
#-- This device is hdpi.
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_AAPT_PREBUILT_DPI := hdpi
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540
$(call inherit-product, frameworks/native/build/phone-hdpi-dalvik-heap.mk)

PRODUCT_PACKAGES += \
	libion

# Locale
PRODUCT_DEFAULT_LANGUAGE := en
PRODUCT_DEFAULT_REGION   := US

# Configs
#-- Audio
PRODUCT_PACKAGES += \
	audio.a2dp.default \
	audio.usb.default \
	audio.r_submix.default \
	audio_policy.default \
	libaudiopolicymanagerdefault \
	libaudiopolicymanager \
	libaudiopolicyservice \
	libaudiopolicyenginedefault \
	libaudio-resampler \
	libaudioroute \
	libaudioutils \
	libaudiospdif \
	libalsautils \
	libeffects \
	libtinyalsa \
	libtinycompress \
	libtinymix \
	libtinyxml \
	libfs_mgr

PRODUCT_PACKAGES += \
	android.hardware.audio@2.0-impl \
	android.hardware.audio.effect@2.0-impl

PRODUCT_COPY_FILES += \
	frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:system/etc/a2dp_audio_policy_configuration.xml \
	frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:/system/etc/audio_policy_volumes.xml \
	frameworks/av/services/audiopolicy/config/default_volume_tables.xml:/system/etc/default_volume_tables.xml \
	frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:/system/etc/r_submix_audio_policy_configuration.xml \
	frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:/system/etc/usb_audio_policy_configuration.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
	frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
	frameworks/native/data/etc/android.software.midi.xml:system/etc/permissions/android.software.midi.xml \
	$(DEVICE_PATH)/configs/audio/audio_policy.conf:system/etc/audio_policy.conf \
	$(DEVICE_PATH)/configs/audio/audio_device.xml:system/etc/audio_device.xml \
	$(DEVICE_PATH)/configs/audio/audio_effects.conf:system/etc/audio_effects.conf \
	$(DEVICE_PATH)/configs/audio/audio_param/AudioParamOptions.xml:system/etc/audio_param/AudioParamOptions.xml

#-- Media
PRODUCT_COPY_FILES += \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video_le.xml:system/etc/media_codecs_google_video_le.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_performance.xml:system/etc/media_codecs_performance.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_mediatek_audio.xml:system/etc/media_codecs_mediatek_audio.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_mediatek_video.xml:system/etc/media_codecs_mediatek_video.xml \
	$(DEVICE_PATH)/configs/media/media_codecs.xml:system/etc/media_codecs.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_sec_primary.xml:system/etc/media_codecs_sec_primary.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_sec_secondary.xml:system/etc/media_codecs_sec_secondary.xml \
	$(DEVICE_PATH)/configs/media/media_profiles.xml:system/etc/media_profiles.xml

PRODUCT_PROPERTY_OVERRIDES += \
	media.sf.omx-plugin=libffmpeg_omx.so,libsomxcore.so

#-- BT
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
	lib_driver_cmd_mt66xx \
	android.hardware.wifi@1.0-service \
	wificond

PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/hostapd/hostapd.accept:system/etc/hostapd/hostapd.accept \
	$(DEVICE_PATH)/configs/hostapd/hostapd.deny:system/etc/hostapd/hostapd.deny \
	$(DEVICE_PATH)/configs/hostapd/hostapd_default.conf:system/etc/hostapd/hostapd_default.conf \
	$(DEVICE_PATH)/configs/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
	$(DEVICE_PATH)/configs/wifi/wpa_supplicant_overlay.conf:system/etc/wifi/wpa_supplicant_overlay.conf \
	$(DEVICE_PATH)/configs/wifi/p2p_supplicant_overlay.conf:system/etc/wifi/p2p_supplicant_overlay.conf

# Radio
#PRODUCT_PACKAGES += \
#	libsecnativefeature

PRODUCT_PROPERTY_OVERRIDES += \
	ro.kernel.android.checkjni=0

#-- Carrier
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/carrier/spn-conf.xml:system/etc/spn-conf.xml

#-- RIL
SIM_COUNT := 2

PRODUCT_PACKAGES += \
	libxml2 \
	libprotobuf-cpp-full \
	SamsungStk

PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml

PRODUCT_PROPERTY_OVERRIDES += \
	ro.kernel.android.checkjni=0 \
	ro.telephony.ril_class=grandpplteRIL

#-- FM
MTK_FM_SUPPORT := true

PRODUCT_PACKAGES += \
	libfmjni \
	FMRadio \
	libfmcust \
	radio.fm.mt6735

# shim / symbols
PRODUCT_PACKAGES += \
	libshim_gps \
	libshim_camera \
	libshim_vt \
	libshim_xlog \
	libshim_asc \
	libshim_gui \
	libshim_audio \
	libshim_ifc

# Platform
PRODUCT_PACKAGES += \
	libem_sensor_jni \
	libstlport \
	libgralloc_extra \
	libgui_ext \
	libui_ext \
	libperfservicenative \
	libnvram \
	busybox

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

# Recovery - twrp
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/recovery.fstab:recovery/root/etc/twrp.fstab

# GPS
PRODUCT_PACKAGES += \
	wifi2agps \
	libepos \
	libcurl
	
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
	$(DEVICE_PATH)/configs/gps/agps_profiles_conf2.xml:system/etc/agps_profiles_conf2.xml \
	$(DEVICE_PATH)/configs/gps/slp_conf:system/etc/slp_conf	

$(call inherit-product, device/common/gps/gps_us_supl.mk)

PRODUCT_COPY_FILES += \

# Charger
# Use cm/lineage images if available, aosp ones otherwise
PRODUCT_PACKAGES += \
	lineage_charger_res_images

# Camera
PRODUCT_PACKAGES += \
	Snap \
	android.hardware.camera.provider@2.4-impl-legacy

#-- perm
PRODUCT_COPY_FILES += \
	frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
	frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
	frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml

# Init
PRODUCT_PACKAGES += \
	libinit_grandpplte

# Power
PRODUCT_PACKAGES += \
	power.mt6737t \
	android.hardware.power@1.0-impl

# Lights
PRODUCT_PACKAGES += \
	lights.mt6737t \
	android.hardware.light@2.0-impl-mediatek

# Keymaster
PRODUCT_PACKAGES += android.hardware.keymaster@3.0-impl

# Memtrack
PRODUCT_PACKAGES += android.hardware.memtrack@1.0-impl

# Local Time
PRODUCT_PACKAGES += local_time.default

# Rootdir
PRODUCT_PACKAGES += \
	enableswap.sh \
	fstab.mt6735 \
	init.modem.rc \
	init.mt6735.rc \
	init.mt6735.usb.rc \
	init.project.rc \
	init.rilcommon.rc \
	init.rilchip.rc \
	init.rilepdg.rc \
	init.volte.rc \
	init.emdlogger1.rc \
	init.usb.configfs.rc \
	init.wifi.rc \
	init.recovery.mt6735.rc \
	init.samsung.rc \
	ueventd.mt6735.rc \
	init.xlog.rc \
	log.sh


#-- sbin

PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/rootdir/sbin/sswap:root/sbin/sswap \
	$(DEVICE_PATH)/rootdir/sbin/busybox:root/sbin/busybox

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	rild.libpath=/system/lib/libsec-ril.so \
	rild.libpath2=/system/lib/libsec-ril-dsds.so \
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
	 
# Misc
PRODUCT_PACKAGES += \
	librs_jni \
	libnl_2
	
# Filesystem management tools
PRODUCT_PACKAGES += \
	f2fstat \
	fibmap.f2fs \
	e2fsck \
	fsck.f2fs \
	mkfs.f2fs \
	setup_fs \
	make_ext4fs

# exFAT
PRODUCT_PACKAGES += \
	fsck.exfat \
	mkfs.exfat

# NTFS
PRODUCT_PACKAGES += \
	fsck.ntfs \
	mkfs.ntfs \
	mount.ntfs
