LOCAL_PATH := $(call my-dir)
# MTK Thermal
#- why would thermal need stuff from net utils??
#- this is an old L.OS patch to adopt MTK Hardware. Was abandoned for some reasons~

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	thermal_shim.cpp

LOCAL_MODULE := libshim_thermal
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	ril_shim.cpp

LOCAL_MODULE := libshim_ril
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)
