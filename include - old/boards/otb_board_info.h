/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017 Piers Finlayson
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
 
#ifndef OTB_BOARD_INFO_H_INCLUDED
#define OTB_BOARD_INFO_H_INCLUDED

// Used to record main board module information in board specific header files
typedef struct otb_hwinfo_main_board_module_info
{
  uint32 num;
  uint32 socket_type;
  uint32 num_headers;
  uint32 num_pins;
  uint8 address;

  // Pointer to an array of pin_infos (not an array of pointers!)
  const otb_eeprom_pin_info (*pin_info)[];
} otb_hwinfo_main_board_module_info;

typedef struct otb_hwinfo_main_module_info
{
  uint32 module_type;
  uint32 socket_type;
  uint8 jack_used;
  uint32 num_headers;
  uint32 num_pins;

  // Pointer to an array of pin_infos (not an array of pointers!)
  const otb_eeprom_pin_info (*pin_info)[];
} otb_hwinfo_main_module_info;

// Used to record board specific information in board specific header files
typedef struct otb_hwinfo_board_info
{
  // Name with no spaces
  char *name;

  uint32 type;  // One of OTB_EEPROM_HW_CODE_*** (MAIN_BOARD, MAIN_MODULE)

  uint32 pin_count;  // GPIO pin count - 0 in case of MAIN_MODULE

  uint32 mod_count;  // Number of modules - 0 in case of MAIN_MODULE 

  // Pointer to an array of pin_infos (not an array of pointers!)
  const otb_eeprom_pin_info (*pin_info)[];

  // Pointer to an array of module infos (not an array of pointers!)
  const otb_hwinfo_main_board_module_info (*module_info)[];  // NULL in case of MAIN_MODULE

  // Pointer to structure containing main module info (NULL in case of MAIN_BOARD)
  const otb_hwinfo_main_module_info *main_mod_info;

} otb_hwinfo_board_info;

// Some extra info only used by otbiot (not hwinfo or rboot)
typedef struct otb_hwinfo_main_board_info_extra
{
  // See otb_eeprom_main_board for valid values and defaults

  // What ESP module this unit is based on
  uint32 esp_module;
  
  // Flash size in bytes - should match flash size encoded at beginning of esp8266 flash
  uint32 flash_size_bytes;
  
  // I2C ADC included (values may be ORed together (support for up to 32 different
  // ADC type/addresses
  uint32 i2c_adc;
  
  // Internal ADC configuration
  uint32 internal_adc_type;
  
} otb_hwinfo_main_board_info_extra;

#endif // OTB_BOARD_INFO_H_INCLUDED
