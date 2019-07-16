LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS+= \
    -DMT6627_FM

LOCAL_MODULE := radio.fm.$(TARGET_BOARD_PLATFORM)
LOCAL_PROPRIETARY_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := fmr_core.c \
	fmr_err.c \
	common.c \
	radio_hw_hal.c

LOCAL_SHARED_LIBRARIES := liblog libdl libmedia libcutils libradio_metadata
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
