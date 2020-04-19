/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017-2020 Piers Finlayson
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

#define OTB_BRZO_I2C
#include "otb.h"
#include "brzo_i2c.h"

MLOG("I2C");

bool otb_brzo_i2c_inited = FALSE;
uint32_t otb_brzo_i2c_clock_stretch_time_out_usec = 0;
extern uint8_t i2c_error;

// Wrap brzo_i2c_setup so can be called multiple times without reinitializing (which
// will be done if clock stretch timeout changes)
#ifndef OTB_RBOOT_BOOTLOADER
void ICACHE_FLASH_ATTR otb_brzo_i2c_setup(uint32_t clock_stretch_time_out_usec)
#else
void otb_brzo_i2c_setup(uint32_t clock_stretch_time_out_usec)
#endif
{
  ENTRY;
  
  if ((!otb_brzo_i2c_inited) ||
      (clock_stretch_time_out_usec != otb_brzo_i2c_clock_stretch_time_out_usec))
  {
    brzo_i2c_setup(clock_stretch_time_out_usec);
    otb_brzo_i2c_inited = TRUE;
    otb_brzo_i2c_clock_stretch_time_out_usec = clock_stretch_time_out_usec;
    MDEBUG("I2C Stack initialized: %d", clock_stretch_time_out_usec);
  }
  
  EXIT;  

  return;
}

