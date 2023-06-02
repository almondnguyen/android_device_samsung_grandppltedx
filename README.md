# Device Tree for Samsung Galaxy J2 Prime (SM-G532x)

WORKING ON:
* SELinux
* Battery

CURRENTLY NOT WORKING/BUGGY:
* 3.5" Headphone not working (change of headphone definition that old audio.primary.mt6735 don't have)
* Camera: Video Recording not working
* Manually configure for 2 active SIMs.

WON'T FIX:
* VoLTE. No Samsung dev has managed to port SRIL IMS to AOSP, so I'd say 'impossible to fix'.
* Dual-SIM manual configuration (LTE+2G). AOSP N does not support that?


DEVELOPMENT IN PROGRESS, MAY EAT YOUR CAT.
----------

SPECSHEET

Component | Details
---------:|:-------------------------
CPU       | MediaTek MT6737T
GPU       | Mali-T720MP2
RAM       | 1.5 GB
Storage   | 8 GB, 16 GB
Battery   | 2600 mAh
Display   | 540 x 960, 16:9 ratio
Rear Cam  | 8MP, Sony imx219
Front Cam | 5MP, Samsung s5k5e3yx
