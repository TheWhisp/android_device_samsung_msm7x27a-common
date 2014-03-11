#!/bin/sh

cd device/samsung/msm7x27a-common/patches/

./apply/install-common.sh
./apply/install-gbbootloader.sh
./apply/install-legacy-cam.sh
./apply/install-legacy-storage.sh
./apply/install-mdpi.sh
