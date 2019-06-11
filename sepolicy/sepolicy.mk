#   samsung: use ks01lte sepolicy | sec_common
#   MTK: try mt6735/7-common (cm-14.1) implementation
#
#- credits to
# ks01ltexx devs for Samsung sepolicy
# woods/nautilus devs for MTK (mt6737) policy

BOARD_SEPOLICY_DIRS := \
	device/samsung/grandppltedx/sepolicy/samsung \
	device/samsung/grandppltedx/sepolicy/mt6737m
