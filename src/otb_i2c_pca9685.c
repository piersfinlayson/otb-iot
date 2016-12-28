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
#include "brzo_i2c.h"

void ICACHE_FLASH_ATTR otb_i2c_pca9685_test_timerfunc(void)
{
  bool rc;
  
  DEBUG("I2C: otb_i2c_pca9685_test_timer_func entry");

  if (otb_i2c_pca9685_led_on)
  {
    // Off
    INFO("PCA9685: LED on - turn it off");
    rc = otb_i2c_pca9685_led_conf(otb_i2c_pca9685_test_addr, 15, 0, OTB_I2C_PCA9685_IO_FULL_OFF);
    if (!rc)
    {
      WARN("PCA9685: Failed to communicate with PCA9685");
    }
    otb_i2c_pca9685_led_on = FALSE;
  }
  else
  {
    // On
    INFO("PCA9685: LED off - turn it on");
    rc = otb_i2c_pca9685_led_conf(otb_i2c_pca9685_test_addr, 15, OTB_I2C_PCA9685_IO_FULL_ON, 0);
    if (!rc)
    {
      WARN("PCA9685: Failed to communicate with PCA9685");
    }
    otb_i2c_pca9685_led_on = TRUE;
  }
  
  DEBUG("I2C: otb_i2c_pca9685_test_timer_func entry");

  return;
}

void ICACHE_FLASH_ATTR otb_i2c_pca9685_test_init(void)
{
  bool rc = FALSE;

  DEBUG("I2C: otb_i2c_pca9685_test_init entry");

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
  
  DEBUG("I2C: otb_i2c_pca9685_test_init exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_led_conf(uint8_t addr, uint8_t led, uint16_t on, uint16_t off)
{
  bool rc = FALSE;
  uint8_t byte[4];
  int ii;
  uint8_t reg;
  
  DEBUG("I2C: otb_i2c_pca9685_led_conf entry");

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
  
#if 0  
  byte[0] = on & 0xff;
  byte[1] = on >> 8;
  byte[2] = off & 0xff;
  byte[3] = off >> 8;

  OTB_ASSERT((led >= 0) && (led < OTB_I2C_PCA9685_REG_IO_NUM)); 
  //reg = OTB_I2C_PCA9685_REG_IO0_ON_L + (led * 4);
  reg = 0xfa;
  rc = otb_i2c_write_reg_seq(addr, reg, 4, byte);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
#endif
  unsigned char bytes[5];
  bytes[0] = 0xfa;
  bytes[1] = on & 0xff;
  bytes[2] = on >> 8;
  bytes[3] = off & 0xff;
  bytes[4] = off >> 8;
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(bytes, 5, FALSE);
  rc = brzo_i2c_end_transaction(); 
  if (rc)
  {
    INFO("Failed %d", rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:
  
  DEBUG("I2C: otb_i2c_pca9685_led_conf exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_init(uint8_t addr)
{
  bool rc = FALSE;
  uint8_t read_mode[2];
  uint16_t mode;

  DEBUG("I2C: otb_i2c_pca9685_init entry");

  // XXX This is bugged - for some reason the second read doesn't work
  // Read the mode
  rc = otb_i2c_read_reg_seq(addr, OTB_I2C_PCA9685_REG_MODE1, 2, read_mode);
  if (!rc)
  {
    WARN("PCA9685: Failed to read mode");
    goto EXIT_LABEL;
  }
  mode = read_mode[0] & (read_mode[1] << 8);
  INFO("PCA9685: Read mode 0x%04x", mode);
  
#if 0  
  // Set prescale (to default for now)
  rc = otb_i2c_write_one_reg(addr,
                             OTB_I2C_PCA9685_REG_PRE_SCALE,
                             OTB_I2C_PCA9685_PRESCALE_DEFAULT);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
#endif
  
  // Set the mode.  Key thing to change is that auto-incrementing of registers is
  // defaulted - this allows us to write more than one register with successive writes
  // without restarting comms for each one (speeding things up).
  mode = 0b00100001;
  unsigned char bytes[2];
#if 0  
  //mode |= OTB_I2C_PCA9685_MODE_AI_REG;
  //mode |= OTB_I2C_PCA9685_MODE_AI_REG | OTB_I2C_PCA9685_MODE_INVERT;
  //mode &= ~OTB_I2C_PCA9685_MODE_TOTEM;
  //rc = otb_i2c_pca9685_set_mode(addr, mode);
  rc = otb_i2c_write_reg_seq(addr,
                             OTB_I2C_PCA9685_REG_MODE1,
                             1,
                             &mode);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
#endif

  bytes[0] = 0;  // general call address
  bytes[1] = 0b00000110; // SWRST data
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(bytes, 2, FALSE);
  rc = brzo_i2c_end_transaction();
  if (rc)
  {
    INFO("sqrst failed: %d", rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }

  bytes[0] = 0x00; // MODE1 register
  bytes[1] = 0b00100001; // reset = 1, AI = 1, sleep = 0, allcall = 1
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(bytes, 2, FALSE);
  rc = brzo_i2c_end_transaction();
  if (rc)
  {
    INFO("Failed: %d", rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("I2C: otb_i2c_pca9685_init exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_set_mode(uint8_t addr, uint16_t mode)
{
  bool rc = FALSE;
  int ii;
  uint8_t mode_byte[2];

  DEBUG("I2C: otb_i2c_pca9685_set_mode entry");

  mode_byte[0] = mode & 0xff;
  mode_byte[1] = mode >> 8;
  rc = otb_i2c_write_reg_seq(addr,
                             OTB_I2C_PCA9685_REG_MODE1,
                             2,
                             mode_byte);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("I2C: otb_i2c_pca9685_set_mode exit");

  return rc;
}




