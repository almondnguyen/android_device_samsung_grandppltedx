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
#-- etc/java-8-openjdk/security/java.security
#    in jdk.tls.disabledAlgorithms, remove TLSv1 and TLSv1.1
echo "Additional patch needed:"
echo "  in /etc/java-8-openjdk/security/java.security"
echo "  in jdk.tls.disabledAlgorithms"
echo "  remove TLSv1 and TLSv1.1"
echo ""

# -----------------------------------
echo "Changing to build directory..."
cd $rootdirectory
