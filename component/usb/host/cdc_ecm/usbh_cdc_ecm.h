/**
  ******************************************************************************
  * The header file for usbh_cdc_ecm.c
  *
  * This module is a confidential and proprietary property of RealTek and
  * possession or use of this module requires written permission of RealTek.
  *
  * Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
  ******************************************************************************
  */

#ifndef USBH_CDC_ECM_H
#define USBH_CDC_ECM_H

/* Includes ------------------------------------------------------------------*/

#include "usbh.h"

/* Macro defines -----------------------------------------------------------*/
#define ECM_ENABLE_PACKETFILTER                                 (0)

/* for RTL8156BG, use this to allow ping/UDP data transfer */
#define ECM_ENABLE_RCR_CONFIGURATION                            (0)

/* Exported defines ----------------------------------------------------------*/

#define CDC_ECM_MAC_STRING_LEN                                  (32)
#define CDC_ECM_MAC_STR_LEN                                     (6)
#define CDC_ECM_MUTICAST_FILTER_STR_LEN                         (20)
#define CDC_ECM_HOST_PIPE_COUNT                                 (16)
#define USBH_CDC_ECM_BULK_BUF_MAX_SIZE                          (512*3)


/* CDC Class Codes */
#define CDC_CLASS_CODE                                          0x02U
#define CDC_COMM_INTERFACE_CLASS_CODE                           0x02U
#define CDC_DATA_INTERFACE_CLASS_CODE                           0x0AU

/* CDC Communication sub class codes */
#define CDC_RESERVED                                            0x00U
#define CDC_ETHERNET_NETWORK_CONTROL_MODEL                      0x06U

/* Table 6: Class-Specific Request Codes for Ethernet subclass */
#define CDC_ECM_SET_ETHERNET_MULTICAST_FILTERS                  0x40U
#define CDC_ECM_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER    0x41U
#define CDC_ECM_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER    0x42U
#define CDC_ECM_SET_ETHERNET_PACKET_FILTER                      0x43U
#define CDC_ECM_GET_ETHERNET_STATISTIC                          0x44U
#define CDC_ECM_SPECIAL_REQUEST_CODE_RESERVED                   0x45U

/* [CDC]Table 20: Class-Specific Notification Codes */
/* Table 11: Class-Specific Notification Codes for Ethernet subclass */
#define CDC_ECM_NOTIFY_NETWORK_CONNECTION                       0x00U
#define CDC_ECM_NOTIFY_RESPONSE_AVAILABLE                       0x01U
#define CDC_ECM_NOTIFY_CONNECTION_SPEED_CHANGE                  0x2AU

/* Communication Interface Class Control Protocol Codes */
#define CDC_CTRL_PROTOCOL_NO_CLASS_SPECIFIC                     0x00U

/*  bDescriptor SubType in Communications Class Functional Descriptors  */
#define CDC_ECM_NETWORK_FUNC_DESCRIPTOR                         0x0FU

/* Exported types ------------------------------------------------------------*/

#pragma pack(push)
#pragma pack(1)

/* USB Host Status */
typedef enum {
	USBH_CDC_ECM_IDLE = 0U,
	USBH_CDC_ECM_DETACHED,
	USBH_CDC_ECM_ATTACH,
	USBH_CDC_ECM_SETUP,
	USBH_CDC_ECM_MAX,
} usbh_ecm_state_t;

/* CDC ECM Ethernet Packet Filter Bitmap  */
typedef enum {
	CDC_ECM_ETH_PACKET_TYPE_PROMISCUOUS     = 0x01U, /* ALL frames received by the networking device are forwarded up to the host (required) */
	CDC_ECM_ETH_PACKET_TYPE_ALL_MULTICAST   = 0x02U, /* ALL multicast frames received by the networking device are forwarded up to the host, not just
                                                        the ones enumerated in the device's multicast address list (required) */
	CDC_ECM_ETH_PACKET_TYPE_DIRECTED        = 0x04U, /* Directed packets received containing a destination address equal to the MAC address of the
                                                        networking device are forwarded up to the host (required) */
	CDC_ECM_ETH_PACKET_TYPE_BROADCAST       = 0x08U, /* All broadcast packets received by the networking device are forwarded up to the host. */
	CDC_ECM_ETH_PACKET_TYPE_MULTICAST       = 0x10U, /* All multicast packets enumerated in the device's multicast address list are forwarded up to the host */
	CDC_ECM_ETH_PACKET_TYPE_MAX,
} usbh_cdc_ecm_eth_packet_filter_bitmap_t;

