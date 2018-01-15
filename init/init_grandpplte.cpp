/*
   Copyright (c) 2018, The Lineage Project. All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/properties.h>

#include "property_service.h"
#include "vendor_init.h"
#include "log.h"
#include "util.h"

#define SERIAL_NUMBER_FILE "/efs/FactoryApp/serial_no"
#define SIMSLOT_FILE "/proc/simslot_count"

using android::base::GetProperty;
using android::base::ReadFileToString;
using android::base::Trim;
using android::init::property_set;

int read_integer(const char* filename) {
	int retval;
	FILE * file;

	/* open the file */
	if (!(file = fopen(filename, "r"))) {
		return -1;
	}
	/* read the value from the file */
	fscanf(file, "%d", &retval);
	fclose(file);

	return retval;
}

void property_override(char const prop[], char const value[]) {	
	prop_info *pi;

	pi = (prop_info*) __system_property_find(prop);
	if (pi)
		__system_property_update(pi, value, strlen(value));
	else
		__system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_override_dual(char const system_prop[],
		char const vendor_prop[], char const value[])
{
	property_override(system_prop, value);
	property_override(vendor_prop, value);
}

void init_dual() {
    android::init::property_set("ro.multisim.set_audio_params", "true");
    android::init::property_set("ro.multisim.simslotcount", "2");
    android::init::property_set("persist.radio.multisim.config", "dsds");
}

void init_single() {
    android::init::property_set("ro.multisim.set_audio_params", "true");
    android::init::property_set("ro.multisim.simslotcount", "1");
    android::init::property_set("persist.radio.multisim.config", "none");
}

void vendor_load_properties() {
	std::string bootloader = GetProperty("ro.bootloader", "");
	std::string platform = GetProperty("ro.board.platform", "");
	std::string device;

	if (platform != ANDROID_TARGET) return;
	int sim_count;

	/* set basic device name */
	property_override_dual("ro.product.device", "ro.vendor.product.device", "grandpplte");

	/* check if the simslot count file exists */
	if (access(SIMSLOT_FILE, F_OK) == 0) {
		sim_count = read_integer(SIMSLOT_FILE);
	}
	
	/* set model + dual sim props */
	if (bootloader.find("G532F") != std::string::npos) {
		/* G532F */
	        property_override_dual("ro.product.name", "ro.vendor.product.name", "grandpplteser");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532F");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532F/DS");
			init_dual();
		}
	}

	if (bootloader.find("G532G") != std::string::npos) {
		/* G532G */
	        property_override_dual("ro.product.name", "ro.vendor.product.name", "grandppltedx");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532G");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532G/DS");
			init_dual();
		}
	}

	if (bootloader.find("G532M") != std::string::npos) {
		/* G532M */
	        property_override_dual("ro.product.name", "ro.vendor.product.name", "grandpplteub");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532M");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532M/DS");
			init_dual();
		}
	}

	if (bootloader.find("G532MT") != std::string::npos) {
		/* G532F */
	        property_override_dual("ro.product.name", "ro.vendor.product.name", "grandppltedtvvj");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532MT");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532MT/DS");
			init_dual();
		}
	}
}
