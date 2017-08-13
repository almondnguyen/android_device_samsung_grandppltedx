#
# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Release name
PRODUCT_RELEASE_NAME := SM-G532G

# Bootanimation
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/samsung/grandppltedx/full_grandppltedx.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := grandppltedx
PRODUCT_NAME := lineage_grandppltedx
PRODUCT_BRAND := Samsung
PRODUCT_MODEL := SM-G532G

# Set build fingerprint / ID / Prduct Name ect.
PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=SM-G532G TARGET_DEVICE=SM-G532G BUILD_FINGERPRINT=samsung/grandppltedx/grandpplte:6.0.1/MMB29T/G532GDXU1AQF3:user/release-keys PRIVATE_BUILD_DESC="grandppltedx-user 6.0.1 MMB29T G532GDXU1AQF3 release-keys"
