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

#ifndef OTB_MBUS_H_INCLUDED
#define OTB_MBUS_H_INCLUDED

#define OTB_MBUS_MCP23017_ADDR 0x20

extern bool otb_mbus_hat_installed;

#ifdef OTB_MBUS_C
bool otb_mbus_hat_installed = FALSE;
brzo_i2c_info *otb_mbus_i2c_bus;
uint8_t otb_mbus_mcp23017_addr;
bool otb_mbus_enabled = FALSE;
#endif // OTB_MBUS_C

bool otb_mbus_hat_enable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_mbus_hat_disable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);


#endif // OTB_MBUS_H_INCLUDED