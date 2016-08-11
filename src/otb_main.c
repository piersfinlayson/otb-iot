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

void configModeCallback();
char ssid[32];

void ICACHE_FLASH_ATTR user_init(void)
{
  // According to docs required as first step, to enable us timer
  system_timer_reinit();

  otb_util_init_logging();

  DEBUG("OTB: user_init entry");
  
  // Log some useful info
  otb_util_log_useful_info(FALSE);

  // Do some sanity checking
  otb_util_check_defs();
  
  // Initialize GPIO.  Must happen before we clear reset (as this uses GPIO)  
  otb_gpio_init();
  
  // Reset GPIO - pull pin 16 high
  otb_util_clear_reset();

  // Initialize wifi - mostly this just disables wifi until we're ready to turn it on!
  otb_wifi_init();

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

  // Initialize and load config
  otb_conf_init();
  otb_conf_load();
  
  otb_gpio_apply_boot_state();
  otb_led_wifi_update(OTB_LED_NEO_COLOUR_BLUE, TRUE);

#if 0  
  if (otb_gpio_get(OTB_GPIO_RESET_PIN, TRUE))
  {
#endif
    system_init_done_cb((init_done_cb_t)otb_wifi_kick_off);
#if 0
  }
  else
  {
    system_init_done_cb((init_done_cb_t)otb_gpio_reset_kick_off);
  }
#endif
  
  DEBUG("OTB: user_init exit");

  return;
}
