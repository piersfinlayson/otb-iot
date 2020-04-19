/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2020 Piers Finlayson
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

#define OTB_I2C_24XXYY_C
#include "otb.h"

MLOG("otb-i2c-24xxyy");

bool otb_i2c_24xxyy_init(uint8_t addr, otb_i2c_bus_t *bus)
{
  bool success = FALSE;
  i2c_cmd_handle_t cmd = NULL;
  uint8_t val[1];

  ENTRY;

  ESP_ALLOC_WARN_AND_GOTO_EXIT_LABEL(success, cmd, i2c_cmd_link_create());
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, TRUE);
  i2c_master_read(cmd, val, 1, I2C_MASTER_LAST_NACK);
  ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success, i2c_master_cmd_begin(bus->num, cmd, 1000 / portTICK_RATE_MS));
  i2c_cmd_link_delete(cmd);
  cmd = NULL;
  success = TRUE;
  MDETAIL("Successfully initialized device at address: 0x%02x", addr);

EXIT_LABEL:

  if (cmd != NULL)
  {
    i2c_cmd_link_delete(cmd);
    cmd = NULL;
  }

  EXIT;

  return success;
}

