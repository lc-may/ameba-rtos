/*
*******************************************************************************
* Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*******************************************************************************
*/

#include "osif.h"
#include "hci_config.h"
#include "hci/hci_common.h"
#include "hci_uart.h"
#include "hci_platform.h"
#include "bt_debug.h"
#include "ameba.h"
#include "platform_autoconf.h"
#if defined(CONFIG_WLAN) && CONFIG_WLAN
#include "wifi_conf.h"
extern int wifi_set_ips_internal(u8 enable);
#endif

#define HCI_PHY_EFUSE_OFFSET       0x740
#define HCI_PHY_EFUSE_LEN          0x70
#define HCI_LGC_EFUSE_OFFSET       0x1B0
#define HCI_LGC_EFUSE_LEN          0x50
#define HCI_MAC_ADDR_LEN           6
#define HCI_CONFIG_SIGNATURE       0x8723ab55
#define HCI_CONFIG_HDR_LEN         6

#define HCI_CFG_BAUDRATE           BIT0
#define HCI_CFG_FLOWCONTROL        BIT1
#define HCI_CFG_BD_ADDR            BIT2
#define LEFUSE(x)                  ((x)-HCI_LGC_EFUSE_OFFSET)
#define PEFUSE(x)                  ((x)-HCI_PHY_EFUSE_OFFSET)

uint32_t hci_cfg_sw_val = 0xFF;    // Open BT Trace log & FW log use 0xDD

static uint8_t bt_manual_tx_power_gain_enable = 0;
static uint8_t bt_manual_gain_index_le;
static uint8_t bt_manual_gain_index_br;
static uint8_t bt_manual_gain_index_edr2m;
static uint8_t bt_manual_gain_index_edr3m;

static uint8_t hci_phy_efuse[HCI_PHY_EFUSE_LEN]  = {0};
static uint8_t hci_lgc_efuse[HCI_LGC_EFUSE_LEN]  = {0};
unsigned char hci_init_config[] = {
	/* Header */
	0x55, 0xAB, 0x23, 0x87,

	/* Length */
	0x40, 0x00,

	/* BT MAC Address */
	0x30, 0x00, 0x06, 0x8A, 0xD5, 0x23, 0x4C, 0xE0, 0x00,

	/* LOG Uart Baudrate 115200 */
	0x08, 0x00, 0x04, 0x00, 0xC2, 0x01, 0x00,

	/* HCI Uart Baudrate 3M */
	0x0C, 0x00, 0x04, 0x01, 0x80, 0x92, 0x04,

	/* HCI Uart Flow Control */
	0x18, 0x00, 0x01, 0x5C,

	/* BT Wake Host */
	0x42, 0x00, 0x01, 0x09,

	/* Host Wake BT */
	0x43, 0x00, 0x01, 0x07,

	/* RF: phy_flow_ctrl_para(1), iqm_txgaink_module(1), iqm_txgain_flatk_module(4), iqm_txgain_10dBm_raw_index(1), lbt_ant_gain(1) */
	0x78, 0x02, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07,

	/* RF: iqm_max_txgain_1M(1), iqm_max_txgain_2M(1), iqm_max_txgain_3M(1), iqm_max_txgain_LE1M(1), iqm_max_txgain_LE2M(1), iqm_max_txgain_LE1M_adv(1), iqm_max_txgain_LE2M_adv(1), iqm_max_txgain_LR(1) */
	0x82, 0x02, 0x08, 0x3C, 0x40, 0x40, 0x3C, 0x3C, 0x45, 0x41, 0x3C,

	/* RF: iqm_max_txgain_LE1M_2402(1), iqm_max_txgain_LE1M_2480(1), iqm_max_txgain_LE2M_2402(1), iqm_max_txgain_LE2M_2480(1) */
	0x8A, 0x02, 0x04, 0x70, 0x70, 0x70, 0x70,

	/* tmeterx4_txgaink_module(1)*/
	0x8E, 0x02, 0x01, 0x62
};

