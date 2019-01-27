# Hardcode Linaro 4.8 toolchain
# Possibly not needed. I like stuff

export ANDROID_EABI_TOOLCHAIN="prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-UB-4.8/bin"
export ANDROID_EABI_TOOLCHAIN_PATH=":prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-UB-4.8/bin"

# This is optional. It always errors out on my side
# Assertion `cnt
# Your mileage may vary
export LC_ALL=C

add_lunch_combo lineage_grandppltedx-userdebug
add_lunch_combo lineage_grandppltedx-eng
