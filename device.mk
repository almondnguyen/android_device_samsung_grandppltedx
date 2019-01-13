#
# Copyright (C) 2012 The Android Open-Source Project
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

# Define Path
DEVICE_PATH := device/samsung/grandppltedx
VENDOR_PATH := vendor/samsung/grandppltedx
#-- Credit: GearLabs | mt6737-common
# Because Samsung use their own 'phone structure' (lets just call that)
# Example: system/bin/macloader | system/lib/libsec-ril.so
# Others:  -------------------- | system/lib/mtk-ril.so
# So not inherit mt6737.mk
MT6737_PATH := device/mediatek/mt6737-common


# tempo fix. 

#-- audio policy service is loaded before manager AND NEED IT
# make fails
# TODO: Better solution?
$(shell (mkdir -p out/target/product/grandppltedx/obj/SHARED_LIBRARIES/libaudiopolicymanager_intermediates))
$(shell (mkdir -p out/target/product/grandppltedx/obj/lib))
$(shell (cp device/samsung/grandppltedx/dummy out/target/product/grandppltedx/obj/SHARED_LIBRARIES/libaudiopolicymanager_intermediates/export_includes))
$(shell (cp vendor/samsung/grandppltedx/proprietary/system/lib/libaudiopolicymanager.so out/target/product/grandppltedx/obj/lib/libaudiopolicymanager.so))

#-- install-recovery.sh too. is not copied correctly!
$(shell (mkdir -p out/target/product/grandppltedx/ota_temp/SYSTEM/bin))
$(shell (cp vendor/samsung/grandppltedx/proprietary/system/bin/install-recovery.sh out/target/product/grandppltedx/ota_temp/SYSTEM/bin/install-recovery.sh))


# Overlay
DEVICE_PACKAGE_OVERLAYS += $(MT6737_PATH)/overlay

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml

# Display
#-- This device is xhdpi.
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi
PRODUCT_AAPT_PREBUILT_DPI := xhdpi

TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540

PRODUCT_PACKAGES += \
    libion

#-- Dalvik heap configurations
$(call inherit-product-if-exists, frameworks/native/build/phone-xhdpi-2048-dalvik-heap.mk)

#-- Call hwui memory config
$(call inherit-product-if-exists, frameworks/native/build/phone-xhdpi-2048-hwui-memory.mk)

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

# Radio
#-- Radio dependencies
PRODUCT_PACKAGES += \
    muxreport \
    terservice

#-- Carrier
PRODUCT_COPY_FILES += \
	$(DEVICE_PATH)/configs/carrier/apns-conf.xml:system/etc/apns-conf.xml \
	$(DEVICE_PATH)/configs/carrier/spn-conf.xml:system/etc/spn-conf.xml

#-- Telephony
PRODUCT_PROPERTY_OVERRIDES += \
	ro.telephony.sim.count=2 \
	persist.radio.default.sim=0 \
	persist.radio.multisim.config=dsds

#-- FM
PRODUCT_PACKAGES += \
    libfmjni \
    FMRadio

# GPS
PRODUCT_PACKAGES += \
    libcurl

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/agps_profiles_conf2.xml:system/etc/agps_profiles_conf2.xml

# Power
PRODUCT_PACKAGES += \
    power.default \
    power.mt6737t

# Lights
PRODUCT_PACKAGES += \
	lights.mt6753

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

# Ramdisk
PRODUCT_PACKAGES += \
	enableswap.sh \
	factory_init.rc \
	factory_init.project.rc \
	fstab.mt6735 \
	init.modem.rc \
	init.mt6735.rc \
	init.mt6735.usb.rc \
	init.project.rc \
	init.rilcommon.rc \
	init.rilepdg.rc \
	init.rilchip.rc \
	init.volte.rc \
    init.usb.configfs.rc \
	init.wifi.rc \
	meta_init.rc \
	meta_init.modem.rc \
	meta_init.project.rc \
	meta_init.usb.rc \
	ueventd.mt6735.rc

ADDITIONAL_DEFAULT_PROPERTIES += \
	rild.libpath=/system/lib/libsec-ril.so \
	rild.libpath2=/system/lib/libsec-ril-dsds.so \
	ro.zygote=zygote32 \
	persist.sys.usb.config=mtp

# Vendor
$(call inherit-product-if-exists, vendor/samsung/grandppltedx/grandppltedx-vendor.mk)
