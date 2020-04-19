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

#define OTB_I2C_PCF8574_C
#include "otb.h"

MLOG("PCF8574");

void ICACHE_FLASH_ATTR otb_i2c_pcf8574_test_timerfunc(void)
{
  bool rc;
  
  ENTRY;

  if (otb_i2c_pcf8574_led_on)
  {
    // Off
    MDETAIL("LED on - turn it off");
    rc = otb_i2c_pcf8574_led_conf(otb_i2c_pcf8574_test_addr, 0, FALSE);
    if (!rc)
    {
      MWARN("Failed to communicate with PCF8574");
    }
    otb_i2c_pcf8574_led_on = FALSE;
  }
  else
  {
    // On
    MDETAIL("LED off - turn it on");
    rc = otb_i2c_pcf8574_led_conf(otb_i2c_pcf8574_test_addr, 0, TRUE);
    if (!rc)
    {
      MWARN("Failed to communicate with PCF8574");
    }
    otb_i2c_pcf8574_led_on = TRUE;
  }
  
  ENTRY;

  return;
}

void ICACHE_FLASH_ATTR otb_i2c_pcf8574_test_init(void)
{
  bool rc = FALSE;

  ENTRY;

  otb_i2c_pcf8574_led_on = FALSE;
  otb_i2c_pcf8574_test_addr = OTB_I2C_PCF8574_BASE_ADDR;
  rc = otb_i2c_pcf8574_init(otb_i2c_pcf8574_test_addr);
  if (!rc)
  {
    MWARN("Failed to init PCF8574 at address 0x%02x", otb_i2c_pcf8574_test_addr);
    //goto EXIT_LABEL;
  }
  
  otb_util_timer_set((os_timer_t*)&otb_i2c_pcf8574_test_timer, 
                     (os_timer_func_t *)otb_i2c_pcf8574_test_timerfunc,
                     NULL,
                     1000,
                     1);
                     
  MDETAIL("Initialized test");
  
EXIT_LABEL:
  
  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_pcf8574_led_conf(uint8_t addr, uint8_t led, bool on)
{
  bool rc = FALSE;
  uint8_t io;
  uint8_t gpio_val;
  int ii;
  uint8_t reg;
  
  ENTRY;

  // Figure out which bit within this register this led is on
  io = 1 << (led|8);

#if 0
  rc = otb_i2c_read_one_val(addr, &gpio_val);
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
  
  rc = otb_i2c_write_one_val(addr, gpio_val);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pcf8574_init(uint8_t addr)
{
  bool rc = FALSE;
  uint8_t conf;

  ENTRY;

  // Set all outputs to 0
  rc = otb_i2c_write_one_val(addr, 0);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:

  EXIT;

  return rc;
}

