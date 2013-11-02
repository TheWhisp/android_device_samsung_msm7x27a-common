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
cd system/core
echo "Applying system/core patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/system_core/*.patch
cd $rootdirectory
cd system/vold
echo "Applying system/vold patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/system_vold/*.patch
cd $rootdirectory
cd external/wpa_supplicant_8
echo "Applying external/wpa_supplicant_8 patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/external_wpa_supplicant_8/*.patch
cd $rootdirectory
cd packages/apps/Settings
echo "Applying packages/apps/Settings patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/packages_apps_Settings/*.patch
cd $rootdirectory
