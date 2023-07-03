#include <stdio.h>
#include <stdlib.h>


#define WIFI_SHELL_MODULE "wifi"

// #define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE)
#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_DONE |		\
								NET_EVENT_WIFI_RAW_SCAN_RESULT)

// decimal of FA 0B BC 0D
uint8_t identifier[] = {250, 11, 188, 13};

uint8_t wifi_scan_finished;

struct net_mgmt_event_callback wifi_shell_mgmt_cb;


int wifi_freq_to_channel(int frequency) {
	int channel = 0;

	if ((frequency <= 2424) && (frequency >= 2401)) {
		channel = 1;
	} else if ((frequency <= 2453) && (frequency >= 2425)) {
		channel = 6;
	} else if ((frequency <= 2484) && (frequency >= 2454)) {
		channel = 11;
	} else if ((frequency <= 5320) && (frequency >= 5180)) {
		channel = ((frequency - 5180) / 5) + 36;
	} else if ((frequency <= 5720) && (frequency >= 5500)) {
		channel = ((frequency - 5500) / 5) + 100;
	} else if ((frequency <= 5895) && (frequency >= 5745)) {
		channel = ((frequency - 5745) / 5) + 149;
	} else {
		channel = frequency;
	}

	return channel;
}


enum wifi_frequency_bands wifi_freq_to_band(int frequency) {
	enum wifi_frequency_bands band = WIFI_FREQ_BAND_2_4_GHZ;

	if ((frequency  >= 2401) && (frequency <= 2495)) {
		band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if ((frequency  >= 5170) && (frequency <= 5895)) {
		band = WIFI_FREQ_BAND_5_GHZ;
	} else {
		band = WIFI_FREQ_BAND_6_GHZ;
	}

	return band;
}


void handle_wifi_scan_done(struct net_mgmt_event_callback *cb) {
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Scan request failed (%d)", status->status);
	} else {
		wifi_scan_finished = 1;  // set global variable to indicate scanning is not longer in progress
		// LOG_INF("Scan request done");
	}
}


static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint32_t mgmt_event, struct net_if *iface) {
	switch (mgmt_event) {
	case NET_EVENT_WIFI_RAW_SCAN_RESULT:
		handle_wifi_raw_scan_result(cb);  // this func call is basically instantaneous
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);  // this func call is basically instantaneous
		break;
	default:
		break;
	}
}


static int wifi_scan(void) {
	wifi_scan_finished = 0;  // set global variable to indicate scanning currently in progresss
	
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		LOG_ERR("Scan request failed");
		return -ENOEXEC;
	}
	return 0;
}