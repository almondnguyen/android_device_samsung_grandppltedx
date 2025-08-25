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
lib/libxml2.so
"
for blob in $BLOBS_LIST
do
    sed -i 's/\([Uu][Cc][Nn][Vv]_[A-Za-z_]*\)_55/\1_56/g' "$BLOB_ROOT/$blob"
done


"$MY_DIR"/setup-makefiles.sh
