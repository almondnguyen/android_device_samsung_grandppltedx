LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE           := fstab.mt6735
#LOCAL_MODULE_TAGS      := optional eng
#LOCAL_MODULE_CLASS     := ETC
#LOCAL_SRC_FILES        := fstab.mt6735
#LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
#include $(BUILD_PREBUILT)


# Init scripts

#include $(CLEAR_VARS)
#LOCAL_MODULE           := init.modem.rc
#LOCAL_MODULE_TAGS      := optional eng
#LOCAL_MODULE_CLASS     := ETC
#LOCAL_SRC_FILES        := init.modem.rc
#LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE           := init.rc
#LOCAL_MODULE_TAGS      := optional eng
#LOCAL_MODULE_CLASS     := ETC
#LOCAL_SRC_FILES        := init.rc
#LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
#include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE           := init.wifi.rc
LOCAL_MODULE_TAGS      := optional eng
LOCAL_MODULE_CLASS     := ETC
LOCAL_SRC_FILES        := init.wifi.rc
LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE           := init.mt6735.usb.rc
#LOCAL_MODULE_TAGS      := optional eng
#LOCAL_MODULE_CLASS     := ETC
#LOCAL_SRC_FILES        := init.mt6735.usb.rc
#LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE           := init.mt6735.rc
#LOCAL_MODULE_TAGS      := optional eng
#LOCAL_MODULE_CLASS     := ETC
#LOCAL_SRC_FILES        := init.mt6735.rc
#LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE           := ueventd.mt6735.rc
#LOCAL_MODULE_TAGS      := optional eng
#LOCAL_MODULE_CLASS     := ETC
#LOCAL_SRC_FILES        := ueventd.mt6735.rc
#LOCAL_MODULE_PATH      := $(TARGET_ROOT_OUT)
#include $(BUILD_PREBUILT)
