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

void vendor_load_properties() {
	int sim_count = 0;

	/* check if the simslot count file exists */
	if (access(SIMSLOT_FILE, F_OK) == 0) {
		sim_count = read_integer(SIMSLOT_FILE);
	}

	/* check whether device is dual sim */
	if (sim_count == 1) {
		android::init::property_set("ro.multisim.simslotcount", "1");
		android::init::property_set("persist.radio.multisim.config", "none");
	} else {
		android::init::property_set("ro.multisim.simslotcount", "2");
		android::init::property_set("persist.radio.multisim.config", "dsds");
	}
	android::init::property_set("ro.multisim.set_audio_params", "true");
}
