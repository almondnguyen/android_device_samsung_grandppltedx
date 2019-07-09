LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mrdump_status.c

LOCAL_MODULE := libmrdump
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES = liblog

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mrdump_tool.c

LOCAL_MODULE := mrdump_tool
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES = libmrdump liblog

include $(BUILD_EXECUTABLE)

