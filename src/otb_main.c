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

#define OTB_MAIN_C
#include "otb.h"

char otb_log_s[OTB_MAIN_MAX_LOG_LENGTH];
char otb_compile_date[12];
char otb_compile_time[9];
char otb_version_id[OTB_MAIN_MAX_VERSION_LENGTH];
void configModeCallback();
char ssid[32];
char OTB_MAIN_CHIPID[OTB_MAIN_CHIPID_STR_LENGTH];
char OTB_MAIN_DEVICE_ID[20];

void ICACHE_FLASH_ATTR otb_main_log_fn(char *text)
{
  if (!OTB_MAIN_DISABLE_OTB_LOGGING)
  {
    ets_printf(text);
    ets_printf("\n");
  }
}

void ICACHE_FLASH_ATTR otb_reset(void)
{
  DEBUG("OTB: otb_reset entry");

  // Log resetting and include \n so this log isn't overwritten
  INFO("OTB: Resetting ...");

  // Delay to give any serial logs time to output.  Can't use arduino delay as
  // may be in wrong context
  otb_util_delay_ms(1000);

  // Reset by pulling reset GPIO (connected to reset) low
  // XXX Only works for GPIO 16
  WRITE_PERI_REG(RTC_GPIO_OUT,
                   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(0));

  DEBUG("OTB: otb_reset exit");
 
  return;
}

void ICACHE_FLASH_ATTR user_init(void)
{
  
  // Set up serial logging
  uart_div_modify(0, UART_CLK_FREQ / OTB_MAIN_BAUD_RATE);

  DEBUG("OTB: user_init entry");
  
  // Configure ESP SDK logging (only)
  system_set_os_print(OTB_MAIN_SDK_LOGGING);

  // Initialize GPIO.  Must happen before we clear reset (as this uses GPIO)  
  gpio_init();
  
  // Reset GPIO - pull pin 16 high
  otb_util_clear_reset();

  // Initialize wifi - mostly this just disables wifi until we're ready to turn it on!
  otb_wifi_init();


  // Set up and log some useful info
  os_sprintf(otb_compile_date, "%s", __DATE__);
  os_sprintf(otb_compile_time, "%s", __TIME__);
  os_snprintf(otb_version_id,
              OTB_MAIN_MAX_VERSION_LENGTH,
              "%s/%s/%s/%s",
              OTB_MAIN_OTB_IOT,
              OTB_MAIN_FW_VERSION,
              otb_compile_date, 
              otb_compile_time);
  // First log needs a line break!
  INFO("\nOTB: %s", otb_version_id);
  INFO("OTB: Boot slot: %d", otb_rboot_get_slot(FALSE));
  os_sprintf(OTB_MAIN_CHIPID, "%06x", system_get_chip_id());
  INFO("OTB: ESP device %s", OTB_MAIN_CHIPID);
  os_sprintf(OTB_MAIN_DEVICE_ID, "OTB-IOT.%s", OTB_MAIN_CHIPID);
  
  otb_util_check_defs();
  
#if 0
  OTB_WIFI_STATION_CONFIG wifi_conf;
  // Some code to burn an SSID/password into the flash
  wifi_set_opmode_current(STATION_MODE);
  strcpy(wifi_conf.ssid, "some_ssid");
  strcpy(wifi_conf.password, "some_password");
  wifi_conf.bssid_set = FALSE;
  strcpy(wifi_conf.bssid, "");
  otb_wifi_set_stored_conf(&wifi_conf);
  INFO("Pausing for 20s ...");
  otb_util_delay_ms(20000);
#endif  

  // Schedule timer to kick off wifi processing
  os_timer_disarm((os_timer_t*)&init_timer);
  os_timer_setfn((os_timer_t*)&init_timer, (os_timer_func_t *)otb_init_wifi, NULL);
  os_timer_arm((os_timer_t*)&init_timer, 500, 0);

  DEBUG("OTB: user_init exit");

  return;
}

void ICACHE_FLASH_ATTR otb_init_wifi(void *arg)
{
  bool rc;
  OTB_WIFI_STATION_CONFIG wifi_conf;
  uint8_t wifi_status = OTB_WIFI_STATUS_NOT_CONNECTED;

  DEBUG("OTB: otb_init_wifi entry");
  
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
  
  if ((wifi_status != OTB_WIFI_STATUS_CONNECTED) &&
      (wifi_status != OTB_WIFI_STATUS_CONNECTING))
  {
    // Try AP mode instead.  We _always_ reboot after this, cos either it worked in which
    // case we reboot to try the new credentials, or it didn't in which case we'll give
    // station mode another try when we boot up again.
    otb_wifi_try_ap(OTB_WIFI_DEFAULT_AP_TIMEOUT);
    //otb_reset();
    //OTB_ASSERT(FALSE);
  }
  
  DEBUG("OTB: otb_init_wifi entry");
 
  return; 
}

void ICACHE_FLASH_ATTR otb_init_mqtt(void *arg)
{
  DEBUG("OTB: otb_init_mqtt entry");
  
  INFO("OTB: Set up MQTT stack");
  otb_mqtt_initialize(OTB_MQTT_SERVER,
                      OTB_MQTT_PORT,
                      0,
                      OTB_MAIN_DEVICE_ID,
                      "user",
                      "pass",
                      OTB_MQTT_KEEPALIVE);

  // Now set up DS18B20 init
  os_timer_disarm((os_timer_t*)&init_timer);
  os_timer_setfn((os_timer_t*)&init_timer, (os_timer_func_t *)otb_init_ds18b20, NULL);
  os_timer_arm((os_timer_t*)&init_timer, 500, 0);  

  DEBUG("OTB: otb_init_mqtt entry");

  return;
}

void ICACHE_FLASH_ATTR otb_init_ds18b20(void *arg)
{
  DEBUG("OTB: otb_init_ds18b20 entry");

  INFO("OTB: Set up One Wire bus");
  otb_ds18b20_initialize(OTB_DS18B20_DEFAULT_GPIO);

  os_timer_disarm((os_timer_t*)&init_timer);

  DEBUG("OTB: otb_init_mqtt exit");

  return;
}

