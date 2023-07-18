#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	PermissionCache.cpp

LOCAL_SHARED_LIBRARIES := \
	libbinder \
	libutils

LOCAL_MODULE := libshim_binder
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)