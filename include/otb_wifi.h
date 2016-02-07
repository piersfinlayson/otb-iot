/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016 Piers Finlayson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
 
// WIFI_STATUS_CODES
#define OTB_WIFI_STATUS_NOT_CONNECTED       0
#define OTB_WIFI_STATUS_PERMANENTLY_FAILED  1
#define OTB_WIFI_STATUS_CONNECTED           2
#define OTB_WIFI_STATUS_CONNECTING          3
#define OTB_WIFI_STATUS_TIMED_OUT           4
#define OTB_WIFI_STATUS_TRYING_AP           5
#define OTB_WIFI_STATUS_AP_DONE             6
#define OTB_WIFI_STATUS_AP_TIMED_OUT        7

typedef struct station_config OTB_WIFI_STATION_CONFIG;

// Functions
void otb_wifi_init(void);
void otb_wifi_event_handler(System_Event_t *event);
extern uint8_t otb_wifi_try_sta(char *ssid,
                            char *password,
                            bool bssid_set,
                            char *bssid,
                            uint32_t timeout);
uint8_t otb_wifi_process_test(uint8_t rc);
extern void otb_wifi_timerfunc(void *arg);
uint8_t otb_wifi_test_ap(otb_util_timeout *timeout);
extern uint8_t otb_wifi_test_connected(otb_util_timeout *timeout);
extern void otb_wifi_try_ap(uint32_t timeout);
extern bool otb_wifi_get_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf);
extern void otb_wifi_store_station_connect_error();
extern void otb_wifi_get_ip_string(uint32_t addr, char *addr_s);
extern void otb_wifi_set_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf);
extern bool otb_wifi_get_mac_addr(uint8_t if_id, char *mac);

#ifdef OTB_WIFI_C
HttpdBuiltInUrl otb_wifi_ap_urls[] =
{
	{"*", cgiRedirectApClientToHostname, OTB_MAIN_DEVICE_ID},
	{"/", cgiRedirect, "/wifi"},
	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
	{"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},
  {NULL, NULL, NULL}
};

static volatile os_timer_t otb_wifi_timer;
static otb_util_timeout otb_wifi_timeout;
bool otb_wifi_ap_mode_done;
static uint8_t otb_wifi_status;
#endif
