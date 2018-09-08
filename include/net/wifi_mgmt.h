/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WiFi L2 stack public header
 */

#ifndef __WIFI_MGMT_H__
#define __WIFI_MGMT_H__

#include <net/net_mgmt.h>
#include <net/wifi.h>

/* Management part definitions */

#define _NET_WIFI_LAYER	NET_MGMT_LAYER_L2
#define _NET_WIFI_CODE	0x156
#define _NET_WIFI_BASE	(NET_MGMT_IFACE_BIT |			\
			 NET_MGMT_LAYER(_NET_WIFI_LAYER) |	\
			 NET_MGMT_LAYER_CODE(_NET_WIFI_CODE))
#define _NET_WIFI_EVENT	(_NET_WIFI_BASE | NET_MGMT_EVENT_BIT)

enum net_request_wifi_cmd {
	NET_REQUEST_WIFI_CMD_OPEN = 1,
	NET_REQUEST_WIFI_CMD_CLOSE,
	NET_REQUEST_WIFI_CMD_SCAN,
	NET_REQUEST_WIFI_CMD_CONNECT,
	NET_REQUEST_WIFI_CMD_DISCONNECT,
	NET_REQUEST_WIFI_CMD_GET_STATION,
	NET_REQUEST_WIFI_CMD_START_AP,
	NET_REQUEST_WIFI_CMD_STOP_AP,
	NET_REQUEST_WIFI_CMD_DEL_STATION,
	NET_REQUEST_WIFI_CMD_NPI,
};

#define NET_REQUEST_WIFI_OPEN					\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_OPEN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_OPEN);

#define NET_REQUEST_WIFI_CLOSE					\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CLOSE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CLOSE);

#define NET_REQUEST_WIFI_SCAN					\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_SCAN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN);

#define NET_REQUEST_WIFI_CONNECT				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT);

#define NET_REQUEST_WIFI_DISCONNECT				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_DISCONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT);

#define NET_REQUEST_WIFI_GET_STATION				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_GET_STATION)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_GET_STATION);

#define NET_REQUEST_WIFI_START_AP				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_START_AP)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_START_AP);

#define NET_REQUEST_WIFI_STOP_AP				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_STOP_AP)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_STOP_AP);

#define NET_REQUEST_WIFI_DEL_STATION				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_DEL_STATION)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_DEL_STATION);

#define NET_REQUEST_WIFI_NPI				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_NPI)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_NPI);

enum net_event_wifi_cmd {
	NET_EVENT_WIFI_CMD_SCAN_RESULT = 1,
	NET_EVENT_WIFI_CMD_SCAN_DONE,
	NET_EVENT_WIFI_CMD_CONNECT_RESULT,
	NET_EVENT_WIFI_CMD_DISCONNECT_RESULT,
	NET_EVENT_WIFI_CMD_NEW_STATION,
};

#define NET_EVENT_WIFI_SCAN_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_RESULT)

#define NET_EVENT_WIFI_SCAN_DONE				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_DONE)

#define NET_EVENT_WIFI_CONNECT_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_CONNECT_RESULT)

#define NET_EVENT_WIFI_DISCONNECT_RESULT			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_DISCONNECT_RESULT)

#define NET_EVENT_WIFI_NEW_STATION			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_NEW_STATION)


/* Each result is provided to the net_mgmt_event_callback
 * via its info attribute (see net_mgmt.h)
 */
struct wifi_scan_result {
	u8_t bssid[NET_LINK_ADDR_MAX_LENGTH];
	u8_t ssid[WIFI_SSID_MAX_LEN];
	u8_t ssid_length;

	u8_t channel;
	enum wifi_security_type security;
	s8_t rssi;
};

struct wifi_connect_req_params {
	u8_t *ssid;
	u8_t ssid_length; /* Max 32 */

	u8_t *psk;
	u8_t psk_length; /* Min 8 - Max 64 */

	u8_t channel;
	enum wifi_security_type security;
};

struct wifi_start_ap_req_params {
	u8_t *ssid;
	u8_t ssid_length; /* Max 32 */

	u8_t *psk;
	u8_t psk_length; /* Min 8 - Max 64 */

	u8_t channel;
	enum wifi_security_type security;
};

struct wifi_npi_req_params {
	int ictx_id;

	u8_t *tx_buf;
	u32_t tx_len;
	u8_t *rx_buf;
	u32_t rx_len;
};

struct wifi_new_station_params {
	u8_t status;
	u8_t mac[NET_LINK_ADDR_MAX_LENGTH];
};

struct wifi_status {
	int status;
};

#ifdef CONFIG_WIFI_OFFLOAD

#include <net/net_if.h>

typedef void (*scan_result_cb_t)(struct net_if *iface, int status,
				 struct wifi_scan_result *entry);

struct net_wifi_mgmt_api{
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;
	enum ethernet_hw_caps (*get_capabilities)(struct device *dev);

	/* cb parameter is the cb that should be called for each
	 * result by the driver. The wifi mgmt part will take care of
	 * raising the necessary event etc...
	 */
	int (*open)(struct device *dev);
	int (*close)(struct device *dev);
	int (*scan)(struct device *dev, scan_result_cb_t cb);
	int (*connect)(struct device *dev,
		       struct wifi_connect_req_params *params);
	int (*disconnect)(struct device *dev);
	int (*get_station)(struct device *dev, u8_t *signal);
	int (*notify_ip)(struct device *dev, u8_t *ipaddr, u8_t len);
	int (*start_ap)(struct device *dev,
			struct wifi_start_ap_req_params *params);
	int (*stop_ap)(struct device *dev);
	int (*del_station)(struct device *dev, u8_t *mac);
	int (*npi_send)(struct device *dev,
			struct wifi_npi_req_params);
};

void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status);
void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status);
void wifi_mgmt_raise_new_station_event(struct net_if *iface,
		struct wifi_new_station_params *params);

#endif /* CONFIG_WIFI_OFFLOAD */

#endif /* __WIFI_MGMT_H__ */
