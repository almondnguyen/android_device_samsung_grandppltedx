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

#-- Credit: 
# GearLabs | mt6737-common
# MAD Team | android_vendor_MAD
# Darklord | woods (motorola-mt6737m) dev

$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/core.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Define Path
DEVICE_PATH := device/samsung/grandppltedx

# Vendor
$(call inherit-product-if-exists, vendor/samsung/grandppltedx/grandppltedx-vendor.mk)

# temporary fix. 
include device/samsung/grandppltedx/configs/tempo-fix.mk

# Overlay
DEVICE_PACKAGE_OVERLAYS += $(DEVICE_PATH)/overlay

# Include permission.mk
#-- Separated for easier editing
include device/samsung/grandppltedx/configs/permissions.mk

# Display
#-- This device is hdpi.
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_AAPT_PREBUILT_DPI := hdpi

TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540

PRODUCT_PACKAGES += \
    libion

# Dalvik heap configurations
$(call inherit-product-if-exists, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)

# Locale
PRODUCT_DEFAULT_LANGUAGE := en
PRODUCT_DEFAULT_REGION   := US

# Configs
#-- Audio
PRODUCT_PACKAGES += \
    audio.a2dp.default \
    audio.r_submix.default \
    libaudiopolicymanagerdefault \
    libtinyalsa \
    libtinycompress \
    libtinymix \
    libtinyxml \
    libfs_mgr

PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:/system/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:/system/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:/system/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:/system/etc/usb_audio_policy_configuration.xml \
	$(DEVICE_PATH)/configs/audio/AudioParamOptions.xml:system/etc/audio_param/AudioParamOptions.xml \
	$(DEVICE_PATH)/configs/audio/audio_policy.conf:system/etc/audio_policy.conf \
	$(DEVICE_PATH)/configs/audio/audio_device.xml:system/etc/audio_device.xml \
    $(DEVICE_PATH)/configs/audio/audio_effects.conf:system/etc/audio_effects.conf

#-- Media
PRODUCT_COPY_FILES += \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video_le.xml:system/etc/media_codecs_google_video_le.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_performance.xml:system/etc/media_codecs_performance.xml \
	$(DEVICE_PATH)/configs/media/media_profiles.xml:system/etc/media_profiles.xml \
	$(DEVICE_PATH)/configs/media/media_codecs.xml:system/etc/media_codecs.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_mediatek_audio.xml:system/etc/media_codecs_mediatek_audio.xml \
	$(DEVICE_PATH)/configs/media/media_codecs_mediatek_video.xml:system/etc/media_codecs_mediatek_video.xml

# Wifi
PRODUCT_PACKAGES += \
	dhcpcd.conf \
	hostapd \
	libwpa_client \
	wpa_supplicant \
	wpa_supplicant.conf \
	lib_driver_cmd_mt66xx

PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
	$(DEVICE_PATH)/configs/wifi/wpa_supplicant_overlay.conf:system/etc/wifi/wpa_supplicant_overlay.conf \
	$(DEVICE_PATH)/configs/wifi/p2p_supplicant_overlay.conf:system/etc/wifi/p2p_supplicant_overlay.conf

# Radio
#-- Radio dependencies
PRODUCT_PACKAGES += \
    muxreport \
    terservice

PRODUCT_PACKAGES += \
    libsecnativefeature

PRODUCT_PROPERTY_OVERRIDES += \
    ro.kernel.android.checkjni=0

#-- Carrier
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/carrier/apns-conf.xml:system/etc/apns-conf.xml \
	$(DEVICE_PATH)/configs/carrier/spn-conf.xml:system/etc/spn-conf.xml

#-- RIL
#-- use open-sourced libril @ android_hardware_samsung
BOARD_PROVIDE_LIBRIL := true
ifeq ($(TARGET_DEVICE),grandppltedx)
    # Dual SIM variant
	SIM_COUNT := 2
endif

BOARD_NEEDS_IMS_TYPE_FIELD := true
BOARD_MODEM_TYPE := mt6625

PRODUCT_PACKAGES += \
	libril

#-- FM
PRODUCT_PACKAGES += \
    libfmjni \
    FMRadio

# Mediatek platform
PRODUCT_PACKAGES += \
	mtk_symbols

# Recovery - twrp
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/recovery.fstab:recovery/root/etc/twrp.fstab

# Thermal
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/thermal/.ht120.mtc:system/etc/.tp/.ht120.mtc \
    $(DEVICE_PATH)/configs/thermal/thermal.conf:system/etc/.tp/thermal.conf \
    $(DEVICE_PATH)/configs/thermal/thermal.off.conf:system/etc/.tp/thermal.off.conf

# GPS
PRODUCT_PACKAGES += \
    libcurl

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/agps_profiles_conf2.xml:system/etc/agps_profiles_conf2.xml

# Init
PRODUCT_PACKAGES += \
	libinit_grandpplte

# Power
PRODUCT_PACKAGES += \
    power.default \
    power.mt6737t

# Camera
PRODUCT_PACKAGES += \
    Snap

# Lights
PRODUCT_PACKAGES += \
	lights.mt6737t

# Rootdir
PRODUCT_PACKAGES += \
	enableswap.sh \
	factory_init.rc \
	factory_init.project.rc \
	fstab.mt6735 \
	init.modem.rc \
	init.mt6735.rc \
	init.mt6735.usb.rc \
	init.project.rc \
	init.ril.rc \
	init.volte.rc \
    init.usb.configfs.rc \
	init.wifi.rc \
	meta_init.rc \
	meta_init.modem.rc \
	meta_init.project.rc \
	meta_init.usb.rc \
	init.recovery.mt6735.rc \
	ueventd.mt6735.rc

#-- Modded init.rc
# Because Sam.
PRODUCT_COPY_FILE += \
	$(ROOT_PATH)/init.rc:root/init.rc
#-- sbin
#PRODUCT_COPY_FILES += \
#	$(DEVICE_PATH)/rootdir/sbin/sswap:root/sbin/sswap
#	$(DEVICE_PATH)/rootdir/sbin/ffu:root/sbin/ffu
#   $(DEVICE_PATH)/rootdir/sbin/busybox:root/sbin/busybox \

ADDITIONAL_DEFAULT_PROPERTIES += \
	rild.libpath=/system/lib/libsec-ril.so \
	rild.libpath2=/system/lib/libsec-ril-dsds.so \
	ro.zygote=zygote32 \
	ro.mount.fs=EXT4 \
	ro.adb.secure=0 \
	ro.secure=0 \
	ro.allow.mock.location=0 \
	ro.debuggable=1
#	persist.sys.usb.config=mtp \
#	persist.service.acm.enable=0

# Charger Mode
PRODUCT_PACKAGES += \
	charger \
    charger_res_images

# Misc
PRODUCT_PACKAGES += \
    librs_jni \
    libnl_2
