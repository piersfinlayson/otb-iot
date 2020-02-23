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
#include "otb.h"

MLOG("WIFI");

void ICACHE_FLASH_ATTR otb_wifi_init(void)
{
  uint8 rc;
  
  ENTRY;

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
  otb_wifi_ap_sta_con_count = 0;
  otb_wifi_sta_second_count = 0;
  otb_wifi_sta_connected = FALSE;
  
  otb_led_wifi_update(OTB_LED_NEO_COLOUR_OFF, TRUE);

  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_config_sta_ip(void)
{
  bool rc;
  struct ip_info ip, ipq;
  uint8_t *dns;
  int ii, dns_num;
  ip_addr_t dns_server;

  ENTRY;

  // Belt and braces - make sure DHCPC not running
  otb_wifi_dhcpc_started = FALSE;
  wifi_station_dhcpc_stop();

  // Set IP information
  IP4_ADDR(&ip.ip,
           otb_conf->ip.ipv4[0],
           otb_conf->ip.ipv4[1],
           otb_conf->ip.ipv4[2],
           otb_conf->ip.ipv4[3]);
  IP4_ADDR(&ip.netmask,
           otb_conf->ip.ipv4_subnet[0],
           otb_conf->ip.ipv4_subnet[1],
           otb_conf->ip.ipv4_subnet[2],
           otb_conf->ip.ipv4_subnet[3]);
  IP4_ADDR(&ip.gw,
           otb_conf->ip.gateway[0],
           otb_conf->ip.gateway[1],
           otb_conf->ip.gateway[2],
           otb_conf->ip.gateway[3]);
  MDETAIL("Set IP: 0x%08x Mask: 0x%08x Gw: 0x%08x", ip.ip.addr, ip.netmask.addr, ip.gw.addr);
  rc = wifi_set_ip_info(STATION_IF, &ip);
  if (!rc)
  {
    MDETAIL("Failed to set ip info");
    goto EXIT_LABEL;
  }

  // Set DNS servers
  for (ii = 0, dns_num = 0; ii < 2; ii++)
  {
    if (ii == 0)
    {
      dns = otb_conf->ip.dns1;
    }
    else
    {
      dns = otb_conf->ip.dns2;
    }
    
    if (!otb_util_ip_is_all_val(dns, 0))
    {
      IP4_ADDR(&dns_server,
               dns[0],
               dns[1],
               dns[2],
               dns[3]);
      MDETAIL("Set DNS: 0x%08x", dns_server.addr);
      espconn_dns_setserver(dns_num, &dns_server);
      dns_num++;
    }
  }

  rc = TRUE;

EXIT_LABEL:

  // Double check IP info provisioned
  if (rc)
  {
    rc = wifi_get_ip_info(STATION_IF, &ipq);
    if (rc)
    {
      if (os_memcmp(&ip, &ipq, sizeof(ip)))
      {
        MWARN("Provisioned IP address incorrect IP: 0x%08x Mask: 0x%08x Gw: 0x%08x", ipq.ip.addr, ipq.netmask.addr, ipq.gw.addr);
      }
    }
    else
    {
      MWARN("Couldn't verify provisioned IP address");
    }
    
  }

  if (!rc)
  {
    MWARN("Failed to set manual IP - reverting to DHCP");
    goto EXIT_LABEL;
    otb_wifi_dhcpc_started = FALSE;
    wifi_station_dhcpc_start();
  }

  EXIT;

  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_event_handler(System_Event_t *event)
{
  ENTRY;
  
  switch (event->event)
  {
    case EVENT_STAMODE_CONNECTED:
      // Not really connected til gets IP!
      MDEBUG("Event - station connected");
      otb_wifi_sta_connected = FALSE;
      otb_wifi_status = OTB_WIFI_STATUS_CONNECTING;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_ORANGE, TRUE);
      break;
      
    case EVENT_STAMODE_DISCONNECTED:
      if (otb_wifi_status == OTB_WIFI_STATUS_CONNECTED)
      {
        // Only log if interesting - when disconnected keeps doing this!
        MDETAIL("Event - station disconnected");
      }
      otb_wifi_sta_connected = FALSE;
      otb_wifi_status = OTB_WIFI_STATUS_CONNECTING;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_PURPLE, TRUE);
      break;
      
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      MDETAIL("Event - station authmode changed");
      otb_wifi_sta_connected = FALSE;
      otb_wifi_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
      break;
      
    case EVENT_STAMODE_GOT_IP:
      // We get this even if not running DHCP (and have set static IP)
      MDEBUG("Event - station got IP");
      otb_wifi_sta_connected = TRUE;
      otb_wifi_status = OTB_WIFI_STATUS_CONNECTED;
      INFO("OTB: WiFi connected");
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_YELLOW, TRUE);
      break;
      
    case EVENT_STAMODE_DHCP_TIMEOUT:
      MERROR("DHCP timed out");
      otb_wifi_sta_connected = FALSE;
      otb_wifi_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
      break;
      
    case EVENT_SOFTAPMODE_STACONNECTED:
      MDETAIL("Event - AP mode - station connected");
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_PINK, TRUE);
      if (otb_wifi_ap_enabled)
      {
        if (otb_wifi_ap_sta_con_count < 0)
        {
          MWARN("WiFi AP stations connected was negative");
        }
        otb_wifi_ap_sta_con_count = 0;
        otb_wifi_ap_sta_con_count++;
      };
      goto EXIT_LABEL;
      break;
      
    case EVENT_SOFTAPMODE_STADISCONNECTED: 
      // Assume SDK will be retrying
      MDETAIL("Event - AP mode - station disconnected");
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
      if (otb_wifi_ap_enabled)
      {
        // This seems to happen if station is connected when otb-iot reboots
        // When it comes back we get a disconnection triggered
        otb_wifi_ap_sta_con_count--;
        if (otb_wifi_ap_sta_con_count < 0)
        {
          MDETAIL("WiFi AP stations connected went negative");
        }
        otb_wifi_ap_sta_con_count = 0;
      };
      goto EXIT_LABEL;
      break;
      
    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      MDEBUG("Event - AP mode - probe request received");
      goto EXIT_LABEL;
      break;

