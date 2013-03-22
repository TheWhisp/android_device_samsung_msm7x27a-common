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
 *  Filename:      userial_vendor.c
 *
 *  Description:   Contains vendor-specific userial functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_userial_vendor"

#include <utils/Log.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "bt_vendor_qcom.h"
#include "userial_vendor.h"

int bt_hci_init_transport()
{
  struct termios   term;
  int fd = -1;
  int retry = 0;

  fd = open(BLUETOOTH_UART_DEVICE_PORT, (O_RDWR | O_NOCTTY));

  while ((fd == -1) && (retry < 7)) {
    ALOGE("init_transport: Cannot open %s: %s\n. Retry after 2 seconds",
        BLUETOOTH_UART_DEVICE_PORT, strerror(errno));
    usleep(2000000);
    fd = open(BLUETOOTH_UART_DEVICE_PORT, (O_RDWR | O_NOCTTY));
    retry++;
  }

  if (fd == -1)
  {
    ALOGE("init_transport: Cannot open %s: %s\n",
        BLUETOOTH_UART_DEVICE_PORT, strerror(errno));
    return -1;
  }

  /* Sleep (0.5sec) added giving time for the smd port to be successfully
     opened internally. Currently successful return from open doesn't
     ensure the smd port is successfully opened.
     TODO: Following sleep to be removed once SMD port is successfully
     opened immediately on return from the aforementioned open call */
     usleep(500000);

  if (tcflush(fd, TCIOFLUSH) < 0)
  {
    ALOGE("init_uart: Cannot flush %s\n", BLUETOOTH_UART_DEVICE_PORT);
    close(fd);
    return -1;
  }

  if (tcgetattr(fd, &term) < 0)
  {
    ALOGE("init_uart: Error while getting attributes\n");
    close(fd);
    return -1;
  }

  cfmakeraw(&term);

  /* JN: Do I need to make flow control configurable, since 4020 cannot
   * disable it?
   */
  term.c_cflag |= (CRTSCTS | CLOCAL);

  if (tcsetattr(fd, TCSANOW, &term) < 0)
  {
    ALOGE("init_uart: Error while getting attributes\n");
    close(fd);
    return -1;
  }

  ALOGI("Done intiailizing UART\n");
  return fd;
}

int bt_hci_deinit_transport(int pFd)
{
    close(pFd);
    return TRUE;
}
