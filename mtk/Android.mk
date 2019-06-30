MTK_LIB_PATH := device/samsung/grandppltedx/mtk

ifneq ($(filter grandppltedx,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(MTK_LIB_PATH))

endif
