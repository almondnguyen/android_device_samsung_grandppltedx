#-- audio policy service is loaded before manager AND NEED IT
# make fails
# TODO: Better solution?

$(shell (mkdir -p out/target/product/grandppltedx/obj/SHARED_LIBRARIES/libaudiopolicymanager_intermediates))
$(shell (mkdir -p out/target/product/grandppltedx/obj/lib))
$(shell (cp device/samsung/grandppltedx/dummy out/target/product/grandppltedx/obj/SHARED_LIBRARIES/libaudiopolicymanager_intermediates/export_includes))
$(shell (cp vendor/samsung/grandppltedx/proprietary/system/lib/libaudiopolicymanager.so out/target/product/grandppltedx/obj/lib/libaudiopolicymanager.so))

#-- cm-14.1 stuff
$(shell (cp device/samsung/grandppltedx/dummy out/target/product/grandppltedx/obj/lib/libaudiopolicymanager.so.toc))

#-- install-recovery.sh is not copied correctly
$(shell (mkdir -p out/target/product/grandppltedx/ota_temp/SYSTEM/bin))
$(shell (cp vendor/samsung/grandppltedx/proprietary/system/bin/install-recovery.sh out/target/product/grandppltedx/ota_temp/SYSTEM/bin/install-recovery.sh))

#-- Hostapd wants its libdriver_cmd
