
TARGET_CAMERA_OPEN_SOURCE := true

TARGET_BOARD_BACK_CAMERA_ROTATION := false
TARGET_BOARD_FRONT_CAMERA_ROTATION := false
TARGET_BOARD_CAMERA_ROTATION_CAPTURE := false

#support hal1.0
TARGET_BOARD_CAMERA_HAL_VERSION := 1.0

# camera sensor type
CAMERA_SENSOR_TYPE_BACK := "imx219_mipi_raw"
CAMERA_SENSOR_TYPE_FRONT := "s5k5e3yx_mipi_raw"

# select camera 2M,3M,5M,8M
CAMERA_SUPPORT_SIZE := 8M
FRONT_CAMERA_SUPPORT_SIZE := 5M
TARGET_BOARD_NO_FRONT_SENSOR := false
TARGET_BOARD_CAMERA_FLASH_CTRL := false

#read sensor otp to isp
TARGET_BOARD_CAMERA_READOTP_TO_ISP := true

#otp version, v0(OTP on Grandprime, Z3) v1(OTP on J1MINI) v2(Without OTP on TabG)
TARGET_BOARD_CAMERA_OTP_VERSION := 0

#read otp method 1:from kernel 0:from user
TARGET_BOARD_CAMERA_READOTP_METHOD := 1

#face detect
TARGET_BOARD_CAMERA_FACE_DETECT := true
TARGET_BOARD_CAMERA_FD_LIB := omron

#sensor interface
TARGET_BOARD_BACK_CAMERA_INTERFACE := mipi
TARGET_BOARD_FRONT_CAMERA_INTERFACE := mipi

#select camera zsl cap mode
TARGET_BOARD_CAMERA_CAPTURE_MODE := true

#select camera not support autofocus
TARGET_BOARD_CAMERA_NO_AUTOFOCUS_DEV := false

#uv denoise enable
TARGET_BOARD_CAMERA_CAPTURE_DENOISE := false

#y denoise enable
TARGET_BOARD_CAMERA_Y_DENOISE := true

#select continuous auto focus
TARGET_BOARD_CAMERA_CAF := true

#select ACuteLogic awb algorithm
TARGET_BOARD_USE_ALC_AWB := true

#pre_allocate capture memory
TARGET_BOARD_CAMERA_PRE_ALLOC_CAPTURE_MEM := true

#sc8830g isp ver 0;sc9630 isp ver 1;tshark2 isp version 2
TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION := 2

#support auto anti-flicker
TARGET_BOARD_CAMERA_ANTI_FLICKER := true

#multi cap memory mode
TARGET_BOARD_MULTI_CAP_MEM := true

#select mipi d-phy mode(none, phya, phyb, phyab)
TARGET_BOARD_FRONT_CAMERA_MIPI := phyc
TARGET_BOARD_BACK_CAMERA_MIPI := phyab

#select ccir pclk src(source0, source1)
TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK := source0
TARGET_BOARD_BACK_CAMERA_CCIR_PCLK := source0

#third lib
TARGET_BOARD_USE_THIRD_LIB := true
TARGET_BOARD_USE_THIRD_AWB_LIB_A := true
TARGET_BOARD_USE_ALC_AE_AWB := false
TARGET_BOARD_USE_THIRD_AF_LIB_A := true

#face beauty
TARGET_BOARD_CAMERA_FACE_BEAUTY := true

#hdr effect enable
TARGET_BOARD_CAMERA_HDR_CAPTURE := true

#use media extensions
TARGET_USES_MEDIA_EXTENSIONS := true
################################################################################ 

# compilation error.
BOARD_USE_SAMSUNG_CAMERAFORMAT_YUV420SP := true
TARGET_HAS_LEGACY_CAMERA_HAL1 := true
USE_DEVICE_SPECIFIC_CAMERA := true
TARGET_CAMERA_APP := Camera2

