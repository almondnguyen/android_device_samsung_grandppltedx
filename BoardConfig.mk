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

# This variable is set first, so it can be overridden
# by BoardConfigVendor.mk

#-include device/samsung/galaxys2-common/BoardCommonConfig.mk

#Sub_Zero2

#Bootloader
TARGET_BOOTLOADER_BOARD_NAME := mt6735
TARGET_NO_BOOTLOADER := true

#Platform
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_BOARD_PLATFORM := mt6737t
TARGET_NO_FACTORYIMAGE := true
TARGET_BOARD_PLATFORM_GPU := mali-t720mp2

#Architecture
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a53
TARGET_KERNEL_ARCH := arm
TARGET_KERNEL_HEADER_ARCH := arm
TARGET_CPU_CORTEX_A53 := true
TARGET_CPU_SMP := true


#Partition & Block
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 3229614080
BOARD_USERDATAIMAGE_PARTITION_SIZE := 3900702720
BOARD_FLASH_BLOCK_SIZE := 131072

#Kernel
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_CMDLINE := bootopt=64S3,32N2,32N2 androidboot.selinux=permissive
BOARD_KERNEL_BASE := 0x3fffc000
BOARD_MKBOOTIMG_ARGS := --base 0x3fffc000 --pagesize 4096 --kernel_offset 0x00008000 --ramdisk_offset 0x04004000 --second_offset 0x00f04000 --tags_offset 0x0e004000 --board 1480056755
MTK_APPENDED_DTB_SUPPORT := yes

#FS
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true

#Display
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540

#boot.img
BOARD_RAMDISK_OFFSET := 04004000
BOARD_SECOND_OFFSET := 00f04000
BOARD_TAGS_OFFSET := 0e004000
BOARD_PAGE_SIZE := 2048
BOARD_SECOND_SIZE := 0
BOARD_DT_SIZE := 485376

#Sub_Zero2



#TARGET_BOARD_INFO_FILE := device/samsung/grandppltedx/board-info.txt

# Device specific headers
#TARGET_SPECIFIC_HEADER_PATH += device/samsung/i9100/include

# Bluetooth
#BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/samsung/grandppltedx/bluetooth

# Inline kernel building
TARGET_KERNEL_SOURCE := kernel/samsung/grandppltedx
TARGET_KERNEL_CONFIG := mt6737t-grandpplte_defconfig

# assert
TARGET_OTA_ASSERT_DEVICE := grandpplte,grandppltedx,SM-G532G

# Use the non-open-source parts, if they're present
-include vendor/samsung/grandppltedx/BoardConfigVendor.mk
