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

#define OTB_EEPROM_C
#include "otb.h"

MLOG("otb-eeprom");

void otb_eeprom_read(void)
{
  bool rc;
  bool i2c_bus_init = FALSE;

  ENTRY;

  // Initialize internal I2C bus
  rc = otb_i2c_initialize_bus_internal();
  if (!rc)
  {
    MWARN("Failed to initialize internal I2C bus");
    goto EXIT_LABEL;
  }
  i2c_bus_init = TRUE;

  // Read EEPROM
  rc = otb_eeprom_data_read();
  if (!rc)
  {
    MWARN("Failed to read EEPROM");
    goto EXIT_LABEL;
  }

  // Do something with EEPROM data
  rc = otb_eeprom_data_process();
  if (!rc)
  {
    MWARN("Failed to process EEPROM data");
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  if (i2c_bus_init)
  {
    // Uninitialize internal I2C bus
    otb_i2c_uninitialize_bus_internal();
  }

  EXIT;

  return;
}

bool otb_eeprom_data_read(void)
{
  bool success = FALSE;
  uint8_t val[4];
  uint32_t magic;
  int ii;
  bool match;

  ENTRY;

  success = otb_i2c_24xxyy_init(OTB_EEPROM_MAIN_BOARD_ADDR, &otb_i2c_bus);
  if (!success)
  {
    goto EXIT_LABEL;
  }

  success = otb_i2c_24xx128_read_data(OTB_EEPROM_MAIN_BOARD_ADDR,
                                      0x0,
                                      val,
                                      4,
                                      &otb_i2c_bus);
  if (!success)
  {
    goto EXIT_LABEL;
  }

  magic = OTB_EEPROM_INFO_MAGIC;
  match = TRUE;
  for (ii = 0; ii < 4; ii++)
  {
    MDEBUG("Checking byte %d eeprom: 0x%02x magic 0x%02x", ii, val[ii], ((magic >> (8*ii)) & 0xff));
    if (val[ii] != ((magic >> (8*ii)) & 0xff))
    {
      match = FALSE;
      break;
    }
  }

  if (match)
  {
    MDETAIL("Found otb-iot format eeprom");
  }

EXIT_LABEL:

  EXIT;

  return success;
}

bool otb_eeprom_data_process(void)
{
  bool success = FALSE;

  ENTRY;

  EXIT;

  return success;
}
