/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_INCLUDE_HARDWARE_GPS_MTK_H
#define ANDROID_INCLUDE_HARDWARE_GPS_MTK_H

#include <hardware/gps_internal.h>

__BEGIN_DECLS

// MTK extended GpsAidingData values.
#define GPS_DELETE_HOT_STILL 0x2000
#define GPS_DELETE_EPO      0x4000

// ====================vzw debug screen API =================
/**
 * Name for the VZW debug interface.
 */
#define VZW_DEBUG_INTERFACE      "vzw-debug"

#define VZW_DEBUG_STRING_MAXLEN      200

/** Represents data of VzwDebugData. */
typedef struct {
    /** set to sizeof(VzwDebugData) */
    size_t size;

    char  vzw_msg_data[VZW_DEBUG_STRING_MAXLEN];
} VzwDebugData;


typedef void (* vzw_debug_callback)(VzwDebugData* vzw_message);

/** Callback structure for the Vzw debug interface. */
typedef struct {
    vzw_debug_callback vzw_debug_cb;
} VzwDebugCallbacks;


/** Extended interface for VZW DEBUG support. */
typedef struct {
    /** set to sizeof(VzwDebugInterface) */
    size_t          size;

    /** Registers the callbacks for Vzw debug message. */
    int  (*init)( VzwDebugCallbacks* callbacks );

    /** Set Vzw debug screen enable/disable **/
    void (*set_vzw_debug_screen)(bool enabled);
} VzwDebugInterface;


__END_DECLS

#endif /* ANDROID_INCLUDE_HARDWARE_GPS_MTK_H */

