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

#ifndef OTB_I2C_H
#define OTB_I2C_H

#define OTB_I2C_BUS_INTERNAL_SDA_PIN  0
#define OTB_I2C_BUS_INTERNAL_SCL_PIN  2
#define OTB_I2C_CLOCK_STRETCH_DEFAULT_US 200

typedef struct
{
  i2c_config_t info;
  i2c_port_t num;
  bool installed;
} otb_i2c_bus_t;

bool otb_i2c_initialize_bus_internal(void);
void otb_i2c_uninitialize_bus_internal(void);
bool otb_i2c_initialize_bus(otb_i2c_bus_t *bus,
                            uint8_t sda_pin,
                            uint8_t scl_pin);
void otb_i2c_uninitialize_bus(otb_i2c_bus_t *bus);
bool otb_i2c_write_data(uint8_t addr,
                        uint8_t *write_buf,
                        uint16_t write_bytes,
                        otb_i2c_bus_t *bus);
bool otb_i2c_read_data(uint8_t addr,
                       uint8_t *read_buf,
                       uint16_t read_bytes,
                       otb_i2c_bus_t *bus);
bool otb_i2c_write_then_read_data(uint8_t addr,
                                  uint8_t *write_buf,
                                  uint16_t write_bytes,
                                  uint8_t *read_buf,
                                  uint16_t read_bytes,
                                  otb_i2c_bus_t *bus);

#endif // OTB_I2C_H
