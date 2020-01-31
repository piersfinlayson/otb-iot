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

#define OTB_I2C_24XXYY_C
#include "otb.h"
#include "brzo_i2c.h"

#ifdef OTB_RBOOT_BOOTLOADER
#undef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif // OTB_RBOOT_BOOTLOADER

void ICACHE_FLASH_ATTR otb_i2c_24xxyy_test_timerfunc(void)
{
  bool rc;
  uint8_t buf[1];
  uint8_t word_addr = 0;
  
  DEBUG("24XXYY: otb_i2c_24xxyy_test_timer_func entry");

  if (otb_i2c_24xxyy_written)
  {
    // Off
    DETAIL("24XXYY: Time to read");
    buf[0] = 0;
    rc = otb_i2c_24xxyy_read_bytes(otb_i2c_24xxyy_test_addr, word_addr, buf, 1, &otb_i2c_bus_internal);
    if (!rc)
    {
      WARN("24XXYY: Failed to communicate with 24XXYY");
    }
    else
    {
      DETAIL("24XXYY: Read byte 0x%02x", buf[0]);
    }
    
    // Now toggle back
    otb_i2c_24xxyy_written = FALSE;
  }
  else
  {
    // On
    DETAIL("24XXYY: Time to write");
    buf[0] = otb_i2c_24xxyy_next_byte;
    rc = otb_i2c_24xxyy_write_bytes(otb_i2c_24xxyy_test_addr, word_addr, buf, 1, &otb_i2c_bus_internal);
    if (!rc)
    {
      WARN("24XXYY: Failed to communicate with 24XXYY");
    }
    else
    {
      DETAIL("24XXYY: Written byte 0x%02x", otb_i2c_24xxyy_next_byte);
      otb_i2c_24xxyy_next_byte++;
    }
    otb_i2c_24xxyy_written = TRUE;
  }
  
  DEBUG("24XXYY: otb_i2c_24xxyy_test_timer_func entry");

  return;
}

#ifndef OTB_RBOOT_BOOTLOADER

void ICACHE_FLASH_ATTR otb_i2c_24xxyy_test_init(void)
{
  bool rc = FALSE;

  DEBUG("24XXYY: otb_i2c_24xxyy_test_init entry");

  // Start off with a read
  otb_i2c_24xxyy_written = TRUE;
  otb_i2c_24xxyy_next_byte = 0;
  otb_i2c_24xxyy_test_addr = OTB_I2C_24XXYY_BASE_ADDR;
  rc = otb_i2c_24xxyy_init(otb_i2c_24xxyy_test_addr, &otb_i2c_bus_internal);
  if (!rc)
  {
    WARN("24XXYY: Failed to init 24XXYY at address 0x%02x", otb_i2c_24xxyy_test_addr);
    goto EXIT_LABEL;
  }
  
  otb_util_timer_set((os_timer_t*)&otb_i2c_24xxyy_test_timer, 
                     (os_timer_func_t *)otb_i2c_24xxyy_test_timerfunc,
                     NULL,
                     1000,
                     1);
                     
  DETAIL("24XXYY: Initialized test");
  
EXIT_LABEL:
  
  DEBUG("24XXYY: otb_i2c_24xxyy_test_init exit");

  return;
}

#endif // OTB_RBOOT_BOOTLOADER

