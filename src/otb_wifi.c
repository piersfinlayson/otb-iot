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
#include "otb.h"

void ICACHE_FLASH_ATTR otb_wifi_init(void)
{
  uint8 rc;
  
  DEBUG("WIFI: otb_wifi_init entry");

  wifi_station_set_auto_connect(0);

  wifi_set_event_handler_cb(otb_wifi_event_handler);

  DEBUG("WIFI: otb_wifi_init exit");

  return;
}

void ICACHE_FLASH_ATTR otb_wifi_event_handler(System_Event_t *event)
{
  DEBUG("WIFI: otb_wifi_event_handler entry");
  
  switch (event->event)
  {
    case EVENT_STAMODE_CONNECTED:
      INFO("WIFI: Event - station connected");
      break;
      
    case EVENT_STAMODE_DISCONNECTED:
      INFO("WIFI: Event - station disconnected");
      break;
      
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      INFO("WIFI: Event - station authmode changed");
      break;
      
    case EVENT_STAMODE_GOT_IP:
      // This is the interesting case!
      INFO("WIFI: Event - station got IP");
      break;
      
    case EVENT_STAMODE_DHCP_TIMEOUT:
      INFO("WIFI: Event - station connected");
      WARN("WIFI: DHCP timed out so rebooting ...");
      otb_reset();
    break;
      
    case EVENT_SOFTAPMODE_STACONNECTED:
      INFO("WIFI: Event - AP mode - station connected");
      break;
      
    case EVENT_SOFTAPMODE_STADISCONNECTED: 
      INFO("WIFI: Event - AP mode - station disconnected");
      break;
      
    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      INFO("WIFI: Event - AP mode - probe request received");
      break;

    default:
      ERROR("WIFI: Unknown wifi event received: %02x", event->event);
      break;
  }
    
  DEBUG("WIFI: otb_wifi_event_handler exit");

  return;
}

// ssid, password = strings, timeout = milliseconds
// Returns one of WIFI_STATUS_CODES
uint8_t ICACHE_FLASH_ATTR otb_wifi_try_sta(char *ssid,
                                           char *password,
                                           bool bssid_set,
                                           char *bssid,
                                           uint32_t timeout)
{
  OTB_WIFI_STATION_CONFIG wifi_conf;
  bool dhcpc_rc = FALSE;
  uint8_t rc = OTB_WIFI_STATUS_NOT_CONNECTED;
  bool mac_rc;
  char mac[OTB_WIFI_MAC_ADDRESS_STRING_LENGTH];

  DEBUG("WIFI: otb_wifi_try_sta entry");

  // First of all check we're disconnected!
  wifi_station_connect();
  
  // Put wifi into station mode.
  if(wifi_get_opmode() != STATION_MODE)
  {
    // No need to store this persistently (wifi_set_opmode instead)
    DEBUG("WIFI: Set STA mode");
    ETS_UART_INTR_DISABLE();
    wifi_set_opmode_current(STATION_MODE);
    ETS_UART_INTR_ENABLE();
  }

  // DON'T log password for security reasons!
  INFO("WIFI: Set SSID to: %s", ssid);
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
  if (!dhcpc_rc)
  {
    WARN("WIFI: failed to start DHCP client");
    rc = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
    goto EXIT;
  }
  
  otb_wifi_timeout.start_time = system_get_time();
  otb_wifi_timeout.end_time = otb_wifi_timeout.start_time + timeout * 1000;
  
  rc = otb_wifi_test_connected(&otb_wifi_timeout); 
  DEBUG("WIFI:test_connected result %d", rc);

  rc = otb_wifi_process_test(rc);
  DEBUG("WIFI:process_test result %d", rc);

EXIT:  

  DEBUG("WIFI: otb_wifi_try_sta exit");
  return(rc);
}

