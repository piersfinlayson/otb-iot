/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017-2018 Piers Finlayson
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
 
#ifndef OTB_MEZ_BOARD_NIXIE_V0_2_H_INCLUDED
#define OTB_MEZ_BOARD_NIXIE_V0_2_H_INCLUDED

// This is the default hardware configuration for otbiot if no eeprom info is
// provided
#if defined(OTB_EEPROM_C) || defined(OTB_HWINFO_C)

const otb_hwinfo_main_module_info otb_hwinfo_mez_board_nixie_v0_2_main_module_info = 
{
  OTB_EEPROM_MODULE_TYPE_NIXIE_V0_2,  // module_type
  OTB_EEPROM_MODULE_TYPE_DOUBLE_MEZZ, // socket_type
  0,  // jack_used
  0, // num_headers
  0 // num_pins
};

const otb_hwinfo_board_info otb_hwinfo_mez_board_nixie_v0_2 =
{
  "nixie_v0_2",
  OTB_EEPROM_HW_CODE_MAIN_MODULE,
  0,
  0,
  NULL,
  NULL,
  &otb_hwinfo_mez_board_nixie_v0_2_main_module_info
};

#endif // defined(OTB_EEPROM_C) || defined(OTB_HWINFO_C)

#endif // OTB_MEZ_BOARD_NIXIE_V0_2_H_INCLUDED
