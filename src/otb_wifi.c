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
 * This program is distributed in the hope that it will be useful, but WITfHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#define OTB_WIFI_C
#define OTB_DEBUG_DISABLE
#include "otb.h"

void ICACHE_FLASH_ATTR otb_wifi_init(void)
{
  uint8 rc;
  
  DEBUG("WIFI: otb_wifi_init entry");

  wifi_station_set_auto_connect(0);

  wifi_set_event_handler_cb(otb_wifi_event_handler);
  
  // otb_wifi_ap_list_struct.first = NULL;
  // otb_wifi_ap_list_struct.count = 0;
  // otb_wifi_ap_list_struct.searched = FALSE;
  // otb_wifi_ap_list_struct.callback = NULL;
  // otb_wifi_ap_list_struct.callback_arg = NULL;
  os_memset(&otb_wifi_ap_list_struct, 0, sizeof(otb_wifi_ap_list_struct));

  otb_wifi_ap_mode_done = FALSE;
  otb_wifi_ap_running = FALSE;
  otb_wifi_dhcpc_started = FALSE;
  
  otb_led_wifi_update(OTB_LED_NEO_COLOUR_OFF, TRUE);

  DEBUG("WIFI: otb_wifi_init exit");

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_event_handler(System_Event_t *event)
{
  DEBUG("WIFI: otb_wifi_event_handler entry");
  
  switch (event->event)
  {
    case EVENT_STAMODE_CONNECTED:
      // Not really connected til gets IP!
      DEBUG("WIFI: Event - station connected");
      otb_wifi_status = OTB_WIFI_STATUS_CONNECTING;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_ORANGE, TRUE);
      break;
      
    case EVENT_STAMODE_DISCONNECTED:
      if (otb_wifi_status == OTB_WIFI_STATUS_CONNECTED)
      {
        // Only log if interesting - when disconnected keeps doing this!
        INFO("WIFI: Event - station disconnected");
      }
      otb_wifi_status = OTB_WIFI_STATUS_CONNECTING;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_PURPLE, TRUE);
      break;
      
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      INFO("WIFI: Event - station authmode changed");
      otb_wifi_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
      break;
      
    case EVENT_STAMODE_GOT_IP:
      DEBUG("WIFI: Event - station got IP");
      otb_wifi_status = OTB_WIFI_STATUS_CONNECTED;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_YELLOW, TRUE);
      break;
      
    case EVENT_STAMODE_DHCP_TIMEOUT:
      ERROR("WIFI: DHCP timed out");
      otb_wifi_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
      break;
      
    case EVENT_SOFTAPMODE_STACONNECTED:
      INFO("WIFI: Event - AP mode - station connected");
      goto EXIT_LABEL;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_PINK, TRUE);
      break;
      
    case EVENT_SOFTAPMODE_STADISCONNECTED: 
      // Assume SDK will be retrying
      INFO("WIFI: Event - AP mode - station disconnected");
      goto EXIT_LABEL;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
      break;
      
    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      DEBUG("WIFI: Event - AP mode - probe request received");
      goto EXIT_LABEL;
      break;

    default:
      ERROR("WIFI: Unknown wifi event received: %02x", event->event);
      goto EXIT_LABEL;
      break;
  }
  
  if ((otb_wifi_status == OTB_WIFI_STATUS_CONNECTING) ||
       (otb_wifi_status == OTB_WIFI_STATUS_PERMANENTLY_FAILED))
  {
    DEBUG("WIFI: Disconnected - start AP");
    if (!otb_wifi_ap_running)
    {
      otb_wifi_ap_quick_start();
    }
    if (!otb_wifi_timeout_is_set)
    {
      otb_wifi_timeout_set_timer();
    } 
    if (!otb_util_timer_is_set((os_timer_t *)&otb_wifi_timer))
    {
      // Start up the wifi timer.  This fires every second until the station is connected.
      // This is different from the wifi timeout timer which only runs while station is
      // disconnected
      otb_util_timer_set((os_timer_t*)&otb_wifi_timer, 
                         (os_timer_func_t *)otb_wifi_timerfunc,
                         NULL,
                         1000,
                         1);
    }
  }
  
