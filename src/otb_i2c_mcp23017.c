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

#define OTB_I2C_MCP23017_C
#include "otb.h"

MLOG("MCP23017");

void ICACHE_FLASH_ATTR otb_i2c_mcp23017_output_regs(uint8_t addr, brzo_i2c_info *info)
{
  bool rc;
  uint8_t conf;
  for (int ii = 0; ii < 0x20; ii++)
  {
    rc = otb_i2c_read_one_reg_info(addr, ii, &conf, info);
    if (!rc)
    {
      MWARN("Failed to read conf");
      goto EXIT_LABEL;
    }
    MDETAIL("Reg: 0x%02x 0x%02x", ii, conf);
  }
  EXIT_LABEL:
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_mcp23017_init(uint8_t addr, brzo_i2c_info *info)
{
  bool rc = FALSE;
  uint8_t conf;

  ENTRY;

  // Set the mode
  conf = 0;
  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_IOCON1, conf, info);
  if (!rc)
  {
    MWARN("Failed to write a reg");
    goto EXIT_LABEL;
  }

  // Set direction of all ports to output  
  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_IODIRA, 0, info);
  if (!rc)
  {
    MWARN("Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_GPIOA, 0, info);
  if (!rc)
  {
    MWARN("Failed to write a reg");
    goto EXIT_LABEL;
  }

  // Set direction of all ports to output  
  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_IODIRB, 0, info);
  if (!rc)
  {
    MWARN("Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_GPIOB, 0, info);
  if (!rc)
  {
    MWARN("Failed to write a reg");
    goto EXIT_LABEL;
  }

  rc = TRUE;
  
EXIT_LABEL:

  EXIT;

  return rc;
}

// GPA and GPB format: 0b76543210
bool ICACHE_FLASH_ATTR otb_i2c_mcp23017_write_gpios(uint8_t gpa, uint8_t gpb, uint8_t addr, brzo_i2c_info *info)
{
  bool rc = FALSE;

  ENTRY;

  MDEBUG("Write 0x%02x 0x%02x", gpa, gpb);

  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_GPIOA, gpa, info);
  if (!rc)
  {
    MWARN("Failed to write GPA value");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_write_one_reg_info(addr, OTB_I2C_MCP23017_REG_GPIOB, gpb, info);
  if (!rc)
  {
    MWARN("Failed to write GPB value");
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  EXIT;

  return rc;
}

// GPA and GPB format: 0b76543210
bool ICACHE_FLASH_ATTR otb_i2c_mcp23017_read_gpios(uint8_t *gpa, uint8_t *gpb, uint8_t addr, brzo_i2c_info *info)
{
  bool rc = FALSE;

  ENTRY;

  rc = otb_i2c_read_reg_seq_info(addr, OTB_I2C_MCP23017_REG_GPIOA, 1, gpa, info);
  if (!rc)
  {
    MWARN("Failed to read GPA value");
    goto EXIT_LABEL;
  }

  rc = otb_i2c_read_reg_seq_info(addr, OTB_I2C_MCP23017_REG_GPIOB, 1, gpb, info);
  if (!rc)
  {
    MWARN("Failed to read GPB value");
    goto EXIT_LABEL;
  }

  MDEBUG("Read  0x%02x 0x%02x", *gpa, *gpb);

EXIT_LABEL:

  EXIT;

  return rc;
}





