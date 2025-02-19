/*
 *******************************************************************************
 * Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <osif.h>

#include "platform_autoconf.h"
#include <log_service.h>
#include <bt_utils.h>
#include <rtk_bt_def.h>
#include <rtk_bt_common.h>
#include <rtk_bt_att_defs.h>
#include <rtk_bt_gattc.h>
#include <atcmd_bt_impl.h>

#include <rtk_client_config.h>
#include <rtk_bas_client.h>
#include <rtk_gaps_client.h>
#include <rtk_simple_ble_client.h>
#include <rtk_cte_client.h>

static int atcmd_bt_gattc_exchange_mtu(int argc, char **argv)
{
	(void)argc;
#if defined(RTK_BLE_MGR_LIB) && RTK_BLE_MGR_LIB
	(void)argv;
	GATTC_AT_PRINTK("GATTC exchange MTU is not support!");
	return -1;
#else
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = (uint16_t)str_to_int(argv[0]);
	ret = rtk_bt_gattc_exchange_mtu(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GATTC exchange MTU failed! err:0x%x", ret);
		return -1;
	}

	return 0;
#endif
}

static int atcmd_bt_gattc_discover(int argc, char **argv)
{
#if defined(RTK_BLE_MGR_LIB) && RTK_BLE_MGR_LIB
	(void)argc;
	uint16_t ret = 0;

	GATTC_AT_PRINTK("GATT Client based on ble_mgr\r\n");

	ret = rtk_bt_gattc_discover_all(str_to_int(argv[0]));
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GATTC Discover failed! err: 0x%x", ret);
		return -1;
	} else {
		GATTC_AT_PRINTK("GATTC Discovering ...");
	}

	return 0;

#else
	uint16_t ret = 0;
	rtk_bt_gattc_discover_param_t disc_param = {0};
	disc_param.profile_id = GCS_CLIENT_PROFILE_ID;

	GATTC_AT_PRINTK("GATT Client based on common API\r\n");
	if (argc < 2) {
		AT_PRINTK("%s, wrong args num: %d", __func__, argc);
		return -1;
	}

	disc_param.conn_handle = str_to_int(argv[0]);
	disc_param.type = (rtk_bt_gattc_discover_type_t)str_to_int(argv[1]);

	switch (disc_param.type) {
	case 0:
		if (2 != argc) {
			goto WRONG_ARG_NUM;
		}
		break;
	case 1:
		if (3 != argc) {
			goto WRONG_ARG_NUM;
		}
		if (strlen(argv[2]) == (2 + 4)) { /* 0xNNNN 2+4 */
			disc_param.disc_primary_by_uuid.uuid_type = BT_UUID_TYPE_16;
			hexnum_str_to_array(argv[2], (uint8_t *)disc_param.disc_primary_by_uuid.uuid, 2);
		} else if (strlen(argv[2]) == (2 + 16 * 2)) { /* 0x001122...ff 2+16*2 */
			disc_param.disc_primary_by_uuid.uuid_type = BT_UUID_TYPE_128;
			hexnum_str_to_array(argv[2], (uint8_t *)disc_param.disc_primary_by_uuid.uuid, 16);
		} else {
			GATTC_AT_PRINTK("GATTC Discover failed, wrong uuid!");
			return -1;
		}
		break;
	case 2:
		if (4 != argc) {
			goto WRONG_ARG_NUM;
		}
		disc_param.disc_inc.start_handle = str_to_int(argv[2]);
		disc_param.disc_inc.end_handle = str_to_int(argv[3]);
		break;
	case 3:
		if (4 != argc) {
			goto WRONG_ARG_NUM;
		}
		disc_param.disc_char_all.start_handle = str_to_int(argv[2]);
		disc_param.disc_char_all.end_handle = str_to_int(argv[3]);
		break;
	case 4:
		if (5 != argc) {
			goto WRONG_ARG_NUM;
		}
		disc_param.disc_char_by_uuid.start_handle = str_to_int(argv[2]);
		disc_param.disc_char_by_uuid.end_handle = str_to_int(argv[3]);
		if (strlen(argv[4]) == (2 + 4)) { /* 0xNNNN 2+4 */
			disc_param.disc_char_by_uuid.uuid_type = BT_UUID_TYPE_16;
			hexnum_str_to_array(argv[4], (uint8_t *)disc_param.disc_char_by_uuid.uuid, 2);
		} else if (strlen(argv[4]) == (2 + 16 * 2)) {
			disc_param.disc_char_by_uuid.uuid_type = BT_UUID_TYPE_128;
			hexnum_str_to_array(argv[4], (uint8_t *)disc_param.disc_char_by_uuid.uuid, 16);
		} else {
			GATTC_AT_PRINTK("GATTC Discover failed, wrong uuid!");
			return -1;
		}
		break;
	case 5:
		if (4 != argc) {
			goto WRONG_ARG_NUM;
		}
		disc_param.disc_descriptor.start_handle = str_to_int(argv[2]);
		disc_param.disc_descriptor.end_handle = str_to_int(argv[3]);
		break;
	default:
		GATTC_AT_PRINTK("GATTC Discover failed, wrong type: %d!", disc_param.type);
		return -1;
		break;
	}

	ret = rtk_bt_gattc_discover(&disc_param);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GATTC Discover failed! err: 0x%x", ret);
		return -1;
	} else {
		GATTC_AT_PRINTK("GATTC Discovering ...");
	}


	return 0;

