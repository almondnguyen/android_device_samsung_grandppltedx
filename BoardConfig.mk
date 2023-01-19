#
# Copyright (C) 2022 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#	  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DEVICE_PATH := device/samsung/grandppltedx
VENDOR_PATH := vendor/samsung/grandppltedx
BOARD_VENDOR := samsung

# device doesn't have /vendor partition
# yet.
TARGET_COPY_OUT_VENDOR := system/vendor

# assert
TARGET_OTA_ASSERT_DEVICE := grandpplte,grandppltedx,grandpplteub,grandpplteser,grandppltedtvvj

# Headers
TARGET_SPECIFIC_HEADER_PATH := $(DEVICE_PATH)/include

# Project Configs
MTK_PROJECT_CONFIG ?= $(DEVICE_PATH)/ProjectConfig.mk
include $(MTK_PROJECT_CONFIG)

# Display
USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true
PRESENT_TIME_OFFSET_FROM_VSYNC_NS := 0
BOARD_EGL_CFG := $(DEVICE_PATH)/configs/egl.cfg
OVERRIDE_RS_DRIVER := libRSDriver_mtk.so

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := MT6737T
TARGET_NO_BOOTLOADER := true
# mt6737t

# Platform
TARGET_INIT_VENDOR_LIB := libinit_grandpplte

ARCH_ARM_HAVE_TLS_REGISTER := true
ARCH_ARM_HAVE_VFP := true
ARCH_ARM_HAVE_NEON := true

TARGET_BOARD_PLATFORM := mt6737t
TARGET_NO_FACTORYIMAGE := true
TARGET_BOARD_PLATFORM_GPU := mali-T720mp2

MTK_GPU_VERSION := mali midgard r18p0
BOARD_HAS_MTK_HARDWARE := true
BOARD_USES_MTK_HARDWARE := true
MTK_HARDWARE := true
BOARD_USES_MTK_MEDIA_PROFILES := true

BOARD_SUPPRESS_SECURE_ERASE := true
BOARD_NO_SECURE_DISCARD := true
BOARD_DISABLE_HW_ID_MATCH_CHECK := true

PRODUCT_SHIPPING_API_LEVEL := 23

# Liblight
TARGET_PROVIDES_LIBLIGHT := true

# Gatekeeper
BOARD_USE_SOFT_GATEKEEPER := true

# Backlight
BACKLIGHT_PATH := /sys/class/leds/lcd-backlight/brightness

# Sensors
TARGET_NO_SENSOR_PERMISSION_CHECK := true

# Lights
TARGET_HAS_BACKLIT_KEYS := false

# Charger (borrow from herolte)
BOARD_CHARGING_MODE_BOOTING_LPM := /sys/class/power_supply/battery/batt_lp_charging
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_CHARGER_SHOW_PERCENTAGE := true
CHARGING_ENABLED_PATH := /sys/class/power_supply/battery/batt_lp_charging

# Architecture
TARGET_SOC		:= mt6737t
TARGET_ARCH		:= arm
TARGET_ARCH_VARIANT	:= armv7-a-neon
TARGET_CPU_ABI		:= armeabi-v7a
TARGET_CPU_ABI2		:= armeabi
TARGET_CPU_VARIANT	:= cortex-a53
TARGET_CPU_SMP		:= true

TARGET_2ND_ARCH		:= 
TARGET_2ND_ARCH_VARIANT	:= 
TARGET_2ND_CPU_ABI	:= 
TARGET_2ND_CPU_ABI2	:=  
TARGET_2ND_CPU_VARIANT	:= 

# Block
BOARD_BOOTIMAGE_PARTITION_SIZE		:= 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE	:= 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE	:= 3229614080
BOARD_CACHEIMAGE_PARTITION_SIZE		:= 419430400
BOARD_FLASH_BLOCK_SIZE			:= 4096
BOARD_USERDATAIMAGE_PARTITION_SIZE	:= 3900702720

BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE	:= ext4
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

# Kernel
BOARD_CUSTOM_BOOTIMG := true

BOARD_KERNEL_IMAGE_NAME := zImage
# al: Test mainline kernel
TARGET_KERNEL_SOURCE	:= kernel/samsung/grandppltedx
#TARGET_KERNEL_SOURCE	:= kernel/samsung/mainline-test
TARGET_KERNEL_CONFIG	:= mt6737t-grandpplte_defconfig
TARGET_PREBUILT_DTB	:= $(DEVICE_PATH)/dt.img

BOARD_KERNEL_CMDLINE	:= bootopt=64S3,32N2,32N2 androidboot.selinux=permissive
BOARD_KERNEL_BASE	:= 0x3fffc000
BOARD_KERNEL_PAGESIZE	:= 2048
BOARD_RAMDISK_OFFSET	:= 0x04004000
BOARD_SECOND_OFFSET	:= 0x00f04000
BOARD_TAGS_OFFSET	:= 0x0e004000
BOARD_KERNEL_OFFSET	:= 0x00008000
BOARD_DT_SIZE		:= 485376
ifeq ($(RECOVERY_VARIANT),twrp)
BOARD_NAME := SRPPI01A000RU
else
BOARD_NAME := SRPPI01A000KU
endif

