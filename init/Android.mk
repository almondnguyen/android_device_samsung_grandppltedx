#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# credit: a3y17lte dev (A3 2017)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := \
	system/core/init \
	external/selinux/libselinux/include \
	system/core/base/include

LOCAL_STATIC_LIBRARIES := libbase
LOCAL_CFLAGS := -Wall -DANDROID_TARGET=\"$(TARGET_BOARD_PLATFORM)\"
LOCAL_SRC_FILES := init_grandpplte.cpp
LOCAL_MODULE := libinit_grandpplte

include $(BUILD_STATIC_LIBRARY)
