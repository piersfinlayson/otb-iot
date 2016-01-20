/*
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
 */

#include "otb.h"

char otb_log_s[OTB_MAIN_MAX_LOG_LENGTH];

void ICACHE_FLASH_ATTR c_setup(void)
{
  OTB_WIFI_STATION_CONFIG wifi_conf;
  bool rc;
  uint8_t wifi_status = OTB_WIFI_STATUS_NOT_CONNECTED;

  DEBUG("OTB: c_setup entry");
  
  otb_util_check_defs();
  
#if 0
  // Some code to burn an SSID/password into the flash
  wifi_set_opmode_current(STATION_MODE);
  strcpy(wifi_conf.ssid, "some_ssid");
  strcpy(wifi_conf.password, "some_password");
  wifi_conf.bssid_set = FALSE;
  strcpy(wifi_conf.bssid, "");
  otb_wifi_set_stored_conf(&wifi_conf);
  delay(20000);
#endif  
  
  INFO("OTB: Set up wifi");
  rc = otb_wifi_get_stored_conf(&wifi_conf);
  if (rc)
  {
    // No point in trying to connect in station mode if we've no stored config - skip
    // straight to AP mode
    wifi_status = otb_wifi_try_sta(wifi_conf.ssid,
                                   wifi_conf.password,
                                   wifi_conf.bssid_set,
                                   wifi_conf.bssid,
                                   OTB_WIFI_DEFAULT_STA_TIMEOUT);
  }
  
  if (wifi_status != OTB_WIFI_STATUS_CONNECTED)
  {
    // Try AP mode instead.  We _always_ reboot after this, cos either it worked in which
    // case we reboot to try the new credentials, or it didn't in which case we'll give
    // station mode another try when we boot up again.
    otb_wifi_try_ap(OTB_WIFI_DEFAULT_AP_TIMEOUT);
    otb_reset();
    delay(1000);
    OTB_ASSERT(FALSE);
  }
  
  INFO("OTB: Set up MQTT stack");
  otb_mqtt_initialize(OTB_MQTT_SERVER,
                      OTB_MQTT_PORT,
                      0,
                      OTB_MAIN_DEVICE_ID,
                      "user",
                      "pass",
                      OTB_MQTT_KEEPALIVE);
  // brief pause to allow this to settle down
  delay(1000);

  // Initialize the one wire bus
  otb_ow_initialize(OTB_OW_DEFAULT_GPIO);

  DEBUG("OTB: c_setup exit");
}
