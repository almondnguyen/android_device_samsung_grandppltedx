echo $1
rootdirectory="$PWD"
# ---------------------------------

dirs="bionic frameworks/av frameworks/base frameworks/native system/core system/netd prebuilts/sdk"

for dir in $dirs ; do
	cd $rootdirectory
	cd $dir
	echo "Applying $dir patches..."
	git apply $rootdirectory/device/samsung/grandppltedx/patches/$dir/*.patch
	echo " "
done

# there are other patches needed: 
#-- frameworks/base/data/keyboard/Generic.kl: 
#    APP_SWITCH: patch to key 254
#-- etc/java-8-openjdk/security/java.security
#    in jdk.tls.disabledAlgorithms, remove TLSv1 and TLSv1.1

#mandirs="system/core system/netd prebuilts/sdk"
#cd $rootdirectory

#for dir in $mandirs ; do
#	echo "Applying $dir patches..."
#	patch -p0 < "$rootdirectory/device/samsung/grandppltedx/patches/$dir/1.patch"
#	echo " "
#done


# -----------------------------------
echo "Changing to build directory..."
cd $rootdirectory