bool ICACHE_FLASH_ATTR otb_i2c_24xxyy_read_bytes(uint8_t addr, uint8_t word_addr, uint8_t *bytes, uint8_t num_bytes, brzo_i2c_info *info)
{
  bool rc = FALSE;
  uint8_t brzo_rc;
  int ii;
  uint8_t reg;
  
  DEBUG("24XXYY: otb_i2c_24xxyy_read_bytes entry");

  // Need to write underlying functions to do sequential reads
  for (ii = 0; ii < num_bytes; ii++)
  {
    // First of all write the address 
    brzo_i2c_start_transaction_info(addr, 100, info);
    reg = word_addr+ii;
    brzo_i2c_write_info(&reg, 1, FALSE, info);
    brzo_rc = brzo_i2c_end_transaction_info(info);
    if (!brzo_rc)
    {
      rc = TRUE;
    }
    if (!rc)
    {
      DETAIL("24XXYY: Failed to write addr: 0x%02x", word_addr + ii);
      goto EXIT_LABEL;
    }
  
    brzo_i2c_start_transaction_info(addr, 100, info);
    brzo_i2c_read_info(bytes+ii, 1, FALSE, info);
    brzo_rc = brzo_i2c_end_transaction_info(info);
    if (!brzo_rc)
    {
      rc = TRUE;
    }
    if (!rc)
    {
      DETAIL("24XXYY: Failed to read addr: 0x%02x", word_addr + ii);
      goto EXIT_LABEL;
    }
  }

  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("24XXYY: otb_i2c_24xxyy_read_bytes exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_24xxyy_write_bytes(uint8_t addr, uint8_t word_addr, uint8_t *bytes, uint8_t num_bytes, brzo_i2c_info *info)
{
  bool rc = FALSE;
  int ii;
  int jj;
  uint8_t brzo_rc;
  uint8_t buf[2];
  uint8_t reg;
  
  DEBUG("24XXYY: otb_i2c_24xxyy_write_bytes entry");

  // Need to write underlying functions to do sequential reads
  for (ii = 0; ii < num_bytes; ii++)
  {
    buf[0] = word_addr;
    buf[1] = bytes[ii];
    brzo_i2c_start_transaction_info(addr, 100, info);
    brzo_i2c_write_info(buf, 2, FALSE, info);
    brzo_rc = brzo_i2c_end_transaction_info(info);
    if (!brzo_rc)
    {
      rc = TRUE;
    }
    if (!rc)
    {
      DETAIL("24XXYY: Failed to write addr: 0x%02x", word_addr + ii);
      goto EXIT_LABEL;
    }
    
    // Now do ack polling to check write succeeded
    rc = FALSE;
    jj = 0;
    while (!rc & jj < 100)
    {
      brzo_i2c_start_transaction_info(addr, 100, info);
      reg = word_addr+ii;
      brzo_i2c_write_info(&reg, 1, FALSE, info);
      brzo_rc = brzo_i2c_end_transaction_info(info);
      if (!brzo_rc)
      {
        rc = TRUE;
      }
      jj++;
    }
    if (!rc)
    {
      DETAIL("24XXYY: Device failed to perform write");
      break;
    }
    else
    {
      DETAIL("24XXYY: Polls: %d", jj+1);
    }
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("24XXYY: otb_i2c_24xxyy_write_bytes exit");

  return rc;
}

#ifdef OTB_RBOOT_BOOTLOADER
bool otb_i2c_24xxyy_init(uint8_t addr, brzo_i2c_info *info)
#else
bool ICACHE_FLASH_ATTR otb_i2c_24xxyy_init(uint8_t addr, brzo_i2c_info *info)
#endif // OTB_RBOOT_BOOTLOADER
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  uint8_t val;

  DEBUG("24XXYY: otb_i2c_24xxyy_init entry");

  // Just try and read the current word (0x0?)
  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_read_info(&val, 1, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  else
  {
    DETAIL("24XXYY: Failed: %d", brzo_rc);
  }
  
EXIT_LABEL:

  DEBUG("24XXYY: otb_i2c_24xxyy_init exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_24xx128_read_data(uint8_t addr, uint16_t start_addr, uint16_t bytes, uint8_t *buf, brzo_i2c_info *info)
{
  bool rc = FALSE;
  uint8_t brzo_rc;
  uint8_t start_addr_b[2];

  // Reading is achieved as follows:
  // Perform a write operation, with the MSB folllowed by LSB of start_addr
  // Perform read operations, for the total number of bytes required

  DEBUG("24XXYY: otb_i2c_24xx128_read_data entry");
  
  // 16 KB (128kbit) eeprom
  OTB_ASSERT(bytes <= (128*1024/8));

  start_addr_b[0] = start_addr >> 8;
  start_addr_b[1] = start_addr & 0xff;
  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_write_info(start_addr_b, 2, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (brzo_rc)
  {
    DEBUG("24XXYY: write of address to read from failed: %d", brzo_rc);
    goto EXIT_LABEL;
  }
  
  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_read_info(buf, bytes, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (brzo_rc)
  {
    DEBUG("24XXYY: read failed: %d", brzo_rc);
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:
  
  DEBUG("24XXYY: otb_i2c_24xx128_read_data exit");
  
  return rc;
}