static uint8_t  hci_cfg_bd_addr[HCI_MAC_ADDR_LEN] = {0};
static uint32_t hci_cfg_flag                      = 0;

void hci_platform_cfg_bd_addr(uint8_t *bdaddr)
{
	for (uint8_t i = 0; i < HCI_MAC_ADDR_LEN; i++) {
		hci_cfg_bd_addr[i] = bdaddr[i];
	}

	hci_cfg_flag |= HCI_CFG_BD_ADDR;
}

void hci_platform_bt_rf_calibration(void)
{
	// DAC DCK
	/* 0x741[2] 0 for bt dac dck valid */
	if ((hci_phy_efuse[PEFUSE(0x741)] & BIT2) == 0) {
		struct bt_rfk_param p_temp_pram = {0};

		/* DCK_KOSEN_Q_BT_0dB (0x752[23:18]) */
		/* DCK_DOS_Q_BT_0dB (0x752[11:6]) */
		/* DCK_KOSEN_I_BT_0dB (0x752[17:12]) */
		/* DCK_DOS_I_BT_0dB (0x752[5:0]) */
		p_temp_pram.type = BT_DAC_DCK;
		p_temp_pram.rfk_data1 = (hci_phy_efuse[PEFUSE(0x754)] & 0xFC) >> 2;
		p_temp_pram.rfk_data2 = (hci_phy_efuse[PEFUSE(0x752)] & 0xC0) >> 6 | (hci_phy_efuse[PEFUSE(0x753)] & 0x0F) << 2;
		p_temp_pram.rfk_data3 = (hci_phy_efuse[PEFUSE(0x753)] & 0xF0) >> 4 | (hci_phy_efuse[PEFUSE(0x754)] & 0x03) << 4;
		p_temp_pram.rfk_data4 = hci_phy_efuse[PEFUSE(0x752)] & 0x3F;

		wifi_btcoex_bt_rfk(&p_temp_pram);
	}

	//TX LOK
	/* 0x741[0] 0 for tx lok valid */
	if ((hci_phy_efuse[PEFUSE(0x741)] & BIT0) == 0) {
		struct bt_rfk_param p_temp_pram = {0};

		/* TXMOD_IDAC_IS[4:0] (0x743[4:0]) */
		/* TXMOD_IDAC_QS[4:0] (0x745[4:0]) */
		/* TXMOD_IDAC2_IS[3:0] (0x744[3:0]) */
		/* TXMOD_IDAC2_QS[3:0] (0x746[3:0]) */
		p_temp_pram.type = BT_LOK;
		p_temp_pram.rfk_data1 = hci_phy_efuse[PEFUSE(0x743)] & 0x1F;
		p_temp_pram.rfk_data2 = hci_phy_efuse[PEFUSE(0x745)] & 0x1F;
		p_temp_pram.rfk_data3 = hci_phy_efuse[PEFUSE(0x744)] & 0x0F;
		p_temp_pram.rfk_data4 = hci_phy_efuse[PEFUSE(0x746)] & 0x0F;

		wifi_btcoex_bt_rfk(&p_temp_pram);
	}

	//TX LOK Res
	/* 0x741[4] 0 for tx lok res valid */
	if ((hci_phy_efuse[PEFUSE(0x741)] & BIT4) == 0) {
		struct bt_rfk_param p_temp_pram = {0};

		/* LO_resolution */
		p_temp_pram.type = BT_LOK_RES;
		p_temp_pram.rfk_data1 = hci_phy_efuse[PEFUSE(0x761)];

		wifi_btcoex_bt_rfk(&p_temp_pram);
	}
}

void hci_platform_bt_rx_dck(void)
{
	struct bt_rfk_param p_temp_pram = {0};

	p_temp_pram.type = BT_RX_DCK;

	wifi_btcoex_bt_rfk(&p_temp_pram);
}

int hci_platform_get_write_phy_efuse_data(uint8_t *data, uint8_t len)
{
	memcpy(data, hci_phy_efuse, len);
	return HCI_SUCCESS;
}

