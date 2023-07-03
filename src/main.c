/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi scan sample
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


static void handle_wifi_raw_scan_result(struct net_mgmt_event_callback *cb)
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
		int basic_id_flag = 0;
		int location_vector_flag = 0;
		int authentication_flag = 0;
		int self_id_flag = 0;
		int system_flag = 0;
		int operator_id_flag = 0;

		int num_msg_in_pack = raw->data[odid_identifier_idx+7];
		for (int msg_num=0; msg_num<num_msg_in_pack; msg_num++) {
			int msg_type = (raw->data[odid_identifier_idx + 8 + msg_num*25] - 2) / 16;  // either 0 (Basic ID), 1 (Location/Vector), 2 (Authentication), 3 (Self-ID), 4 (System), or 5 (Operator ID)
			
			switch(msg_type) {  // Decode the raw data into useful RID information
				case 0:  // Basic ID Message
					enum UA_TYPE ua_type = raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] % 16;
					enum ID_TYPE id_type = raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] / 16;  // floor division
					if (PRINT_INFO) {
						printf("ID TYPE: %s.  ", ID_TYPE_STRING[id_type]);
						printf("UA TYPE: %s.  ", UA_TYPE_STRING[ua_type]);
					}
					
					switch(id_type) {
						// Using curly bracs around each case to define each case as it's own frame to prevent redeclaration errors for id_buf
						case 0:{  // None --> null ID
							char id_buf[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '\0'};
							if (PRINT_INFO) {
								printf("NULL UAV ID: %s.\n\n", id_buf);
							}
							break;
						}
						case 1:  // Serial Number
						case 2:{  // CAA Registration
							// Loop through the Serial number in the raw decimal data and build a new string containing the serial number in ASCII
							char id_buf[21];  // initalize char array to hold the serial number (which is at most 20 ASCII chars)
							for (int i=0; i<20; i++) {
								int decimal_val = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2+i];
								id_buf[i] = ASCII_DICTIONARY[decimal_val];
							}
							id_buf[20] = '\0';
							if (PRINT_INFO) {
								printf("SERIAL NUMBER/CAA REGISTRATION NUMBER: %s.\n\n", id_buf);
							}
							break;
						}
						case 3:{  // UTM UUID (should be encoded as a 128-bit UUID (32-char hex string))
							char id_buf[33];
							for (int i=0; i<16; i++) {
								int decimal_val = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2+i];
								id_buf[2*i] = HEX_DICTIONARY[decimal_val][0];  // 1st hex char in an array of 2 hex chars
								id_buf[2*i+1] = HEX_DICTIONARY[decimal_val][1];  // 2nd hex char in an array of 2 hex chars
							}
							id_buf[32] = '\0';
							if (PRINT_INFO) {
								printf("UTM UUID: %s.\n\n", id_buf);
							}
							break;
						}							
						case 4:{  // Specific Session ID (1st byte is an in betwen 0 and 255, and 19 remaining bytes are alphanumeric code, according to this: https://www.rfc-editor.org/rfc/rfc9153.pdf)
							char id_buf[21];
							id_buf[0] = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2];
							// sprintf(id_buf[0], "%d", raw->data[odid_identifier_idx + 8 + msg_num*25 + 2]);
							// Loop through the Session ID in the raw decimal data and build a new string containing the serial number in ASCII
							for (int i=1; i<20; i++) {
								int decimal_val = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2+i];
								id_buf[i] = ASCII_DICTIONARY[decimal_val];
							}
							id_buf[20] = '\0';
							if (PRINT_INFO) {
								printf("SPECIFIC SESSION ID: %s.\n\n", id_buf);
							}
							break;
						}
					}
					basic_id_flag = 1;
					break;
				case 1:  // Location/Vector Message
					enum OPERATIONAL_STATUS op_status = raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] / 16;  // floor division
					// funky modulos & floor division to get the value of each bit
					enum HEIGHT_TYPE height_type_flag = (raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] % 8) / 4;  // 0: Above Takeoff. 1: AGL
					enum E_W_DIRECTION_SEGMENT direction_segment_flag = (raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] % 4) / 2;  // 0: <180, 1: >=180
					enum SPEED_MULTIPLIER speed_multiplier_flag = raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] % 2;;  // 0: x0.25, 1: x0.75

					uint8_t track_direction = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2];  // direction measured clockwise from true north. Should be between 0-179
					if (direction_segment_flag) {  // If E_W_DIRECTION_SEGMENT is true, add 180 to the track_direction, according to ASTM.
						track_direction += 180;
					}

					uint8_t speed = raw->data[odid_identifier_idx + 8 + msg_num*25 + 3];  // ground speed in m/s
					if (speed_multiplier_flag) {
						speed = 0.75*speed + 255*0.25;  // as defined in ASTM
					}
					else {
						speed *= 0.25;
					}

					uint8_t vertical_speed = raw->data[odid_identifier_idx + 8 + msg_num*25 + 4] * 0.5;  // vertical speed in m/s (positive = up, negatve = down); multiply by 0.5 as defined in ASTM

					// lat and lon Little Endian encoded
					int32_t lat_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 5];
					int32_t lat_lsb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 6] << 8;  // multiply by 2^8
					int32_t lat_msb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 7] << 16;  // multiply by 2^16
					int32_t lat_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 8] << 24;  // multiply by 2^24
					int32_t lat_int = lat_msb + lat_msb1 + lat_lsb1 + lat_lsb;
					float lat = (float) lat_int / 10000000.0;  // divide by 10^7, according to ASTM

					int32_t lon_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 9];
					int32_t lon_lsb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 10] << 8;  // multiply by 2^8
					int32_t lon_msb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 11] << 16;  // multiply by 2^16
					int32_t lon_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 12] << 24;  // multiply by 2^24
					int32_t lon_int = lon_msb + lon_msb1 + lon_lsb1 + lon_lsb;
					float lon = (float) lon_int / 10000000.0;  // divide by 10^7, according to ASTM

					// altitudes and height Little Endian encoded
					uint16_t pressure_altitude_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 13];
					uint16_t pressure_altitude_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 14] << 8;
					uint16_t pressure_altitude = (pressure_altitude_msb + pressure_altitude_lsb) * 0.5 - 1000;

					uint16_t geodetic_altitude_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 15];
					uint16_t geodetic_altitude_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 16] << 8;
					uint16_t geodetic_altitude = (geodetic_altitude_msb + geodetic_altitude_lsb) * 0.5 - 1000;

					uint16_t height_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 17];
					uint16_t height_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 18] << 8;
					uint16_t height = (height_msb + height_lsb) * 0.5 - 1000;

					enum VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY vertical_accuracy = raw->data[odid_identifier_idx + 8 + msg_num*25 + 19] / 16;  // floor division
					enum VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY horizontal_accuracy = raw->data[odid_identifier_idx + 8 + msg_num*25 + 19] % 16;
					
					enum VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY baro_alt_accuracy = raw->data[odid_identifier_idx + 8 + msg_num*25 + 20] / 16;  // floor division
					enum SPEED_ACCURACY speed_accuracy = raw->data[odid_identifier_idx + 8 + msg_num*25 + 20] % 16;

					// 1/10ths of seconds since the last hour relatve to UTC time
					uint16_t timestamp_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 21];
					uint16_t timestamp_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 21] << 8;
					uint16_t timestamp = timestamp_msb + timestamp_lsb;

					uint8_t timestamp_accuracy_int = (raw->data[odid_identifier_idx + 8 + msg_num*25 + 22] % 15);  // modulo 15 to get rid of bits 7-4. Between 0.1 s and 1.5 s (0 s = unknown)
					float timestamp_accuracy = timestamp_accuracy_int * 0.1;
					
					if (PRINT_INFO) {
						printf("OPERATIONAL STATUS: %s.  ", OPERATIONAL_STATUS_STRING[op_status]);
						printf("HEIGHT TYPE: %s.  ", HEIGHT_TYPE_STRING[height_type_flag]);
						printf("DIRECTION SEGMENT FLAG: %s.  ", E_W_DIRECTION_SEGMENT_STRING[direction_segment_flag]);
						printf("SPEED MULTIPLIER FLAG: %s.  ", SPEED_MULTIPLIER_STRING[speed_multiplier_flag]);
						printf("HEADING (deg): %d.  ", track_direction);
						printf("SPEED (m/s): %d.  ", speed);
						printf("VERTICAL SPEED (m/s): %d.  ", vertical_speed);
						printf("LAT: %d.  ", lat_int);
						printf("LON: %d.  ", lon_int);
						printf("PRESSURE ALT: %d.  ", pressure_altitude);
						printf("GEO ALT: %d.  ", geodetic_altitude);
						printf("HEIGHT: %d.  ", height);
						printf("HORIZONTAL ACCURACY: %s.  ", VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY_STRING[horizontal_accuracy]);
						printf("VERTICAL ACCURACY: %s.  ", VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY_STRING[vertical_accuracy]);
						printf("BARO ALT ACCURACY: %s.  ", VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY_STRING[baro_alt_accuracy]);
						printf("SPEED ACCURACY: %s.  ", SPEED_ACCURACY_STRING[speed_accuracy]);
						printf("TIMESTAMP: %d.  ", timestamp);
						printf("TIMESTAMP_ACCURACY (btwn 0.1-1.5s): %f\n\n", timestamp_accuracy);
					}

					location_vector_flag = 1;
					break;
				case 3:  // Self ID Message
					enum SELF_ID_TYPE self_id_type = raw->data[odid_identifier_idx + 8 + msg_num*25 + 1];
					// Loop through the self id description in the raw decimal data and build a new string containing the self id description in ASCII
					char self_id_description_buf[24];  // initalize char array to hold self id description
					for (int i=0; i<23; i++) {
						int decimal_val = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2 + i];
						self_id_description_buf[i] = ASCII_DICTIONARY[decimal_val];
					}
					self_id_description_buf[23] = '\0';
					
					if (PRINT_INFO) {
						printf("SELF ID TYPE: %s.  ", SELF_ID_TYPE_STRING[self_id_type]);
						printf("SELF ID: %s.\n\n", self_id_description_buf);
					}
					self_id_flag = 1;
					break;
				case 4:  // System Message
					enum OPERATOR_LOCATION_ALTITUDE_SOURCE_TYPE operator_location_type = raw->data[odid_identifier_idx + 8 + msg_num*25 + 1] % 3; // mod 3 so that we only consider bits 1 and 0
					
					// lat and lon Little Endian encoded
					int32_t operator_lat_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2];
					int32_t operator_lat_lsb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 3] << 8;  // multiply by 2^8
					int32_t operator_lat_msb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 4] << 16;  // multiply by 2^16
					int32_t operator_lat_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 5] << 24;  // multiply by 2^24
					int32_t operator_lat_int = operator_lat_msb + operator_lat_msb1 + operator_lat_lsb1 + operator_lat_lsb;
					float operator_lat = (float) operator_lat_int / 10000000.0;

					int32_t operator_lon_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 6];
					int32_t operator_lon_lsb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 7] << 8;  // multiply by 2^8
					int32_t operator_lon_msb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 8] << 16;  // multiply by 2^16
					int32_t operator_lon_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 9] << 24;  // multiply by 2^24
					int32_t operator_lon_int = operator_lon_msb + operator_lon_msb1 + operator_lon_lsb1 + operator_lon_lsb;
					float operator_lon = (float) operator_lon_int / 10000000.0;

					// number of aircraft in the area
					uint16_t area_count_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 10];
					uint16_t area_count_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 11] << 8;  // multiply by 2^8
					uint16_t area_count = area_count_msb + area_count_lsb;

					// radius of cylindrical area with the group of aircraft
					uint16_t area_radius = raw->data[odid_identifier_idx + 8 + msg_num*25 + 12] * 10;

					// floor and ceiling Little Endian encoded
					uint16_t area_ceiling_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 13];
					uint16_t area_ceiling_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 14] << 8;
					uint16_t area_ceiling = (area_ceiling_msb + area_ceiling_lsb) * 0.5 - 1000;

					uint16_t area_floor_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 15];
					uint16_t area_floor_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 16] << 8;
					uint16_t area_floor = (area_floor_msb + area_floor_lsb) * 0.5 - 1000;

					enum UA_CATEGORY ua_category = raw->data[odid_identifier_idx + 8 + msg_num*25 + 17] / 16;  // floor division
					enum UA_CLASS ua_classification = raw->data[odid_identifier_idx + 8 + msg_num*25 + 17] % 16;
					

					// operator altitude Little Endian encoded
					uint16_t operator_altitude_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 18];
					uint16_t operator_altitude_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 19] << 8;
					uint16_t operator_altitude = (operator_altitude_msb + operator_altitude_lsb) * 0.5 - 1000;

					// current time in seconds since 00:00:00 01/01/2019
					uint32_t system_timestamp_lsb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 20];
					uint32_t system_timestamp_lsb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 21] << 8;
					uint32_t system_timestamp_msb1 = raw->data[odid_identifier_idx + 8 + msg_num*25 + 22] << 16;
					uint32_t system_timestamp_msb = raw->data[odid_identifier_idx + 8 + msg_num*25 + 23] << 24;
					uint32_t system_timestamp = system_timestamp_msb + system_timestamp_msb1 + system_timestamp_lsb1 + system_timestamp_lsb;

					if (PRINT_INFO) {
						printf("OPERATOR LOCATION SOURCE TYPE: %s.  ", OPERATOR_LOCATION_ALTITUDE_SOURCE_TYPE_STRING[operator_location_type]);
						printf("OPERATOR LAT: %d.  ", operator_lat_int);
						printf("OPERATOR LON: %d.  ", operator_lon_int);
						printf("OPERATOR ALT: %d.  ", operator_altitude);
						printf("AREA COUNT: %d.  ", area_count);
						printf("AREA RADIUS: %d.  ", area_radius);
						printf("AREA CEILING: %d.  ", area_ceiling);
						printf("AREA FLOOR: %d.  ", area_floor);
						printf("UA CATEGORY: %s.  ", UA_CATEGORY_STRING[ua_category]);
						printf("UA CLASS: %s.  ", UA_CLASS_STRING[ua_classification]);
						printf("TIMESTAMP (secs from 00:00:00 01/01/2019): %d.\n\n", system_timestamp);
					}

					system_flag = 1;
					break;
				case 5:  // Operator ID Message
					// Loop through the operator id in the raw decimal data and build a new string containing the operator id in ASCII
					char operator_id_buf[21];  // initalize char array to hold the operator id string (which is at most 20 ASCII chars)
					for (int i=0; i<20; i++) {
						int decimal_val = raw->data[odid_identifier_idx + 8 + msg_num*25 + 2 + i];
						operator_id_buf[i] = ASCII_DICTIONARY[decimal_val];
					}
					operator_id_buf[20] = '\0';
					if (PRINT_INFO) {
						printf("OPERATOR ID (CAA-issued License): %s.\n\n", operator_id_buf);
					}
					operator_id_flag = 1;
					break;
			}
		}
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