WRONG_ARG_NUM:
	GATTC_AT_PRINTK("GATTC Discover failed: type (%d) with wrong args number!", disc_param.type);
	return -1;
#endif
}

static int atcmd_bt_gattc_read(int argc, char **argv)
{
	uint16_t ret = 0;
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
	uint32_t handle_count = 0;
	uint16_t handles_arr[10] = {0};
#endif
	rtk_bt_gattc_read_param_t read_param = {0};

	read_param.conn_handle = str_to_int(argv[0]);
	read_param.profile_id = GCS_CLIENT_PROFILE_ID;
	read_param.type = str_to_int(argv[1]);

	switch (read_param.type) {
	case 0:
		if (3 != argc) {
			goto WRONG_ARG_NUM;
		}
		read_param.by_handle.handle = str_to_int(argv[2]);
		break;
	case 1:
		if (5 != argc) {
			goto WRONG_ARG_NUM;
		}
		read_param.by_uuid.start_handle = str_to_int(argv[2]);
		read_param.by_uuid.end_handle = str_to_int(argv[3]);
#if defined(RTK_BLE_MGR_LIB) && RTK_BLE_MGR_LIB
		read_param.by_uuid.char_uuid.is_uuid16 = true;
		read_param.by_uuid.char_uuid.p.uuid16 = str_to_int(argv[4]);
#else
		if (strlen(argv[4]) == (2 + 4)) {
			read_param.by_uuid.uuid_type = BT_UUID_TYPE_16;
			hexnum_str_to_array(argv[4], (uint8_t *)read_param.by_uuid.uuid, 2);
		} else if (strlen(argv[4]) == (2 + 16 * 2)) {
			read_param.by_uuid.uuid_type = BT_UUID_TYPE_128;
			hexnum_str_to_array(argv[4], (uint8_t *)read_param.by_uuid.uuid, 16);
		} else {
			GATTC_AT_PRINTK("GATTC Read failed, wrong uuid!");
			return -1;
		}
#endif
		break;
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
	case 2:
		GATTC_AT_PRINTK("GATTC Read_multiple not supported temporarily!");
		return -1;
	case 3:
		if (argc < 4) {
			goto WRONG_ARG_NUM;
		}
		handle_count = str_to_int(argv[2]);
		for (uint32_t i = 0; i < handle_count; i++) {
			handles_arr[i] = str_to_int(argv[3 + i]);
		}
		read_param.multiple_variable.handle_count = handle_count;
		read_param.multiple_variable.handles = handles_arr;

		break;
#endif
	default:
		GATTC_AT_PRINTK("GATTC Read failed, wrong type: %d!", read_param.type);
		return -1;
		break;
	}

	ret = rtk_bt_gattc_read(&read_param);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GATTC Read failed! err:0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("GATTC Reading ...");
	return 0;

WRONG_ARG_NUM:
	GATTC_AT_PRINTK("GATTC Read failed: type (%d) with wrong args number!", read_param.type);
	return -1;
}