bool hci_platform_check_lmp_subver(uint16_t lmp_subver)
{
	if (HCI_DEFAULT_LMP_SUBVER == lmp_subver) {
		return true;
	} else {
		return false;
	}
}

static uint8_t hci_platform_read_efuse(void)
{
	uint8_t i, *pbuf;

	/* Read Logic Efuse */
	pbuf = osif_mem_alloc(RAM_TYPE_DATA_ON, 1024);
	if (!pbuf || _FAIL == OTP_LogicalMap_Read(pbuf, 0, OTP_LMAP_LEN)) {
		BT_LOGE("OTP_LogicalMap_Read failed\r\n");
		if (pbuf) {
			osif_mem_free(pbuf);
		}
		return HCI_FAIL;
	}

	memcpy(hci_lgc_efuse, pbuf + HCI_LGC_EFUSE_OFFSET, HCI_LGC_EFUSE_LEN);

	/* Read Physical Efuse */
	for (i = 0; i < HCI_PHY_EFUSE_LEN; i++) {
		OTP_Read8((HCI_PHY_EFUSE_OFFSET + i), (hci_phy_efuse + i));
	}
#if 0
	BT_DUMPA("Read Logic Efuse:\r\n", hci_lgc_efuse, HCI_LGC_EFUSE_LEN);
	BT_DUMPA("Read Phy Efuse:\r\n", hci_phy_efuse, HCI_PHY_EFUSE_LEN);
#endif
	if (pbuf) {
		osif_mem_free(pbuf);
	}

	return HCI_SUCCESS;
}

void hci_platform_set_tx_power_gain_index(uint32_t index)
{
	bt_manual_tx_power_gain_enable = 1;
	bt_manual_gain_index_le = (uint8_t)(index & 0xFF);
	bt_manual_gain_index_br = (uint8_t)((index >> 8) & 0xFF);
	bt_manual_gain_index_edr2m = (uint8_t)((index >> 16) & 0xFF);
	bt_manual_gain_index_edr3m = (uint8_t)((index >> 24) & 0xFF);
}

