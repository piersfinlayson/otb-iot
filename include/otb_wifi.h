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

typedef void (otb_wifi_ap_otb_callback)(void *);

struct otb_wifi_ap
{
  struct otb_wifi_ap *next;

  char ssid[32];
  
  int8 rssi;
  
  uint8 authmode;
  
} otb_wifi_ap;

struct otb_wifi_ap_list
{
  struct otb_wifi_ap *first;

  uint8 ap_count;
  
#define OTB_WIFI_AP_SEARCH_NOT_STARTED  0
#define OTB_WIFI_AP_SEARCH_STARTED      1
#define OTB_WIFI_AP_SEARCH_DONE         2
  uint8 searched;

  otb_wifi_ap_otb_callback *callback;

  void *callback_arg;

} otb_wifi_ap_list;

extern struct otb_wifi_ap_list otb_wifi_ap_list_struct;

// Functions
void otb_wifi_init(void);
void otb_wifi_event_handler(System_Event_t *event);
void otb_wifi_timeout_cancel_timer(void);
bool otb_wifi_timeout_is_set(void);
void otb_wifi_timeout_set_timer(void);
void otb_wifi_timeout_timerfunc(void *arg);
extern uint8_t otb_wifi_try_sta(char *ssid,
                            char *password,
                            bool bssid_set,
                            char *bssid);
void otb_wifi_kick_off(void);
uint8_t otb_wifi_process_test(uint8_t rc);
extern void otb_wifi_timerfunc(void *arg);
void otb_wifi_ap_mode_done_fn(void);
void otb_wifi_ap_done_timerfunc(void *arg);
bool otb_wifi_set_station_config(char *ssid, char *password);
extern bool otb_wifi_try_ap();
extern bool otb_wifi_get_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf);
extern void otb_wifi_store_station_connect_error();
extern void otb_wifi_get_ip_string(uint32_t addr, char *addr_s);
extern bool otb_wifi_set_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf);
extern bool otb_wifi_get_mac_addr(uint8_t if_id, char *mac);
bool otb_wifi_station_scan(otb_wifi_ap_otb_callback callback, void *arg);
void otb_wifi_ap_free_list(void);
void otb_wifi_station_scan_callback(void *arg, STATUS status);

#ifdef OTB_WIFI_C

static volatile os_timer_t otb_wifi_timer;
static volatile os_timer_t otb_wifi_timeout_timer;
bool otb_wifi_ap_mode_done;
static uint8_t otb_wifi_status;
struct otb_wifi_ap_list otb_wifi_ap_list_struct;
#endif
