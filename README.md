# Device Tree for Samsung Galaxy J2 Prime (SM-G532x)

TESTING:
* Doze/Battery Life

WORKING ON:
* Headphone:
```
E/APM::AudioPolicyEngine(  319): getDeviceForInputSource() no default device defined
W/APM_AudioPolicyManager(  319): getInputForAttr() could not find device for source 1998
```
* RadioFM (probably due to headphone not working; it scans then crashes)
* RIL (bad/unstable signal strength on LTE; manual configure for dual-SIM; log spam)

CURRENTLY NOT WORKING/BUGGY:
* Headphone
* GPS (bin runs but doesn't work)
* Camera: Video Recording
* RadioFM
* Wi-Fi Hotspot
* No Icon/Percentage while offline charging. (it still charges.)
* Manually configure for 2 active SIMs.

WON'T FIX:
* VoLTE. No Samsung dev has managed to port SRIL IMS to AOSP, so I'd say 'impossible to fix'.


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



