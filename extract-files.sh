#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2025 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

DEVICE=grandpplte
VENDOR=samsung

PATCHELF=patchelf/patchelf-0_9
PATCHELF_0_18=patchelf/patchelf-0_18

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$MY_DIR" ]]; then MY_DIR="$PWD"; fi

CM_ROOT="$MY_DIR"/../../..

HELPER="$CM_ROOT"/vendor/cm/build/tools/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
. "$HELPER"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

while [ "$1" != "" ]; do
    case $1 in
        -n | --no-cleanup )     CLEAN_VENDOR=false
                                ;;
        -s | --section )        shift
                                SECTION=$1
                                CLEAN_VENDOR=false
                                ;;
        * )                     SRC=$1
                                ;;
    esac
    shift
done

if [ -z "$SRC" ]; then
    SRC=adb
fi

# Initialize the helper
setup_vendor "$DEVICE" "$VENDOR" "$CM_ROOT" false "$CLEAN_VENDOR"

extract "$MY_DIR"/proprietary-files.txt "$SRC" "$SECTION"

# Fix proprietary blobs
BLOB_ROOT="$CM_ROOT"/vendor/"$VENDOR"/"$DEVICE"/proprietary

echo "Fixing proprietary blobs"

# ICU 55 > ICU 56
BLOBS_LIST="
lib/libaudio_param_parser.so
"
for blob in $BLOBS_LIST
do
    sed -i 's/\([Uu][Cc][Nn][Vv]_[A-Za-z_]*\)_55/\1_56/g' "$BLOB_ROOT/$blob"
done

# __pthread_gettid
BLOBS_LIST="
lib/libmtkjpeg.so
lib/libvcodecdrv.so
"
for blob in $BLOBS_LIST
do
    "$PATCHELF_0_18" --add-needed "libc_shim.so" "$BLOB_ROOT/$blob"
done

# CameraParameters
BLOBS_LIST="
lib/libcam_utils.so
"
for blob in $BLOBS_LIST
do
    "$PATCHELF_0_18" --add-needed "libcamera_client_shim.so" "$BLOB_ROOT/$blob"
done

# __xlog_buf_printf
BLOBS_LIST="
lib/hw/hwcomposer.mt6737t.so
lib/lib3a.so
lib/lib3a_sample.so
lib/libJpgDecPipe.so
lib/libMtkOmxAdpcmDec.so
lib/libMtkOmxAdpcmEnc.so
lib/libMtkOmxAlacDec.so
lib/libMtkOmxFlacDec.so
lib/libMtkOmxG711Dec.so
lib/libMtkOmxGsmDec.so
lib/libMtkOmxMp3Dec.so
lib/libMtkOmxRawDec.so
lib/libMtkOmxVdecEx.so
lib/libMtkOmxVenc.so
lib/libMtkOmxVorbisEnc.so
lib/libSwJpgCodec.so
lib/libcamalgo.so
lib/libdngop.so
lib/libdpframework.so
lib/libfeatureio.so
lib/libh264enc_sb.ca7.so
lib/libpq_prot.so
xbin/mnld
"
for blob in $BLOBS_LIST
do
    "$PATCHELF_0_18" --add-needed "liblog_shim.so" "$BLOB_ROOT/$blob"
done

# SSL_*
BLOBS_LIST="
bin/mtk_agpsd
"
for blob in $BLOBS_LIST
do
    "$PATCHELF_0_18" --add-needed "libssl_shim.so" "$BLOB_ROOT/$blob"
done

# GraphicBuffer
BLOBS_LIST="
lib/libMtkOmxVenc.so
lib/libcam.camnode.so
lib/libcam.client.so
lib/libcam_utils.so
lib/libmtk_mmutils.so
lib/libsecimaging.so
"
for blob in $BLOBS_LIST
do
    "$PATCHELF_0_18" --add-needed "libui_shim.so" "$BLOB_ROOT/$blob"
done

"$MY_DIR"/setup-makefiles.sh
