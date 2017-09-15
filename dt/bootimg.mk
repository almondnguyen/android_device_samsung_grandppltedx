LOCAL_PATH := $(call my-dir)

#FLASH_IMAGE_TARGET ?= $(PRODUCT_OUT)/recovery.tar

ifdef TARGET_PREBUILT_DTB
	BOARD_MKBOOTIMG_ARGS += --dt $(TARGET_PREBUILT_DTB)
endif

INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img

$(INSTALLED_BOOTIMAGE_TARGET): $(MKBOOTIMG) $(INSTALLED_DTIMAGE_TARGET) $(boot_kernel) $(boot_ramdisk)
	@echo -e ${CL_GRN}"----- Making boot image ------"${CL_RST} #
	$(hide) $(MKBOOTIMG) $(INTERNAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --output $@ --ramdisk $(boot_ramdisk)
	@echo -e ${CL_CYN}"Made boot image: $@"${CL_RST}
	@echo -e ${CL_GRN}"----- Lying about SEAndroid state to Samsung bootloader ------"${CL_RST}
	$(hide) echo -n "SEANDROIDENFORCE" >> $(INSTALLED_BOOTIMAGE_TARGET)
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
	#$(hide) tar -C $(PRODUCT_OUT) -H ustar -c boot.img > $(FLASH_IMAGE_TARGET)
	#@echo -e ${CL_CYN}"Made Odin flashable boot tar: ${FLASH_IMAGE_TARGET}"${CL_RST}