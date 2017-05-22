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
 
#ifndef OTB_MAIN_BOARD_D1_MINI_V0_4_H_INCLUDED
#define OTB_MAIN_BOARD_D1_MINI_V0_4_H_INCLUDED

// This is the default hardware configuration for otbiot if no eeprom info is
// provided
#if defined(OTB_EEPROM_C) || defined(OTB_HWINFO_C)

#define OTB_HWINFO_MAIN_BOARD_D1_MINI_VANILLA_GPIO_PIN_NUM       17
#define OTB_HWINFO_MAIN_BOARD_D1_MINI_VANILLA_MODULE_NUM         0

const otb_eeprom_pin_info otb_hwinfo_main_board_d1_mini_vanilla_main_board_pin_info[OTB_HWINFO_MAIN_BOARD_D1_MINI_VANILLA_GPIO_PIN_NUM] =
{
  // GPIO 0
  {0,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_V33},

  // GPIO 1
  {1,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_TX,
   OTB_EEPROM_PIN_MODULE_1,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 2
  {2,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_V33},

  // GPIO 3
  {3,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RX,
   OTB_EEPROM_PIN_MODULE_1,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 4
  {4,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 5
  {5,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 6
  {6,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RESERVED,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_RSVD_FLASH,
   OTB_EEPROM_PIN_PULLED_NA},

  // GPIO 7
  {7,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RESERVED,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_RSVD_FLASH,
   OTB_EEPROM_PIN_PULLED_NA},

  // GPIO 8
  {8,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RESERVED,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_RSVD_FLASH,
   OTB_EEPROM_PIN_PULLED_NA},

  // GPIO 9
  {9,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RESERVED,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_RSVD_FLASH,
   OTB_EEPROM_PIN_PULLED_NA},

  // GPIO 10
  {10,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RESERVED,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_RSVD_FLASH,
   OTB_EEPROM_PIN_PULLED_NA},

  // GPIO 11
  {11,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_RESERVED,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_RSVD_FLASH,
   OTB_EEPROM_PIN_PULLED_NA},

  // GPIO 12
  {12,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 13
  {13,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 14
  {14,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},

  // GPIO 15
  {15,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_V0},

  // GPIO 16
  {16,
   OTB_EEPROM_PIN_HEADER_NONE,
   OTB_EEPROM_PIN_USE_GPIO,
   OTB_EEPROM_PIN_MODULE_NONE,
   OTB_EEPROM_PIN_FINFO_NONE,
   OTB_EEPROM_PIN_PULLED_FLOAT},
};

const otb_hwinfo_main_board_info otb_hwinfo_main_board_d1_mini_vanilla_board_info =
{
  OTB_HWINFO_MAIN_BOARD_D1_MINI_VANILLA_GPIO_PIN_NUM,
  OTB_HWINFO_MAIN_BOARD_D1_MINI_VANILLA_MODULE_NUM,
  &otb_hwinfo_main_board_d1_mini_vanilla_main_board_pin_info,
  NULL,
};

const otb_hwinfo_main_board_info_extra otb_hwinfo_main_board_d1_mini_vanilla_board_info_extra = 
{
  OTB_EEPROM_HW_ESP12,
  OTB_EEPROM_HW_FLASH_SIZE_BYTES_4M,
  OTB_EEPROM_HW_I2C_ADC_NONE,
  OTB_EEPROM_HW_INT_ADC_3V3_220K_100K
};
#endif // defined(OTB_EEPROM_C) || defined(OTB_HWINFO_C)

#endif // OTB_MAIN_BOARD_D1_MINI_V0_4_H_INCLUDED
