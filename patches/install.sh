echo "Obtaining build directory..."
rootdirectory="$PWD"
cd $rootdirectory
cd frameworks/base
echo "Applying frameworks/base patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_base/*.patch
cd $rootdirectory
cd frameworks/native
echo "Applying frameworks/native patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_native/*.patch
cd $rootdirectory
cd frameworks/av
echo "Applying frameworks/av patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/frameworks_av/*.patch
cd $rootdirectory
cd external/bluetooth/bluez
echo "Applying external/bluetooth/bluez patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/external_bluetooth_bluez/*.patch
cd $rootdirectory
cd hardware/qcom/display
echo "Applying hardware/qcom/display patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/hardware_qcom_display/*.patch
cd $rootdirectory