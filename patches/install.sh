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

## Cherry-picks
cd dalvik
git fetch http://review.cyanogenmod.org/CyanogenMod/android_dalvik refs/changes/56/47756/1 && git cherry-pick FETCH_HEAD
cd $rootdirectory
