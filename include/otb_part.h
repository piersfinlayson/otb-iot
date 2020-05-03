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
 * this
 */

#ifndef OTB_PART_H
#define OTB_PART_H

//
// !!!!!
//
// MUST BE CONSISTENT WITH partiton-table/table.csv
//
// !!!!!
//

#define OTB_PART_BOOT_LOC  0x000000
#define OTB_PART_BOOT_LEN  0x8000
#define OTB_PART_TABLE_LOC 0x008000
#define OTB_PART_TABLE_LEN 0x1000
#define OTB_PART_NVS_LOC   0x009000
#define OTB_PART_NVS_LEN   0x3000
#define OTB_PART_OTA_LOC   0x00D000
#define OTB_PART_OTA_LEN   0x2000
#define OTB_PART_PHY_LOC   0x00F000
#define OTB_PART_PHY_LEN   0x1000
#define OTB_PART_FACT_LOC  0x010000
#define OTB_PART_FACT_LEN  0xF0000
#define OTB_PART_LOG_LOC   0x100000
#define OTB_PART_LOG_LEN   0x1000
#define OTB_PART_REB_LOC   0x101000
#define OTB_PART_REB_LEN   0x1000
#define OTB_PART_CONF_LOC  0x200000
#define OTB_PART_CONF_LEN  0x1000
#define OTB_PART_OTA1_LOC  0x210000
#define OTB_PART_OTA1_LEN  0xF0000
#define OTB_PART_OTA2_LOC  0x310000
#define OTB_PART_OTA2_LEN  0xF0000

#endif // OTB_PART_H