uint8_t ICACHE_FLASH_ATTR otb_wifi_process_test(uint8_t rc)
{
  struct ip_info ip_inf;
  bool ip_info_rc;
  char addr_s[OTB_WIFI_MAX_IPV4_STRING_LEN];

  DEBUG("WIFI: otb_wifi_process_test entry");

  if (rc == OTB_WIFI_STATUS_CONNECTED)
  {
    ip_info_rc = wifi_get_ip_info(STATION_IF, &ip_inf);
    if (ip_info_rc)
    {
      // Should prob store these off somewhere handy
      otb_wifi_get_ip_string(ip_inf.ip.addr, addr_s);
      INFO("WIFI: Address: %s", addr_s);
      otb_wifi_get_ip_string(ip_inf.netmask.addr, addr_s);
      INFO("WIFI: Netmask: %s", addr_s);
      otb_wifi_get_ip_string(ip_inf.gw.addr, addr_s);
      INFO("WIFI: Gateway: %s", addr_s);
      
      // Now set up MQTT init
      os_timer_disarm((os_timer_t*)&init_timer);
      os_timer_setfn((os_timer_t*)&init_timer, (os_timer_func_t *)otb_init_mqtt, NULL);
      os_timer_arm((os_timer_t*)&init_timer, 500, 0);  
    }
    else
    {
      // Shucks, failed at the last hurdle - can't get IP
      WARN("WIFI: failed to get IP address");
      rc = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
    }    
  }
  else if ((rc == OTB_WIFI_STATUS_CONNECTING) ||
           (rc == OTB_WIFI_STATUS_PERMANENTLY_FAILED))
  {
    // Reschedule this in another second
    os_timer_disarm((os_timer_t *)&otb_wifi_timer);
    os_timer_setfn((os_timer_t *)&otb_wifi_timer, (os_timer_func_t *)otb_wifi_timerfunc, NULL);
    os_timer_arm((os_timer_t *)&otb_wifi_timer, 1000, 0); 
  }
  else if (rc == OTB_WIFI_STATUS_TIMED_OUT)
  {
    rc = OTB_WIFI_STATUS_TRYING_AP;
    otb_wifi_try_ap(OTB_WIFI_DEFAULT_AP_TIMEOUT);
  }
  else
  {
    OTB_ASSERT(FALSE);
  }
  
  otb_wifi_status = rc;   
  
  DEBUG("WIFI: otb_wifi_process_test exit");

  return(otb_wifi_status);  
}

void ICACHE_FLASH_ATTR otb_wifi_timerfunc(void *arg)
{
  uint8_t rc;

  DEBUG("WIFI: otb_wifi_timerfunc entry");

  OTB_ASSERT((otb_wifi_status == OTB_WIFI_STATUS_CONNECTING) ||
             (otb_wifi_status == OTB_WIFI_STATUS_TRYING_AP));

  if ((otb_wifi_status == OTB_WIFI_STATUS_CONNECTING) ||
      (otb_wifi_status == OTB_WIFI_STATUS_PERMANENTLY_FAILED))
  {
    rc = otb_wifi_test_connected(&otb_wifi_timeout); 
    DEBUG("WIFI:test_connected result %d", rc);

    rc = otb_wifi_process_test(rc);
    DEBUG("WIFI:process_test result %d", rc);
  }
  else
  {
    // OTB_WIFI_STATUS_TRYING_AP
    rc = otb_wifi_test_ap(&otb_wifi_timeout);
    DEBUG("WIFI:test_ap result %d", rc);
  }

  DEBUG("WIFI: otb_wifi_timerfunc exit");

  return;
}

 
uint8_t ICACHE_FLASH_ATTR otb_wifi_test_ap(otb_util_timeout *timeout)
{

  DEBUG("WIFI: otb_wifi_test_ap entry");

  if (otb_wifi_ap_mode_done)
  {
    INFO("WIFI: AP mode has completed ... rebooting");
    otb_wifi_status = OTB_WIFI_STATUS_AP_DONE;
    otb_reset();
  }
  else
  {
    if (otb_util_timer_finished(timeout))
    {
      INFO("WIFI: AP mode has timed out ... rebooting");
      otb_wifi_status = OTB_WIFI_STATUS_AP_TIMED_OUT;
      otb_reset();
    }
  }
  
  // If got here, need to reset timer so we're called again.
  os_timer_disarm((os_timer_t *)&otb_wifi_timer);
  os_timer_setfn((os_timer_t *)&otb_wifi_timer, (os_timer_func_t *)otb_wifi_timerfunc, NULL);
  os_timer_arm((os_timer_t *)&otb_wifi_timer, 1000, 0); 

  DEBUG("WIFI: otb_wifi_test_ap entry");
  
  return otb_wifi_status;
}

