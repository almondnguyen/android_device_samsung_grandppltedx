LOCAL_PATH := $(call my-dir)

# ifc*
include $(CLEAR_VARS)
LOCAL_SRC_FILES := ifc/thermal_shim.cpp
LOCAL_MODULE := libshim_thermal
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)

# Parcel
include $(CLEAR_VARS)
LOCAL_SRC_FILES := ril/ril_shim.cpp
LOCAL_MODULE := libshim_ril
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)

# Cam and FILEPATHE
include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := libbinder liblog libgui libui libicuuc libicui18n libmedia
LOCAL_SRC_FILES := camera/camera_shim.c camera/mtk_cam.cpp
LOCAL_MODULE := libshim_camera
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)

# MTK_AGPSD + OPENSSL
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := external/boringssl/include
LOCAL_SHARED_LIBRARIES := liblog

LOCAL_SRC_FILES := agps/mtkagpsd_shim.c
LOCAL_MODULE := libshim_agpsd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := liblog

# XLOG for everything MTK
LOCAL_SRC_FILES := xlog/xlog_shim.c
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := libshim_xlog
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)
