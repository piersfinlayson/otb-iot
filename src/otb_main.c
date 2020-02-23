/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-8 Piers Finlayson
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

MLOG("MAIN");

void ICACHE_FLASH_ATTR user_init(void)
{
  int stack;

  // According to docs required as first step, to enable us timer
  // Note that this means that maximum os_timer_arm (not us) value is about
  // 432s (0x689D0 in ms)
  system_timer_reinit();

  otb_util_init_logging();

  ENTRY;

  // See if user wants to override log level
  system_init_done_cb((init_done_cb_t)otb_util_check_for_log_level);
  ets_printf("\r\n");
  
  EXIT;

  return;
}

#if ESP_SDK_VERSION >= 030000
// user_pre_init is required from SDK v3.0.0 onwards
// It is used to register the parition map with the SDK, primarily to allow
// the app to use the SDK's OTA capability.  We don't make use of that in 
// otb-iot and therefore the only info we provide is the mandatory stuff:
// - RF calibration data
// - Physical data
// - System parameter
// The location and length of these are from the 2A SDK getting started guide
void ICACHE_FLASH_ATTR user_pre_init(void)
{
  bool rc = false;
  static const partition_item_t part_table[] = 
  {
    {SYSTEM_PARTITION_RF_CAL,
     OTB_BOOT_RF_CAL,
     OTB_BOOT_RF_CAL_LEN},
    {SYSTEM_PARTITION_PHY_DATA,
     OTB_BOOT_PHY_DATA,
     OTB_BOOT_PHY_DATA_LEN},
    {SYSTEM_PARTITION_SYSTEM_PARAMETER,
     OTB_BOOT_SYS_PARAM,
     OTB_BOOT_SYS_PARAM_LEN},
  };

  // This isn't an ideal approach but there's not much point moving on unless
  // or until this has succeeded cos otherwise the SDK will just barf and 
  // refuse to call user_init()
  while (!rc)
  {
    rc = system_partition_table_regist(part_table,
				       sizeof(part_table)/sizeof(part_table[0]),
                                       4);
  }

  return;
}
#endif // ESP_SDK_VERSION >= 030000
