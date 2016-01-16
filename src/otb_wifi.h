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
#define OTB_WIFI_STATUS_NOT_CONNECTED 0
#define OTB_WIFI_STATUS_PERMANENTLY_FAILED 1
#define OTB_WIFI_STATUS_CONNECTED 2

typedef struct station_config OTB_WIFI_STATION_CONFIG;

// Functions
extern uint8_t otb_wifi_try_sta(char *ssid,
                            char *password,
                            bool bssid_set,
                            char *bssid,
                            uint32_t timeout);
extern uint8_t otb_wifi_wait_until_connected(uint32_t timeout);
extern void otb_wifi_try_ap(uint32_t timeout);
extern bool otb_wifi_get_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf);
extern void otb_wifi_store_station_connect_error();
extern void otb_wifi_get_ip_string(uint32_t addr, char *addr_s);
extern void otb_wifi_set_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf);
extern bool otb_wifi_get_mac_addr(uint8_t if_id, char *mac);


