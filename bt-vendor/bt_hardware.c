/*
 * Copyright (C) 2013  Rudolf Tammekivi <rtammekivi@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see [http://www.gnu.org/licenses/].
 */

#define LOG_TAG "bt_vendor_qcom"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <termios.h>

#include <bt_hardware.h>
#include <bt_helper.h>

char bt_hardware_power_state_path[64];

int bt_hardware_power(bool enable)
{
	char *value = enable ? "1" : "0";

	if (bt_hardware_power_state_path == NULL)
		return -1;

	write_value(bt_hardware_power_state_path, value, 1);

	// TODO: Replace this.
	if (enable)
		bt_hardware_download_firmware();

	return 0;
}

int bt_hardware_serial(bool _open)
{
	static int fd;
	struct termios term;

	if (_open)
		fd = open(BT_HS_UART_DEVICE, (O_RDWR | O_NOCTTY));
	else
		close(fd);

	if (_open) {
		if (tcflush(fd, TCIOFLUSH) < 0) {
			ALOGE("init_uart: Cannot flush");
			close(fd);
			return -1;
		}

		if (tcgetattr(fd, &term) < 0) {
			ALOGE("init_uart: Error while getting attributes\n");
			close(fd);
			return -1;
		}

		cfmakeraw(&term);

		term.c_cflag |= (CRTSCTS | CLOCAL);

		if (tcsetattr(fd, TCSANOW, &term) < 0) {
			ALOGE("init_uart: Error while getting attributes\n");
			close(fd);
			return -1;
		}
	}

	return fd;
}

int bt_hardware_download_firmware(void)
{
	// TODO: Replace this.
	/* Run this as system call, because bt should wait until it is finished
	 * downloading the firmware.
	 * --force-hw-sleep - Disable In-Band Sleep (Use H4 Protocol).
	 * --board-address - Program MAC address. */
	char mac_argument[256];
	snprintf(mac_argument, 256, "/system/bin/hci_qcomm_init"
		" --force-hw-sleep --board-address "
		"%02X:%02X:%02X:%02X:%02X:%02X",
		bt_vendor_local_bdaddr[0], bt_vendor_local_bdaddr[1],
		bt_vendor_local_bdaddr[2], bt_vendor_local_bdaddr[3],
		bt_vendor_local_bdaddr[4], bt_vendor_local_bdaddr[5]);
	system(mac_argument);
	return 0;
}

int bt_hardware_init(void)
{
	return bt_get_rfkill_state_path(bt_hardware_power_state_path,
		sizeof(bt_hardware_power_state_path));
}
