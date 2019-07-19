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

/* credit: a3y17lte devs (A3 2017) */
/* currently not support G532MT | MT/DS because not know codename */

#include <stdlib.h>
#include <string.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/properties.h>

#include "property_service.h"
#include "vendor_init.h"

using android::base::GetProperty;
using android::base::ReadFileToString;
using android::base::Trim;

#define SERIAL_NUMBER_FILE "/efs/FactoryApp/serial_no"

int read_integer(const char* filename)
{
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

void property_override(char const prop[], char const value[])
{	
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
    property_set("ro.multisim.set_audio_params", "true");
    property_set("ro.multisim.simslotcount", "2");
    property_set("persist.radio.multisim.config", "dsds");
}

void init_single() {
    property_set("ro.multisim.set_audio_params", "true");
    property_set("ro.multisim.simslotcount", "1");
    property_set("persist.radio.multisim.config", "none");
}

void vendor_load_properties()
{
	std::string platform;
	std::string bootloader = GetProperty("ro.bootloader", "");
	std::string device;

	platform = GetProperty("ro.board.platform", "");
	if (platform != ANDROID_TARGET)
		return;

	/* check if the simslot count file exists */
	if (access(SIMSLOT_FILE, F_OK) == 0) {
		int sim_count= read_integer(SIMSLOT_FILE);

		/* set the dual sim props */
	
	if (bootloader.find("G532F") == 0) {
		/* G532F */
	        property_override_dual("ro.product.device", "ro.vendor.product.device", "grandpplteser");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532F");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532F/DS");
			init_dual();
	}

	if (bootloader.find("G532G") == 0) {
		/* G532G */
	        property_override_dual("ro.product.device", "ro.vendor.product.device", "grandppltedx");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532G");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532G/DS");
			init_dual();
	}

	if (bootloader.find("G532M") == 0) {
		/* G532M */
	        property_override_dual("ro.product.device", "ro.vendor.product.device", "grandpplteub");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532M");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532M/DS");
			init_dual();
	}

	if (bootloader.find("G532MT") == 0) {
		/* G532MT */
	        property_override_dual("ro.product.device", "ro.vendor.product.device", "grandppltedtvvj");
		if (sim_count == 1) {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532MT");
			init_single();
		} else {
			property_override_dual("ro.product.model", "ro.vendor.product.model", "SM-G532MT/DS");
			init_dual();
	}

		/* set serial number */
	char const *serial_number_file = SERIAL_NUMBER_FILE;
	std::string serial_number;

	if (ReadFileToString(serial_number_file, &serial_number)) {
        	serial_number = Trim(serial_number);
        	property_override("ro.serialno", serial_number.c_str());
	}
}
