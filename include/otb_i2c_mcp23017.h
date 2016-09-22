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

#ifndef OTB_I2C_MCP23017_H
#define OTB_I2C_MCP23017_H

// MCP23017 base address
#define OTB_I2C_MCP23017_BASE_ADDR   0x20

// MCP23017 registers - IOCON.BANK = 0
#define OTB_I2C_MCP23017_REG_IODIRA       0x00
#define OTB_I2C_MCP23017_REG_IODIRB       0x01
#define OTB_I2C_MCP23017_REG_IPOLA        0x02
#define OTB_I2C_MCP23017_REG_IPOLB        0x03
#define OTB_I2C_MCP23017_REG_GPINTENA     0x04
#define OTB_I2C_MCP23017_REG_GPINTENB     0x05
#define OTB_I2C_MCP23017_REG_DEFVALA      0x06
#define OTB_I2C_MCP23017_REG_DEFVALB      0x07
#define OTB_I2C_MCP23017_REG_INTCONA      0x08
#define OTB_I2C_MCP23017_REG_INTCONB      0x09
#define OTB_I2C_MCP23017_REG_IOCON1       0x0A
#define OTB_I2C_MCP23017_REG_IOCON2       0x0B
#define OTB_I2C_MCP23017_REG_GPPUA        0x0C
#define OTB_I2C_MCP23017_REG_GPPBU        0x0D
#define OTB_I2C_MCP23017_REG_INTFA        0x0E
#define OTB_I2C_MCP23017_REG_INTFB        0x0F
#define OTB_I2C_MCP23017_REG_INTCAPA      0x10
#define OTB_I2C_MCP23017_REG_INTCAPB      0x11
#define OTB_I2C_MCP23017_REG_GPIOA        0x12
#define OTB_I2C_MCP23017_REG_GPIOB        0x13
#define OTB_I2C_MCP23017_REG_OLATA        0x14
#define OTB_I2C_MCP23017_REG_OTATB        0x15

// Number of IOs supported by a MCP23017
#define OTB_I2C_MCP23017_REG_IO_NUM       16

// MCP23017 configuration bits
#define OTB_I2C_MCP23017_CONF_BANK        0b10000000
#define OTB_I2C_MCP23017_CONF_MIRROR      0b01000000
#define OTB_I2C_MCP23017_CONF_SEQOP       0b00100000
#define OTB_I2C_MCP23017_CONF_DISSLW      0b00010000
#define OTB_I2C_MCP23017_CONF_HAEN        0b00001000
#define OTB_I2C_MCP23017_CONF_ODR         0b00000100
#define OTB_I2C_MCP23017_CONF_INTPOL      0b00000010

#ifndef OTB_I2C_MCP23017_C


#else

bool otb_i2c_mcp23017_led_on;
static volatile os_timer_t otb_i2c_mcp23017_test_timer;
uint8_t otb_i2c_mcp23017_test_addr;

#endif // OTB_I2C_MCP23017_C

void otb_i2c_mcp23017_test_timerfunc(void);
void otb_i2c_mcp23017_test_init(void);
bool otb_i2c_mcp23017_led_conf(uint8_t, uint8_t led, bool on);
bool otb_i2c_mcp23017_init(uint8_t addr);
uint8_t otb_i2c_mcp23017_get_io_reg(uint8_t io);

#endif // OTB_I2C_MCP23017_H
