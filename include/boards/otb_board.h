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
 

#ifndef OTB_BOARD_H_INCLUDED
#define OTB_BOARD_H_INCLUDED

#include "otb_board_info.h"
#include "otb_main_board_d1_mini_vanilla.h"
#include "otb_main_board_otbiot_v0_3.h"
#include "otb_main_board_otbiot_v0_4.h"
#include "otb_main_board_otbiot_v0_5.h"
#include "otb_mez_board_nixie_v0_2.h"
#include "otb_mez_board_prog_v0_2.h"
#include "otb_mez_board_temp_v0_2.h"

// Set up default hardware information, in case info can't be read from eeprom
// XXX May want lots of different compile time cases in here - but for now just
// assume a d1 mini
#ifndef OTB_EEPROM_C
extern const otb_hwinfo_board_info *otb_eeprom_def_board_info;
extern const otb_hwinfo_main_board_info_extra *otb_eeprom_def_board_info_extra;
#else // OTB_EEPROM_C
const otb_hwinfo_board_info *otb_eeprom_def_board_info = &otb_hwinfo_main_board_d1_mini_vanilla_board_info;
const otb_hwinfo_main_board_info_extra *otb_eeprom_def_board_info_extra = &otb_hwinfo_main_board_d1_mini_vanilla_board_info_extra;
#endif // OTB_EEPROM_C

#ifdef OTB_HWINFO_C
#define OTB_HWINFO_DEFAULT_BOARD_TYPE otb_hwinfo_main_board_otbiot_v0_5_board_info
const otb_hwinfo_board_info *otb_hwinfo_boards[] =
{
  &otb_hwinfo_main_board_d1_mini_vanilla_board_info,
  &otb_hwinfo_main_board_otbiot_v0_3_board_info,
  &otb_hwinfo_main_board_otbiot_v0_4_board_info,
  &otb_hwinfo_main_board_otbiot_v0_5_board_info,
  &otb_hwinfo_mez_board_nixie_v0_2,
  &otb_hwinfo_mez_board_prog_v0_2,
  &otb_hwinfo_mez_board_temp_v0_2,
  NULL
};
#endif

#endif // OTB_BOARD_H_INCLUDED
