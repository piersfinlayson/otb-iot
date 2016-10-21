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

#ifdef OTB_RBOOT
typedef enum {
    SPI_FLASH_RESULT_OK,
    SPI_FLASH_RESULT_ERR,
    SPI_FLASH_RESULT_TIMEOUT
} SpiFlashOpResult;

typedef struct{
        uint32  deviceId;
        uint32  chip_size;    // chip size in byte
        uint32  block_size;
        uint32  sector_size;
        uint32  page_size;
        uint32  status_mask;
} SpiFlashChip;

#define ICACHE_FLASH_ATTR 

#endif // OTB_RBOOT

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
#define OTB_BOOT_SDK_RESERVED        0x3fc000  // length 0x4000  = 16KB
#define OTB_BOOT_ESP_USER_BIN        0x3fc000  // length ??? ESP SDK user bin
#define OTB_BOOT_ROM_0_LEN            0xfe000
#define OTB_BOOT_ROM_1_LEN            0xfe000
#define OTB_BOOT_ROM_2_LEN            0xfa000

void otb_flash_init(void);

#ifndef OTB_SUPER_BIG_FLASH_8266 // > 4MB Flash for ESP8266

// Map straight through to SDK functions
#ifndef OTB_RBOOT
#define SPI_FLASH_ERASE_SECTOR  spi_flash_erase_sector
#define SPI_FLASH_WRITE         spi_flash_write
#define SPI_FLASH_READ          spi_flash_read
#else // OTB_RBOOT
#define SPI_FLASH_ERASE_SECTOR  SPIEraseSector
#define SPI_FLASH_WRITE         SPIWrite
#define SPI_FLASH_READ          SPIRead
#endif // OTB_RBOOT

#else

// OTB externs
// Accessed from SDK (or created by ROM - not sure which)
extern SpiFlashChip *flashchip;
uint32 otb_flash_size_sdk;
uint32 otb_flash_size_actual;

SpiFlashOpResult otb_flash_erase_sector_big(uint16 sector);
SpiFlashOpResult otb_flash_write_big(uint32 des_addr, uint32 *src_addr, uint32 size);
SpiFlashOpResult otb_flash_read_big(uint32 src_addr, uint32 *des_addr, uint32 size);

#ifndef OTB_RBOOT
#define SPI_FLASH_ERASE_SECTOR(SECTOR)   otb_flash_size_actual ? otb_flash_erase_sector_big(SECTOR) : spi_flash_erase_sector(SECTOR)
#define SPI_FLASH_WRITE(DST, SRC, LEN)   otb_flash_size_actual ? otb_flash_write_big(DST, SRC, LEN) : spi_flash_write(DST, SRC, LEN)
#define SPI_FLASH_READ(SRC, DST, LEN)    otb_flash_size_actual ? otb_flash_read_big(SRC, DST, LEN) : spi_flash_read(SRC, DST, LEN)
#else // OTB_RBOOT
#define SPI_FLASH_ERASE_SECTOR(SECTOR)   otb_flash_size_actual ? otb_flash_erase_sector_big(SECTOR) : SPIEraseSector(SECTOR)
#define SPI_FLASH_WRITE(DST, SRC, LEN)   otb_flash_size_actual ? otb_flash_write_big(DST, (void*)SRC, LEN) : SPIWrite(DST, SRC, LEN)
#define SPI_FLASH_READ(SRC, DST, LEN)    otb_flash_size_actual ? otb_flash_read_big(SRC, (void*)DST, LEN) : SPIRead(SRC, DST, LEN)
#endif // OTB_RBOOT

#endif // OTB_SUPER_BIG_FLASH_8266

#ifdef OTB_FLASH_INC_FUNCS

