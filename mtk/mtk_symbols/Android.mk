LOCAL_PATH := device/samsung/grandppltedx/mtk/mtk_symbols

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    icu55.c \
    mtk_asc.cpp \
    mtk_audio.cpp \
    mtk_fence.cpp \
    mtk_omx.cpp \
    mtk_ui.cpp \
    mtk_audioCompat.c \
    mtk_gps.cpp


#    mtk_xlog.cpp \

# only for 64bit libraries
LOCAL_SRC_FILES_64 := mtk_parcel.cpp

LOCAL_SHARED_LIBRARIES := libbinder liblog libgui libui libicuuc libicui18n libmedia
LOCAL_MODULE := mtk_symbols
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)


# MTK cam
include $(CLEAR_VARS)
LOCAL_SRC_FILES := mtk_cam.cpp
LOCAL_MODULE := libshim_general
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)