EXIT_LABEL:  
  
  // Don't need to worry about connected - timer will catch this
    
  DEBUG("WIFI: otb_wifi_event_handler exit");

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_timeout_cancel_timer(void)
{

  DEBUG("WIFI: otb_wifi_timeout_cancel_timer entry");

  otb_util_timer_cancel((os_timer_t *)&otb_wifi_timeout_timer);

  DEBUG("WIFI: otb_wifi_timeout_cancel_timer exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_timeout_is_set(void)
{
  bool rc;
  
  DEBUG("WIFI: otb_wifi_timeout_is_set entry");
  
  rc = otb_util_timer_is_set((os_timer_t*)&otb_wifi_timeout_timer);
  
  DEBUG("WIFI: otb_wifi_timeout_is_set exit");
  
  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_timeout_set_timer(void)
{

  DEBUG("WIFI: otb_wifi_set_timeout_timer entry");

  // No args (NULL arg) and don't repeat this (the 0 arg)  
  otb_wifi_timeout_cancel_timer();
  otb_util_timer_set((os_timer_t*)&otb_wifi_timeout_timer,
                     otb_wifi_timeout_timerfunc,
                     NULL,
                     OTB_WIFI_DEFAULT_DISCONNECTED_TIMEOUT,
                     0);
  
  DEBUG("WIFI: otb_wifi_set_timeout_timer exit");
  
  return;
}

char ALIGN4 otb_wifi_timeout_timerfunc_string[] = "WIFI: Station disconnected timeout";
void ICACHE_FLASH_ATTR otb_wifi_timeout_timerfunc(void *arg)
{

  DEBUG("WIFI: otb_wifi_timeout_timerfunc entry");
  
  if (otb_wifi_status != OTB_WIFI_STATUS_CONNECTED)
  {
    otb_reset(otb_wifi_timeout_timerfunc_string);
  }
  
  DEBUG("WIFI: otb_wifi_timeout_timerfunc exit");
  
  return;
}

// ssid, password = strings, timeout = milliseconds
char ALIGN4 otb_wifi_try_sta_error_string[] = "WIFI: Failed to start DHCP client";
// Returns one of WIFI_STATUS_CODES
uint8_t ICACHE_FLASH_ATTR otb_wifi_try_sta(char *ssid,
                                           char *password,
                                           bool bssid_set,
                                           char *bssid)
{
  OTB_WIFI_STATION_CONFIG wifi_conf;
  bool dhcpc_rc = FALSE;
  uint8_t rc = OTB_WIFI_STATUS_NOT_CONNECTED;
  bool mac_rc;
  char mac[OTB_WIFI_MAC_ADDRESS_STRING_LENGTH];

  DEBUG("WIFI: otb_wifi_try_sta entry");

  // First of all check we're disconnected!
  wifi_station_disconnect();
  
  ETS_UART_INTR_DISABLE();
  strcpy((char *)wifi_conf.ssid, ssid); 
  strcpy((char *)wifi_conf.password, password);
  
  wifi_conf.bssid_set = bssid_set;
  if (bssid_set)
  {
    DEBUG("WIFI: Set BSSID to %s", bssid);
    strcpy((char *)wifi_conf.bssid, bssid);
  }
  // Don't bother setting config persistently (wifi_station_set_config)
  wifi_station_set_config_current(&wifi_conf);

  // Get MAC addresses of both interfaces
  mac_rc = otb_wifi_get_mac_addr(STATION_IF, mac);
  if (mac_rc)
  {
    INFO("WIFI: Station MAC: %s", mac);
  }
  mac_rc = otb_wifi_get_mac_addr(SOFTAP_IF, mac);
  if (mac_rc)
  {
    INFO("WIFI:      AP MAC: %s", mac);
  }
  
  INFO("WIFI: Trying to connect ...", ssid);
  wifi_station_connect();
  ETS_UART_INTR_ENABLE();
  
  dhcpc_rc = wifi_station_dhcpc_start();
  if (dhcpc_rc)
  {
    otb_wifi_dhcpc_started = TRUE;    
  }
  else
  {
    rc = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
    goto EXIT;
  }
  
EXIT:  

  DEBUG("WIFI: otb_wifi_try_sta exit");
  return(rc);
}

char ALIGN4 otb_wifi_kick_off_error_string[] = "WIFI: Failed to start AP";
void ICACHE_FLASH_ATTR otb_wifi_kick_off(void)
{
  bool rc;
  bool rc1;
  bool rc2;
  DEBUG("WIFI: otb_wifi_kick_off entry");

  otb_led_wifi_update(OTB_LED_NEO_COLOUR_PURPLE, TRUE);
  
  rc = wifi_set_opmode_current(STATION_MODE);
  rc1 = otb_wifi_try_sta(otb_conf->ssid,
                         otb_conf->password,
                         FALSE,
                         NULL);
  rc2 = otb_wifi_try_ap();
  
  if (!rc2)
  {
    otb_reset(otb_wifi_kick_off_error_string);
  }
  
  // Start up the wifi timer.  This fires every second until the station is connected.
  // This is different from the wifi timeout timer which only runs while station is
  // disconnected
  otb_util_timer_set((os_timer_t*)&otb_wifi_timer, 
                     (os_timer_func_t *)otb_wifi_timerfunc,
                     NULL,
                     1000,
                     1);
                     
  DEBUG("WIFI: otb_wifi_kick_off exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_wifi_timerfunc(void *arg)
{
  struct ip_info ip_inf;
  bool ip_info_rc;
  uint8_t rc;
  char addr_s[OTB_WIFI_MAX_IPV4_STRING_LEN];
  bool dhcpc_rc;

  DEBUG("WIFI: otb_wifi_timerfunc entry");

  if (!otb_wifi_dhcpc_started)
  {
    dhcpc_rc = wifi_station_dhcpc_start();
    if (dhcpc_rc)
    {
      otb_wifi_dhcpc_started = TRUE;    
    }
  }

  if (otb_wifi_status == OTB_WIFI_STATUS_CONNECTED)
  {
    // If we've been told we're connected, we can move on
    ip_info_rc = wifi_get_ip_info(STATION_IF, &ip_inf);
    if (ip_info_rc)
    {
      INFO("WIFI: Station connected");
      // Should prob store these off somewhere handy
      otb_wifi_get_ip_string(ip_inf.ip.addr, addr_s);
      INFO("WIFI: Address: %s", addr_s);
      otb_wifi_get_ip_string(ip_inf.netmask.addr, addr_s);
      INFO("WIFI: Netmask: %s", addr_s);
      otb_wifi_get_ip_string(ip_inf.gw.addr, addr_s);
      INFO("WIFI: Gateway: %s", addr_s);

      // Now set up MQTT init to fire in half a second
      otb_util_timer_cancel((os_timer_t*)&otb_wifi_timer);
      otb_util_timer_set((os_timer_t*)&init_timer, 
                         (os_timer_func_t *)otb_init_mqtt,
                         NULL,
                         500,
                         0);
      
      // Disable wifi timer now connect (so this function doesn't get called again)
      otb_util_timer_cancel((os_timer_t*)&otb_wifi_timer);
      otb_wifi_timeout_cancel_timer();
      otb_wifi_ap_stop();
    }
    else
    {
      // Shucks, failed at the last hurdle - can't get IP.  Kick off wifi and reconnect
      // in attempt to fix.
      WARN("WIFI: failed to get IP address");
      otb_wifi_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      wifi_station_disconnect();
      wifi_station_connect();
      if (!otb_wifi_ap_running)
      {
        otb_wifi_ap_quick_start();
      }
      if (!otb_wifi_timeout_is_set())
      {
        otb_wifi_timeout_set_timer();
      }
    }    
  }

  DEBUG("WIFI: otb_wifi_timerfunc exit");

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_ap_mode_done_fn(void)
{

  DEBUG("WIFI: otb_wifi_ap_mode_done_fn entry");

  otb_wifi_ap_mode_done = TRUE;
  // We can repurpose the wifi_timeout for this purpose - as we no longer need it (when
  // this timer pops we're going to reboot)
  // Strictly we don't need to cancel here as will cancel again in set, but let's be 
  // explicit (as we're changing the function).
  otb_util_timer_cancel((os_timer_t*)&otb_wifi_timeout_timer);
  otb_util_timer_set((os_timer_t*)&otb_wifi_timeout_timer, 
                     (os_timer_func_t *)otb_wifi_ap_done_timerfunc,
                     NULL,
                     1000,
                     0);
  
  DEBUG("WIFI: otb_wifi_ap_mode_done_fn exit");
  
  return;
}
 
char ALIGN4 otb_wifi_ap_done_timerfunc_error_string[] = "WIFI: Station configuration changed";
void ICACHE_FLASH_ATTR otb_wifi_ap_done_timerfunc(void *arg)
{

  DEBUG("WIFI: otb_wifi_ap_done_timerfunc entry");

  OTB_ASSERT(otb_wifi_ap_mode_done);

  otb_wifi_status = OTB_WIFI_STATUS_AP_DONE;
  otb_reset(otb_wifi_ap_done_timerfunc_error_string);
  
  DEBUG("WIFI: otb_wifi_ap_done_timerfunc entry");
  
  return;
}

uint8 ICACHE_FLASH_ATTR otb_wifi_set_station_config(char *ssid,
                                                    char *password,
                                                    bool commit)
{
  uint8 rc;
  
  DEBUG("WIFI: otb_wifi_set_station_config entry");
  
  rc = otb_conf_store_sta_conf(ssid, password, commit);

  DEBUG("WIFI: otb_wifi_set_station_config exit");
  
  return(rc);
}

// Only use this one after first time
bool ICACHE_FLASH_ATTR otb_wifi_ap_quick_start(void)
{
  bool rc;

  DEBUG("WIFI: otb_wifi_ap_quick_start entry");

  rc = wifi_set_opmode_current(STATIONAP_MODE);

  otb_wifi_ap_running = TRUE;

  otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);

  INFO("WIFI: Started own AP");

  DEBUG("WIFI: otb_wifi_ap_quick_start exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_wifi_try_ap(void)
{
  struct softap_config ap_conf;
  struct softap_config ap_conf_current;
  bool rc;

  DEBUG("WIFI: otb_wifi_try_ap entry");
  
  INFO("WIFI: Configure AP with SSID: %s", OTB_MAIN_DEVICE_ID);
  wifi_softap_get_config(&ap_conf);
  os_memcpy(&ap_conf_current, &ap_conf, sizeof(ap_conf));
  DEBUG("WIFI: Stored SSID: %s", ap_conf.ssid);
  DEBUG("WIFI: Stored ssid_len: %d", ap_conf.ssid_len);
  DEBUG("WIFI: Stored SSID hidden: %d", ap_conf.ssid_hidden);
  DEBUG("WIFI: Stored password: %s", ap_conf.password);
  DEBUG("WIFI: Stored channel: %d", ap_conf.channel);
  DEBUG("WIFI: Stored authmode: %d", ap_conf.authmode);
  DEBUG("WIFI: Stored max_connection: %d", ap_conf.max_connection);
  DEBUG("WIFI: Stored beacon_interval: %d", ap_conf.beacon_interval);
  strcpy(ap_conf.ssid, OTB_MAIN_DEVICE_ID);
  ap_conf.ssid_len = strlen(OTB_MAIN_DEVICE_ID); 
  ap_conf.ssid_hidden = FALSE;
  ap_conf.password[0] = 0;
  ap_conf.channel = 1;
  ap_conf.authmode = AUTH_OPEN;
  ap_conf.max_connection = 4;
  ap_conf.beacon_interval = 100;
  ETS_UART_INTR_DISABLE();
  // We need to be in stations _and_ AP mode, as station mode is required to scan
  // Should test rc
  wifi_set_opmode_current(STATIONAP_MODE);
  if (os_memcmp(&ap_conf, &ap_conf_current, sizeof(ap_conf)))
  {
    // We can't store this until in SoftAP mode
    INFO("WIFI: Storing new AP config to flash");
    rc = wifi_softap_set_config(&ap_conf);
  }
  ETS_UART_INTR_ENABLE();
  otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
  INFO("WIFI: Started own AP");
  
  // Not testing return code - if this fails, user can try again when they connect
  // otb_wifi_station_scan(NULL, NULL);

  otb_httpd_start();
  
  otb_wifi_timeout_set_timer();
  otb_wifi_ap_running = TRUE;

  DEBUG("WIFI: otb_wifi_try_ap exit");

  return(rc);
}

bool ICACHE_FLASH_ATTR otb_wifi_ap_stop(void)
{
  bool rc;

  DEBUG("WIFI: otb_wifi_ap_stop entry");
  
  otb_wifi_ap_running = FALSE;

  otb_wifi_timeout_cancel_timer();
  
  if (!otb_wifi_ap_enabled)
  {
    rc = wifi_set_opmode_current(STATION_MODE);
    INFO("WIFI: Stopped own AP");
  }

  DEBUG("WIFI: otb_wifi_ap_stop exit");
  
  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_store_station_connect_error()
{
  DEBUG("WIFI: otb_wifi_store_station_connect_error entry");
  
  /// XXX
  
  DEBUG("WIFI: otb_wifi_store_station_connect_error exit");
}

void ICACHE_FLASH_ATTR otb_wifi_get_ip_string(uint32_t addr, char *addr_s)
{
  uint8_t byte[4];
  
  DEBUG("WIFI: otb_wifi_get_ip_string entry");
  
  // IP address A.B.C.D, A = byte 0, B = byte 1 ...
  byte[0] = addr & 0xff;
  byte[1] = (addr >> 8) & 0xff;
  byte[2] = (addr >> 16) & 0xff;
  byte[3] = (addr >> 24) & 0xff;

  os_sprintf(addr_s, "%d.%d.%d.%d", byte[0], byte[1], byte[2], byte[3]);
  
  DEBUG("WIFI: otb_wifi_get_ip_string exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_get_mac_addr(uint8_t if_id, char *mac)
{
  uint8_t mac_addr[6];
  bool rc = FALSE;
  
  DEBUG("WIFI: otb_wifi_get_mac_address entry");
  
  OTB_ASSERT((if_id == STATION_IF) || (if_id == SOFTAP_IF));
  
  rc = wifi_get_macaddr(if_id, mac_addr);
  os_snprintf(mac,
              OTB_WIFI_MAC_ADDRESS_STRING_LENGTH,
              "%x:%x:%x:%x:%x:%x",
              mac_addr[0],
              mac_addr[1],
              mac_addr[2],
              mac_addr[3],
              mac_addr[4],
              mac_addr[5]);

  DEBUG("WIFI: otb_wifi_get_mac_address exit");
  
  return (rc);
}

bool ICACHE_FLASH_ATTR otb_wifi_station_scan(otb_wifi_ap_otb_callback callback, void *arg)
{
  bool rc = TRUE;
  
  DEBUG("WIFI: otb_wifi_station_scan entry");

  INFO("WIFI: Start station scan");

  if (otb_wifi_ap_list_struct.searched == OTB_WIFI_AP_SEARCH_STARTED)
  {
    WARN("WIFI: Won't scan - scan already in progress");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  otb_wifi_ap_free_list();
  rc = wifi_station_scan(NULL, (scan_done_cb_t)otb_wifi_station_scan_callback);
  otb_wifi_ap_list_struct.callback = callback;
  otb_wifi_ap_list_struct.callback_arg = arg;
  otb_wifi_ap_list_struct.searched = OTB_WIFI_AP_SEARCH_STARTED;

EXIT_LABEL:

  DEBUG("WIFI: otb_wifi_station_scan exit");

  return rc;
}

void ICACHE_FLASH_ATTR otb_wifi_ap_free_list(void)
{
  struct otb_wifi_ap *ap;
  struct otb_wifi_ap *next_ap;
  DEBUG("WIFI: otb_wifi_ap_free_list entry");
  
  // Free of all AP information
  ap = otb_wifi_ap_list_struct.first;
  while (ap != NULL)
  {
    next_ap = ap->next;
    os_free(ap);
    ap = next_ap;
  }
  
  // Reset otb_wifi_ap_list_struct values
  // otb_wifi_ap_list_struct.first = NULL;
  // otb_wifi_ap_list_struct.ap_count = 0;
  // otb_wifi_ap_list_struct.searched = FALSE;
  // otb_wifi_ap_list_struct.callback = NULL;
  // otb_wifi_ap_list_struct.callback_arg = NULL;
  os_memset(&otb_wifi_ap_list_struct, 0, sizeof(otb_wifi_ap_list_struct));
  
  DEBUG("WIFI: otb_wifi_ap_free_list exit");

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_station_scan_callback(void *arg, STATUS status)
{
  struct bss_info *bss_link;
  struct otb_wifi_ap *ap;
  struct otb_wifi_ap *prev_ap;
  
  DEBUG("WIFI: otb_wifi_station_scan_ballback entry");

  INFO("WIFI: Station scan completed");

  otb_wifi_ap_list_struct.ap_count = 0;

  if (status != OK)
  {
    ERROR("WIFI: Station scan returned %d", status);
    goto EXIT_LABEL;
  }
  
  bss_link = (struct bss_info *)arg;
  ap = NULL;
  prev_ap = NULL;
  while (bss_link != NULL)
  {
    ap = (struct otb_wifi_ap *)os_zalloc(sizeof(otb_wifi_ap));
    if (ap)
    {
      os_strncpy(ap->ssid, bss_link->ssid, 32);
      ap->rssi = bss_link->rssi;
      ap->authmode = bss_link->authmode;
      ap->next = NULL;
      if (otb_wifi_ap_list_struct.ap_count > 0)
      {
        prev_ap->next = ap;
        prev_ap = ap;
        ap = NULL;
      }
      else
      {
        otb_wifi_ap_list_struct.first = ap;
        prev_ap = ap;
        ap = NULL;
      }
      otb_wifi_ap_list_struct.ap_count++;
    }
    else
    {
      WARN("WIFI: Couldn't alloc struct for AP %s", bss_link->ssid);
    }
    bss_link = bss_link->next.stqe_next;
    if (otb_wifi_ap_list_struct.ap_count >= 255)
    {
      WARN("WIFI: Too many APs found discarding %d",
           (otb_wifi_ap_list_struct.ap_count - 255));
      break;
    }
  }
  otb_wifi_ap_list_struct.searched = OTB_WIFI_AP_SEARCH_DONE;
  
  if (otb_wifi_ap_list_struct.callback != NULL)
  {
    otb_wifi_ap_list_struct.callback(otb_wifi_ap_list_struct.callback_arg);
  }

EXIT_LABEL:
  
  DEBUG("WIFI: otb_wifi_station_scan_ballback exit");

  return;
}

// Used to enable AP while station is connected
bool ICACHE_FLASH_ATTR otb_wifi_ap_enable(void)
{
  bool rc;
  
  DEBUG("WIFI: otb_wifi_ap_enable entry");
  
  otb_wifi_ap_enabled = TRUE;
  
  rc = otb_conf_store_ap_enabled(TRUE);
  
  otb_wifi_ap_quick_start();
  
  DEBUG("WIFI: otb_wifi_ap_enable exit");
  
  return(rc);
}

// Used to disable AP while station is connected
bool ICACHE_FLASH_ATTR otb_wifi_ap_disable(void)
{
  bool rc;
  
  DEBUG("WIFI: otb_wifi_ap_disable entry");
  
  otb_wifi_ap_enabled = FALSE;
  
  rc = otb_conf_store_ap_enabled(FALSE);
  
  if (otb_wifi_status == OTB_WIFI_STATUS_CONNECTED)  
  {
    otb_wifi_ap_stop();
  }
  
  DEBUG("WIFI: otb_wifi_ap_disable exit");
  
  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_ap_keep_alive(void)
{

  DEBUG("WIFI: otb_wifi_ap_keep_alive entry");
  
  otb_wifi_ap_enabled = TRUE;  
  DEBUG("WIFI: otb_wifi_ap_keep_alive exit");
  
  return;
}

int8 ICACHE_FLASH_ATTR otb_wifi_get_rssi(void)
{
  int8 rc;

  DEBUG("WIFI: otb_wifi_get_rssi entry");
  
  // Strictly we shouldn't call until we're sure we're connected, but currently only do
  // this on receipt of MQTT message, which means we're connected!
  rc = wifi_station_get_rssi();
  
  DEBUG("WIFI: otb_wifi_get_rssi exit");

  return rc;
}

void ICACHE_FLASH_ATTR otb_wifi_mqtt_do_rssi(char *msg)
{
  int8 rssi;
  char strength[8];
  
  DEBUG("WIFI: otb_wifi_mqtt_do_rssi entry");

  rssi = otb_wifi_get_rssi();
  // 31 is error code according to the SDK
  if (rssi != 31)
  {
    os_snprintf(strength, 8, "%d", rssi);
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_RSSI, OTB_MQTT_STATUS_OK, strength, "");
  }
  else
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_RSSI,
                         OTB_MQTT_STATUS_ERROR,
                         "Internal error",
                         "");
  }

  DEBUG("WIFI: otb_wifi_mqtt_do_rssi exit");
  
  return;
}
