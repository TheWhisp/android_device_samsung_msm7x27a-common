echo "Obtaining build directory..."
rootdirectory="$PWD"
cd $rootdirectory
cd frameworks/native
echo "Applying frameworks/native patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_native/*.patch
cd $rootdirectory
cd external/wpa_supplicant_8
git am $rootdirectory/device/samsung/msm7x27a-common/patches/external_wpa_supplicant_8/*.patch
cd $rootdirectory
