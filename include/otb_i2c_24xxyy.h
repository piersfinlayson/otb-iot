/*
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
 */

#ifndef OTB_I2C_24XXYY_H
#define OTB_I2C_24XXYY_H

// The 24XXYY is a series of I2C flash chips from Microchip.
//
// XX = AA, 1.8 <= Vcc <= 5.5, -40C <= temp <= +85C
// XX = LC, 2.5 <= Vcc <= 5.5, -40C <= temp <= +85C
// XX = C,  4.5 <= Vcc <= 5.5, -40C <= temp <= +125C
//
// YY indicates capacity:
//
// YY = 00, 128 bit
// YY = 01, 1 Kbit
// YY = 02, 2 Kbit
// YY = 04, 4 Kbit
// YY = 08, 8 Kbit
// YY = 16, 16 Kbit
// YY = 32, 32 Kbit
// YY = 64, 64 Kbit
// YY = 128, 128 Kbit
// YY = 512, 1512 Kbit
// YY = 1024x, 1024 Kbit
//  
// See the datasheet for the 24XX00 at:
// http://ww1.microchip.com/downloads/en/DeviceDoc/21178H.pdf
//
// Memory is arranged by:
// - Block, which are indicated via LSB address bits
// - Page, which varies in size by chip
// - Word, which is a byte of data
//
// Number of blocks supported by device:
// 
// YY = 00, 1
// YY = 01, 1
// YY = 02, 1
// YY = 04, 2
// YY = 08, 4
// YY = 16, 8
//
// Programming the flash chip is done as follows:
// - Write address + w bit
// - Write the word address (only 4 lsb used)
// - Generate stop (causes flash to perform write, ack generated)
// - Poll by write address + w bit
// - When ack returned write complete
//
// All devices except the 24AA00 support page writes as well, where bytes within pages
// can be sequentially written (getting acks), until stop sent.  Note bytes can only be
// sent within page - if go past page will wrap to first byte within page.
// Page size is:
//
// 24XX00 - n/a
// 24XX01 - 8 bytes
// 24XX02 - 8 bytes
// 24XX04 - 16 bytes
// 24XX08 - 16 bytes
// 24XX16 - 16 bytes
// 24XX16 - 32 bytes
//
// Reading is done as follows:
//
// Current address:
// - Write address + r bit
// - Read provides current address - not acknowledged by master, which generates stop
//
// Random address:
// - Write address + w bit
// - Write word address, wait for ack, generate new start
// - As per current address
//
// Sequential read:
// - As current address, but master acks word, which causes next to be transmitted
//
// All devices except the 24XX00 have variants which allow write protecting the whole
// or a subset of the device

// 24XXYY base address - (3 LSB are don't care, there is no configurability)
#define OTB_I2C_24XXYY_BASE_ADDR        0x50

// Number of bytes supported by a 24AA00 - 4 bits
#define OTB_I2C_24AA00_BYTES            0x10


// Note 24XXYY has no registers except IO pin state, and no configuration

#ifndef OTB_I2C_24XXYY_C


#else

bool otb_i2c_24xxyy_written;
static volatile os_timer_t otb_i2c_24xxyy_test_timer;
uint8_t otb_i2c_24xxyy_test_addr;
uint8_t otb_i2c_24xxyy_next_byte;

#endif // OTB_I2C_24XXYY_C

void otb_i2c_24xxyy_test_timerfunc(void);
void otb_i2c_24xxyy_test_init(void);
bool otb_i2c_24xxyy_read_bytes(uint8_t addr, uint8_t word_addr, uint8_t *bytes, uint8_t num_bytes);
bool otb_i2c_24xxyy_write_bytes(uint8_t addr, uint8_t word_addr, uint8_t *bytes, uint8_t num_bytes);
bool otb_i2c_24xxyy_init(uint8_t addr);

#endif // OTB_I2C_24XXYY_H
