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
	libandroidicu \
	libssl \
	libcrypto

LOCAL_MODULE := libshim_gps
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
