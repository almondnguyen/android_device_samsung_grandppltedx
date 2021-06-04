#!/bin/bash
CURRENT_DIR=${PWD}
LOS_REPO=
function warn() {
echo -e "\n Those Patches Are Only For Pure LineageOS"
echo -e "\n Are You Sure Thats You Want To Apply? y/n"
read answr
[ "$answr" != "y" ] && exit 1
}
function find() {
echo -e "\n Current Directory: $CURRENT_DIR"
echo -e "\n Finding LineageOS Repo Root Dir.."
sleep 2
cd ../../../../..
LOS_REPO=${PWD}
}
warn

find

sleep 1

echo -e "\n Finded LOS Directoty: $LOS_REPO"
sleep 0.5
echo -e "\n Start Patching.."
sleep 1.5
cd $CURRENT_DIR
bash oms_patches.sh ~/los
exit 0
