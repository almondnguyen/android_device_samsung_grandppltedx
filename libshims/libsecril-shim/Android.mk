#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# libsec-ril shim
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	secril-shim.cpp

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libril \
	libcutils \
	libbinder

LOCAL_CFLAGS := -Wall -Werror

LOCAL_MODULE := libsecril-shim
LOCAL_VENDOR_MODULE := true

include $(BUILD_SHARED_LIBRARY)
