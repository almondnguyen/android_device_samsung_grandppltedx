# This is optional. It always errors out on my side
# Assertion `cnt
# Your mileage may vary
export LC_ALL=C

# recommended by Lineage
# sometimes not work, do it in ~/.profile
export ANDROID_JACK_VM_ARGS="-Dfile.encoding=UTF-8 -XX:+TieredCompilation -Xmx6G"

# 8GB-dual
add_lunch_combo lineage_grandppltedx-userdebug
add_lunch_combo lineage_grandppltedx-eng

# 16GB-single
