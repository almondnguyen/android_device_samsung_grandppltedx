#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# Camera Shim
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	camera_shim.cpp \
	libshim_nv21e_camera.c

LOCAL_MODULE := libshim_camera
LOCAL_SHARED_LIBRARIES := libui

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

## libshim_mmsdk
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	MediaBuffer.cpp \

LOCAL_SHARED_LIBRARIES := \
	libgui \
	libui \
	libstagefright_foundation

LOCAL_MODULE := libshim_mmsdk

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -O3 -Wno-unused-variable -Wno-unused-parameter
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
