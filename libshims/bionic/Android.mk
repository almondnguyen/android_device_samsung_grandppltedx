#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := bionic_shim.cpp
LOCAL_SHARED_LIBRARIES := libc
LOCAL_MODULE := libshim_bionic
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
