LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := mtk_xlog.c

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := liblog_mtk
include $(BUILD_SHARED_LIBRARY)
