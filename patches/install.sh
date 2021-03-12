echo $1
rootdirectory="$PWD"
# ---------------------------------

dirs="bionic frameworks/av frameworks/base frameworks/native"

for dir in $dirs ; do
	cd $rootdirectory
	cd $dir
	echo "Applying $dir patches..."
	git apply $rootdirectory/device/samsung/grandppltedx/patches/$dir/*.patch
	echo " "
done

# Almond: didnt know anything about git patches, so use diff instead to fix older patches
mandirs="system/core system/netd"
cd $rootdirectory

for dir in $mandirs ; do
	echo "Applying $dir patches..."
	patch -p0 < "$rootdirectory/device/samsung/grandppltedx/patches/$dir/1.patch"
	echo " "
done


# -----------------------------------
echo "Changing to build directory..."
cd $rootdirectory
