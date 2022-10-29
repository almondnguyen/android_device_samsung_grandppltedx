#!/bin/sh

echo $1
rootdirectory="$PWD"
devicetree="device/samsung/grandppltedx"
# ---------------------------------

dirs="\
 bionic \
 build/make/core build/soong \
 frameworks/av frameworks/base frameworks/native
 hardware/interfaces hardware/ril \
 system/core system/sepolicy system/bt \
 external/wpa_supplicant_8 \
"

# red + nocolor
RED='\033[0;31m'
NC='\033[0m'

for dir in $dirs ; do
	cd $rootdirectory
	cd $dir
    echo -e "\n${RED}Applying ${NC}$dir ${RED}patches...${NC}\n"
	git apply -v $rootdirectory/$devicetree/patches/$dir/*.patch
done

# -----------------------------------
echo -e "Done !\n"
cd $rootdirectory
