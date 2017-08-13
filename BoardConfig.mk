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
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a
TARGET_CPU_VARIANT := cortex-a53
BOARD_KERNEL_PAGESIZE := 2048
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 3229614080
BOARD_USERDATAIMAGE_PARTITION_SIZE := 3900702720
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
