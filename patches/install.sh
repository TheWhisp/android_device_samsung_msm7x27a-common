echo "Obtaining build directory..."
rootdirectory="$PWD"
cd $rootdirectory
cd frameworks/base
echo "Applying frameworks/base patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_base/*.patch
cd $rootdirectory
cd frameworks/native
echo "Applying frameworks/native patches..."
git revert 35c7cd72f91eedc77ab693ebb94121bdd57cc22c
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_native/*.patch
cd $rootdirectory
cd frameworks/av
echo "Applying frameworks/av patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_av/*.patch
cd $rootdirectory
cd hardware/qcom/display
echo "Applying hardware/qcom/display patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/hardware_qcom_display/*.patch
cd $rootdirectory
cd hardware/libhardware_legacy
echo "Applying hardware/libhardware_legacy patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/hardware_libhardware_legacy/*.patch
cd $rootdirectory
cd system/core
echo "Applying system/core patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/system_core/*.patch
cd $rootdirectory
