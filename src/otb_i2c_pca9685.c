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

#define OTB_I2C_PCA9685_C
#include "otb.h"

void ICACHE_FLASH_ATTR otb_i2c_pca9685_test_timerfunc(void)
{
  bool rc;
  
  if (otb_i2c_pca9685_led_on)
  {
    // Off
    INFO("PCA9685: LED on - turn it off");
    rc = otb_i2c_pca9685_led_conf(otb_i2c_pca9685_test_addr, 0, 0, OTB_I2C_PCA9685_IO_FULL_OFF);
    if (!rc)
    {
      WARN("PCA9685: Failed to communicate with PCA9685");
    }
    otb_i2c_pca9685_led_on = FALSE;
  }
  else
  {
    // On
    INFO("PCA9685: LED on - turn it on");
    rc = otb_i2c_pca9685_led_conf(otb_i2c_pca9685_test_addr, 0, OTB_I2C_PCA9685_IO_FULL_ON, 0);
    if (!rc)
    {
      WARN("PCA9685: Failed to communicate with PCA9685");
    }
    otb_i2c_pca9685_led_on = TRUE;
  }
  
  return;
}

void ICACHE_FLASH_ATTR otb_i2c_pca9685_test_init(void)
{
  bool rc = FALSE;

  otb_i2c_pca9685_led_on = FALSE;
  otb_i2c_pca9685_test_addr = OTB_I2C_PCA9685_BASE_ADDR;
  rc = otb_i2c_pca9685_init(otb_i2c_pca9685_test_addr);
  if (!rc)
  {
    WARN("PCA9685: Failed to init PCA9685 at address 0x%02x", otb_i2c_pca9685_test_addr);
    goto EXIT_LABEL;
  }
  
  otb_util_timer_set((os_timer_t*)&otb_i2c_pca9685_test_timer, 
                     (os_timer_func_t *)otb_i2c_pca9685_test_timerfunc,
                     NULL,
                     1000,
                     1);
                     
  INFO("PCA9685: Initialized test");
  
EXIT_LABEL:
  
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_led_conf(uint8_t addr, uint8_t led, uint16_t on, uint16_t off)
{
  bool rc = FALSE;
  uint8_t byte[4];
  int ii;
  uint8_t reg;
  
  // on and off are only 13-bit values
  OTB_ASSERT(!(on & 0b1110000000000000));
  OTB_ASSERT(!(off & 0b1110000000000000));
  
  // Can't be fully off _and_ on at the same time!
  OTB_ASSERT(!((on & OTB_I2C_PCA9685_IO_FULL_ON) && (off & OTB_I2C_PCA9685_IO_FULL_OFF)));
  
  // Can't be fully on and have other on values - or fully off and other off values
  OTB_ASSERT(!((on & OTB_I2C_PCA9685_IO_FULL_ON) && (on & 0b111111111111))); 
  OTB_ASSERT(!((off & OTB_I2C_PCA9685_IO_FULL_ON) && (off & 0b111111111111))); 
  
  // Can't be fully on and have off values, or vice versa
  OTB_ASSERT(!((on & OTB_I2C_PCA9685_IO_FULL_ON) && (off & 0b111111111111))); 
  OTB_ASSERT(!((off & OTB_I2C_PCA9685_IO_FULL_ON) && (on & 0b111111111111))); 
  
  byte[0] = on & 0xff;
  byte[1] = on >> 8;
  byte[2] = off & 0xff;
  byte[3] = off >> 8;
  
  // Start bus, and then call the PCA9685
  otb_i2c_pca9685_start();
  rc = otb_i2c_pca9685_call(addr);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  // Signal the start register, based on LED value (first LED = 0)
  OTB_ASSERT((led >= 0) && (led < OTB_I2C_PCA9685_REG_IO_NUM)); 
  reg = OTB_I2C_PCA9685_REG_IO0_ON_L + (led * 4);
  otb_i2c_pca9685_reg(reg);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  // Now write each of the 4 registers - two for on and two for off
  for (ii = 0; ii < 4; ii++)
  {
    otb_i2c_pca9685_val(byte[ii]);
    if (!rc)
    {
      goto EXIT_LABEL;
    }
  }

  otb_i2c_pca9685_stop();
  
  rc = TRUE;
  
EXIT_LABEL:

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_init(uint8_t addr)
{
  bool rc = FALSE;
  uint8_t read_mode[2];
  uint16_t mode = 0;
  int ii;

#if 0
  // XXX This is bugged - for some reason the second read doesn't work
  // Read the mode
  for (ii = 0; ii < 2; ii++)
  {
    rc = otb_i2c_pca9685_read_reg(addr, OTB_I2C_PCA9685_REG_MODE1+ii, read_mode+ii);
    if (!rc)
    {
      WARN("PCA9685: Failed to read mode %d", ii);
      goto EXIT_LABEL;
    }
    os_delay_us(1000);
  }
  mode = read_mode[0] & (read_mode[1] << 8);
  INFO("PCA9685: Read mode %02x", mode);
#endif
  
  // Set the mode.  Key thing to change is that auto-incrementing of registers is
  // defaulted - this allows us to write more than one register with successive writes
  // without restarting comms for each one (speeding things up).
  mode |= OTB_I2C_PCA9685_MODE_AI_REG | OTB_I2C_PCA9685_MODE_INVERT;
  mode &= ~ OTB_I2C_PCA9685_MODE_TOTEM;
  rc = otb_i2c_pca9685_set_mode(addr, mode);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = otb_i2c_pca9685_write_one_reg(addr,
                                     OTB_I2C_PCA9685_REG_PRE_SCALE,
                                     OTB_I2C_PCA9685_PRESCALE_DEFAULT);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
  bool rc = FALSE;
  
  // Read pointer register.  To do this start the bus, write to the right address,
  // write the reg we want to read, and then stop.
  // Then start the bus, calling the address indicating a read, and then do the read.
  otb_i2c_pca9685_start();
  
  rc = otb_i2c_pca9685_call(addr);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = otb_i2c_pca9685_reg(reg);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  otb_i2c_pca9685_stop();
  
  otb_i2c_pca9685_start();

  i2c_master_writeByte((addr << 1) | 0b1); // Read
  if (!i2c_master_checkAck())
  {
    INFO("I2C: No ack");
    goto EXIT_LABEL;
  }
  
  *val = i2c_master_readByte();
  INFO("PCA9685: Read value: 0x%02x", *val);
  i2c_master_send_ack();
  
  rc = TRUE;

EXIT_LABEL:

  otb_i2c_pca9685_stop();

  DEBUG("I2C: otb_i2c_ads_read exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_set_mode(uint8_t addr, uint16_t mode)
{
  bool rc = FALSE;
  int ii;

  // Mode is a 2 byte value - so 2 registers
  for (ii = 0; ii < 2; ii++)
  {
    rc = otb_i2c_pca9685_write_one_reg(addr,
                                       OTB_I2C_PCA9685_REG_MODE1 + ii,
                                       (uint8_t)(mode & 0xFF));
    if (!rc)
    {
      goto EXIT_LABEL;
    }
    mode = mode >> 8;
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_write_one_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
  bool rc = FALSE;

  otb_i2c_pca9685_start();
  
  rc = otb_i2c_pca9685_call(addr);
  if (!rc)
  {
    WARN("PCA9685: Failed to call address %02x", addr);
    goto EXIT_LABEL;
  }
  
  rc = otb_i2c_pca9685_reg(reg);
  if (!rc)
  {
    WARN("PCA9685: Failed to write reg %02x", reg);
    goto EXIT_LABEL;
  }
  
  rc = otb_i2c_pca9685_val(val);
  if (!rc)
  {
    WARN("PCA9685: Failed to write val %02x", val);
    goto EXIT_LABEL;
  }
  otb_i2c_pca9685_stop();
  
  rc = TRUE;
  
EXIT_LABEL:

  return rc;
}

void ICACHE_FLASH_ATTR otb_i2c_pca9685_start()
{
  INFO("PCA9685: Start I2C bus");

  i2c_master_start();
  
  return;
}

void ICACHE_FLASH_ATTR otb_i2c_pca9685_stop()
{
  INFO("PCA9685: Stop I2C bus");

  i2c_master_stop();
  
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_call(uint8_t addr)
{
  bool rc = FALSE;

  INFO("PCA9685: Call addr: %02x", addr);

  // Write the address and check for ack
  i2c_master_writeByte((addr << 1) | 0b0);
  if (!i2c_master_checkAck())
  {
    INFO("I2C: No ack");
    goto EXIT_LABEL;
  }

  rc = TRUE;

EXIT_LABEL:

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_reg(uint8_t reg)
{
  bool rc = FALSE;

  INFO("PCA9685: Write reg: %02x", reg);

  // Write register
  i2c_master_writeByte(reg);
  if (!i2c_master_checkAck())
  {
    INFO("I2C: No ack");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;

EXIT_LABEL:

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_val(uint8_t val)
{
  bool rc = FALSE;

  INFO("PCA9685: Write val: %02x", val);

  // Write  value
  i2c_master_writeByte(val);
  if (!i2c_master_checkAck())
  {
    INFO("I2C: No ack");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;

EXIT_LABEL:

  return rc;
}
