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
cd build
echo "Applying build patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/build/*.patch
cd $rootdirectory
cd packages/apps/Settings
echo "Applying packages/apps/Settings patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/packages_apps_settings/*.patch
cd $rootdirectory
cd system/core
echo "Applying system/core patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/system_core/*.patch
cd $rootdirectory

## Cherry-picks
cd dalvik
git fetch http://review.cyanogenmod.org/CyanogenMod/android_dalvik refs/changes/57/47757/1 && git cherry-pick FETCH_HEAD
cd $rootdirectory
