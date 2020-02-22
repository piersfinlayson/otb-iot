/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2020 Piers Finlayson
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

#define OTB_WIFI_STA_MAX_SECOND_COUNT       30

#define OTB_WIFI_CONFIG_CMD_MIN       0
#define OTB_WIFI_CONFIG_CMD_SSID      0
#define OTB_WIFI_CONFIG_CMD_PASSWORD  1
#define OTB_WIFI_CONFIG_CMD_MAX       1

#define OTB_WIFI_CMD_GET   0x100
#define OTB_WIFI_CMD_SET   0x200

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
bool otb_wifi_config_sta_ip(void);
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
bool otb_wifi_use_dhcp(void);
extern void otb_wifi_timerfunc(void *arg);
void otb_wifi_station_connect(void);
void otb_wifi_ap_mode_done_fn(void);
void otb_wifi_ap_done_timerfunc(void *arg);
uint8 otb_wifi_set_station_config(char *ssid, char *password, bool commit);
bool otb_wifi_ap_quick_start(void);
extern bool otb_wifi_try_ap();
bool otb_wifi_ap_stop(void);
extern void otb_wifi_store_station_connect_error();
extern void otb_wifi_get_ip_string(uint32_t addr, char *addr_s);
extern bool otb_wifi_get_mac_addr(uint8_t if_id, char *mac);
bool otb_wifi_station_scan(otb_wifi_ap_otb_callback callback, void *arg);
void otb_wifi_ap_free_list(void);
void otb_wifi_station_scan_callback(void *arg, STATUS status);
bool otb_wifi_ap_enable(void);
bool otb_wifi_ap_disable(void);
void otb_wifi_ap_keep_alive(void);
int8 otb_wifi_get_rssi(void);
void otb_wifi_mqtt_do_rssi(char *msg);
bool otb_wifi_config_handler(unsigned char *, void *, unsigned char *);

extern bool otb_wifi_ap_running;

#ifdef OTB_WIFI_C

static volatile uint8_t otb_wifi_sta_second_count;
static volatile int8_t otb_wifi_ap_sta_con_count;
static volatile os_timer_t otb_wifi_timer;
static volatile os_timer_t otb_wifi_timeout_timer;
static volatile os_timer_t otb_wifi_timeout_timer;
static volatile bool otb_wifi_sta_connected;
bool otb_wifi_ap_mode_done;
static uint8_t otb_wifi_status;
struct otb_wifi_ap_list otb_wifi_ap_list_struct;
bool otb_wifi_ap_running;
bool otb_wifi_ap_enabled;
bool otb_wifi_dhcpc_started;
#endif
