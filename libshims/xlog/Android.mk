#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# XLOG Shim
include $(CLEAR_VARS)

LOCAL_SRC_FILES := xlog_shim.cpp

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_MODULE := libshim_xlog

LOCAL_PROPRIETARY_MODULE := false
include $(BUILD_SHARED_LIBRARY)
