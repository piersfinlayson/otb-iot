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

#define OTB_RECOVERY_C
#include "otb.h"

char otb_log_s[OTB_MAIN_MAX_LOG_LENGTH];
char otb_compile_date[12];
char otb_compile_time[9];
char otb_version_id[OTB_MAIN_MAX_VERSION_LENGTH];

void ICACHE_FLASH_ATTR user_init(void)
{
  char *ws;
  
  // Set up serial logging
  uart_div_modify(0, UART_CLK_FREQ / OTB_MAIN_BAUD_RATE);

  DEBUG("OTB: user_init entry");
  
  // Configure ESP SDK logging (only)
  system_set_os_print(OTB_MAIN_SDK_LOGGING);

  // Initialize GPIO.  Must happen before we clear reset (as this uses GPIO)  
  otb_gpio_init();
  
  // Reset GPIO - pull pin 16 high
  otb_util_clear_reset();

  // Initialize wifi - mostly this just disables wifi until we're ready to turn it on!
  //otb_wifi_init();

  // Set up and log some useful info
  os_sprintf(otb_compile_date, "%s", __DATE__);
  // Get rid of whitespace from date
  ws = os_strstr(otb_compile_date, " ");
  while (ws)
  {
    *ws = '_';
    ws = os_strstr(ws, " ");
  }
  os_sprintf(otb_compile_time, "%s", __TIME__);
  // Get rid of whitespace from time
  ws = os_strstr(otb_compile_time, " ");
  while (ws)
  {
    *ws = '_';
    ws = os_strstr(ws, " ");
  }
  os_snprintf(otb_version_id,
              OTB_MAIN_MAX_VERSION_LENGTH,
              "%s/%s/%s/%s",
              OTB_MAIN_OTB_IOT,
              OTB_MAIN_FW_VERSION,
              otb_compile_date, 
              otb_compile_time);
  // First log needs a line break!
  INFO("\nOTB: %s %s", otb_version_id, "!!!RECOVERY!!!");
  INFO("OTB: Boot slot: %d", otb_rboot_get_slot(FALSE));
  os_sprintf(OTB_MAIN_CHIPID, "%06x", system_get_chip_id());
  INFO("OTB: ESP device %s", OTB_MAIN_CHIPID);
  os_sprintf(OTB_MAIN_DEVICE_ID, "OTB-IOT.%s", OTB_MAIN_CHIPID);
  INFO("");
  INFO("OTB: ---------------------");
  INFO("OTB: !!! Recovery Mode !!!");
  INFO("OTB: ---------------------");
  INFO("");
  
  //otb_util_check_defs();
  
  // Initialize and load config
  // otb_conf_init();
  // otb_conf_load();
  
  // Schedule timer to kick off wifi processing
  // os_timer_disarm((os_timer_t*)&init_timer);
  // os_timer_setfn((os_timer_t*)&init_timer, (os_timer_func_t *)otb_init_wifi, NULL);
  // os_timer_arm((os_timer_t*)&init_timer, 500, 0);

  DEBUG("OTB: user_init exit");

  return;
}
