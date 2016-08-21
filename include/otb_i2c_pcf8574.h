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

#ifndef OTB_I2C_PCF8574_H
#define OTB_I2C_PCF8574_H

// PCF8574 base address - with three address inputs
#define OTB_I2C_PCF8574_BASE_ADDR   0x20

// Number of IOs supported by a PCF8574
#define OTB_I2C_PCF8574_REG_IO_NUM       8

// Note PCF8574 has no registers except IO pin state, and no configuration

#ifndef OTB_I2C_PCF8574_C


#else

bool otb_i2c_pcf8574_led_on;
static volatile os_timer_t otb_i2c_pcf8574_test_timer;
uint8_t otb_i2c_pcf8574_test_addr;

#endif // OTB_I2C_PCF8574_C

void otb_i2c_pcf8574_test_timerfunc(void);
void otb_i2c_pcf8574_test_init(void);
bool otb_i2c_pcf8574_led_conf(uint8_t, uint8_t led, bool on);
bool otb_i2c_pcf8574_init(uint8_t addr);

#endif // OTB_I2C_PCF8574_H
