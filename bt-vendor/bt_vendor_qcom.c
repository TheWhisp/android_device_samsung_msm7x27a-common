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

#define BT_DEBUG 0
#define LOG_TAG "bt_vendor_qcom"

#include <cutils/log.h>
#include <errno.h>

#include <bt_vendor_lib.h>

#include <bt_hardware.h>

const bt_vendor_callbacks_t *bt_vendor_callbacks = NULL;
unsigned char *bt_vendor_local_bdaddr = NULL;

static int bt_vendor_qcom_init(const bt_vendor_callbacks_t *p_cb,
	unsigned char *local_bdaddr)
{
	int ret;

	ALOGD_IF(BT_DEBUG, "initializing.");

	if (p_cb == NULL) {
		ALOGE("init failed: no vendor callbacks.");
		return -EINVAL;
	}

	bt_vendor_callbacks = p_cb;
	bt_vendor_local_bdaddr = local_bdaddr;

	ret = bt_hardware_init();

	return ret;
}

static int bt_vendor_qcom_op(bt_vendor_opcode_t opcode, void *param)
{
	ALOGD_IF(BT_DEBUG, "op: opcode %d", opcode);

	int fd;
	bt_vendor_power_state_t power_state;

	switch (opcode) {
	case BT_VND_OP_POWER_CTRL:
		power_state = *(uint8_t *)param;
		switch (power_state) {
		case BT_VND_PWR_OFF: bt_hardware_power(false); break;
		case BT_VND_PWR_ON: bt_hardware_power(true); break;
		}
		break;
	/* As of current state, firmware is downloaded by the hci_qcomm_init
	 * binary. The binary cannot establish connection when userial is
	 * opened, so we have to download it at power up.
	 * Report dummy success here. */
	case BT_VND_OP_FW_CFG:
		sleep(1); /* There seems to be a race condition. */
		bt_vendor_callbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
		break;
	case BT_VND_OP_SCO_CFG:
		bt_vendor_callbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS);
		break;
	case BT_VND_OP_USERIAL_OPEN:
		fd = bt_hardware_serial(true);
		if (fd > 0) {
			int (*fd_array)[] = (int (*) []) param;
			(*fd_array)[CH_CMD] = fd;
			(*fd_array)[CH_EVT] = fd;
			(*fd_array)[CH_ACL_OUT] = fd;
			(*fd_array)[CH_ACL_IN] = fd;
		}
		/* Set return value to 1 (UART) */
		return 1;
	case BT_VND_OP_USERIAL_CLOSE:
		bt_hardware_serial(false);
		break;
	case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
		break;
	case BT_VND_OP_LPM_SET_MODE:
		bt_vendor_callbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS);
		break;
	case BT_VND_OP_LPM_WAKE_SET_STATE:
		break;
	}

	return 0;
}

static void bt_vendor_qcom_cleanup(void)
{
	ALOGD_IF(BT_DEBUG, "cleanup.");
}

const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
	.size		= sizeof(bt_vendor_interface_t),
	.init		= bt_vendor_qcom_init,
	.op		= bt_vendor_qcom_op,
	.cleanup	= bt_vendor_qcom_cleanup,
};
