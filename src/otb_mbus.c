/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2020 Piers Finlayson
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
 * 
 */

#define OTB_MBUS_C
#include "otb.h"

//
// trigger
//   mbus
//     init    // Enables the M-Bus Hat, disables logging
//     enable  // Powers on the M-Bus
//     disable // Powers off the M-Bus
//     deinit  // Disables the M-Bus Hat, re-enables logging
//     send    // not implemented
//       XXXXXXXX
//     recv    // not implemented

bool ICACHE_FLASH_ATTR otb_mbus_hat_init(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;

  // DEBUG("SERIAL: otb_mbus_hat_init entry");
  
  if (!otb_mbus_hat_installed)
  {
    otb_cmd_rsp_append("hat not installed");
  }

  if (otb_mbus_enabled)
  {
    otb_cmd_rsp_append("enabled");
    goto EXIT_LABEL;
  }

  if (otb_mbus_inited)
  {
    otb_cmd_rsp_append("already initialized");
    goto EXIT_LABEL;
  }

  // The M-Bus Master Hat uses the TX/RX pins so disable logging
  otb_util_disable_logging();

  // Need to use the MCP23017 which is on the internal I2C bus
  otb_mbus_i2c_bus = &otb_i2c_bus_internal;
  otb_mbus_mcp23017_addr = OTB_MBUS_MCP23017_ADDR;
  otb_i2c_initialize_bus_internal();
  otb_i2c_mcp23017_init(otb_mbus_mcp23017_addr, otb_mbus_i2c_bus);
  otb_mbus_inited = TRUE;
  rc = TRUE;

EXIT_LABEL:

  // DEBUG("SERIAL: otb_mbus_hat_init exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_deinit(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  // DEBUG("SERIAL: otb_mbus_hat_deinit entry");
  
  if (otb_mbus_enabled)
  {
    otb_cmd_rsp_append("enabled");
    goto EXIT_LABEL;
  }

  if (!otb_mbus_inited)
  {
    otb_cmd_rsp_append("not initialized");
    goto EXIT_LABEL;
  }

  otb_util_enable_logging();
  otb_mbus_inited = FALSE;
  rc = TRUE;

EXIT_LABEL:

  // DEBUG("SERIAL: otb_mbus_hat_deinit exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_enable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint8_t gpa, gpb;

  DEBUG("SERIAL: otb_mbus_hat_enable entry");

  if (!otb_mbus_inited)
  {
    otb_cmd_rsp_append("not initialized");
    goto EXIT_LABEL;
  }

  if (otb_mbus_enabled)
  {
    otb_cmd_rsp_append("already enabled");
    goto EXIT_LABEL;
  }

  // Need to enable GPIO26 (GPA0 on MCP23017)  
  gpa = 1; // GPA0 = 1
  gpb = 0;
  otb_i2c_mcp23017_write_gpios(gpa,
                              gpb,
                              otb_mbus_mcp23017_addr,
                              otb_mbus_i2c_bus);
  rc = TRUE;
  otb_mbus_enabled = TRUE;

EXIT_LABEL:

  DEBUG("SERIAL: otb_mbus_hat_enable exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_disable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint8_t gpa, gpb;

  DEBUG("SERIAL: otb_mbus_hat_disable entry");

  // Need to enable GPIO26 (GPA0 on MCP23017)  

  if (otb_mbus_enabled)
  {
    gpa = 0; // GPA0 = 0
    gpb = 0;
    otb_i2c_mcp23017_write_gpios(gpa,
                                gpb,
                                otb_mbus_mcp23017_addr,
                                otb_mbus_i2c_bus);
    otb_mbus_enabled = FALSE;
    rc = TRUE;
  }
  else
  {
    otb_cmd_rsp_append("not enabled");
  }

  DEBUG("SERIAL: otb_mbus_hat_disable exit");
  
  return rc;
}

