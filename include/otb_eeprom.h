/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-8 Piers Finlayson
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

#ifndef OTB_EEPROM_H_INCLUDED
#define OTB_EEPROM_H_INCLUDED

#define OTB_EEPROM_MAIN_BOARD_ADDR  0x57

#define OTB_EEPROM_INFO_MAGIC    0xbc13ee7a

void otb_eeprom_read(void);
bool otb_eeprom_data_read(void);
bool otb_eeprom_data_process(void);

#endif // OTB_EEPROM_H_INCLUDED
