#!/system/bin/sh

_date=`date +%F_%H-%M-%S`
logcat -b all -d -f /cache/logcat_${_date}.txt &
dmesg > /cache/kmsg_${_date}.txt
