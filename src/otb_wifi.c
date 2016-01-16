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

#define OTB_WIFI_C
#include "otb.h"

bool otb_wifi_ap_mode_done;

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
  struct ip_info ip_inf;
  bool ip_info_rc;
  char addr_s[OTB_WIFI_MAX_IPV4_STRING_LEN];
  bool mac_rc;
  char mac[OTB_WIFI_MAC_ADDRESS_STRING_LENGTH];

  DEBUG("WIFI: otb_wifi_try_sta entry");
  
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
  INFO("WIFI: Set SSID to %s", ssid);
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
  
  rc = otb_wifi_wait_until_connected(timeout); 
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
      
    }
    else
    {
      // Shucks, failed at the last hurdle - can't get IP
      WARN("WIFI: failed to get IP address");
      rc = OTB_WIFI_STATUS_PERMANENTLY_FAILED;
      goto EXIT;
    }    
    
  }
  DEBUG("WIFI: wait_until_connected result %d", rc);

EXIT:  
  DEBUG("WIFI: otb_wifi_try_sta exit");
  return(rc);
}

// timeout = milliseconds
uint8_t ICACHE_FLASH_ATTR otb_wifi_wait_until_connected(uint32_t timeout)
{
  uint32_t start_time;
  uint32_t current_time;
  uint32_t end_time;
  uint8_t final_status = OTB_WIFI_STATUS_NOT_CONNECTED;
  uint8_t wifi_status;
  DEBUG("WIFI: otb_wifi_wait_until_connected entry");
  
  // system_get_time gets time in microseconds since startup.  It wraps, but after 71
  // minutes.  We only start wifi at startup so unlikely that it'll wrap while we're
  // doing this, but even so, the arithmetic should wrap to make it all add up anyway.
  current_time = system_get_time();
  start_time = current_time;
  end_time = current_time + (timeout * 1000);
  DEBUG("WIFI: End time: %d", end_time);
  
  // Run this loop at least once
  while (TRUE)
  {
    DEBUG("WIFI: Current time: %d", current_time);
    
    // Get status
    wifi_status = wifi_station_get_connect_status();

    switch (wifi_status)
    {
      case STATION_IDLE:
      case STATION_CONNECTING:
        // Still going
        break;
        
      case STATION_WRONG_PASSWORD:
      case STATION_NO_AP_FOUND:
      case STATION_CONNECT_FAIL:
        // Permanent failure - we aren't going to magically recover from these.  We
        // will store off the error and then force a reboot.
        WARN("WIFI: permanent connect failure %d", wifi_status);
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

    if (final_status != OTB_WIFI_STATUS_NOT_CONNECTED)
    {
      break;
    }

    // Wait a second
    otb_util_delay_ms(1000);
    current_time = system_get_time();
    if (start_time < end_time)
    {
      if (current_time > end_time)
      {
        DEBUG("WIFI: Have waited longer than %d ms", timeout);
        break;
      }
    }
    else
    {
      if ((current_time < start_time) && (current_time > end_time))
      {
        DEBUG("WIFI: Have waited longer than %d ms", timeout);
        break;
      }
    }
  }
  
  DEBUG("WIFI: otb_wifi_wait_until_connected exit");
  return final_status;
}

void ICACHE_FLASH_ATTR otb_wifi_try_ap(uint32_t timeout)
{
  uint32_t start_time;
  uint32_t current_time;
  uint32_t end_time;
  bool going_to_wrap = FALSE;
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
  wifi_set_opmode_current(SOFTAP_MODE);
  if (os_memcmp(&ap_conf, &ap_conf_current, sizeof(ap_conf)))
  {
    INFO("WIFI: Storing new AP config to flash");
    wifi_softap_set_config(&ap_conf);
  }
  ETS_UART_INTR_ENABLE();
  INFO("WIFI: Started AP");
  
  // Initialize the captive DNS service
  //stdoutInit();
  captdnsInit();

  // Kick of the HTTP server - otb_wifi_ap_mode_done will signal when done
  otb_wifi_ap_mode_done = FALSE;
  httpdInit(otb_wifi_ap_urls, OTB_HTTP_SERVER_PORT);

  // Wait until either AP mode is done or for the timeout.
  current_time = system_get_time();
  start_time = current_time;
  end_time = current_time + (OTB_WIFI_DEFAULT_AP_TIMEOUT * 1000);
  if (end_time < current_time)
  {
    going_to_wrap = TRUE;
  }
  while (!otb_wifi_ap_mode_done)
  {
    otb_util_delay_ms(1000);
    
    current_time = system_get_time();
    if (!going_to_wrap)
    {
      if (current_time >= end_time)
      {
        break;
      }
    }
    else if ((current_time < start_time) && (current_time >= end_time))
    {
      break;
    }
  }
  
  if (!otb_wifi_ap_mode_done)
  {
    WARN("WIFI: AP Mode timed out");
  }
  
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

  sprintf(addr_s, "%d.%d.%d.%d", byte[0], byte[1], byte[2], byte[3]);
  
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
  snprintf(mac,
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
 