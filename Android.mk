LOCAL_PATH := device/samsung/grandppltedx

ifneq ($(filter grandppltedx,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

endif
