#!/system/bin/sh

_date=`date +%F_%H-%M-%S`
logcat -d -b all -f  /cache/logcat_${_date}.txt &
dmesg > /cache/kmsg_${_date}.txt
