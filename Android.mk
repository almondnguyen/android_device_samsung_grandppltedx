LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),grandpplte)
include $(call all-subdir-makefiles,$(LOCAL_PATH))
endif
