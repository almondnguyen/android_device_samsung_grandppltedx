#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# GPS Shim
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	icu_shim.cpp

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libicuuc \
	libssl \
	libcrypto

LOCAL_MODULE := libshim_gps
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

# mtk_agpsd
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mtkagpsd_shim.c

LOCAL_MODULE := libshim_agpsd

LOCAL_C_INCLUDES := external/boringssl/include

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
