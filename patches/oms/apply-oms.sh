#!/bin/bash

# CONFIRMATION
echo -e "\n These patches are for Pure LineageOS"
echo -e "\n It is also outdated (150 commits behind upstream)"
echo -e "\n Are you sure you want to apply? y/n"
read answr
[ "$answr" != "y" ] && exit 1

# INPUT LOS DIR
echo -e "\n Input your LOS top directory"
echo -e "\n for example: ~/android/n/lineage"
read LOS_REPO

# PATCH START
echo -e "\n LOS Directory: $LOS_REPO"
echo -e "\n Start Patching.."

bash ${LOS_REPO}/device/samsung/grandppltedx/patches/oms/oms_patches.sh ${LOS_REPO}
exit 0
