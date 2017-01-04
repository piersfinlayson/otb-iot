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

#define OTB_I2C_MCP23017_C
#define OTB_DEBUG_DISABLE
#include "otb.h"

void ICACHE_FLASH_ATTR otb_i2c_mcp23017_test_timerfunc(void)
{
  bool rc;
  
  DEBUG("I2C: otb_i2c_mcp23017_test_timer_func entry");

  if (otb_i2c_mcp23017_led_on)
  {
    // Off
    INFO("MCP23017: LED on - turn it off");
    rc = otb_i2c_mcp23017_led_conf(otb_i2c_mcp23017_test_addr, 8, FALSE);
    if (!rc)
    {
      WARN("MCP23017: Failed to communicate with MCP23017");
    }
    otb_i2c_mcp23017_led_on = FALSE;
  }
  else
  {
    // On
    INFO("MCP23017: LED off - turn it on");
    rc = otb_i2c_mcp23017_led_conf(otb_i2c_mcp23017_test_addr, 8, TRUE);
    if (!rc)
    {
      WARN("MCP23017: Failed to communicate with MCP23017");
    }
    otb_i2c_mcp23017_led_on = TRUE;
  }
  
  DEBUG("I2C: otb_i2c_mcp23017_test_timer_func entry");

  return;
}

void ICACHE_FLASH_ATTR otb_i2c_mcp23017_test_init(void)
{
  bool rc = FALSE;

  DEBUG("I2C: otb_i2c_mcp23017_test_init entry");

  otb_i2c_mcp23017_led_on = FALSE;
  otb_i2c_mcp23017_test_addr = OTB_I2C_MCP23017_BASE_ADDR;
  rc = otb_i2c_mcp23017_init(otb_i2c_mcp23017_test_addr);
  if (!rc)
  {
    WARN("MCP23017: Failed to init MCP23017 at address 0x%02x", otb_i2c_mcp23017_test_addr);
    goto EXIT_LABEL;
  }
  
  otb_util_timer_set((os_timer_t*)&otb_i2c_mcp23017_test_timer, 
                     (os_timer_func_t *)otb_i2c_mcp23017_test_timerfunc,
                     NULL,
                     1000,
                     1);
                     
  INFO("MCP23017: Initialized test");
  
EXIT_LABEL:
  
  DEBUG("I2C: otb_i2c_mcp23017_test_init exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_mcp23017_led_conf(uint8_t addr, uint8_t led, bool on)
{
  bool rc = FALSE;
  uint8_t io;
  uint8_t gpio_reg;
  uint8_t gpio_val;
  int ii;
  uint8_t reg;
  
  DEBUG("I2C: otb_i2c_mcp23017_led_conf entry");

  // Get which register this GPIO is on
  gpio_reg = otb_i2c_mcp23017_get_io_reg(led);
  INFO("MCP23017: Using register 0x%02x", gpio_reg);
  
  // Figure out which bit within this register this led is on
  io = 1 << (led|8);

#if 0
  rc = otb_i2c_read_one_reg(addr, gpio_reg, &gpio_val);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
#endif  
  
  if (on)
  {
    //gpio_val |= io;
    gpio_val = 0xff;
  }
  else
  {
    //gpio_val &= ~io;
    gpio_val = 0;
  }
  
  rc = otb_i2c_write_one_reg(addr, gpio_reg, gpio_val);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("I2C: otb_i2c_mcp23017_led_conf exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_mcp23017_init(uint8_t addr)
{
  bool rc = FALSE;
  uint8_t conf;

  DEBUG("I2C: otb_i2c_mcp23017_init entry");

  // Read the mode (on power on or after reset should be set to all zeros)
  rc = otb_i2c_read_one_reg(addr, OTB_I2C_MCP23017_REG_IOCON1, &conf);
  if (!rc)
  {
    WARN("MCP23017: Failed to read conf");
    goto EXIT_LABEL;
  }
  INFO("MCP23017: Read conf 0x%02x", conf);

  // Set the mode
  conf = 0;
  rc = otb_i2c_write_one_reg(addr, OTB_I2C_MCP23017_REG_IOCON1, conf);
  if (!rc)
  {
    WARN("MCP23017: Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_write_one_reg(addr, OTB_I2C_MCP23017_REG_IOCON2, conf);
  if (!rc)
  {
    WARN("MCP23017: Failed to write a reg");
    goto EXIT_LABEL;
  }

  // Set direction of all ports to output  
  rc = otb_i2c_write_one_reg(addr, OTB_I2C_MCP23017_REG_IODIRA, 0);
  if (!rc)
  {
    WARN("MCP23017: Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_write_one_reg(addr, OTB_I2C_MCP23017_REG_GPIOA, 0);
  if (!rc)
  {
    WARN("MCP23017: Failed to write a reg");
    goto EXIT_LABEL;
  }

  // Set direction of all ports to output  
  rc = otb_i2c_write_one_reg(addr, OTB_I2C_MCP23017_REG_IODIRB, 0);
  if (!rc)
  {
    WARN("MCP23017: Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_write_one_reg(addr, OTB_I2C_MCP23017_REG_GPIOB, 0);
  if (!rc)
  {
    WARN("MCP23017: Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("I2C: otb_i2c_mcp23017_init exit");

  return rc;
}

uint8_t ICACHE_FLASH_ATTR otb_i2c_mcp23017_get_io_reg(uint8_t io)
{
  uint8_t bank_reg;

  DEBUG("otb_i2c_mcp23017_get_io_reg entry");
  
  OTB_ASSERT(io < OTB_I2C_MCP23017_REG_IO_NUM);
  
  if (io < 8)
  {
    bank_reg = OTB_I2C_MCP23017_REG_GPIOA;
  }
  else
  {
    bank_reg = OTB_I2C_MCP23017_REG_GPIOB;
  }

  DEBUG("otb_i2c_mcp23017_get_io_reg exit");

  return bank_reg;
}



