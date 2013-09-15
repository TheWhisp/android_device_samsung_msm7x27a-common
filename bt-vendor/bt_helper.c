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
#include <errno.h>
#include <fcntl.h>

int bt_get_rfkill_state_path(char *state_path, size_t size)
{
	char path[64];

	int fd;
	int id;

	char buf[16];
	size_t buf_size;

	for (id = 0; ;id++) {
		snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);

		fd = open(path, O_RDONLY);
		if (fd < 0) {
			ALOGE("open(%s) failed: %s (%d)", path, strerror(errno), errno);
			return -1;
		}

		buf_size = read(fd, &buf, sizeof(buf));
		close(fd);

		/* Found the correct rfkill device. */
		if (buf_size >= 9 && memcmp(buf, "bluetooth", 9) == 0)
			break;
	}

	snprintf(state_path, size, "/sys/class/rfkill/rfkill%d/state", id);

	return 0;
}

int write_value(const char *path, const char *value, size_t size)
{
	int ret;
	int fd;
	size_t buf_size;
	char *buffer;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		ALOGE("open(%s) failed: %s (%d)", path, strerror(errno), errno);
		ret = -1;
		goto out;
	}

	buf_size = write(fd, value, size);
	if (buf_size != size) {
		ALOGE("write(%s) failed: %s (%d)", path, strerror(errno), errno);
		ret = -1;
		goto out;
	}

	ret = 0;

out:
	if (fd >= 0) close(fd);
	return ret;
}
