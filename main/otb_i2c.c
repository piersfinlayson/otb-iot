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

#define OTB_I2C_C
#include "otb.h"

MLOG("otb-i2c");

bool otb_i2c_initialize_bus_internal(void)
{
  bool rc;
  
  ENTRY;

  OTB_ASSERT(!otb_i2c_bus.installed);

  rc = otb_i2c_initialize_bus(&otb_i2c_bus,
                              OTB_I2C_BUS_INTERNAL_SDA_PIN,
                              OTB_I2C_BUS_INTERNAL_SCL_PIN);

  EXIT;
  
  return rc;
}

void otb_i2c_uninitialize_bus_internal(void)
{
  
  ENTRY;

  otb_i2c_uninitialize_bus(&otb_i2c_bus);

  EXIT;
  
  return;
}

bool otb_i2c_initialize_bus(otb_i2c_bus_t *bus,
                            uint8_t sda_pin,
                            uint8_t scl_pin)
{
  bool success = FALSE;

  ENTRY;
  
  OTB_ASSERT(bus != NULL);
  OTB_ASSERT(!bus->installed);

  bus->num = I2C_NUM_0;
  os_memset(&(bus->info), 0, sizeof(bus->info));
  bus->info.mode = I2C_MODE_MASTER;
  bus->info.sda_io_num = sda_pin;
  bus->info.scl_io_num = scl_pin;
  bus->info.sda_pullup_en = 1; // Might as well, but have hardware pullups
  bus->info.scl_pullup_en = 1; // Might as well, but have hardware pullups
  bus->info.clk_stretch_tick = TICKS_US(OTB_I2C_CLOCK_STRETCH_DEFAULT_US);
  ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success, i2c_driver_install(bus->num, bus->info.mode));
  bus->installed = TRUE;
  ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success, i2c_param_config(bus->num, &(bus->info)));

  MDETAIL("I2C Bus %d SDA %d SCL %d stretch %d initialized successfully",
          bus->num,
          bus->info.sda_io_num,
          bus->info.scl_io_num,
          bus->info.clk_stretch_tick);

  success = TRUE;

EXIT_LABEL:

  if (!success && (bus->installed))
  {
    ESP_ERR_WARN(i2c_driver_delete(bus->num))
    bus->installed = FALSE;
  }

  EXIT;

  return(success);
}

void otb_i2c_uninitialize_bus(otb_i2c_bus_t *bus)
{
  int num;

  ENTRY;
  
  OTB_ASSERT(bus != NULL);
  OTB_ASSERT(bus->installed);

  num = bus->num; // Saved in case we want to clear bus->num before logging
  ESP_ERR_WARN(i2c_driver_delete(bus->num))
  bus->installed = FALSE;
  os_memset(&(bus->info), 0, sizeof(bus->info));

  MDETAIL("I2C Bus %d uninitialized", num);

  EXIT;

  return;
}

bool otb_i2c_write_data(uint8_t addr,
                        uint8_t *write_buf,
                        uint16_t write_bytes,
                        otb_i2c_bus_t *bus)
{
  i2c_cmd_handle_t cmd;
  bool success;

  ENTRY;

  ESP_ALLOC_WARN_AND_GOTO_EXIT_LABEL(success, cmd, i2c_cmd_link_create());
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, TRUE);
  i2c_master_write(cmd, write_buf, write_bytes, TRUE);
  ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success,
                                   i2c_master_cmd_begin(bus->num,
                                                        cmd,
                                                        1000 / portTICK_RATE_MS));
  i2c_cmd_link_delete(cmd);
  cmd = NULL;
  success = TRUE;

EXIT_LABEL:

  if (cmd != NULL)
  {
    i2c_cmd_link_delete(cmd);
    cmd = NULL;
  }

  EXIT;

  return success;
}                        

bool otb_i2c_read_data(uint8_t addr,
                       uint8_t *read_buf,
                       uint16_t read_bytes,
                       otb_i2c_bus_t *bus)
{
  i2c_cmd_handle_t cmd;
  bool success;

  ENTRY;

  ESP_ALLOC_WARN_AND_GOTO_EXIT_LABEL(success, cmd, i2c_cmd_link_create());
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, TRUE);
  i2c_master_read(cmd, read_buf, read_bytes, I2C_MASTER_LAST_NACK);
  ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success,
                                   i2c_master_cmd_begin(bus->num,
                                                        cmd,
                                                        1000 / portTICK_RATE_MS));
  i2c_cmd_link_delete(cmd);
  cmd = NULL;
  success = TRUE;

EXIT_LABEL:

  if (cmd != NULL)
  {
    i2c_cmd_link_delete(cmd);
    cmd = NULL;
  }

  EXIT;

  return success;
}                        

bool otb_i2c_write_then_read_data(uint8_t addr,
                                  uint8_t *write_buf,
                                  uint16_t write_bytes,
                                  uint8_t *read_buf,
                                  uint16_t read_bytes,
                                  otb_i2c_bus_t *bus)
{
  bool success;

  ENTRY;

  success = otb_i2c_write_data(addr, write_buf, write_bytes, bus) &&
            otb_i2c_read_data(addr, read_buf, read_bytes, bus);

  EXIT;

  return success;
}
