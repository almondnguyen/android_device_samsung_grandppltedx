LOCAL_PATH := $(call my-dir)
###############################################################################
# Define MTK FM Radio Chip solution
###############################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	custom.cpp
	
LOCAL_C_INCLUDES := $(JNI_H_INCLUDE)

LOCAL_CFLAGS+= \
    -DMT6627_FM

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := libfmcust
LOCAL_PROPRIETARY_MODULE := false
LOCAL_MODULE_OWNER := mtk

include $(BUILD_SHARED_LIBRARY)

