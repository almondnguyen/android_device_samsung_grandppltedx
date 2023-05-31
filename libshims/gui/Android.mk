#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# GUI Shim
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gui_shim.cpp \
    dump_tunel_shim.cpp

LOCAL_SHARED_LIBRARIES := \
	libui \
	libgui \
	libdpframework \
    liblog

LOCAL_STATIC_LIBRARIES := \
    libaudiopolicycomponents

LOCAL_C_INCLUDES += system/core/include/ frameworks/av/include/ hardware/libhardware/include/    

LOCAL_MODULE := libshim_gui
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
