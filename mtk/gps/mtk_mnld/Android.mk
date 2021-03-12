# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.

# MediaTek Inc. (C) 2017. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.     
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.


# Copyright 2005 The Android Open Source Project

###############################################################################
# Configuration
###############################################################################
# build start
###############################################################################
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MY_LOCAL_PATH := $(LOCAL_PATH)

MTK_GPS_CHIP = MTK_GPS_MT6735
$(warning feature_option=$(MTK_GPS_CHIP))

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/utility/inc \
  $(LOCAL_PATH)/mnl_agps_interface/inc \
  $(LOCAL_PATH)/mnl_at_cmd_interface/inc \
  $(LOCAL_PATH)/mnl_flp_interface/inc \
  $(LOCAL_PATH)/mnl_nlp_interface/inc \
  $(LOCAL_PATH)/mnld_entity/inc \
  $(LOCAL_PATH)/mnl/inc \
  $(LOCAL_PATH)/curl/inc \
  $(LOCAL_PATH)/libnvram \
  external/libxml2/include \

LOCAL_SRC_FILES := \
	mnld_entity/src/mnl2hal_interface.c \
	utility/src/data_coder.c \
	utility/src/mtk_lbs_utility.c \
	mnl_agps_interface/src/mnl_agps_interface.c \
	mnl_agps_interface/src/mnl2agps_interface.c \
	mnl_agps_interface/src/agps2mnl_interface.c \
	mnl_flp_interface/src/mnl_flp_interface.c \
	mnl_flp_interface/src/mnl_flp_test_interface.c \
	mnl_nlp_interface/src/mnl2nlps.c \
	mnl_at_cmd_interface/src/mnl_at_interface.c \
	mnld_entity/src/mnld.c \
	mnld_entity/src/mnld_uti.c \
	mnld_entity/src/gps_controller.c \
	mnld_entity/src/nmea_parser.c \
	mnld_entity/src/epo.c \
	mnld_entity/src/qepo.c \
	mnld_entity/src/mnl_common.c \
	mnld_entity/src/op01_log.c \
	mnld_entity/src/gps_dbg_log.c \
	mnld_entity/src/mpe.c \
	mnl/src/pseudo_mnl.c \

LOCAL_MODULE := mnld

LOCAL_MULTILIB := 32
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_EXECUTABLES)

LOCAL_CFLAGS += -DMTK_GPS_CO_CLOCK_DATA_IN_MD
LOCAL_CFLAGS += -DCONFIG_GPS_USER_LOAD

LOCAL_STATIC_LIBRARIES +=  libsupl libepos
LOCAL_SHARED_LIBRARIES +=  libmnl libcurl libcutils libc libm libnvram libcrypto libssl libz

LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := libmnl.so

include $(BUILD_EXECUTABLE)

include $(MY_LOCAL_PATH)/mnl/libs/Android.mk
include $(MY_LOCAL_PATH)/curl/libs/Android.mk
###############################################################################