BOARD_MKBOOTIMG_ARGS := \
	--base $(BOARD_KERNEL_BASE) \
	--pagesize $(BOARD_KERNEL_PAGESIZE) \
	--kernel_offset $(BOARD_KERNEL_OFFSET) \
	--ramdisk_offset $(BOARD_RAMDISK_OFFSET) \
	--second_offset $(BOARD_SECOND_OFFSET) \
	--tags_offset $(BOARD_TAGS_OFFSET) \
	--board $(BOARD_NAME) \
	--dt $(TARGET_PREBUILT_DTB)

# CMHW
BOARD_USES_LINEAGE_HARDWARE := true
BOARD_USES_CYANOGEN_HARDWARE := true
BOARD_HARDWARE_CLASS += $(DEVICE_PATH)/lineagehw

# Recovery
BOARD_HAS_NO_SELECT_BUTTON := true
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/fstab.mt6735

# Move symlinks here
TARGET_LD_SHIM_LIBS := \
	/system/vendor/bin/mtk_agpsd|libshim_gps.so \
	/system/vendor/lib/libcam_utils.so|libshim_camera.so \
	/system/vendor/lib/libcam_utils.so|libshim_gui.so \
	/system/vendor/lib/liblog.so|libshim_xlog.so \
	/system/vendor/lib/libui_ext.so|libshim_gui.so \
	/system/vendor/lib/libgui_ext.so|libshim_gui.so \
	/system/vendor/lib/audio.primary.mt6737t.so|libshim_audio.so \
	/system/vendor/bin/thermal|libshim_ifc.so \
	/system/vendor/bin/libdpframework.so|libshim_xlog.so

LD_PRELOADS += mtk_symbols.so

# Audio
BOARD_USES_MTK_AUDIO := true
USE_XML_AUDIO_POLICY_CONF := 1
SUPPRESS_MTK_AUDIO_BLOB_ERR_MSG := true

# Exclude AudioFx
TARGET_EXCLUDES_AUDIOFX := true

# Bluetooth
MTK_BT_SUPPORT := yes
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_MTK := true
BOARD_BLUETOOTH_DOES_NOT_USE_RFKILL := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(DEVICE_PATH)/bluetooth

# Media
MTK_MEDIA_PROFILES := true
BOARD_USES_MTK_MEDIA_PROFILES := true

TARGET_OMX_LEGACY_RESCALING := true

BOARD_CANT_REALLOCATE_OMX_BUFFERS := true

# GPS
BOARD_GPS_LIBRARIES := true
BOARD_MEDIATEK_USES_GPS := true

# RIL
#> accompanies https://github.com/almondnguyen/android_device_samsung_grandppltedx/commit/5eacb461d71c013e97552f122f1ee28bed8b9d01
TARGET_BUILD_MTK_RIL := true
BOARD_PROVIDES_RILD := true
BOARD_PROVIDES_LIBRIL := true
BOARD_RIL_CLASS := ../../../device/samsung/grandppltedx/ril
#BOARD_CONNECTIVITY_MODULE := conn_soc

# Power HAL
TARGET_POWERHAL_VARIANT := mtk
TARGET_POWERHAL_SET_INTERACTIVE_EXT := $(DEVICE_PATH)/power/power.c

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

# Camera
USE_CAMERA_STUB := true
TARGET_HAS_LEGACY_CAMERA_HAL1 := true
TARGET_NEEDS_LEGACY_CAMERA_HAL1_DYN_NATIVE_HANDLE := true
TARGET_USES_NON_TREBLE_CAMERA := true
BOARD_USE_SAMSUNG_CAMERAFORMAT_YUV420SP := true

# system properties
TARGET_SYSTEM_PROP += $(DEVICE_PATH)/system.prop

# SEAndroid
#BOARD_SEPOLICY_DIRS := device/samsung/grandppltedx/sepolicy/samsung device/samsung/grandppltedx/sepolicy/mtk

BOARD_SECCOMP_POLICY += $(DEVICE_PATH)/seccomp

# Enable Minikin text layout engine (will be the default soon)
USE_MINIKIN := true

# Configure jemalloc for low memory
MALLOC_SVELTE := true

# Webkit
ENABLE_WEBGL := true
TARGET_FORCE_CPU_UPLOAD := true

# Disable API check
WITHOUT_CHECK_API := true

# Manifest
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/configs/manifest.xml

# Misc
EXTENDED_FONT_FOOTPRINT := true

#-- Disable ODEX
#-- not buildable on Jammy
WITH_DEXPREOPT := false
DONT_DEXPREOPT_PREBUILTS := true
