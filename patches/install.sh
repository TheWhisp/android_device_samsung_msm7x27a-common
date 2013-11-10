echo "Obtaining build directory..."
rootdirectory="$PWD"
cd $rootdirectory
#cd frameworks/base
#echo "Applying frameworks/base patches..."
#git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_base/*.patch
#cd $rootdirectory
cd frameworks/av
echo "Applying frameworks/av patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_av/0*.patch
cd $rootdirectory
cd system/core
echo "Applying system/core patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/system_core/*.patch
cd $rootdirectory
cd hardware/libhardware
echo "Applying hardware/libhardware patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/hardware_libhardware/*.patch
cd $rootdirectory
cd hardware/libhardware_legacy
echo "Applying hardware/libhardware_legacy patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/hardware_libhardware_legacy/*.patch
cd $rootdirectory