static uint8_t hci_platform_parse_config(void)
{
	uint8_t *p, i;
	uint16_t entry_offset, entry_len, config_length;
	uint32_t config_header;
	//uint16_t tx_flatk;

	if (sizeof(hci_init_config) <= HCI_CONFIG_HDR_LEN) {
		return HCI_IGNORE;
	}

	p = hci_init_config;
	memcpy((void *)&config_header, (void *)p, sizeof(uint32_t));
	if (HCI_CONFIG_SIGNATURE != config_header) {
		return HCI_FAIL;
	}

	memcpy((void *)&config_length, (void *)(p + 4), sizeof(uint16_t));
	if (config_length != (uint16_t)(sizeof(hci_init_config) - HCI_CONFIG_HDR_LEN)) {
		/* Fix the len, just avoid the length is not corect */
		p[4] = (uint8_t)((sizeof(hci_init_config) - HCI_CONFIG_HDR_LEN) >> 0);
		p[5] = (uint8_t)((sizeof(hci_init_config) - HCI_CONFIG_HDR_LEN) >> 8);
	}

	p += HCI_CONFIG_HDR_LEN;
	while (p < hci_init_config + sizeof(hci_init_config)) {
		memcpy((void *)&entry_offset, (void *)p, sizeof(uint16_t));
		entry_len = *(uint8_t *)(p + 2);
		p += 3;

		switch (entry_offset) {
		case 0x000c:
			hci_set_work_baudrate(p);
			break;
		case 0x0018:
			/* MP Mode, Close Flow Control */
			if (hci_is_mp_mode()) {
				p[0] = p[0] & (~BIT2);
			}
			break;
		case 0x0030:
			/* Set ConfigBuf MacAddr, Use Customer Assign or Efuse */
			if (hci_cfg_flag & HCI_CFG_BD_ADDR) {
				for (i = 0; i < HCI_MAC_ADDR_LEN; i++) {
					p[i] = hci_cfg_bd_addr[i];
				}
			} else {
				if ((hci_lgc_efuse[4] != 0xff) || (hci_lgc_efuse[5] != 0xff) || (hci_lgc_efuse[6] != 0xff) || \
					(hci_lgc_efuse[7] != 0xff) || (hci_lgc_efuse[8] != 0xff) || (hci_lgc_efuse[9] != 0xff)) {
					for (i = 0; i < HCI_MAC_ADDR_LEN; i++) {
						p[i] = hci_lgc_efuse[4 + HCI_MAC_ADDR_LEN - 1 - i];
					}
				}
			}
			BT_LOGA("Bluetooth init BT_ADDR in cfgbuf [%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
					p[5], p[4], p[3], p[2], p[1], p[0]);
			break;
		case 0x0278:
			if (hci_lgc_efuse[LEFUSE(0x1be)] != 0xff) {
				p[0] = hci_lgc_efuse[LEFUSE(0x1be)];
				if (hci_lgc_efuse[LEFUSE(0x1be)] & BIT1) {
					p[1] = hci_lgc_efuse[LEFUSE(0x1bf)];
				}

				if (hci_lgc_efuse[LEFUSE(0x1be)] & BIT2) {
					p[2] = hci_lgc_efuse[LEFUSE(0x1c0)];
					p[3] = hci_lgc_efuse[LEFUSE(0x1c1)];
					p[4] = hci_lgc_efuse[LEFUSE(0x1c2)];
					p[5] = hci_lgc_efuse[LEFUSE(0x1c3)];
				}

				if (hci_lgc_efuse[LEFUSE(0x1be)] & BIT5) {
					p[6] = hci_lgc_efuse[LEFUSE(0x1c4)];
					p[7] = hci_lgc_efuse[LEFUSE(0x1c5)];
				}
			}
			break;
		case 0x0282:
			for (i = 0; i < entry_len; i++) {
				if (bt_manual_tx_power_gain_enable) {
					if (i == 0) { // br
						BT_LOGA("0x0282 BR gain 0x%02x\r\n", bt_manual_gain_index_br);
						p[i] = bt_manual_gain_index_br;
					} else if (i == 1) { // edr 2m
						BT_LOGA("0x0282 EDR 2M gain 0x%02x\r\n", bt_manual_gain_index_edr2m);
						p[i] = bt_manual_gain_index_edr2m;
					} else if (i == 2) { // edr 3m
						BT_LOGA("0x0282 EDR 3M gain 0x%02x\r\n", bt_manual_gain_index_edr3m);
						p[i] = bt_manual_gain_index_edr3m;
					} else { // le
						BT_LOGA("0x0282 LE gain 0x%02x\r\n", bt_manual_gain_index_le);
						p[i] = bt_manual_gain_index_le;
					}
				} else {
					if (hci_lgc_efuse[LEFUSE(0x1c8 + i)] != 0xff) {
						p[i] = hci_lgc_efuse[LEFUSE(0x1c8 + i)];
					}
				}
			}
			break;
		case 0x028a:
			for (i = 0; i < entry_len; i++) {
				if (bt_manual_tx_power_gain_enable) {
					BT_LOGA("0x028a LE gain 0x%02x\r\n", bt_manual_gain_index_le);
					p[i] = bt_manual_gain_index_le;
				} else {
					if (hci_lgc_efuse[LEFUSE(0x1d0 + i)] != 0xff) {
						p[i] = hci_lgc_efuse[LEFUSE(0x1d0 + i)];
					}
				}
			}
			break;
		case 0x028e:
			if (hci_lgc_efuse[LEFUSE(0x1d4)] != 0xff) {
				p[0] = hci_lgc_efuse[LEFUSE(0x1d4)];
			}
			break;
		default:
			break;
		}

		p +=  entry_len;
	}

	return HCI_SUCCESS;
}

