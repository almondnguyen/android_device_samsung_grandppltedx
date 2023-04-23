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

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "property_service.h"
#include "vendor_init.h"
#include "log.h"
#include "util.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#define SERIAL_NUMBER_FILE "/efs/FactoryApp/serial_no"
#define SIMSLOT_FILE "/proc/simslot_count"

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

void init_dual() {
    property_set("ro.multisim.set_audio_params", "true");
    property_set("ro.multisim.simslotcount", "2");
    property_set("persist.radio.multisim.config", "dsds");
}

void init_single() {
    property_set("ro.multisim.set_audio_params", "true");
    property_set("ro.multisim.simslotcount", "1");
    property_set("persist.radio.multisim.config", "ss");
}

void vendor_load_properties() {
    std::string bootloader = property_get("ro.bootloader");
    std::string platform;
    int sim_count;

    /* set basic device name */
    property_override("ro.product.device","grandpplte");

    /* check if the simslot count file exists */
    if (access(SIMSLOT_FILE, F_OK) == 0) {
        sim_count = read_integer(SIMSLOT_FILE);
    }
    
    /* set model + dual sim props */
    if (bootloader.find("G532F") != std::string::npos) {
        /* G532F */
        property_override("ro.build.fingerprint", "samsung/grandpplteser/grandpplte:6.0.1/MMB29T/G532FXWU1ASB1:user/release-keys");
        property_override("ro.build.description", "grandpplteser-user 6.0.1 MMB29T G532FXWU1ASB1 release-keys");
        property_override("ro.product.name", "grandpplteser");
        property_override("ro.product.model", "SM-G532F");
    }

    if (bootloader.find("G532G") != std::string::npos) {
        /* G532G */
        /* SEA is grandppltedx; SWA is grandpplteins*/
        /* no major differences actually, so just name it -dx*/
        property_override("ro.build.fingerprint", "samsung/grandppltedx/grandpplte:6.0.1/MMB29T/G532DXU1ASA5:user/release-keys");
        property_override("ro.build.description", "grandppltedx-user 6.0.1 MMB29T G532GDXU1ASA5 release-keys");
        property_override("ro.product.name", "grandppltedx");
        property_override("ro.product.model", "SM-G532G/DS");
    }

    if (bootloader.find("G532M") != std::string::npos) {
        /* G532M */
        property_override("ro.build.fingerprint", "samsung/grandpplteub/grandpplte:6.0.1/MMB29T/G532MUMU1ASA1:user/release-keys");
        property_override("ro.build.description", "grandpplteub-user 6.0.1 MMB29T G532MUMU1ASA1 release-keys");
        property_override("ro.product.name", "grandpplteub");
        property_override("ro.product.model", "SM-G532M");
    }

    if (bootloader.find("G532MT") != std::string::npos) {
        /* G532MT */
        property_override("ro.build.fingerprint", "samsung/grandppltedtvvj/grandpplte:6.0.1/MMB29T/G532MTVJU1ASA1:user/release-keys");
        property_override("ro.build.description", "grandppltedtvvj-user 6.0.1 MMB29T G532MTVJU1ASA1 release-keys");
        property_override("ro.product.name", "grandppltedtvvj");
        property_override("ro.product.model", "SM-G532MT");
    }

    if (sim_count == 1) {
        init_single();
    } else {
        init_dual();
    }

}