#if ESP_SDK_VERSION >= 020100
// Introduced in SDK 2.1.0
    case EVENT_OPMODE_CHANGED:
      // Could check this was expected but not bothering for now
      MDEBUG("Event - opmode changed");
      goto EXIT_LABEL;
      break;
#endif // ESP_SDK_VERSION >= 020100

#if ESP_SDK_VERSION >= 030000
// Introduced in SDK 3.0.0
    case EVENT_SOFTAPMODE_DISTRIBUTE_STA_IP:
      // Could check this was expected but not bothering for now
      MDEBUG("Event - soft AP mode distribute station IP");
      goto EXIT_LABEL;
      break;
#endif // ESP_SDK_VERSION >= 030000

    default:
      MERROR("Unknown wifi event received: %02x", event->event);
      goto EXIT_LABEL;
      break;
  }

  if (!otb_wifi_sta_connected && !otb_wifi_timeout_is_set())
  {
    otb_wifi_timeout_set_timer();
  } 

  if ((otb_wifi_status == OTB_WIFI_STATUS_CONNECTING) ||
       (otb_wifi_status == OTB_WIFI_STATUS_PERMANENTLY_FAILED))
  {
    MDEBUG("Disconnected - start AP");
    if (!otb_wifi_ap_running)
    {
      otb_wifi_ap_quick_start();
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
    
  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_timeout_cancel_timer(void)
{

  ENTRY;

  otb_util_timer_cancel((os_timer_t *)&otb_wifi_timeout_timer);

  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_timeout_is_set(void)
{
  bool rc;
  
  ENTRY;
  
  rc = otb_util_timer_is_set((os_timer_t*)&otb_wifi_timeout_timer);
  
  EXIT;
  
  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_timeout_set_timer(void)
{

  ENTRY;

  // No args (NULL arg) and don't repeat this (the 0 arg)  
  otb_wifi_timeout_cancel_timer();
  otb_util_timer_set((os_timer_t*)&otb_wifi_timeout_timer,
                     otb_wifi_timeout_timerfunc,
                     NULL,
                     OTB_WIFI_DEFAULT_DISCONNECTED_TIMEOUT,
                     0);
  
  EXIT;
  
  return;
}

char ALIGN4 otb_wifi_timeout_timerfunc_string[] = "WIFI: Station disconnected timeout";
void ICACHE_FLASH_ATTR otb_wifi_timeout_timerfunc(void *arg)
{

  ENTRY;
  
  if (otb_wifi_status != OTB_WIFI_STATUS_CONNECTED)
  {
    otb_reset(otb_wifi_timeout_timerfunc_string);
  }
  
  EXIT;
  
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
  bool hostname_rc;
  char mac[OTB_WIFI_MAC_ADDRESS_STRING_LENGTH];

  ENTRY;

  // First of all check we're disconnected!
  wifi_station_disconnect();
  
  ETS_UART_INTR_DISABLE();
  strcpy((char *)wifi_conf.ssid, ssid); 
  strcpy((char *)wifi_conf.password, password);
  
  wifi_conf.bssid_set = bssid_set;
  if (bssid_set)
  {
    MDEBUG("Set BSSID to %s", bssid);
    strcpy((char *)wifi_conf.bssid, bssid);
  }
  // Don't bother setting config persistently (wifi_station_set_config)
  wifi_station_set_config_current(&wifi_conf);

  // Get MAC addresses of both interfaces
  mac_rc = otb_wifi_get_mac_addr(STATION_IF, mac);
  if (mac_rc)
  {
    MDETAIL("Station MAC: %s", mac);
  }
  mac_rc = otb_wifi_get_mac_addr(SOFTAP_IF, mac);
  if (mac_rc)
  {
    MDETAIL("     AP MAC: %s", mac);
  }
  
  hostname_rc = wifi_station_set_hostname(OTB_MAIN_DEVICE_ID);
  if (hostname_rc)
  {
    MDETAIL("Set wifi station hostname to: %s", OTB_MAIN_DEVICE_ID);
  }
  else
  {
    MWARN("Failed to set station hostname to: %s", OTB_MAIN_DEVICE_ID);
  }
  ETS_UART_INTR_ENABLE();

  MDETAIL("Trying to connect ...", ssid);
  if (otb_wifi_use_dhcp())
  {
    dhcpc_rc = wifi_station_dhcpc_start();
    if (dhcpc_rc)
    {
      otb_wifi_dhcpc_started = TRUE;    
    }
    else
    {
      rc = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      goto EXIT_LABEL;
    }
  }
  else
  {
    // Configure IP information
    // If config_sta_ip failed it starts DHCPC so should get IP
    // automatically - so do nothing
    otb_wifi_config_sta_ip();
  }
  
  otb_wifi_station_connect();

EXIT_LABEL:  

  EXIT;
  return(rc);
}

char ALIGN4 otb_wifi_kick_off_error_string[] = "WIFI: Failed to start AP";
void ICACHE_FLASH_ATTR otb_wifi_kick_off(void)
{
  bool rc;
  bool rc1;
  bool rc2;
  ENTRY;

  otb_led_wifi_update(OTB_LED_NEO_COLOUR_PURPLE, TRUE);
  
  rc = wifi_set_opmode_current(STATION_MODE);
  if (!rc)
  {
    MERROR("Failed to put device into station mode");
  }
  INFO("OTB: WiFi enable")
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
                     
  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_use_dhcp(void)
{
  bool rc;

  ENTRY;

  rc = (otb_conf->ip.manual == OTB_IP_DHCP_DHCP);

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_wifi_timerfunc(void *arg)
{
  struct ip_info ip_inf;
  ip_addr_t dns;
  bool ip_info_rc;
  char addr_s[OTB_WIFI_MAX_IPV4_STRING_LEN];
  bool dhcpc_rc;

  ENTRY;

  otb_wifi_sta_second_count++;

  
  if (otb_wifi_use_dhcp() && !otb_wifi_dhcpc_started)
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
      MDETAIL("Station connected");
      // Should prob store these off somewhere handy
      otb_wifi_get_ip_string(ip_inf.ip.addr, addr_s);
      MDETAIL("Address: %s", addr_s);
      otb_wifi_get_ip_string(ip_inf.netmask.addr, addr_s);
      MDETAIL("Netmask: %s", addr_s);
      otb_wifi_get_ip_string(ip_inf.gw.addr, addr_s);
      MDETAIL("Gateway: %s", addr_s);
      dns = espconn_dns_getserver(0);
      otb_wifi_get_ip_string(dns.addr, addr_s);
      MDETAIL("DNS server 0: %s", addr_s);
      dns = espconn_dns_getserver(1);
      otb_wifi_get_ip_string(dns.addr, addr_s);
      MDETAIL("DNS server 1: %s", addr_s);

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
      otb_wifi_sta_connected = TRUE;
    }
    else
    {
      // Shucks, failed at the last hurdle - can't get IP.  Kick off wifi and reconnect
      // in attempt to fix.
      MWARN("failed to get IP address");
      otb_wifi_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      wifi_station_disconnect();
      otb_wifi_station_connect();
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
  
  if (!otb_wifi_sta_connected &&
      (otb_wifi_sta_second_count > OTB_WIFI_STA_MAX_SECOND_COUNT))
  {
    // We're giving up.  We won't try again until we reboot
    MWARN("Failed to connect in %ds - allowing AP to run", OTB_WIFI_STA_MAX_SECOND_COUNT);
    otb_util_timer_cancel((os_timer_t*)&otb_wifi_timer);
    wifi_station_disconnect();
  }

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_station_connect(void)
{
  ENTRY;
  
  otb_wifi_sta_connected = FALSE;
  otb_wifi_sta_second_count = 0;
  wifi_station_connect();
  
  EXIT;
}

void ICACHE_FLASH_ATTR otb_wifi_ap_mode_done_fn(void)
{

  ENTRY;

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
  
  EXIT;
  
  return;
}
 
char ALIGN4 otb_wifi_ap_done_timerfunc_error_string[] = "WIFI: Station configuration changed";
void ICACHE_FLASH_ATTR otb_wifi_ap_done_timerfunc(void *arg)
{

  ENTRY;

  OTB_ASSERT(otb_wifi_ap_mode_done);

  otb_wifi_status = OTB_WIFI_STATUS_AP_DONE;
  otb_reset(otb_wifi_ap_done_timerfunc_error_string);
  
  ENTRY;
  
  return;
}

uint8 ICACHE_FLASH_ATTR otb_wifi_set_station_config(char *ssid,
                                                    char *password,
                                                    bool commit)
{
  uint8 rc;
  
  ENTRY;
  
  rc = otb_conf_store_sta_conf(ssid, password, commit);

  EXIT;
  
  return(rc);
}

// Only use this one after first time
bool ICACHE_FLASH_ATTR otb_wifi_ap_quick_start(void)
{
  bool rc;

  ENTRY;

  rc = wifi_set_opmode_current(STATIONAP_MODE);

  otb_wifi_ap_running = TRUE;

  otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);

  MDETAIL("Started own AP");

  EXIT;
  
  return(rc);
}

char ALIGN4 otb_wifi_httpd_start_failed_str[] = "WIFI: Failed to initialize HTTP stack";

bool ICACHE_FLASH_ATTR otb_wifi_try_ap(void)
{
  struct softap_config ap_conf;
  struct softap_config ap_conf_current;
  bool rc;

  ENTRY;
  
  MDETAIL("Configure AP with SSID: %s", OTB_MAIN_DEVICE_ID);
  wifi_softap_get_config(&ap_conf);
  os_memcpy(&ap_conf_current, &ap_conf, sizeof(ap_conf));
  MDEBUG("Stored SSID: %s", ap_conf.ssid);
  MDEBUG("Stored ssid_len: %d", ap_conf.ssid_len);
  MDEBUG("Stored SSID hidden: %d", ap_conf.ssid_hidden);
  MDEBUG("Stored password: %s", ap_conf.password);
  MDEBUG("Stored channel: %d", ap_conf.channel);
  MDEBUG("Stored authmode: %d", ap_conf.authmode);
  MDEBUG("Stored max_connection: %d", ap_conf.max_connection);
  MDEBUG("Stored beacon_interval: %d", ap_conf.beacon_interval);
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
    MDETAIL("Storing new AP config to flash");
    rc = wifi_softap_set_config(&ap_conf);
  }
  ETS_UART_INTR_ENABLE();
  otb_led_wifi_update(OTB_LED_NEO_COLOUR_RED, TRUE);
  MDETAIL("Started own AP");
  
  // Not testing return code - if this fails, user can try again when they connect
  // otb_wifi_station_scan(NULL, NULL);

  if (!otb_httpd_start(TRUE))
  {
    otb_reset(otb_wifi_httpd_start_failed_str);
  }
  
  otb_wifi_timeout_set_timer();
  otb_wifi_ap_running = TRUE;

  EXIT;

  return(rc);
}

bool ICACHE_FLASH_ATTR otb_wifi_ap_stop(void)
{
  bool rc;

  ENTRY;
  
  otb_wifi_ap_running = FALSE;

  otb_wifi_timeout_cancel_timer();
  
  if (!otb_wifi_ap_enabled)
  {
    rc = wifi_set_opmode_current(STATION_MODE);
    otb_wifi_ap_sta_con_count = 0;
    otb_httpd_stop();
    MDETAIL("Stopped own AP");
  }

  EXIT;
  
  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_get_ip_string(uint32_t addr, char *addr_s)
{
  uint8_t byte[4];
  
  ENTRY;
  
  // IP address A.B.C.D, A = byte 0, B = byte 1 ...
  byte[0] = addr & 0xff;
  byte[1] = (addr >> 8) & 0xff;
  byte[2] = (addr >> 16) & 0xff;
  byte[3] = (addr >> 24) & 0xff;

  os_sprintf(addr_s, "%d.%d.%d.%d", byte[0], byte[1], byte[2], byte[3]);
  
  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_get_mac_addr(uint8_t if_id, char *mac)
{
  uint8_t mac_addr[6];
  bool rc = FALSE;
  
  ENTRY;
  
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

  EXIT;
  
  return (rc);
}

bool ICACHE_FLASH_ATTR otb_wifi_station_scan(otb_wifi_ap_otb_callback callback, void *arg)
{
  bool rc = TRUE;
  
  ENTRY;

  MDETAIL("Start station scan");

  if (otb_wifi_ap_list_struct.searched == OTB_WIFI_AP_SEARCH_STARTED)
  {
    MWARN("Won't scan - scan already in progress");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  otb_wifi_ap_free_list();
  rc = wifi_station_scan(NULL, (scan_done_cb_t)otb_wifi_station_scan_callback);
  otb_wifi_ap_list_struct.callback = callback;
  otb_wifi_ap_list_struct.callback_arg = arg;
  otb_wifi_ap_list_struct.searched = OTB_WIFI_AP_SEARCH_STARTED;

EXIT_LABEL:

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_wifi_ap_free_list(void)
{
  struct otb_wifi_ap *ap;
  struct otb_wifi_ap *next_ap;
  ENTRY;
  
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
  
  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_station_scan_callback(void *arg, STATUS status)
{
  struct bss_info *bss_link;
  struct otb_wifi_ap *ap;
  struct otb_wifi_ap *prev_ap;
  
  ENTRY;

  MDETAIL("Station scan completed");

  otb_wifi_ap_list_struct.ap_count = 0;

  if (status != OK)
  {
    MERROR("Station scan returned %d", status);
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
      MWARN("Couldn't alloc struct for AP %s", bss_link->ssid);
    }
    bss_link = bss_link->next.stqe_next;
    if (otb_wifi_ap_list_struct.ap_count >= 255)
    {
      MWARN("Too many APs found discarding %d",
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
  
  EXIT;

  return;
}

// Used to enable AP while station is connected
bool ICACHE_FLASH_ATTR otb_wifi_ap_enable(void)
{
  bool rc;
  
  ENTRY;
  
  otb_wifi_ap_enabled = TRUE;
  
  rc = otb_conf_store_ap_enabled(TRUE);
  
  otb_wifi_ap_quick_start();
  
  EXIT;
  
  return(rc);
}

// Used to disable AP while station is connected
bool ICACHE_FLASH_ATTR otb_wifi_ap_disable(void)
{
  bool rc;
  
  ENTRY;
  
  otb_wifi_ap_enabled = FALSE;
  
  rc = otb_conf_store_ap_enabled(FALSE);
  
  if (otb_wifi_status == OTB_WIFI_STATUS_CONNECTED)  
  {
    otb_wifi_ap_stop();
  }
  
  EXIT;
  
  return(rc);
}

void ICACHE_FLASH_ATTR otb_wifi_ap_keep_alive(void)
{

  ENTRY;
  
  otb_wifi_ap_enabled = TRUE;  
  EXIT;
  
  return;
}

int8 ICACHE_FLASH_ATTR otb_wifi_get_rssi(void)
{
  int8 rc;

  ENTRY;
  
  // Strictly we shouldn't call until we're sure we're connected, but currently only do
  // this on receipt of MQTT message, which means we're connected!
  rc = wifi_station_get_rssi();
  
  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_wifi_mqtt_do_rssi(char *msg)
{
  int8 rssi;
  char strength[8];
  
  ENTRY;

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

  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_wifi_config_handler(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32_t cmd = (uint32_t)arg;
  uint8 wifi_rc;
  char *ssid;
  char *password;
  
  ENTRY;

  OTB_ASSERT(((cmd & 0xff) >= OTB_WIFI_CONFIG_CMD_MIN) &&
             ((cmd & 0xff) <= OTB_WIFI_CONFIG_CMD_MAX));
  OTB_ASSERT((cmd & OTB_WIFI_CMD_GET) || (cmd & OTB_WIFI_CMD_SET));

  if (cmd & OTB_WIFI_CMD_GET)
  {
    switch (cmd & 0xff)
    {
      case OTB_WIFI_CONFIG_CMD_SSID:
        otb_cmd_rsp_append("%s", otb_conf->ssid);
        rc = TRUE;
        break;

      case OTB_WIFI_CONFIG_CMD_PASSWORD:
        otb_cmd_rsp_append("%s", otb_conf->password);
        rc = TRUE;
        break;

      default:
        otb_cmd_rsp_append("internal error");
        rc = FALSE;
        goto EXIT_LABEL;
        break;
    }
  }
  else
  {
    if ((next_cmd == NULL) || (next_cmd[0] == 0))
    {
        otb_cmd_rsp_append("no value provided");
        rc = FALSE;
        goto EXIT_LABEL;
    }
    switch (cmd & 0xff)
    {
      case OTB_WIFI_CONFIG_CMD_SSID:
      case OTB_WIFI_CONFIG_CMD_PASSWORD:
        ssid = otb_conf->ssid;
        password = otb_conf->password;
        if ((cmd & 0xff) == OTB_WIFI_CONFIG_CMD_SSID)
        {
          ssid = next_cmd;
        }
        else
        {
          password = next_cmd;
        }
        MDEBUG("Change ssid from %s to %s password from %s to %s", otb_conf->ssid, ssid, otb_conf->password, password);
        wifi_rc = otb_wifi_set_station_config(ssid, password, TRUE);
        if (wifi_rc == OTB_CONF_RC_NOT_CHANGED)
        {
          otb_cmd_rsp_append("unchanged");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        else if (wifi_rc == OTB_CONF_RC_ERROR)
        {
          rc = FALSE;
          goto EXIT_LABEL;
        }
        rc = TRUE;
        break;

      default:
        otb_cmd_rsp_append("internal error");
        rc = FALSE;
        goto EXIT_LABEL;
        break;
    }
  }
    
  rc = TRUE;

EXIT_LABEL:
  
  EXIT;
  
  return rc;

}