/* CDC ECM Ethernet Statistics Feature Selector Codes */
typedef enum {
	CDC_ECM_FEATURE_SELECTOR_RESERVED = 0U,
	CDC_ECM_FEATURE_SELECTOR_XMIT_OK,
	CDC_ECM_FEATURE_SELECTOR_RCV_OK,
	CDC_ECM_FEATURE_SELECTOR_XMIT_ERROR,
	CDC_ECM_FEATURE_SELECTOR_RCV_ERROR,
	CDC_ECM_FEATURE_SELECTOR_RCV_NO_BUFFER,
	CDC_ECM_FEATURE_SELECTOR_DIRECTED_BYTES_XMIT,
	CDC_ECM_FEATURE_SELECTOR_DIRECTED_FRAMES_XMIT,
	CDC_ECM_FEATURE_SELECTOR_MULTICAST_BYTES_XMIT,
	CDC_ECM_FEATURE_SELECTOR_MULTICAST_FRAMES_XMIT,
	CDC_ECM_FEATURE_SELECTOR_BROADCAST_BYTES_XMIT,
	CDC_ECM_FEATURE_SELECTOR_BROADCAST_FRAMES_XMIT,
	CDC_ECM_FEATURE_SELECTOR_DIRECTED_BYTES_RCV,
	CDC_ECM_FEATURE_SELECTOR_DIRECTED_FRAMES_RCV,
	CDC_ECM_FEATURE_SELECTOR_MULTICAST_BYTES_RCV,
	CDC_ECM_FEATURE_SELECTOR_MULTICAST_FRAMES_RCV,
	CDC_ECM_FEATURE_SELECTOR_BROADCAST_BYTES_RCV,
	CDC_ECM_FEATURE_SELECTOR_BROADCAST_FRAMES_RCV,
	CDC_ECM_FEATURE_SELECTOR_RCV_CRC_ERROR,
	CDC_ECM_FEATURE_SELECTOR_TRANSMIT_QUEUE_LENGTH,
	CDC_ECM_FEATURE_SELECTOR_RCV_ERROR_ALIGNMENT,
	CDC_ECM_FEATURE_SELECTOR_XMIT_ONE_COLLISION,
	CDC_ECM_FEATURE_SELECTOR_XMIT_MORE_COLLISIONS,
	CDC_ECM_FEATURE_SELECTOR_XMIT_DEFERRED,
	CDC_ECM_FEATURE_SELECTOR_XMIT_MAX_COLLISIONS,
	CDC_ECM_FEATURE_SELECTOR_RCV_OVERRUN,
	CDC_ECM_FEATURE_SELECTOR_XMIT_UNDERRUN,
	CDC_ECM_FEATURE_SELECTOR_XMIT_HEARTBEAT_FAILURE,
	CDC_ECM_FEATURE_SELECTOR_XMIT_TIMES_CRS_LOST,
	CDC_ECM_FEATURE_SELECTOR_XMIT_LATE_COLLISIONS,
	CDC_ECM_FEATURE_SELECTOR_MAX,
} usbh_cdc_ecm_statistic_feature_selector_t;

/* ConnectionSpeedChange Data Structure */
typedef union {
	u32 DLBitRate;      /* Contains the downlink bit rate, in bits per second, as sent on the IN pipe */
	u32 ULBitRate;    /* Contains the uplink bit rate, in bits per second, as sent on the OUT pipe */
} usbh_cdc_ecm_speed_change_t;

/* States for CDC State Machine */
typedef enum {
	CDC_ECM_TRANSFER_STATE_IDLE = 0U,
	//bulk out
	CDC_ECM_TRANSFER_STATE_TX,
	CDC_ECM_TRANSFER_STATE_TX_BUSY,
	//bulk in
	CDC_ECM_TRANSFER_STATE_RX,
	CDC_ECM_TRANSFER_STATE_RX_BUSY,
	//int in
	CDC_ECM_INT_TRANSFER_STATE_RX,
	CDC_ECM_INT_TRANSFER_STATE_RX_BUSY,
} usbh_cdc_ecm_transfer_state_t;

/* CDC ECM state */
typedef enum {
	CDC_ECM_STATE_IDLE = 0U,
	CDC_ECM_STATE_ALT_SETTING,

#if ECM_ENABLE_PACKETFILTER
	CDC_ECM_STATE_SET_ETHERNET_MULTICAST_FILTER,
	CDC_ECM_STATE_SET_ETHERNET_PACKET_FILTER,
	CDC_ECM_STATE_GET_ETHERNET_STATISTIC,
#endif

#if ECM_ENABLE_RCR_CONFIGURATION
	CDC_ECM_STATE_RCR_GET, //RTL8156,
	CDC_ECM_STATE_RCR_SET,
#endif

//	CDC_ECM_STATE_SET_MAC, //RTL8152,

	CDC_ECM_STATE_SWITCH_TO_TRANSFER,
	CDC_ECM_STATE_TRANSFER,
	CDC_ECM_STATE_ERROR,
} usbh_cdc_ecm_state_t;

