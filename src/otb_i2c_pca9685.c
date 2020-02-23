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

#define OTB_I2C_PCA9685_C
#include "otb.h"
#include "brzo_i2c.h"

MLOG("PCA9685");

// TRIGGER - just sets a gpio
// SET - SDA or SCL, valid pin provided
// SET - addr, need to get next_cmd

bool ICACHE_FLASH_ATTR otb_i2c_pca_gpio_cmd(unsigned char *next_cmd,
                                            void *arg,
                                            unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  otb_conf_ads *ads = NULL;
  char *pin_t;
  int8_t pin;
  int8_t value;
  bool store_conf = FALSE;
  uint8_t addr_b;
    
  ENTRY;
  
  // Double check what we're being asked to do is valid
  cmd = (int)arg;

  MDEBUG("cmd: 0x%08x", cmd);

  OTB_ASSERT(((cmd & OTB_CMD_GPIO_TRIGGER) &&
              ((cmd & OTB_CMD_GPIO_PCA_INIT) ||
               (cmd & OTB_CMD_GPIO_PCA_GPIO))) ||
             ((cmd & OTB_CMD_GPIO_SET_CONFIG) &&
              ((cmd & OTB_CMD_GPIO_PCA_SDA) ||
               (cmd & OTB_CMD_GPIO_PCA_SCL) ||
               (cmd & OTB_CMD_GPIO_PCA_STORE_CONF) ||
               (cmd & OTB_CMD_GPIO_PCA_ADDR))));

  if (cmd & OTB_CMD_GPIO_PCA_GPIO)
  {
    pin = atoi(prev_cmd);
    if ((pin < 0) || (pin >= OTB_I2C_PCA9685_NUM_PINS))
    {
      rc = FALSE;
      otb_cmd_rsp_append("Invalid pin");
      goto EXIT_LABEL;
    }
    if (cmd & OTB_CMD_GPIO_SET_CONFIG)
    {
      if (next_cmd == NULL)
      {
        rc = FALSE;
        otb_cmd_rsp_append("No value provided");
        goto EXIT_LABEL;
      }
      value = atoi(next_cmd);
      if ((value < 0) ||
          (value > 1) ||
          ((value == 0) && (next_cmd[0] != '0')))
      {
        rc = FALSE;
        otb_cmd_rsp_append("Invalid val provided");
        goto EXIT_LABEL;
      }
    }
  }
  else if ((cmd & OTB_CMD_GPIO_PCA_SDA) ||
           (cmd & OTB_CMD_GPIO_PCA_SCL))
  {
    // No need to check whether the pin is valid or reserved - this has already been done
    pin = otb_gpio_get_pin(prev_cmd);
  }
  else if (cmd & OTB_CMD_GPIO_PCA_ADDR)
  {
    // Get address
    if (next_cmd != NULL)
    {
      if (!otb_i2c_mqtt_get_addr(next_cmd, &addr_b) ||
          (addr_b < OTB_I2C_PCA9685_BASE_ADDR ||
          (addr_b >= (OTB_I2C_PCA9685_BASE_ADDR + OTB_I2C_PCA9685_NUM_ADDRS))))
      {
        rc = FALSE;
        otb_cmd_rsp_append("invalid PCA address");
        goto EXIT_LABEL;
      }
    }
    else
    {
      rc = FALSE;
      otb_cmd_rsp_append("no address provided");
      goto EXIT_LABEL;
    }
  }
  
  switch(cmd)
  {
    case OTB_CMD_GPIO_TRIGGER|OTB_CMD_GPIO_PCA_INIT:
      if ((otb_i2c_pca9685_sda == -1) || (otb_i2c_pca9685_scl == -1))
      {
        otb_cmd_rsp_append("SDA or SCL unset");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      else
      {
        // Initialize the I2C stack
        otb_gpio_set(otb_i2c_pca9685_sda, 1, FALSE);
        otb_gpio_set(otb_i2c_pca9685_scl, 1, FALSE);
        otb_i2c_initialize_bus(&otb_i2c_pca9685_brzo_i2c_info, otb_i2c_pca9685_sda, otb_i2c_pca9685_scl);
        os_memset(otb_i2c_pca9685_desired_state, 0, sizeof(uint8_t));
        os_memset(otb_i2c_pca9685_current_state, 0, sizeof(uint8_t));

        // Initialize the PCA9685
        rc = otb_i2c_pca9685_init2();
        if (!rc)
        {
          otb_cmd_rsp_append("Failed to init PCA9685");
          goto EXIT_LABEL;
        }

        // Set PCA9685 pins to desired values
        rc = otb_i2c_pca9685_set2();
        if (!rc)
        {
          otb_cmd_rsp_append("Failed to init PCA9685");
          goto EXIT_LABEL;
        }

        otb_i2c_pca9685_inited = TRUE;
        rc = TRUE;
        DEBUG("PCA9685 I2C stack inited");
      }
      break;

    case OTB_CMD_GPIO_TRIGGER|OTB_CMD_GPIO_PCA_GPIO:
      // Have pin and value
      if (!otb_i2c_pca9685_inited)
      {
        otb_cmd_rsp_append("Not inited");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      otb_i2c_pca9685_desired_state[pin] = value;
      MDEBUG("set pin %d to value %d", pin, value);
      rc = otb_i2c_pca9685_set2();
      if (!rc)
      {
        otb_cmd_rsp_append("Error setting pin %d to %d", pin, value);
        rc = FALSE;
        goto EXIT_LABEL;
      }
      rc = TRUE;
      break;
      
    case OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_ADDR:
      MDEBUG("Use addr: 0x%02x", addr_b);
      otb_i2c_pca9685_addr = addr_b;
      rc = TRUE;
      break;
    
    case OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_SDA:
      MDEBUG("SDA pin: %d", pin);
      otb_i2c_pca9685_sda = pin;
      rc = TRUE;
      break;

    case OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_SCL:
      MDEBUG("SCL pin: %d", pin);
      otb_i2c_pca9685_scl = pin;
      rc = TRUE;
      break;

    case OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_STORE_CONF:
      //  XXX unimplemented
      // store_conf = TRUE;
      otb_cmd_rsp_append("Unimplemented");
      rc = FALSE;
      goto EXIT_LABEL;
      break;
      
    default:
      OTB_ASSERT(FALSE);
      break;
  }

EXIT_LABEL:

  // If successful store off new config
  if (store_conf)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }

  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_valid_pin(unsigned char *to_match)
{
  bool rc = FALSE;
  int8_t pin;
  
  ENTRY;

  pin = atoi(to_match);

  if ((pin < 0) || (pin >= OTB_I2C_PCA9685_NUM_PINS))
  {
    goto EXIT_LABEL;
  }

  rc = TRUE;  

EXIT_LABEL:  

  EXIT;

 return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_init2()
{
  bool rc = FALSE;
  uint8_t bytes[5];
  uint8_t brzo_rc;

  ENTRY;

  // Set the mode
  bytes[0] = 0x00; // MODE1 register
  bytes[1] = 0b00100001; // reset = 1, AI = 1, sleep = 0, allcall = 1
  brzo_i2c_start_transaction_info(otb_i2c_pca9685_addr, 100, &otb_i2c_pca9685_brzo_i2c_info);
  brzo_i2c_write_info(bytes, 2, FALSE, &otb_i2c_pca9685_brzo_i2c_info);
  brzo_rc = brzo_i2c_end_transaction_info(&otb_i2c_pca9685_brzo_i2c_info);
  if (brzo_rc)
  {
    MWARN("Failed to set PCA9685 mode: %d", brzo_rc);
    goto EXIT_LABEL;
  }

  rc = TRUE;

EXIT_LABEL:

  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_set2()
{
  bool rc = FALSE;
  uint8_t bytes[5];
  uint8_t brzo_rc;
  int ii;
  uint8_t desired_state;

  ENTRY;

  bytes[1] = 0b0;
  bytes[3] = 0b0;
  for (ii = 0; ii < OTB_I2C_PCA9685_NUM_PINS; ii++)
  {
    desired_state = otb_i2c_pca9685_desired_state[ii] ? 1 : 0;
    bytes[0] = OTB_I2C_PCA9685_REG_IO0_ON_L + (ii * 4);
    bytes[2] = 0b0;
    bytes[4] = 0b0;
    if (desired_state)
    {
      bytes[2]= 0b00010000;
    }
    else
    {
      bytes[4]= 0b00010000;
    }
    brzo_i2c_start_transaction_info(otb_i2c_pca9685_addr, 100, &otb_i2c_pca9685_brzo_i2c_info);
    brzo_i2c_write_info(bytes, 5, FALSE, &otb_i2c_pca9685_brzo_i2c_info);
    brzo_rc = brzo_i2c_end_transaction_info(&otb_i2c_pca9685_brzo_i2c_info);
    if (brzo_rc)
    {
      MDETAIL("Failed to set pin: %d, rc: %d", ii, brzo_rc);
      rc = FALSE;
      goto EXIT_LABEL;
    }
    otb_i2c_pca9685_current_state[ii] = desired_state;
    bytes[2] = 0b0;
    bytes[4] = 0b0;
  }

  rc = TRUE;

EXIT_LABEL:

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_i2c_pca9685_test_timerfunc(void)
{
  bool rc;
  
  ENTRY;

  if (otb_i2c_pca9685_led_on)
  {
    // Off
    MDETAIL("LED on - turn it off");
    rc = otb_i2c_pca9685_led_conf(otb_i2c_pca9685_test_addr, 15, 0, OTB_I2C_PCA9685_IO_FULL_OFF);
    if (!rc)
    {
      MWARN("Failed to communicate with PCA9685");
    }
    otb_i2c_pca9685_led_on = FALSE;
  }
  else
  {
    // On
    MDETAIL("LED off - turn it on");
    rc = otb_i2c_pca9685_led_conf(otb_i2c_pca9685_test_addr, 15, OTB_I2C_PCA9685_IO_FULL_ON, 0);
    if (!rc)
    {
      MWARN("Failed to communicate with PCA9685");
    }
    otb_i2c_pca9685_led_on = TRUE;
  }
  
  ENTRY;

  return;
}

void ICACHE_FLASH_ATTR otb_i2c_pca9685_test_init(void)
{
  bool rc = FALSE;

  ENTRY;

  otb_i2c_pca9685_led_on = FALSE;
  otb_i2c_pca9685_test_addr = OTB_I2C_PCA9685_BASE_ADDR;
  rc = otb_i2c_pca9685_init(otb_i2c_pca9685_test_addr);
  if (!rc)
  {
    MWARN("Failed to init PCA9685 at address 0x%02x", otb_i2c_pca9685_test_addr);
    goto EXIT_LABEL;
  }
  
  otb_util_timer_set((os_timer_t*)&otb_i2c_pca9685_test_timer, 
                     (os_timer_func_t *)otb_i2c_pca9685_test_timerfunc,
                     NULL,
                     1000,
                     1);
                     
  MDETAIL("Initialized test");
  
EXIT_LABEL:
  
  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_led_conf(uint8_t addr, uint8_t led, uint16_t on, uint16_t off)
{
  bool rc = FALSE;
  uint8_t byte[4];
  int ii;
  uint8_t reg;
  
  ENTRY;

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
    MDETAIL("Failed %d", rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:
  
  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_init(uint8_t addr)
{
  bool rc = FALSE;
  uint8_t read_mode[2];
  uint16_t mode;

  ENTRY;

  // XXX This is bugged - for some reason the second read doesn't work
  // Read the mode
  rc = otb_i2c_read_reg_seq(addr, OTB_I2C_PCA9685_REG_MODE1, 2, read_mode);
  if (!rc)
  {
    MWARN("Failed to read mode");
    goto EXIT_LABEL;
  }
  mode = read_mode[0] & (read_mode[1] << 8);
  MDETAIL("Read mode 0x%04x", mode);
  
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
    MDETAIL("sqrst failed: %d", rc);
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
    MDETAIL("Failed: %d", rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:

  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_pca9685_set_mode(uint8_t addr, uint16_t mode)
{
  bool rc = FALSE;
  int ii;
  uint8_t mode_byte[2];

  ENTRY;

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

  EXIT;

  return rc;
}




