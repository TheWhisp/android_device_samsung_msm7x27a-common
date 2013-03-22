echo "I: Obtaining build directory..."
rootdirectory="$PWD"
cd $rootdirectory
cd build
echo "I: Applying build patches..."
git am $rootdirectory/device/samsung/msm7x27a-common/patches/android_build/*.patch
cd $rootdirectory