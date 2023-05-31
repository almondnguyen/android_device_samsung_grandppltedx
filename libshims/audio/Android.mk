#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# Audio Shim
include $(CLEAR_VARS)

LOCAL_SRC_FILES := audio_shim.cpp

LOCAL_MODULE := libshim_audio
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