// timeout = milliseconds
uint8_t ICACHE_FLASH_ATTR otb_wifi_test_connected(otb_util_timeout *timeout)
{
  uint32_t current_time;
  uint8_t final_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
  uint8_t wifi_status;
  DEBUG("WIFI: otb_wifi_test_connected entry");
  
  // Get status
  wifi_status = wifi_station_get_connect_status();

  switch (wifi_status)
  {
    case STATION_IDLE:
    case STATION_CONNECTING:
      // Still going
      final_status = OTB_WIFI_STATUS_CONNECTING;
      break;
      
    // Have discovered can return -1!
    case 0xFF:
    case STATION_WRONG_PASSWORD:
    case STATION_NO_AP_FOUND:
    case STATION_CONNECT_FAIL:
      // Permanent failure - store error, but keep going as could still connect
      // - timeout will catch and move us onwards.s
      WARN("WIFI: Permanent connect failure %d", wifi_status);
      otb_wifi_store_station_connect_error();
      final_status = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      break;
      
    case STATION_GOT_IP:
      // We're done
      INFO("WIFI: Connected");
      final_status = OTB_WIFI_STATUS_CONNECTED;
      break;
      
    default:
      // Note 255 actually means not in stations mode, but we should be!
      ERROR("WIFI: got invalid return code from connect_status %d", wifi_status);
      OTB_ASSERT(FALSE);
      break;
  }

  if ((final_status != OTB_WIFI_STATUS_CONNECTED) &&
      (otb_util_timer_finished(timeout)))
  {
    final_status = OTB_WIFI_STATUS_TIMED_OUT;
  }

  DEBUG("WIFI: otb_wifi_test_connected exit");

  return final_status;
}

void ICACHE_FLASH_ATTR otb_wifi_try_ap(uint32_t timeout)
{
  struct softap_config ap_conf;
  struct softap_config ap_conf_current;

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
  wifi_set_opmode_current(STATIONAP_MODE);
  if (os_memcmp(&ap_conf, &ap_conf_current, sizeof(ap_conf)))
  {
    // We can't store this until in SoftAP mode
    INFO("WIFI: Storing new AP config to flash");
    wifi_softap_set_config(&ap_conf);
  }
  ETS_UART_INTR_ENABLE();
  INFO("WIFI: Started AP");
  
  // Initialize the captive DNS service
  //stdoutInit();
  captdnsInit();

  // Start up the file system to serve files
  espFsInit((void*)(webpages_espfs_start));
  
  // Kick of the HTTP server - otb_wifi_ap_mode_done will signal when done
  otb_wifi_ap_mode_done = FALSE;
  httpdInit(otb_wifi_ap_urls, OTB_HTTP_SERVER_PORT);
  
  otb_wifi_timeout.start_time = system_get_time();
  otb_wifi_timeout.end_time = otb_wifi_timeout.start_time + timeout * 1000;
  
  os_timer_disarm((os_timer_t *)&otb_wifi_timer);
  os_timer_setfn((os_timer_t *)&otb_wifi_timer, (os_timer_func_t *)otb_wifi_timerfunc, NULL);
  os_timer_arm((os_timer_t *)&otb_wifi_timer, 1000, 0); 
  
  DEBUG("WIFI: otb_wifi_try_ap exit");
}

// wifi_conf = address to store struct in
bool ICACHE_FLASH_ATTR otb_wifi_get_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf)
{
  bool rc;
  DEBUG("WIFI: otb_wifi_get_current_conf entry");
  
  // Will reimplement in future to store in OTB-IOT's own way
  rc = wifi_station_get_config(wifi_conf);

  DEBUG("WIFI: otb_wifi_get_current_conf exit");
  
  return rc;
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

void ICACHE_FLASH_ATTR otb_wifi_set_stored_conf(OTB_WIFI_STATION_CONFIG *wifi_conf)
{
  bool rc;
  
  DEBUG("WIFI: otb_wifi_set_stored_conf entry");
  
  ETS_UART_INTR_DISABLE();
  wifi_station_set_config(wifi_conf);
  ETS_UART_INTR_ENABLE();
  
  DEBUG("WIFI: otb_wifi_set_stored_conf exit");
  
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
 
