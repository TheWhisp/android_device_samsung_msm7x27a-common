/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 *
 *  Filename:      bt_power.c
 *
 *  Description:   Contains Bluetooth power control functions via rfkill
 *
 ******************************************************************************/

#define LOG_TAG "bt_power"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include "bt_power.h"

static char *rfkill_state_path = NULL;

static int init_rfkill()
{
    char path[64];
    char buf[16];
    int fd, sz, id;

    for (id = 0; ; id++) {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            ALOGE("init_rfkill : open(%s) failed: %s (%d)\n", \
                 path, strerror(errno), errno);
            return 0;
        }

        sz = read(fd, &buf, sizeof(buf));
        close(fd);

        if (sz >= 9 && memcmp(buf, "bluetooth", 9) == 0) {
            asprintf(&rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", id);
            break;
        }
    }

    return 1;
}

int set_bluetooth_power(char *power) {
    int ret = 1;

    ALOGI("Trying to set Bluetooth power %s", *power == '1' ? "ON" : "OFF");

    if(rfkill_state_path == NULL) {
        init_rfkill();
    }

    int fd = open(rfkill_state_path, O_WRONLY);

    if(fd < 0) {
        ALOGE("Can't open %s", rfkill_state_path);
        return 0;
    }

    int sz = write(fd, power, 1);

    if(sz < 0) {
        ALOGE("Can't write to %s", rfkill_state_path);
        ret = 0;
    }

    close(fd);
    return ret;
}
