#
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
#

DEVICE_PATH := device/samsung/grandppltedx

TARGET_SPECIFIC_HEADER_PATH := $(DEVICE_PATH)/include

# Display
USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true
PRESENT_TIME_OFFSET_FROM_VSYNC_NS := 0
MTK_HWC_SUPPORT := yes
MTK_HWC_VERSION := 1.4.1

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := mt6737t

# Platform
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_BOARD_PLATFORM := mt6737t
TARGET_NO_BOOTLOADER := true
TARGET_NO_FACTORYIMAGE := true
TARGET_BOARD_PLATFORM_GPU := mali-T720mp2
MTK_GPU_VERSION := mali midgard m7p0

BOARD_HAS_MTK_HARDWARE := true
BOARD_USES_MTK_HARDWARE := true
MTK_HARDWARE := true
BOARD_USES_MTK_MEDIA_PROFILES := true

TARGET_PROVIDES_LIBLIGHT := true

# Omx
TARGET_OMX_LEGACY_RESCALING := true

# Backlight
BACKLIGHT_PATH := /sys/class/leds/lcd-backlight/brightness

# Architecture
# is 32-bit
TARGET_ARCH         := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI      := armeabi-v7a
TARGET_CPU_ABI2     := armeabi
TARGET_CPU_VARIANT  := cortex-a53
TARGET_CPU_SMP      := true

TARGET_2ND_CPU_ABI      := 
TARGET_2ND_CPU_ABI2     := 
TARGET_2ND_ARCH         := 
TARGET_2ND_ARCH_VARIANT := 
TARGET_2ND_CPU_VARIANT  := 

# Block
BOARD_BOOTIMAGE_PARTITION_SIZE        := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE    := 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE      := 3229614080
BOARD_USERDATAIMAGE_PARTITION_SIZE    := 3900702720
BOARD_CACHEIMAGE_PARTITION_SIZE       := 419430400
BOARD_FLASH_BLOCK_SIZE                := 4096

BOARD_USERDATAIMAGES_FILE_SYSTEM_TYPE := ext4 
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE    := ext4
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE     := ext4

# Audio
USE_CUSTOM_AUDIO_POLICY := 1
BOARD_USES_MTK_AUDIO := true

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
#BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED := 0

# CMHW
BOARD_USES_CYANOGEN_HARDWARE := true
BOARD_HARDWARE_CLASS += $(DEVICE_PATH)/cmhw

# GPS
BOARD_GPS_LIBRARIES := true

# RIL
BOARD_RIL_CLASS := ../../../device/samsung/grandppltedx/ril

# Wifi
BOARD_WLAN_DEVICE := MediaTek
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_mt66xx
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_mt66xx
WIFI_DRIVER_FW_PATH_PARAM := /dev/wmtWifi
WIFI_DRIVER_FW_PATH_AP := AP
WIFI_DRIVER_FW_PATH_STA := STA
WIFI_DRIVER_FW_PATH_P2P := P2P
WIFI_DRIVER_STATE_CTRL_PARAM := /dev/wmtWifi
WIFI_DRIVER_STATE_ON := 1
WIFI_DRIVER_STATE_OFF := 0

# Kernel
BOARD_CUSTOM_BOOTIMG := true
#-- use prebuilt
USE_PREBUILT_KERNEL := true

ifeq ($(USE_PREBUILT_KERNEL),true)
	TARGET_PREBUILT_KERNEL  := device/samsung/grandppltedx/prebuilt/kernel
else
	BOARD_KERNEL_IMAGE_NAME := zImage
	TARGET_KERNEL_SOURCE    := kernel/samsung/grandppltedx
	TARGET_KERNEL_CONFIG    := mt6737t-grandpplte-lineage_defconfig
endif

BOARD_KERNEL_CMDLINE  := bootopt=64S3,32N2,32N2 androidboot.selinux=disabled

BOARD_KERNEL_BASE     := 0x3fffc000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_RAMDISK_OFFSET  := 0x04004000
BOARD_SECOND_OFFSET   := 0x00f04000
BOARD_TAGS_OFFSET     := 0x0e004000
BOARD_KERNEL_OFFSET   := 0x00008000
BOARD_KERNEL_BOARD    := SRPPI01A000KU
BOARD_DT_SIZE         := 485376
BOARD_HASH_TYPE       := sha1


BOARD_MKBOOTIMG_ARGS := --base $(BOARD_KERNEL_BASE) --pagesize $(BOARD_KERNEL_PAGESIZE) --kernel_offset $(BOARD_KERNEL_OFFSET) --ramdisk_offset $(BOARD_RAMDISK_OFFSET) --second_offset $(BOARD_SECOND_OFFSET) --tags_offset $(BOARD_TAGS_OFFSET) --dt $(DEVICE_PATH)/dt.img --board $(BOARD_KERNEL_BOARD) --dt_size $(BOARD_DT_SIZE)

# twrp is broken in cm-13.0. use base
#TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/fstab.mt6735

# GPS
BOARD_GPS_LIBRARIES := true

# system properties
TARGET_SYSTEM_PROP += $(DEVICE_PATH)/system.prop

# SELinux
BOARD_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy
BOARD_SECCOMP_POLICY += $(COMMON_PATH)/seccomp
# testing

# assert
TARGET_OTA_ASSERT_DEVICE := grandpplte,grandppltedx

# Inherit from mt6735-common
#FORCE_32_BIT := true
#include device/cyanogen/mt6735-common/BoardConfigCommon.mk
