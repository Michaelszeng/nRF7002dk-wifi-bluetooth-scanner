/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi and Bluetooth Scanner + RID Data Parser
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(scan, CONFIG_LOG_DEFAULT_LEVEL);


#include <nrfx_clock.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>

#include "net_private.h"

#include "enums.h"
#include "utils.h"
#include "wifi_scan.h"


void handle_wifi_raw_scan_result(struct net_mgmt_event_callback *cb)
{
	struct wifi_raw_scan_result *raw =
		(struct wifi_raw_scan_result *)cb->info;
	int channel;
	int band;
	int rssi;
	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

	rssi = raw->rssi;
	channel = wifi_freq_to_channel(raw->frequency);
	band = wifi_freq_to_band(raw->frequency);

	int odid_identifier_idx = contains(raw->data, sizeof(raw->data), identifier, sizeof(identifier));

	// if (rssi >= -14) {
	if (odid_identifier_idx != -1) {
		LOG_INF("%-4u (%-6s) | %-4d | %s |      %-4d        ",
			channel,
			wifi_band_txt(band),
			rssi,
			net_sprint_ll_addr_buf(raw->data + 10, WIFI_MAC_ADDR_LEN, mac_string_buf, sizeof(mac_string_buf)), raw->frame_length);
		
		if (PRINT_INFO) {
			k_sleep(K_SECONDS(0.01));
			log_hexdump(raw->data, sizeof(raw->data));
			k_sleep(K_SECONDS(0.01));
			printf("\n\n\n");
		}

		
		// flags to hold whether or not a certain message was received
		msg_flags_t msg_flags;
		msg_flags.basic_id_flag = 0;
		msg_flags.location_vector_flag = 0;
		msg_flags.authentication_flag = 0;
		msg_flags.self_id_flag = 0;
		msg_flags.system_flag = 0;
		msg_flags.operator_id_flag = 0;

		uint8_t num_msg_in_pack = raw->data[odid_identifier_idx+7];

		parse_hex(raw->data, sizeof(raw->data), odid_identifier_idx, num_msg_in_pack, &msg_flags);
		
	}
}



int main(void)
{
	LOG_INF("==================================PROGRAM STARTING==================================");

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock / MHZ(1));  // should be nrf7002dk_nrf5340_cpuapp with 128 MHz

	wifi_scan_finished = 1;

	while(1) {
		if (wifi_scan_finished == 1) {  // only perform a scan if not another scan is currently in progress
			wifi_scan();
		}
		k_sleep(K_SECONDS(0.01));
	}

	LOG_INF("EXITED LOOP");
	
	return 0;
}
