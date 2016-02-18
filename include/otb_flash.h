/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016 Piers Finlayson
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

#ifndef OTB_FLASH_H
#define OTB_FLASH_H

// Flash map

// ROMs 0 and 1 are for upgradeable firmware
// ROM 2 contains recovery firmware in case both ROMs 0 and 1 are corrupted
// and provides basic function to allow the other ROMs to be recovered
//
// Note everything should be on a 4KB (0x1000) boundary, as sector length is
// 0x1000, and the flash has to be erased in sectors before being rewritten
#define OTB_BOOT_BOOT_LOCATION            0x0  // length 0x1000  = 4KB
#define OTB_BOOT_BOOT_CONFIG_LOCATION  0x1000  // length 0x1000  = 4KB
#define OTB_BOOT_ROM_0_LOCATION        0x2000  // length 0xFE000 = 896KB
#define OTB_BOOT_LOG_LOCATION        0x100000  // length 0x0400  = 1KB
#define OTB_BOOT_LAST_REBOOT_REASON  0x101000  // length 0x0200  = 0.5KB
#define OTB_BOOT_LAST_REBOOT_LEN       0x1000
#define OTB_BOOT_RESERVED2           0x101000  // length 0x1000  = 4KB
#define OTB_BOOT_RESERVED3           0x102000  // length 0xFE000 = 896KB
#define OTB_BOOT_CONF_LOCATION       0x200000  // length 0x1000  = 4KB
#define OTB_BOOT_CONF_LEN              0x1000
#define OTB_BOOT_RESERVED5           0x201000  // length 0x1000  = 4KB
#define OTB_BOOT_ROM_1_LOCATION      0x202000  // length 0xFE000 = 896KB
#define OTB_BOOT_RESERVED6           0x300000  // length 0x1000  = 4KB
#define OTB_BOOT_RESERVED7           0x301000  // length 0x1000  = 4KB
#define OTB_BOOT_ROM_2_LOCATION      0x302000  // length 0xFA000 = 880KB
#define OTB_BOOT_SDK_RESERVED        0x30c000  // length 0x4000  = 16KB

#endif // OTB_FLASH_H