static int atcmd_bt_gattc_write(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	rtk_bt_gattc_write_param_t write_param = {0};

	write_param.conn_handle = str_to_int(argv[0]);
	write_param.profile_id = GCS_CLIENT_PROFILE_ID;
	write_param.type = str_to_int(argv[1]);
	write_param.handle = str_to_int(argv[2]);
	write_param.length = str_to_int(argv[3]);

	if (write_param.length != strlen(argv[4]) / 2) {
		goto WRONG_ARG_LEN;
	}
	write_param.data = (void *)osif_mem_alloc(RAM_TYPE_DATA_ON, write_param.length);
	if (!write_param.data) {
		GATTC_AT_PRINTK("GATTC Write failed: cant alloc memory");
		return -1;
	}

	hexdata_str_to_array(argv[4], (uint8_t *)write_param.data, write_param.length);

	ret = rtk_bt_gattc_write(&write_param);
	if (RTK_BT_OK != ret) {
		osif_mem_free((void *)write_param.data);
		GATTC_AT_PRINTK("GATTC Write failed! err: 0x%x", ret);
		return -1;
	}

	osif_mem_free((void *)write_param.data);
	GATTC_AT_PRINTK("GATTC Writing ...");
	return 0;

WRONG_ARG_LEN:
	GATTC_AT_PRINTK("GATTC Write failed: wrong args length!");
	return -1;
}

static int atcmd_bt_gattc_enable_cccd(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	rtk_bt_gattc_update_cccd_param_t en_cccd = {0};
	uint16_t cccd_bit = 0;

	en_cccd.conn_handle = str_to_int(argv[0]);
	en_cccd.profile_id = GCS_CLIENT_PROFILE_ID;
#if defined(RTK_BLE_MGR_LIB) && RTK_BLE_MGR_LIB
	en_cccd.srv_cfg = false;
	en_cccd.srv_uuid.is_uuid16 = true;
	en_cccd.char_uuid.is_uuid16 = true;
	en_cccd.srv_uuid.p.uuid16 = str_to_int(argv[1]);
	en_cccd.char_uuid.p.uuid16 = str_to_int(argv[2]);
#else
	en_cccd.char_val_handle = str_to_int(argv[1]);
	en_cccd.cccd_handle = str_to_int(argv[2]);
#endif
	cccd_bit = str_to_int(argv[3]);

	en_cccd.bnotify = cccd_bit & (1 << 0);
	en_cccd.bindicate = cccd_bit & (1 << 1);

	ret = rtk_bt_gattc_enable_notify_or_indicate(&en_cccd);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GATTC enable cccd failed! err: 0x%x", ret);
		return -1;
	} else {
		GATTC_AT_PRINTK("GATTC enable cccd going ...");
	}

	return 0;
}

static int atcmd_bt_gattc_disable_cccd(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	rtk_bt_gattc_update_cccd_param_t dis_cccd = {0};
	uint16_t cccd_bit = 0;

	dis_cccd.conn_handle = str_to_int(argv[0]);
	dis_cccd.profile_id = GCS_CLIENT_PROFILE_ID;
#if defined(RTK_BLE_MGR_LIB) && RTK_BLE_MGR_LIB
	dis_cccd.srv_cfg = false;
	dis_cccd.srv_uuid.is_uuid16 = true;
	dis_cccd.char_uuid.is_uuid16 = true;
	dis_cccd.srv_uuid.p.uuid16 = str_to_int(argv[1]);
	dis_cccd.char_uuid.p.uuid16 = str_to_int(argv[2]);
#else
	dis_cccd.char_val_handle = str_to_int(argv[1]);
	dis_cccd.cccd_handle = str_to_int(argv[2]);
#endif
	cccd_bit = str_to_int(argv[3]);

	dis_cccd.bnotify = cccd_bit & (1 << 0);
	dis_cccd.bindicate = cccd_bit & (1 << 1);
	ret = rtk_bt_gattc_disable_notify_or_indicate(&dis_cccd);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GATTC disable cccd failed! err: 0x%x", ret);
		return -1;
	} else {
		GATTC_AT_PRINTK("GATTC disable cccd going ...");
	}

	return 0;
}


