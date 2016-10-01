//////////////////////////////////////////////////
// rBoot OTA and config API for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
// OTA code based on SDK sample from Espressif.
//////////////////////////////////////////////////

#ifdef RBOOT_INTEGRATION
#include <rboot-integration.h>
#endif

#include <string.h>
#include <c_types.h>
#include <spi_flash.h>

#include "rboot-api.h"

extern void ets_printf(char*, ...);

#ifdef __cplusplus
extern "C" {
#endif

// get the rboot config
rboot_config ICACHE_FLASH_ATTR rboot_get_config() {
	rboot_config conf;
	spi_flash_read(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)&conf, sizeof(rboot_config));
	return conf;
}

void * pvPortMalloc(size_t);
void vPortFree(void *);
// write the rboot config
// preserves the contents of the rest of the sector,
// so the rest of the sector can be used to store user data
// updates checksum automatically (if enabled)
bool ICACHE_FLASH_ATTR rboot_set_config(rboot_config *conf) {
	uint8 *buffer;
#ifdef BOOT_CONFIG_CHKSUM
	uint8 chksum;
	uint8 *ptr;
#endif
	
	buffer = (uint8*)pvPortMalloc(SECTOR_SIZE);
	if (!buffer) {
		//os_printf("No ram!\r\n");
		return false;
	}
	
#ifdef BOOT_CONFIG_CHKSUM
	chksum = CHKSUM_INIT;
	for (ptr = (uint8*)conf; ptr < &conf->chksum; ptr++) {
		chksum ^= *ptr;
	}
	conf->chksum = chksum;
#endif
	
	spi_flash_read(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)buffer, SECTOR_SIZE);
	memcpy(buffer, conf, sizeof(rboot_config));
	spi_flash_erase_sector(BOOT_CONFIG_SECTOR);
	spi_flash_write(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)buffer, SECTOR_SIZE);
	
	vPortFree(buffer);
	return true;
}

// get current boot rom
uint8 ICACHE_FLASH_ATTR rboot_get_current_rom() {
	rboot_config conf;
	conf = rboot_get_config();
	return conf.current_rom;
}

// set current boot rom
bool ICACHE_FLASH_ATTR rboot_set_current_rom(uint8 rom) {
	rboot_config conf;
	conf = rboot_get_config();
	if (rom >= conf.count) return false;
	conf.current_rom = rom;
	return rboot_set_config(&conf);
}

rboot_write_status status;
// create the write status struct, based on supplied start address
rboot_write_status * ICACHE_FLASH_ATTR rboot_write_init(uint32 start_addr) {
	memset((void *)&status, 0, sizeof(status));
	//rboot_write_status status = {0};
	status.start_addr = start_addr;
	status.start_sector = start_addr / SECTOR_SIZE;
        status.last_sector_erased = status.start_sector - 1;
        //ets_printf("RBOOT OTA: Start addr: 0x%x Start sector: 0x%x Last sector erased: 0x%x\n", status.start_addr, status.start_sector, status.last_sector_erased);
	//status.max_sector_count = 200;
	//os_printf("init addr: 0x%08x\r\n", start_addr);
	
	return &status;
}

// function to do the actual writing to flash
// call repeatedly with more data (max len per write is the flash sector size (4k))

// See:  http://bbs.espressif.com/viewtopic.php?f=7&t=1664&sid=5323983d0e4409895ddcf5f4688a02e9&start=10
#define RBOOT_OTA_MAX_LEN 1460

bool ICACHE_FLASH_ATTR rboot_write_flash(rboot_write_status *status, uint8 *data, uint16 len) {
	
	bool ret = false;
	uint8 *buffer = NULL;
	uint16 buf_len;
	uint16 llen;
	uint16 rlen;
	uint8 *ldata;
	
	//ets_printf("RBOOT OTA: Receive data length: %d\r\n", len);

	if (data == NULL || len == 0)
	{
		ret = true;
		goto EXIT_LABEL;
	}
	
	// get a buffer
	buf_len = len >= RBOOT_OTA_MAX_LEN ? RBOOT_OTA_MAX_LEN : len;
#if 0	
	if (len > RBOOT_OTA_MAX_LEN)
	{
	  ets_printf("RBOOT OTA: ****************************************\r\n");
	  ets_printf("RBOOT OTA: ****************************************\r\n");
	  ets_printf("RBOOT OTA: ****************************************\r\n");
	  ets_printf("RBOOT OTA: ****************************************\r\n");
	}
#endif
	//ets_printf("RBOOT OTA: Malloc: %d\r\n", buf_len+status->extra_count);
	buffer = (uint8 *)pvPortMalloc(buf_len + status->extra_count);
	if (buffer == NULL)
	{
		ets_printf("RBOOT OTA: Malloc failed\r\n");
		ret = false;
		goto EXIT_LABEL;
	}
	
	rlen = len;
	ldata = data;
	while (rlen > 0)
	{
	  // Only do a max of 1460 at a time
	  llen = rlen;
	  if (rlen > RBOOT_OTA_MAX_LEN)
	  {
	    llen = RBOOT_OTA_MAX_LEN;
	  }
	  //ets_printf("RBOOT OTA: rlen: %d llen: %d\r\n", rlen, llen);

    // copy in any remaining bytes from last chunk
    memcpy(buffer, status->extra_bytes, status->extra_count);
    // copy in new data
    memcpy(buffer + status->extra_count, ldata, llen);

    // calculate length, must be multiple of 4
    // save any remaining bytes for next go
    llen += status->extra_count;
    rlen += status->extra_count;
    status->extra_count = llen % 4;
    llen -= status->extra_count;
    rlen -= status->extra_count;
    memcpy(status->extra_bytes, buffer + llen, status->extra_count);

    // check data will fit
    //if (status->start_addr + len < (status->start_sector + status->max_sector_count) * SECTOR_SIZE) {

    // Fixed up code which didn't handle len being greater than SECTOR_SIZE - although
    // this is somewhat moot at sticking to 1460 limit!
    while (status->last_sector_erased < ((status->start_addr + llen) / SECTOR_SIZE))
    {
      status->last_sector_erased++;
      //ets_printf("RBOOT OTA: Erase sector 0x%x\r\n", status->last_sector_erased);
      spi_flash_erase_sector(status->last_sector_erased);
    }

  #if 0
      if (len > SECTOR_SIZE) {
        // to support larger writes we would need to erase current
        // (if not already done), next and possibly later sectors too
      } else {
        // check if the sector the write finishes in has been erased yet,
        // this is fine as long as data len < sector size
        if (status->last_sector_erased != (status->start_addr + len) / SECTOR_SIZE) {
          status->last_sector_erased = (status->start_addr + len) / SECTOR_SIZE;
          spi_flash_erase_sector(status->last_sector_erased);
          ets_printf("RBOOT_OTA: Erase sector 0x%x\r\n", status->last_sector_erased);
        }
      }
  #endif

      // write current chunk
      //ets_printf("RBOOT_OTA: write addr: 0x%08x, len: %d\r\n", status->start_addr, len);
      if (spi_flash_write(status->start_addr, (uint32 *)buffer, llen) == SPI_FLASH_RESULT_OK) {
        status->start_addr += llen;
      }
      else
      {
        ret = false;
        goto EXIT_LABEL;
      }
    //}

    rlen -= llen;	
    ldata += llen;
	}
	
	ret = true;
	
EXIT_LABEL:

  if (buffer != NULL)
  {
	  vPortFree(buffer);
	}
	return ret;
}

#ifdef __cplusplus
}
#endif
