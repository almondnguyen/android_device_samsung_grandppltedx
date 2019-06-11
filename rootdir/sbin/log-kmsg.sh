#!/sbin/busybox sh

BB=/sbin/busybox

cd /

$BB echo "DMESG START" > /system/dmesg.txt

while [ 1 ]; do $BB cat /proc/kmsg >> /system/dmesg.txt; done
