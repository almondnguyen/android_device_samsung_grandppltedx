#
# vendor prop for grandpplte
#

# Misc
PRODUCT_PROPERTY_OVERRIDES += \
    config.disable_consumerir=true \
    media.sf.omx-plugin=libffmpeg_omx.so \
    ro.sf.lcd_density=240 \
    persist.sys.lcd_density=240 \
    ro.zygote.preload.enable=0 \
    ro.zygote=zygote32 \
    ro.mount.fs=EXT4 \
    ro.adb.secure=0 \
    ro.secure=0 \
    ro.allow.mock.location=0 \
    ro.debuggable=1 \
    persist.sys.dun.override=0 \
    persist.service.acm.enable=0 \
    persist.sys.usb.config=mtp,adb \
    persist.sys.display.clearMotion=0
    ro.kernel.android.checkjni=0 \

# Dalvik / hwui
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hwui.texture_cache_size=24 \
    ro.hwui.layer_cache_size=16 \
    ro.hwui.path_cache_size=4 \
    ro.hwui.texture_cache_flushrate=0.4 \
    ro.hwui.shape_cache_size=1 \
    ro.hwui.gradient_cache_size=0.5 \
    ro.hwui.drop_shadow_cache_size=2 \
    ro.hwui.r_buffer_cache_size=2 \
    ro.hwui.text_small_cache_width=512 \
    ro.hwui.text_small_cache_height=512 \
    ro.hwui.text_large_cache_width=1024 \
    ro.hwui.text_large_cache_height=1024 \
    dalvik.vm.mtk-stack-trace-file=/data/anr/mtk_traces.txt \
    dalvik.vm.heapstartsize=8m \
    dalvik.vm.heapgrowthlimit=128m \
    dalvik.vm.heapsize=512m \
    dalvik.vm.heaptargetutilization=0.75 \
    dalvik.vm.heapminfree=512k \
    dalvik.vm.heapmaxfree=8m

# Display
PRODUCT_PROPERTY_OVERRIDES += \
    debug.hwui.render_dirty_regions=false \
    debug.hwui.use_buffer_age=false \
    ro.opengles.version=196609 \
    ro.surface_flinger.max_frame_buffer_acquired_buffers=3

# Connectivity
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0 \
    ro.mediatek.wlan.wsc=1 \
    ro.mediatek.wlan.p2p=1 \
    mediatek.wlan.ctia=0 \
    wifi.tethering.interface=ap0 \
    wifi.direct.interface=p2p0 \
    mediatek.wlan.chip=CONSYS_MT6735
    mediatek.wlan.module.postfix=_consys_mt6735 \
    persist.mtk.wcn.combo.chipid=-1 \
    service.wcn.driver.ready=no \
    service.wcn.coredump.mode=0 \

# FRP
PRODUCT_PROPERTY_OVERRIDES += \
    ro.frp.pst=/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/PERSISTENT

# RIL
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.rild.libpath=/vendor/lib/libsec-ril.so \
    vendor.rild.libpath2=/vendor/lib/libsec-ril-dsds.so \
    rild.libargs=-d /dev/ttyC0 \
    ro.ril.telephony.mqanelements=6 \
    ro.telephony.default_network=9
    ro.boot.opt_lte_support=1 \
    telephony.lteOnGsmDevice=1 \
    telephony.lteOnCdmaDevice=0