#if (defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
    (defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET)
/**************************** BAS client related *****************************/
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
static int atcmd_bas_client_srv_discover(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = str_to_int(argv[0]);
	ret = bas_client_srv_discover(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("BAS client discover failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("BAS client discovering ...");
	return 0;
}
#endif

static int atcmd_bas_client_read_battery_level(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = str_to_int(argv[0]);
	ret = bas_client_read_battery_level(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("BAS client read battery level failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("BAS client reading battery level ...");
	return 0;
}

static int atcmd_bas_client_set_notify(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;
	bool enable = true;

	conn_handle = str_to_int(argv[0]);
	enable = (str_to_int(argv[1]) ? true : false);
	ret = bas_client_set_notify(conn_handle, enable);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("BAS client battery level set notify failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("BAS client setting battery notify ...");
	return 0;
}

/**************************** GAPS client related *****************************/
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
static int atcmd_gaps_client_srv_discover(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = str_to_int(argv[0]);
	ret = gaps_client_srv_discover(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GAPS client discover failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("GAPS client discovering ...");
	return 0;
}
#endif

static int atcmd_gaps_client_read_char(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;
	uint16_t char_index = 0;

	conn_handle = str_to_int(argv[0]);
	char_index = str_to_int(argv[1]);
	ret = gaps_client_char_read(conn_handle, char_index);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("GAPS client read charac failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("GAPS client reading charac ...");
	return 0;
}

/************************* Simple ble client related *************************/
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
static int atcmd_simple_ble_client_srv_discover(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = str_to_int(argv[0]);
	ret = simple_ble_client_srv_discover(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("Simple ble client discover failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("Simple ble client discovering ...");
	return 0;
}
#endif

static int atcmd_simple_ble_client_read_charac_v0(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = str_to_int(argv[0]);
	ret = simple_ble_client_read_charac_v0(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("Simple ble client read charac v0 failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("Simple ble client reading charac v0 ...");
	return 0;
}

static int atcmd_simple_ble_client_write_charac_v1(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;
	uint16_t data_len = 0;

	conn_handle = str_to_int(argv[0]);
	data_len = str_to_int(argv[1]);
	void *data = (void *)osif_mem_alloc(RAM_TYPE_DATA_ON, data_len);
	hexdata_str_to_array(argv[2], (uint8_t *)data, data_len);

	ret = simple_ble_client_write_charac_v1(conn_handle, data, data_len);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("Simple ble client write charac v1 failed! err: 0x%x", ret);
		osif_mem_free(data);
		return -1;
	}

	osif_mem_free(data);
	GATTC_AT_PRINTK("Simple ble client writing charac v1 ...");
	return 0;

}

static int atcmd_simple_ble_client_set_notify_charac_v2(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;
	bool enable = true;

	conn_handle = str_to_int(argv[0]);
	enable = (str_to_int(argv[1]) ? true : false);
	ret = simple_ble_client_set_notify_charac_v2(conn_handle, enable);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("Simple ble client set charac v2 notify failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("Simple ble client setting charac v2 notify ...");
	return 0;
}

static int atcmd_simple_ble_client_set_indicate_charac_v3(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;
	bool enable = 0;

	conn_handle = str_to_int(argv[0]);
	enable = (str_to_int(argv[1]) ? true : false);
	ret = simple_ble_client_set_indicate_charac_v3(conn_handle, enable);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("Simple ble client set charac v3 indicate failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("Simple ble client setting charac v3 indicate ...");
	return 0;
}

#if defined(RTK_BLE_5_1_CTE_SUPPORT) && RTK_BLE_5_1_CTE_SUPPORT
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
static int atcmd_cte_client_srv_discover(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;

	conn_handle = str_to_int(argv[0]);
	ret = cte_client_srv_discover(conn_handle);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("CTE client discover failed! err: 0x%x", ret);
		return -1;
	}

	GATTC_AT_PRINTK("CTE client discovering ...");
	return 0;
}
#endif  /* RTK_BLE_MGR_LIB  */

static int atcmd_cte_client_write_charac(int argc, char **argv)
{
	(void)argc;
	uint16_t ret = 0;
	uint16_t conn_handle = 0;
	uint16_t data_len = 0;
	void *data;
	cte_charac_index_e char_idx;

	conn_handle = str_to_int(argv[0]);
	char_idx = str_to_int(argv[1]);
	data_len = str_to_int(argv[2]);
	data = (void *)osif_mem_alloc(RAM_TYPE_DATA_ON, data_len);
	hexdata_str_to_array(argv[3], (uint8_t *)data, data_len);

	ret = cte_client_write_charac(conn_handle, char_idx, data_len, data);
	if (RTK_BT_OK != ret) {
		GATTC_AT_PRINTK("CTE client write characteristic index %u failed! err: 0x%x", ret, char_idx);
		osif_mem_free(data);
		return -1;
	}

	osif_mem_free(data);
	GATTC_AT_PRINTK("CTE client writing characteristic index %u ...", char_idx);
	return 0;
}
#endif/* RTK_BLE_5_1_CTE_SUPPORT  */

#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_SCATTERNET */

static const cmd_table_t gattc_cmd_table[] = {
	{"exch_mtu", atcmd_bt_gattc_exchange_mtu, 2, 2},
	{"disc",    atcmd_bt_gattc_discover,     2, 6},
	{"read",    atcmd_bt_gattc_read,         3, 13},
	{"write",   atcmd_bt_gattc_write,        6, 6},
	{"en_cccd",    atcmd_bt_gattc_enable_cccd,    5, 5},
	{"dis_cccd",    atcmd_bt_gattc_disable_cccd,    5, 5},

#if (defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
    (defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET)
	/* bas client related */
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
	{"bas_disc",    atcmd_bas_client_srv_discover,    2, 2},
#endif
	{"bas_read_bat_lev",    atcmd_bas_client_read_battery_level,    2, 2},
	{"bas_set_notify",    atcmd_bas_client_set_notify,    3, 3},
	/* gaps client related */
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
	{"gaps_disc",    atcmd_gaps_client_srv_discover,    2, 2},
#endif
	{"gaps_read_char",    atcmd_gaps_client_read_char,    3, 3},
	/* simple ble client related */
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
	{"simble_disc",     atcmd_simple_ble_client_srv_discover, 2, 2},
#endif
	{"simble_read",     atcmd_simple_ble_client_read_charac_v0, 2, 2},
	{"simble_write",     atcmd_simple_ble_client_write_charac_v1, 4, 4},
	{"simble_set_notify",     atcmd_simple_ble_client_set_notify_charac_v2, 3, 3},
	{"simble_set_indicate",     atcmd_simple_ble_client_set_indicate_charac_v3, 3, 3},
	/* cte client related */
#if defined(RTK_BLE_5_1_CTE_SUPPORT) && RTK_BLE_5_1_CTE_SUPPORT
#if !defined(RTK_BLE_MGR_LIB) || !RTK_BLE_MGR_LIB
	{"cte_disc",        atcmd_cte_client_srv_discover, 2, 2},
#endif
	{"cte_write",       atcmd_cte_client_write_charac, 5, 5},
#endif
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_SCATTERNET */

	{NULL,},
};

int atcmd_bt_gattc(int argc, char *argv[])
{
#if (defined(CONFIG_NEW_ATCMD) && CONFIG_NEW_ATCMD) && (!defined(ATCMD_BT_CUT_DOWN) || !ATCMD_BT_CUT_DOWN)
	return atcmd_bt_excute(argc, argv, gattc_cmd_table, "[AT+BLEGATTC]");
#else
	return atcmd_bt_excute(argc, argv, gattc_cmd_table, "[ATBC][gattc]");
#endif
}