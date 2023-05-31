#
# Copyright (C) 2023 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

LOCAL_PATH:= $(call my-dir)

# Thermal Shim
include $(CLEAR_VARS)
LOCAL_SRC_FILES := ifc_shim.c

LOCAL_SHARED_LIBRARIES := liblog libcutils

LOCAL_MODULE := libshim_ifc

LOCAL_C_INCLUDES += system/core/libnetutils/include
LOCAL_MODULE_TAGS := optional

LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
