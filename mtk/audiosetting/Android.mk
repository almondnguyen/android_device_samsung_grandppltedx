LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES += audiosetting.cpp
LOCAL_MODULE := libaudiosetting
LOCAL_SHARED_LIBRARIES := libnativehelper libcutils libutils 
LOCAL_C_INCLUDES := frameworks/base/include/media

include $(BUILD_SHARED_LIBRARY)
