# Copyright 2017 The Android Open Source Project
#
# Oreo RIL library for MTK devices - Version (0.9).
# by: daniel_hk(https://github.com/danielhk)
#
# start with the AOSP libril
#
# 2017/7/29: Initial Oero port for MT6752	- by: daniel_hk
# 2017/8/19: add RIL_CHANNEL_QUEUING flag	- by: daniel_hk
# 2017/8/27: add RIL_UNSOL_PENDING		- by: daniel_hk
#

ifeq ($(BOARD_PROVIDES_LIBRIL),true)
LOCAL_PATH:= $(call my-dir)

ril_src_files := \
    ril.cpp \
    ril_event.cpp\
    RilSapSocket.cpp \
    ril_service.cpp \
    sap_service.cpp

ril_shared_libs := \
    liblog \
    libutils \
    libcutils \
    libhardware_legacy \
    libbinder \
    librilmtk \
    librilutils \
    mtk-ril \
    libhidlbase  \
    libhidltransport \
    libhwbinder \
    android.hardware.radio@1.0 \
    android.hardware.radio.deprecated@1.0

#ifeq ($(BOARD_USES_MTK_HARDWARE), true)
#ril_shared_libs += \
#    vendor.mediatek.hardware.radio@1.1 \
#    vendor.mediatek.hardware.radio@2.0 \
#    vendor.mediatek.hardware.radio.deprecated@1.1
#endif

ril_inc := external/nanopb-c \
    $(LOCAL_PATH)/../include

ril_cflags := -Wno-unused-parameter -DANDROID_SIM_COUNT_2 -DANDROID_MULTI_SIM

ifeq ($(BOARD_USES_MTK_HARDWARE),true)
  ril_cflags += -DMTK_HARDWARE
endif

# something wrong, cause channel occupied after oNewCommandConnect
# workaround but not complete yet
ifeq ($(BOARD_USES_RIL_UNSOL_PENDING),true)
  ril_cflags += -DRIL_UNSOL_PENDING
endif

# queuing request when channel busy
# emulate the enqueue action but not fully working yet
ifeq ($(BOARD_USES_RIL_CHANNEL_QUEUING),true)
  ril_cflags += -DRIL_CHANNEL_QUEUING
endif

include $(CLEAR_VARS)
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := $(ril_src_files)
LOCAL_SHARED_LIBRARIES := $(ril_shared_libs)
LOCAL_STATIC_LIBRARIES := \
    libprotobuf-c-nano-enable_malloc \

LOCAL_CFLAGS := $(ril_cflags)
LOCAL_C_INCLUDES += $(ril_inc)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_MODULE:= libril
LOCAL_CLANG := true
LOCAL_SANITIZE := integer
include $(BUILD_SHARED_LIBRARY)

endif
