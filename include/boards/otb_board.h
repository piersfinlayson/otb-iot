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
#include "otb_main_board_otbiot_v0_4.h"

// Set up default hardware information, in case info can't be read from eeprom
// XXX May want lots of different compile time cases in here - but for now just
// assume a d1 mini
#ifndef OTB_EEPROM_C
extern const otb_hwinfo_main_board_info *otb_eeprom_def_main_board_info;
extern const otb_hwinfo_main_board_info_extra *otb_eeprom_def_main_board_info_extra;
#else // OTB_EEPROM_C
const otb_hwinfo_main_board_info *otb_eeprom_def_main_board_info = &otb_hwinfo_main_board_d1_mini_vanilla_board_info;
const otb_hwinfo_main_board_info_extra *otb_eeprom_def_main_board_info_extra = &otb_hwinfo_main_board_d1_mini_vanilla_board_info_extra;
#endif // OTB_EEPROM_C

#endif // OTB_BOARD_H_INCLUDED