static void bt_power_on(void)
{
	set_reg_value(0x4100C200, BIT1, 1);             //enable BT Power Cut
	osif_delay(5);
	set_reg_value(0x4100C200, BIT17, 0);            //disable ISO of BT
	osif_delay(5);
	set_reg_value(0x4100C214, BIT2, 1);             //enable clk_osc4m_wlbt
	osif_delay(5);
	set_reg_value(0x4100C208, BIT2, 1);             //enable WL RFAFE control circuit
	osif_delay(5);
	set_reg_value(0x4100C480, BIT5 | BIT6, 3);      //enable RFAFE
	osif_delay(5);
	set_reg_value(0x4100C4A8, BIT4, 1);             //enable WL_CKI_80M_RFC, default=1, can skip
	osif_delay(5);
	set_reg_value(0x4100C4A8, BIT3, 0);             //when WL RFAFE enter power off, keep WLRFC not power off
	osif_delay(5);
	set_reg_value(0x4100C208, BIT1, 1);             //release BTON reset
	osif_delay(5);
	set_reg_value(0x4100C214, BIT2, 0);             //disable clk_osc4m_wlbt
	osif_delay(5);
	if (HCI_BT_KEEP_WAKE) {
		set_reg_value(0x4100c280, BIT14, 1);        //enable HOST_WAKE_BT No GPIO
		osif_delay(5);
		set_reg_value(0x4100c280, BIT13, 1);        //HOST_WAKE_BT
		osif_delay(5);
	}
}

void bt_power_off(void)
{
	set_reg_value(0x4100C208, BIT1, 0);             //BTON reset
	osif_delay(5);
#if defined(CONFIG_WLAN) && CONFIG_WLAN
	if (!(wifi_is_running(WLAN0_IDX) || wifi_is_running(WLAN1_IDX)))
#endif
	{
		set_reg_value(0x4100C480, BIT5 | BIT6, 0);  //disable RFAFE (if WIFI active, keep 2'b11)
		osif_delay(5);
		set_reg_value(0x4100C208, BIT2, 0);         //disable WL RFAFE control circuit (if WIFI active, keep 1'b1)
		osif_delay(5);
	}
	set_reg_value(0x4100C200, BIT17, 1);            //enable ISO of BT
	osif_delay(5);
	set_reg_value(0x4100C200, BIT1, 0);             //disable BT Power Cut
	osif_delay(5);
}

void hci_platform_controller_reset(void)
{
	/* BT Controller Power */
	bt_power_on();

	BT_LOGD("BT Reset OK!\r\n");
}

void hci_platform_config_uart_rx(bool op)
{
	hci_uart_config_rx(op);
}

bool rtk_bt_pre_enable(void)
{
	uint32_t lock_status;

#if defined(CONFIG_WLAN) && CONFIG_WLAN
	if (!(wifi_is_running(WLAN0_IDX) || wifi_is_running(WLAN1_IDX))) {
		BT_LOGE("WiFi is OFF! Please Restart BT after Wifi on!\r\n");
		return false;
	}

	if (!hci_is_mp_mode()) {
		wifi_set_lps_enable(FALSE);
		wifi_set_ips_internal(FALSE);
	}
#endif

	lock_status = pmu_get_wakelock_status();
	if (!(lock_status & ((0x01) << PMU_BT_DEVICE))) {
		BT_LOGA("Acuqire BT PMU LOCK \r\n");
		pmu_acquire_wakelock(PMU_BT_DEVICE);
	}

	return true;
}

bool rtk_bt_post_enable(void)
{
	uint32_t lock_status;

	lock_status = pmu_get_wakelock_status();
	if (lock_status & ((0x01) << PMU_BT_DEVICE)) {
		BT_LOGA("Release BT PMU LOCK \r\n");
		pmu_release_wakelock(PMU_BT_DEVICE);
	}

#if defined(CONFIG_WLAN) && CONFIG_WLAN
	if (!hci_is_mp_mode()) {
		wifi_set_lps_enable(wifi_user_config.lps_enable);
		wifi_set_ips_internal(wifi_user_config.ips_enable);
	}
#endif

	return true;
}

