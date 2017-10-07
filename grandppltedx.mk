#
# Copyright (C) 2012 The Android Open-Source Project
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
#

# Include common makefile
#$(call inherit-product, device/samsung/galaxys2-common/common.mk)

$(call inherit-product, device/cyanogen/mt6735-common/mt6735.mk) 

LOCAL_PATH := device/samsung/grandppltedx

# Overlay
#DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# This device is hdpi.
PRODUCT_AAPT_CONFIG := normal xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi
#PRODUCT_LOCALES += hdpi

#PRODUCT_PROPERTY_OVERRIDES += \
#    ro.sf.lcd_density=240

# Lights
#PRODUCT_PACKAGES += \
    lights.mt6753

# Keylayout
#PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/mtk-kpd.kl:system/usr/keylayout/mtk-kpd.kl \
    $(LOCAL_PATH)/configs/ACCDET.kl:system/usr/keylayout/ACCDET.kl \
    $(LOCAL_PATH)/configs/AVRCP.kl:system/usr/keylayout/AVRCP.kl

# Ramdisk
#PRODUCT_PACKAGES += \
#    fstab.mt6735 \
#    init.modem.rc \
#    init.rc \
#    init.mt6735.rc \
#    init.mt6735.usb.rc \
#    init.wifi.rc \
#    ueventd.mt6735.rc

#PRODUCT_COPY_FILES += \
#    $(call find-copy-subdir-files,*,${LOCAL_PATH}/ramdisk,root)

#PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/fstab.mt6735:root/fstab.mt6735 \
    $(LOCAL_PATH)/rootdir/init.modem.rc:root/init.modem.rc \
    $(LOCAL_PATH)/rootdir/init.mt6735.rc:root/init.mt6735.rc \
    $(LOCAL_PATH)/rootdir/init.mt6735.usb.rc:root/init.mt6735.usb.rc \
    $(LOCAL_PATH)/rootdir/ueventd.mt6735.rc:root/ueventd.mt6735.rc \
    $(LOCAL_PATH)/rootdir/init.rc:root/init.rc
#    $(LOCAL_PATH)/rootdir/factory_init.project.rc:root/factory_init.project.rc \
#    $(LOCAL_PATH)/rootdir/factory_init.rc:root/factory_init.rc \    $(LOCAL_PATH)/rootdir/meta_init.modem.rc:root/meta_init.modem.rc \
#    $(LOCAL_PATH)/rootdir/meta_init.project.rc:root/meta_init.project.rc \
#    $(LOCAL_PATH)/rootdir/meta_init.rc:root/meta_init.rc \
    
    
# Thermal
#PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/thermal/.ht120.mtc:system/etc/.tp/.ht120.mtc \
    $(LOCAL_PATH)/configs/thermal/thermal.conf:system/etc/.tp/thermal.conf \
    $(LOCAL_PATH)/configs/thermal/thermal.off.conf:system/etc/.tp/thermal.off.conf \
    $(LOCAL_PATH)/configs/thermal/.thermal_policy_00:system/etc/.tp/.thermal_policy_00

# Proprietary blobs dependency on libstlport
#PRODUCT_PACKAGES +=  \
#    libaudiopolicy
   
    
$(call inherit-product-if-exists, vendor/samsung/grandppltedx/grandppltedx-vendor.mk)
