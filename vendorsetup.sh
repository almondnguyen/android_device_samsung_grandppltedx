# Use uber-4.8 toolchain
# AOSP (or lineage one) will return NO LOGGING..
echo "use different toolchain"
export ANDROID_EABI_TOOLCHAIN="prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-UB-4.9/bin"
export ANDROID_EABI_TOOLCHAIN_PATH=":prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-UB-4.9/bin"

export ANDROID_TOOLCHAIN="prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-UB-4.9/bin"

# This is optional. It always errors out on my side
# Assertion `cnt
# Your mileage may vary
export LC_ALL=C

# recommended by Lineage
# sometimes not work, do it in ~/.profile
#export ANDROID_JACK_VM_ARGS="-Dfile.encoding=UTF-8 -XX:+TieredCompilation -Xmx4G"

# 8GB-dual
add_lunch_combo lineage_grandppltedx-userdebug
add_lunch_combo lineage_grandppltedx-eng

# 16GB-single