uint8_t hci_platform_init(void)
{
	// Move to rtk_bt_enable() to aviod rtk upperstack memory leak when fail
	// if (rtk_bt_pre_enable() == false) {
	//  BT_LOGE("rtk_bt_pre_enable fail!\r\n");
	//  return HCI_FAIL;
	// }

	if (!CHECK_CFG_SW(CFG_SW_BT_FW_LOG)) {
		rtk_bt_fw_log_open();
		BT_LOGA("FW LOG OPEN\r\n");
#if 0
		Pinmux_Config(_PA_22, PINMUX_FUNCTION_BT_UART);
		PAD_PullCtrl(_PA_22, GPIO_PuPd_UP);
#endif
	}

	/* Read Efuse and Parse Configbuf */
	if (HCI_FAIL == hci_platform_read_efuse()) {
		return HCI_FAIL;
	}

	if (HCI_FAIL == hci_platform_parse_config()) {
		return HCI_FAIL;
	}

	/* BT Controller Reset */
	hci_platform_controller_reset();

	/* UART Init */
	if (HCI_FAIL == hci_uart_open()) {
		return HCI_FAIL;
	}

	return HCI_SUCCESS;
}

uint8_t hci_platform_deinit(void)
{
	/* BT Controller Power Off */
	if (!hci_is_mp_mode()) {
		hci_platform_config_uart_rx(false);
		bt_power_off();
	} else {
		BT_LOGA("No need to power off BT controller in MP test\r\n");
	}

	/* UART Deinit */
	hci_uart_close();

	if (!CHECK_CFG_SW(CFG_SW_BT_FW_LOG)) {
		rtk_bt_fw_log_close();
	}

	return HCI_SUCCESS;
}

void hci_platform_get_config(uint8_t **buf, uint16_t *len)
{
	*buf = hci_init_config;
	*len = sizeof(hci_init_config);
}

void hci_platform_debug_enable(void)
{
	uint32_t data;

	/* keep bt wakeup from dlps/lps */
	set_reg_value(0x4100C280, BIT14, 1);        //enable HOST_WAKE_BT No GPIO
	osif_delay(5);
	set_reg_value(0x4100C280, BIT13, 1);        //HOST_WAKE_BT
	osif_delay(5);
	while (1) {
		osif_delay(100);
		data = HAL_READ32(0x4100C284, 0) & 0x1F; // 0x4100C284 [0:4]
		if (data == 4) {
			/* bt active */
			BT_LOGD("bt active\r\n");
			break;
		} else {
			BT_LOGD("wait for bt wakeup from sleep\r\n");
		}
	}
	BT_LOGD("BT wakeup from sleep\r\n");

	set_reg_value(0x4100C9F4, BIT0, 0);                                     //swd close
	osif_delay(5);
	set_reg_value(0x4100C880, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7, 0x3F);     //Uart tx GPIOD_9 PA30
	osif_delay(5);
	set_reg_value(0x4100C280, BIT19, 1);                                    //pinmux
	osif_delay(5);
	set_reg_value(0x4100C878, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7, 0xD);      //Uart Rx GPIOD_7 PB0
	osif_delay(5);
	set_reg_value(0x4100C280, BIT19, 1);                                    //pinmux
	osif_delay(5);
	set_reg_value(0x4100C87C, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7, 0xD);      //Uart CTS GPIOD_8 PA31 Keep Low connected to GND
	osif_delay(5);
	set_reg_value(0x4100C280, BIT19, 1);                                    //pinmux
	osif_delay(5);
	set_reg_value(0x4100C884, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7, 0x40);     //Uart RTS GPIOD_10 PB1
	osif_delay(5);
	set_reg_value(0x4100C280, BIT19, 1);                                    //pinmux
	osif_delay(5);
}