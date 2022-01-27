# Copyright 2017 The Android Open Source Project
#
# Oreo RIL daemon for MTK devices - Version (0.9).
# by: daniel_hk(https://github.com/danielhk)
#
# start with the AOSP rild
#
# 2017/7/29: Initial Oero port for MT6752	- by:daniel_hk
#

ifeq ($(BOARD_PROVIDES_RILD),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE = rild-prop-md1
LOCAL_MODULE_CLASS = STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX = .a
LOCAL_PROPRIETARY_MODULE = true
LOCAL_UNINSTALLABLE_MODULE = true
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = arm64/rild-prop-md1.a
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE = rild-prop-md1
LOCAL_MODULE_CLASS = STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX = .a
LOCAL_PROPRIETARY_MODULE = true
LOCAL_UNINSTALLABLE_MODULE = true
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/rild-prop-md1.a
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	rild.c

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libdl \
	liblog \
	libril

LOCAL_STATIC_LIBRARIES := \
	rild-prop-md1

ifeq ($(SIM_COUNT), 2)
    LOCAL_CFLAGS += -DANDROID_MULTI_SIM
    LOCAL_CFLAGS += -DANDROID_SIM_COUNT_2
endif

# temporary hack for broken vendor rils
LOCAL_WHOLE_STATIC_LIBRARIES := \
	librilutils_static

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE:= rild

include $(BUILD_EXECUTABLE)
endif

