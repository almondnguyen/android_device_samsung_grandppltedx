ifneq ($(filter grandppltedx,$(TARGET_DEVICE)),)
  ifeq ($(TARGET_BUILD_MTK_RIL), true)
    include $(call all-subdir-makefiles)
  endif
endif
