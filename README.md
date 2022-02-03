# Device Tree for Samsung Galaxy J2 Prime (SM-G532x)

WORKING ON:
* RadioFM (a port ROM has RFM confirmed working)
* LiveDisplay (same as above ^)
* Doze (Battery is important for a phone. Kek?)
* Camera (log spamming wildly when opening camera; currently finding a way to read them)

CURRENTLY NOT WORKING/BUGGY:
* GPS (bin runs but doesn't work)
* Camera: Video Recording
* Camera: Stock Camera app (Snap) crashes after taking 1 pic.
* RadioFM
* LiveDisplay
* Wi-Fi Hotspot
* Dozing (aka. kinda bad battery). There's still log spamming.
* No Icon/Percentage while offline charging. (it still charges.)
* Manually configure for 2 active SIMs.

WON'T FIX:
* VoLTE. No Samsung device has managed to port IMS to AOSP, so I'd say 'impossible to fix'.


DEVELOPMENT IN PROGRESS, MAY EAT YOUR CAT.


SPECSHEET

Component | Details
---------:|:-------------------------
CPU       | MediaTek MT6737T
GPU       | Mali-T720MP2
RAM       | 1.5 GB
Storage   | 8 GB, 16 GB
Battery   | 2600 mAh
Display   | 540 x 960, 16:9 ratio
Rear Cam  | 13MP, Sony imx218
Front Cam | 8MP, Samsung s5k5e3yx



