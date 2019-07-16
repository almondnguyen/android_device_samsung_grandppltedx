MTK_LIB_PATH := device/samsung/grandppltedx/mtk

ifneq ($(filter grandppltedx,$(TARGET_DEVICE)),)

# nuclear
#include $(call all-makefiles-under,$(MTK_LIB_PATH))

# individual toggles
#include $(MTK_LIB_PATH)/ccci_lib/Android.mk
#include $(call all-makefiles-under,$(MTK_LIB_PATH)/display)
include $(MTK_LIB_PATH)/gps/Android.mk
#include $(MTK_LIB_PATH)/libbt-vendor-mtk/Android.mk
#include $(MTK_LIB_PATH)/libhwm/Android.mk
include $(MTK_LIB_PATH)/liblog_mtk/Android.mk
include $(MTK_LIB_PATH)/mtk_symbols/Android.mk
include $(MTK_LIB_PATH)/memtrack/Android.mk
include $(MTK_LIB_PATH)/mrdump/Android.mk
include $(MTK_LIB_PATH)/fmradio/Android.mk

endif
