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

#include <stdbool.h>

#define BT_HS_UART_DEVICE "/dev/ttyHS0"

int bt_hardware_power(bool enable);
int bt_hardware_serial(bool _open);
int bt_hardware_download_firmware(void);

int bt_hardware_init(void);

extern unsigned char *bt_vendor_local_bdaddr;