#ifndef OTB_RBOOT
void ICACHE_FLASH_ATTR otb_flash_init(void)
#else // OTB_RBOOT
void __attribute__ ((noinline)) otb_flash_init(int size)
#endif // OTB_RBOOT
{

#ifndef OTB_RBOOT
  DEBUG("UTIL: otb_flash_init entry");
#endif // OTB_RBOOT

#ifdef OTB_SUPER_BIG_FLASH_8266  
  uint32 id_size = 0;
  int ii;

#ifdef OTB_RBOOT
  if (size < 0)
  {
    otb_flash_size_actual = 0;
  }
  else
  {
    // fake up an id_size as it doesn't seem to be set up in rboot
    id_size = size + 0x12;
    ets_printf("id_size = %d\r\n", id_size);
  }
#else
  id_size = (flashchip->deviceId >> 16) & 0xff;
#endif
  otb_flash_size_actual = 1;
  for (ii = 0; ii < id_size; ii++)
  {
    otb_flash_size_actual *= 2;
  }
  otb_flash_size_sdk = flashchip->chip_size;
#ifndef OTB_RBOOT
  INFO("UTIL: Flash size from sdk: %d bytes", otb_flash_size_sdk);
  INFO("UTIL: Flash size actual:   %d bytes", otb_flash_size_actual);
  OTB_ASSERT(otb_flash_size_actual >= otb_flash_size_sdk); 
#else // OTB_RBOOT
  ets_printf("BOOT: Flash size from sdk: %d bytes\r\n", otb_flash_size_sdk);
  ets_printf("BOOT: Flash size actual:   %d bytes\r\n", otb_flash_size_actual);
  ets_printf("BOOT: flashchip pointer: 0x%x\r\n", flashchip);
  ets_printf("BOOT: deviceId:  0x%x\r\n", flashchip->deviceId);
  ets_printf("BOOT: chip_size: %d\r\n", flashchip->chip_size);
#endif // OTB_RBOOT

  if (otb_flash_size_actual == otb_flash_size_sdk)
  {
    // Optimisation so we can avoid a bit of processing when calling spi_flash_ functions
    otb_flash_size_actual = 0;
  }
#endif // OTB_SUPER_BIG_FLASH_8266  

#ifndef OTB_RBOOT
  DEBUG("UTIL: otb_flash_init exit");
#endif // OTB_RBOOT
  
  return;
}

#ifdef OTB_SUPER_BIG_FLASH_8266

SpiFlashOpResult ICACHE_FLASH_ATTR otb_flash_erase_sector_big(uint16 sector)
{
  int8 status;

  flashchip->chip_size = otb_flash_size_actual;

#ifndef OTB_RBOOT
  status = spi_flash_erase_sector(sector);
#else // OTB_RBOOT
  status = SPIEraseSector(sector);
#endif // OTB_RBOOT

  flashchip->chip_size = otb_flash_size_sdk;

  return status;
}

SpiFlashOpResult ICACHE_FLASH_ATTR otb_flash_write_big(uint32 des_addr, uint32 *src_addr, uint32 size)
{
  int8 status;

  flashchip->chip_size = otb_flash_size_actual;

#ifndef OTB_RBOOT
  status = spi_flash_write(des_addr, src_addr, size);
#else // OTB_RBOOT
  status = SPIWrite(des_addr, src_addr, size);
#endif // OTB_RBOOT

  flashchip->chip_size = otb_flash_size_sdk;

  return status;
}

SpiFlashOpResult ICACHE_FLASH_ATTR otb_flash_read_big(uint32 src_addr, uint32 *des_addr, uint32 size)
{
  int8 status;
  
  flashchip->chip_size = otb_flash_size_actual;

#ifndef OTB_RBOOT
  status = spi_flash_read(src_addr, des_addr, size);
#else // OTB_RBOOT
  status = SPIRead(src_addr, des_addr, size);
#endif // OTB_RBOOT

  flashchip->chip_size = otb_flash_size_sdk;

  return status;
}

#endif // OTB_SUPER_BIG_FLASH_8266

#endif // OTB_FLASH_INC_FUNCS

#endif // OTB_FLASH_H
