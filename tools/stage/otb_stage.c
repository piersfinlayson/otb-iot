/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017 Piers Finlayson
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

#include "otb_stage.h"

void ICACHE_FLASH_ATTR user_init(void)
{
  // Set baud rate and turn off SDK logging
  uart_div_modify(0, UART_CLK_FREQ / OTB_STAGE_BAUD_RATE);
  uart_div_modify(1, UART_CLK_FREQ / OTB_STAGE_BAUD_RATE);
  system_set_os_print(0);
  ets_printf("\r\n");

  // Disable wifi
  wifi_set_opmode_current(STATION_MODE);
  wifi_station_set_auto_connect(0);

  // Set up otb_stage_do to run
  system_init_done_cb((init_done_cb_t)otb_stage_do);
}

// Get chipid
// Get mac1
// Get mac2
// Leave GPIOs (0/2 used for internal I2C) stable
void ICACHE_FLASH_ATTR otb_stage_do(void)
{
  bool fn_rc;
  unsigned char mac[6];
  unsigned char mac1[3];
  unsigned char mac2[3];
  uint32_t chipid;
  int ii;

  ets_printf("\r\n");

  // Get chipid
  chipid = system_get_chip_id();
  ets_printf("STAGE: chipid:       %06x\r\n", chipid);

  // Get mac1 and mac2 (mac1 = station, mac2 = ap)
  for (ii = 0; ii < 2; ii++)
  {
    if (ii == 0)
    {
      fn_rc = wifi_get_macaddr(STATION_IF, mac);
      os_memcpy(mac1, mac, 3);
    }
    else
    {
      fn_rc = wifi_get_macaddr(SOFTAP_IF, mac);
      os_memcpy(mac2, mac, 3);
    }
    ets_printf("STAGE: mac%d:         ", ii+1);
    if (fn_rc)
    {
      ets_printf("%02x%02x%02x", mac[0], mac[1], mac[2]);
    }
    else
    {
      ets_printf("unknown");
    }
    ets_printf("\r\n");
  }

  ets_printf("\r\n");
  ets_printf("STAGE: eeprog args:  -i %06x -1 %02x%02x%02x -2 %02x%02x%02x",
             chipid,
             mac1[0],
             mac1[1],
             mac1[2],
             mac2[0],
             mac2[1],
             mac2[2]);

  ets_printf("\r\n");
  ets_printf("STAGE: Now burn my eeprom...\r\n");

  return;
}

void ICACHE_FLASH_ATTR user_pre_init(void)
{
  bool rc = false;
  static const partition_item_t part_table[] =
  {
    {SYSTEM_PARTITION_RF_CAL,
     0x3fb000,
     0x1000},
    {SYSTEM_PARTITION_PHY_DATA,
     0x3fc000,
     0x1000},
    {SYSTEM_PARTITION_SYSTEM_PARAMETER,
     0x3fd000,
     0x1000},
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