/* CDC ECM communication interface */
typedef struct {
	u8  intr_in_pipe;
	u8  intr_in_ep;
	u16 intr_in_packet_size;
	u32 intr_ep_interval;
} usbh_cdc_ecm_comm_if_t ;

/* CDC ECM data interface */
typedef struct {
	u8  bulk_in_pipe;
	u8  bulk_in_ep;
	u16 bulk_in_packet_size;
	u8  bulk_out_pipe;
	u8  bulk_out_ep;
	u16 bulk_out_packet_size;
} usbh_cdc_ecm_data_if_t ;

/* Ethernet Networking Functional Descriptor */
typedef struct {
	u8  bLength;
	u8  bDescriptorType;
	u8  bDescriptorSubtype;                                /* Ethernet Networking functional descriptor subtype */
	u8  iMACAddress;                                    /* Index of string descriptor */
	u32 bmEthernetStatistics;                            /* Indicates which Ethernet statistics functions the device collects. */
	u8  wMaxSegmentSize;                                /* The maximum segment size that the Ethernet device is capable of supporting */
	u16 wNumberMCFilters;                                /* Contains the number of multicast filters that can be configured by the host */
	u8  bNumberPowerFilters;                            /* Contains the number of pattern filters that are available for causing wake-up of the host. */
} usbh_cdc_ecm_network_func_t ;

/* CDC ECM user callback interface */
typedef struct {
	int(* init)(void);
	int(* deinit)(void);
	int(* attach)(void);
	int(* detach)(void);
	int(* setup)(void);
	int(* bulk_received)(u8 *buf, u32 len);
	int(* bulk_send)(usbh_urb_state_t state);
	int(* intr_received)(u8 *buf, u32 len);
} usbh_cdc_ecm_state_cb_t;

/* CDC ECM host */
typedef struct {
	usbh_cdc_ecm_comm_if_t              comm_if;
	usbh_cdc_ecm_data_if_t              data_if;
	usbh_cdc_ecm_network_func_t         func_if;

	u16                                 vid;
	u16                                 pid;

#if ECM_ENABLE_RCR_CONFIGURATION
	u8                                  *rcr; //8156
#endif

	u8                                  *mac_string;//msg get from device
	u8                                  mac[CDC_ECM_MAC_STR_LEN];

	u16                                 feature_selector;
	u32                                 eth_statistic_count;

	u16                                 packet_filter;
	u8                                  muticast_filter[CDC_ECM_MUTICAST_FILTER_STR_LEN];
	u16                                 muticast_filter_len;

	usbh_cdc_ecm_speed_change_t         user_speed_info;

	volatile u8                         next_transfor;

	u32                                 intr_in_busy_tick;
	u32                                 intr_in_idle_tick;
	u8                                  *intr_in_buf;
	u32                                 intr_in_state;

	u32                                 bulk_data_out_len;
	u8                                  bulk_out_zlp;
	u8                                  *bulk_data_out_buf;
	u32                                 bulk_out_idle_tick;
	u32                                 bulk_data_out_state;

	u8                                  *bulk_data_in_buf;
	volatile u32                        bulk_data_in_state;

	u32                                 state;

	usbh_ecm_state_t                    usbh_state;

	u8                                  host_pipe[CDC_ECM_HOST_PIPE_COUNT];
	usbh_cdc_ecm_state_cb_t             *cb;
	usb_host_t                          *host;
} usbh_cdc_ecm_host_t;

#pragma pack(pop)

/* Exported macros -----------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int usbh_cdc_ecm_init(usbh_cdc_ecm_state_cb_t *cb);
int usbh_cdc_ecm_deinit(void);

int usbh_cdc_ecm_choose_config(usb_host_t *host);
#if ECM_ENABLE_PACKETFILTER
int usbh_cdc_ecm_set_ethernet_multicast_filter(u8 *filter, u8 len);
int usbh_cdc_ecm_set_ethernet_packetfilter(u16 filter);
int usbh_cdc_ecm_get_ethernet_statistic(u16 selector);
#endif
#if ECM_ENABLE_RCR_CONFIGURATION
int usbh_cdc_ecm_get_rcr(void);
int usbh_cdc_ecm_set_rcr(void);
#endif
int usbh_cdc_ecm_set_ethernet_start_transfer(void);
int usbh_cdc_ecm_alt_setting(void);

int usbh_cdc_ecm_bulk_send(u8 *buf, u32 len);
int usbh_cdc_ecm_bulk_receive(void);
int usbh_cdc_ecm_intr_receive(u8 *buf, u32 len);

u16 usbh_cdc_ecm_get_usbin_mps(void);
u32 usbh_cdc_ecm_get_intr_interval(void);

#endif  /* USBD_CDC_ECM_H */
