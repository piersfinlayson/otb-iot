//////////////////////////////////////////////////
// rBoot open source boot loader for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
//
//////////////////////////////////////////////////

//
// Modifications from original rboot source code are copyright and licensed as follows:
//

/*
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
 */

#ifdef RBOOT_INTEGRATION
#include <rboot-integration.h>
#endif

// Pretend to be otb_eeprom.c so globals are defined
//#define OTB_EEPROM_C

#include "rboot-private.h"
#include <rboot-hex2a.h>
#include "otb_flash.h"
#undef TRUE // XXX Hack
#undef FALSE // XXX Hack
#include "brzo_i2c.h"
#include "otb_eeprom.h"
#include "pin_map.h"

// Statically allocated buffers for eeprom info - will be blown away when bootloader is finished
char __attribute__((aligned(4))) rboot_eeprom_info[OTB_EEPROM_MAX_MAIN_COMP_LENGTH];
char __attribute__((aligned(4))) rboot_gpio_pins[OTB_EEPROM_MAX_MAIN_COMP_LENGTH];
char __attribute__((aligned(4))) rboot_sdk_init_data[OTB_EEPROM_MAX_MAIN_COMP_LENGTH];

#define OTB_RBOOT_DEFAULT_SOFT_RESET_PIN  14

static uint32 check_image(uint32 readpos) {
	
	uint8 buffer[BUFFER_SIZE];
	uint8 sectcount;
	uint8 sectcurrent;
	uint8 *writepos;
	uint8 chksum = CHKSUM_INIT;
	uint32 loop;
	uint32 remaining;
	uint32 romaddr;
	
	rom_header_new *header = (rom_header_new*)buffer;
	section_header *section = (section_header*)buffer;
	
	if (readpos == 0 || readpos == 0xffffffff) {
		return 0;
	}
	
	// read rom header
	if (SPIRead(readpos, header, sizeof(rom_header_new)) != 0) {
		return 0;
	}
	
	// check header type
	if (header->magic == ROM_MAGIC) {
		// old type, no extra header or irom section to skip over
		romaddr = readpos;
		readpos += sizeof(rom_header);
		sectcount = header->count;
	} else if (header->magic == ROM_MAGIC_NEW1 && header->count == ROM_MAGIC_NEW2) {
		// new type, has extra header and irom section first
		romaddr = readpos + header->len + sizeof(rom_header_new);
#ifdef BOOT_IROM_CHKSUM
		// we will set the real section count later, when we read the header
		sectcount = 0xff;
		// just skip the first part of the header
		// rest is processed for the chksum
		readpos += sizeof(rom_header);
#else
		// skip the extra header and irom section
		readpos = romaddr;
		// read the normal header that follows
		if (SPIRead(readpos, header, sizeof(rom_header)) != 0) {
			return 0;
		}
		sectcount = header->count;
		readpos += sizeof(rom_header);
#endif
	} else {
		return 0;
	}
	
	// test each section
	for (sectcurrent = 0; sectcurrent < sectcount; sectcurrent++) {
		
		// read section header
		if (SPIRead(readpos, section, sizeof(section_header)) != 0) {
			return 0;
		}
		readpos += sizeof(section_header);

		// get section address and length
		writepos = section->address;
		remaining = section->length;
		
		while (remaining > 0) {
			// work out how much to read, up to BUFFER_SIZE
			uint32 readlen = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
			// read the block
			if (SPIRead(readpos, buffer, readlen) != 0) {
				return 0;
			}
			// increment next read and write positions
			readpos += readlen;
			writepos += readlen;
			// decrement remaining count
			remaining -= readlen;
			// add to chksum
			for (loop = 0; loop < readlen; loop++) {
				chksum ^= buffer[loop];
			}
		}
		
#ifdef BOOT_IROM_CHKSUM
		if (sectcount == 0xff) {
			// just processed the irom section, now
			// read the normal header that follows
			if (SPIRead(readpos, header, sizeof(rom_header)) != 0) {
				return 0;
			}
			sectcount = header->count + 1;
			readpos += sizeof(rom_header);
		}
#endif
	}
	
	// round up to next 16 and get checksum
	readpos = readpos | 0x0f;
	if (SPIRead(readpos, buffer, 1) != 0) {
		return 0;
	}
	
	// compare calculated and stored checksums
	if (buffer[0] != chksum) {
		return 0;
	}
	
	return romaddr;
}

#define ETS_UNCACHED_ADDR(addr) (addr)
#define READ_PERI_REG(addr) (*((volatile uint32 *)ETS_UNCACHED_ADDR(addr)))
#define WRITE_PERI_REG(addr, val) (*((volatile uint32 *)ETS_UNCACHED_ADDR(addr))) = (uint32)(val)
#define PERIPHS_RTC_BASEADDR				0x60000700
#define REG_RTC_BASE  PERIPHS_RTC_BASEADDR
#define RTC_GPIO_OUT							(REG_RTC_BASE + 0x068)
#define RTC_GPIO_ENABLE							(REG_RTC_BASE + 0x074)
#define RTC_GPIO_IN_DATA						(REG_RTC_BASE + 0x08C)
#define RTC_GPIO_CONF							(REG_RTC_BASE + 0x090)
#define PAD_XPD_DCDC_CONF						(REG_RTC_BASE + 0x0A0)
#define PERIPHS_GPIO_BASEADDR               0x60000300

// GPIO14 stuff
#define PERIPHS_IO_MUX                                          0x60000800
#define PERIPHS_IO_MUX_MTMS_U           (PERIPHS_IO_MUX + 0x0C)
#define FUNC_GPIO14                     3
#define GPIO14                      (REG_RTC_BASE + 0x10C)
#define PIN_FUNC_SELECT(PIN_NAME, FUNC)  do { \
    WRITE_PERI_REG(PIN_NAME,   \
                                READ_PERI_REG(PIN_NAME) \
                                     &  (~(PERIPHS_IO_MUX_FUNC<<PERIPHS_IO_MUX_FUNC_S))  \
                                     |( (((FUNC&BIT2)<<2)|(FUNC&0x3))<<PERIPHS_IO_MUX_FUNC_S) );  \
    } while (0)

//}}
#define PERIPHS_IO_MUX_FUNC             0x13
#define PERIPHS_IO_MUX_FUNC_S           4
#define BIT2     0x00000004
#define GPIO_OUT_W1TS_ADDRESS             0x04
#define GPIO_OUT_W1TC_ADDRESS             0x08
static uint32 get_gpio16() {
	// set output level to 1
	WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(1));
	
	// read level
	WRITE_PERI_REG(PAD_XPD_DCDC_CONF, (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);	// mux configuration for XPD_DCDC and rtc_gpio0 connection
	WRITE_PERI_REG(RTC_GPIO_CONF, (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable
	WRITE_PERI_REG(RTC_GPIO_ENABLE, READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable
	
	uint32 x = (READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
	
	return x;
}
static uint32 get_gpio(uint8 pin)
{
  uint32 rc = 1;
  uint32 *pin_in = (uint32 *)0x60000318;

  if (pin > 16)
  {
    goto EXIT_LABEL;
  }
  rc = (READ_PERI_REG(0x60000318) & (1 << pin)) >> pin;

EXIT_LABEL:
  
  return rc;
}

static void reset(void)
{
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
    (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);      // mux configuration for XPD_DCDC to output rtc_gpio0

  WRITE_PERI_REG(RTC_GPIO_CONF,
    (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);  //mux configuration for out enable

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
    (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);        //out enable

//  WRITE_PERI_REG(RTC_GPIO_OUT,
//    (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(1));

	WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(0));
	return;
}

#ifdef BOOT_CONFIG_CHKSUM
// calculate checksum for block of data
// from start up to (but excluding) end
static uint8 calc_chksum(uint8 *start, uint8 *end) {
	uint8 chksum = CHKSUM_INIT;
	while(start < end) {
		chksum ^= *start;
		start++;
	}
	return chksum;
}
#endif

// prevent this function being placed inline with main
// to keep main's stack size as small as possible
// don't mark as static or it'll be optimised out when
// using the assembler stub
uint32 NOINLINE find_image() {
	
	uint8 flag;
	uint32 runAddr;
	uint32 flashsize;
	int32 romToBoot;
	uint8 gpio_boot = FALSE;
	uint8 updateConfig = FALSE;
	uint8 buffer[SECTOR_SIZE];
	uint8 roms_tried;
	uint8 ii;

	rboot_config *romconf = (rboot_config*)buffer;
	rom_header *header = (rom_header*)buffer;
	
	if (BOOT_CONFIG_SECTOR * SECTOR_SIZE != OTB_BOOT_BOOT_CONFIG_LOCATION)
	{
		ets_printf("BOOT: !!! Inconsistent build !!!");
		ets_printf("BOOT: !!! Inconsistent build !!!");
		ets_printf("BOOT: !!! Inconsistent build !!!");
		ets_printf("BOOT: !!! Inconsistent build !!!");
		ets_printf("BOOT: !!! Inconsistent build !!!");
	}
	
	// read rom header
	SPIRead(0, header, sizeof(rom_header));
	
	// print and get flash size
	ets_printf("BOOT: Flash Size:   ");
	flag = header->flags2 >> 4;
	if (flag == 0) {
		ets_printf("4 Mbit\r\n");
		flashsize = 0x80000;
	} else if (flag == 1) {
		ets_printf("2 Mbit\r\n");
		flashsize = 0x40000;
	} else if (flag == 2) {
		ets_printf("8 Mbit\r\n");
		flashsize = 0x100000;
	} else if (flag == 3) {
		ets_printf("16 Mbit\r\n");
#ifdef BOOT_BIG_FLASH
		flashsize = 0x200000;
#else
		flashsize = 0x100000; // limit to 8Mbit
#endif
	} else if (flag == 4) {
		ets_printf("32 Mbit\r\n");
#ifdef BOOT_BIG_FLASH
		flashsize = 0x400000;
#else
		flashsize = 0x100000; // limit to 8Mbit
#endif
	} else if (flag == 5) {
		ets_printf("64 Mbit\r\n");
#ifdef BOOT_BIG_FLASH
		flashsize = 0x800000;
#else
		flashsize = 0x100000; // limit to 8Mbit
#endif
	} else if (flag == 6) {
		ets_printf("128 Mbit\r\n");
#ifdef BOOT_BIG_FLASH
		flashsize = 0x1000000;
#else
		flashsize = 0x100000; // limit to 8Mbit
#endif
	} else {
		ets_printf("unknown\r\n");
		// assume at least 4mbit
		flashsize = 0x80000;
	}
	
	// print spi mode
	ets_printf("BOOT: Flash Mode:   ");
	if (header->flags1 == 0) {
		ets_printf("QIO\r\n");
	} else if (header->flags1 == 1) {
		ets_printf("QOUT\r\n");
	} else if (header->flags1 == 2) {
		ets_printf("DIO\r\n");
	} else if (header->flags1 == 3) {
		ets_printf("DOUT\r\n");
	} else {
		ets_printf("unknown\r\n");
	}
	
	// print spi speed
	ets_printf("BOOT: Flash Speed:  ");
	flag = header->flags2 & 0x0f;
	if (flag == 0) ets_printf("40 MHz\r\n");
	else if (flag == 1) ets_printf("26.7 MHz\r\n");
	else if (flag == 2) ets_printf("20 MHz\r\n");
	else if (flag == 0x0f) ets_printf("80 MHz\r\n");
	else ets_printf("unknown\r\n");
	
	// print enabled options
#ifdef BOOT_BIG_FLASH
	ets_printf("BOOT: Option: Big (>1MB) flash\r\n");
#endif
#ifdef BOOT_CONFIG_CHKSUM
	ets_printf("BOOT: Option: Config checksum\r\n");
#endif
#ifdef BOOT_IROM_CHKSUM
	ets_printf("BOOT: Option: IROM checksum\r\n");
#endif
	
	// read boot config
	SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, buffer, SECTOR_SIZE);
	// fresh install or old version?
	if (romconf->magic != BOOT_CONFIG_MAGIC || romconf->version != BOOT_CONFIG_VERSION
#ifdef BOOT_CONFIG_CHKSUM
		|| romconf->chksum != calc_chksum((uint8*)romconf, (uint8*)&romconf->chksum)
#endif
		) {
		// create a default config for a standard 2 rom setup
		ets_printf("BOOT: No valid boot config found\r\n");
		ets_printf("BOOT: Writing default boot config\r\n");
		ets_memset(romconf, 0x00, sizeof(rboot_config));
		romconf->magic = BOOT_CONFIG_MAGIC;
		romconf->version = BOOT_CONFIG_VERSION;
		romconf->count = 3;
		romconf->roms[0] = OTB_BOOT_ROM_0_LOCATION;
		romconf->roms[1] = OTB_BOOT_ROM_1_LOCATION;
		romconf->roms[2] = OTB_BOOT_ROM_2_LOCATION;
		romconf->gpio_rom = 2;
		//romconf->roms[0] = SECTOR_SIZE * (BOOT_CONFIG_SECTOR + 1);
		//romconf->roms[1] = (flashsize / 2) + (SECTOR_SIZE * (BOOT_CONFIG_SECTOR + 1));
#ifdef BOOT_CONFIG_CHKSUM
		romconf->chksum = calc_chksum((uint8*)romconf, (uint8*)&romconf->chksum);
#endif
		// write new config sector
		SPIEraseSector(BOOT_CONFIG_SECTOR);
		SPIWrite(BOOT_CONFIG_SECTOR * SECTOR_SIZE, buffer, SECTOR_SIZE);
	}
	
	// if gpio mode enabled check status of the gpio
	if ((romconf->mode & MODE_GPIO_ROM) && (get_gpio16() == 0)) {
		ets_printf("BOOT: Booting GPIO-selected.\r\n");
		romToBoot = romconf->gpio_rom;
		gpio_boot = TRUE;
		updateConfig = TRUE;
	} else if (romconf->current_rom >= romconf->count) {
		// if invalid rom selected try rom 0
		ets_printf("BOOT: Invalid rom selected, defaulting.\r\n");
		romToBoot = 0;
		romconf->current_rom = 0;
		updateConfig = TRUE;
	} else {
		// try rom selected in the config
		romToBoot = romconf->current_rom;
		if (romconf->current_rom == 2)
		{
			romToBoot = 0;
		}
	}
	
	// try to find a good rom
	roms_tried = 0;
	do {
		roms_tried += 1 << romToBoot;
		runAddr = check_image(romconf->roms[romToBoot]);
		if (runAddr == 0) {
			ets_printf("BOOT: ROM %d is bad.\r\n", romToBoot);
			if (gpio_boot) {
				// don't switch to backup for gpio-selected rom
				ets_printf("BOOT: GPIO boot failed.\r\n");
				return 0;
			} else {
				// for normal mode try each previous rom
				// until we find a good one or run out
				// Try ROM 2 last.
				updateConfig = TRUE;
				if (roms_tried == 1)
				{
					romToBoot = 1;
					ets_printf("BOOT: Try ROM 1\r\n");
				}
				else if (roms_tried == 2)
				{
					romToBoot = 0;
					ets_printf("BOOT: Try ROM 0\r\n");
				}
				else if (roms_tried == 3)
				{
					// Last chance saloon ... try fallback
					// rom
					romToBoot = 2;
					ets_printf("BOOT: Try fallback ROM\r\n");
				}
				else
				//  if (romToBoot == romconf->current_rom) {
				{
					// tried them all and all are bad!
					ets_printf("BOOT: No good ROM available.\r\n");
					updateConfig = FALSE;
					ets_printf("BOOT: Rebooting in 5s ");
					for (ii=0; ii < 5; ii++)
					{
						ets_printf(".");
						if (ii==4) ets_printf("\r\n");
						ets_delay_us(1000000);
					}
					reset();
					return 0;
				}
			}
		}
	} while (runAddr == 0);
	
	// re-write config, if required
	if (updateConfig) {
		// Never write to config to boot from fallback ROM
		romconf->current_rom = romToBoot;
#ifdef BOOT_CONFIG_CHKSUM
		romconf->chksum = calc_chksum((uint8*)romconf, (uint8*)&romconf->chksum);
#endif
		SPIEraseSector(BOOT_CONFIG_SECTOR);
		SPIWrite(BOOT_CONFIG_SECTOR * SECTOR_SIZE, buffer, SECTOR_SIZE);
	}
	
	ets_printf("BOOT: Booting rom %d at 0x%08x\r\n", romToBoot, runAddr);
	// copy the loader to top of iram
	ets_memcpy((void*)_text_addr, _text_data, _text_len);
	// return address to load from
	return runAddr;

}

extern uint8 i2c_error;
void NOINLINE start_otb_boot(void)
{

	// delay to slow boot (help see messages when debugging)
	ets_delay_us(2000000);
	
	ets_printf("\r\nBOOT: OTA-BOOT v0.4a\r\n");

  return;
}

// otb-iot factory reset function
#define FACTORY_RESET_LEN  15  // seconds

void do_factory_reset(void)
{
  uint32 from_loc;
  uint32 to_loc;
  uint32 written;
  uint32 buffer[0x1000/4];

  // Implement factory reset
  ets_printf("BOOT: GPIO14 triggered reset to factory defaults\r\n");
  
  // Erase config sector - that's all we need to do to clear config
  SPIEraseSector(OTB_BOOT_CONF_LOCATION/0x1000);
  ets_printf("BOOT: otb-iot boot config cleared\r\n");

  for (written = 0, from_loc = OTB_BOOT_ROM_2_LOCATION, to_loc = OTB_BOOT_ROM_0_LOCATION;
       written < OTB_BOOT_ROM_2_LEN;
       written += 0x1000, from_loc += 0x1000, to_loc += 0x1000)
  {
    // Dubious this will work as from_loc is on differnet 1MB segment of flash
    SPIRead(from_loc, buffer, 0x1000);
    SPIEraseSector(to_loc/0x1000);
    SPIWrite(to_loc, buffer, 0x1000);
  }
  while (to_loc < (OTB_BOOT_ROM_0_LOCATION+OTB_BOOT_ROM_0_LEN))
  {
    // Zero out spare space on end of slot 0 (recovery slot is smaller)
    SPIEraseSector(to_loc/0x1000);
    to_loc += 0x1000;
  }
  ets_printf("BOOT: Factory image written into slot 0\r\n");

	// Now rewrite SDK init data if have it
	if (otb_eeprom_main_board_sdk_init_data_g->hdr.magic == OTB_EEPROM_MAIN_BOARD_SDK_INIT_DATA_MAGIC)
	{
    SPIEraseSector(OTB_BOOT_ESP_USER_BIN/0x1000);
		SPIWrite(OTB_BOOT_ESP_USER_BIN,
		         otb_eeprom_main_board_sdk_init_data_g->sdk_init_data, 
						 otb_eeprom_main_board_sdk_init_data_g->hdr.length-otb_eeprom_main_board_sdk_init_data_g->hdr.struct_size);
	}
  ets_printf("BOOT: SDK init data rewritten\r\n");

  return;
}

#define OTB_EEPROM_BOOT_DATA_SIZE    1024
void NOINLINE read_eeprom(void)
{
  char rc;

  // Initial internal I2C bus (must be done before try and read eeprom)
  otb_i2c_initialize_bus_internal();

  // Setup memory to read eeprom info into
  otb_eeprom_info_g = (otb_eeprom_info *)rboot_eeprom_info;
  otb_eeprom_main_board_gpio_pins_g = (otb_eeprom_main_board_gpio_pins *)rboot_gpio_pins;
	otb_eeprom_main_board_sdk_init_data_g = (otb_eeprom_main_board_sdk_init_data *)rboot_sdk_init_data;

  // rboot uses the following information from the eeprom:
	// - which pin is used for soft reset (reset to factory defaults)
	// - loads sdk_init_data (in case factory reset is called)
	// - reads flash structure from eeprom XXX Not yet implemented

  // Read the eeprom
  otb_eeprom_read();

EXIT_LABEL:

  return;
}

// Required for brzo_i2c, as the SDK isn't present
uint8 ets_get_cpu_frequency(void);
uint8 system_get_cpu_freq(void)
{
  uint8 ticks_per_us;
  ticks_per_us = ets_get_cpu_frequency();
  return ticks_per_us;
}

void NOINLINE factory_reset(void)
{
  uint32 ii;
  uint32 gpio_state;
	uint32 gpio = OTB_RBOOT_DEFAULT_SOFT_RESET_PIN;

	// Figure out which GPIO is used for soft reset
	if (otb_eeprom_main_board_gpio_pins_g != NULL)
	{
		if (otb_eeprom_main_board_gpio_pins_g->hdr.magic == OTB_EEPROM_GPIO_PIN_INFO_MAGIC)
		{
			for (ii = 0;  ii < otb_eeprom_main_board_gpio_pins_g->num_pins; ii++)
			{
				if (otb_eeprom_main_board_gpio_pins_g->pin_info[ii].use == OTB_EEPROM_PIN_USE_RESET_SOFT)
				{
					gpio = otb_eeprom_main_board_gpio_pins_g->pin_info[ii].num;
					break;
				}
			}
		}
	}
	else
	{
    ets_printf("BOOT: Unknown soft reset pin\r\n");
		goto EXIT_LABEL;
	}
  
  // Check if GPIO14 is depressed
	ets_printf("BOOT: Checking GPIO%d ", gpio);

	// First of all set the pin to high - in case being weakly pulled low from reboot
  PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
  WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + GPIO_OUT_W1TS_ADDRESS, 1<<gpio);

  ets_intr_lock();

  for (ii = 0; ii <= FACTORY_RESET_LEN; ii++)
  {
    // Feed the dog!
    WRITE_PERI_REG(0x60000600 + 0x314, 0x73);
    gpio_state = get_gpio(gpio);
    if (gpio_state)
    {
      // Note hangs at this point and then watchdog resets.
      // Can't figure out why hangs - may be something to do with interrupts.
      ets_printf("o\r\n");
      ets_intr_unlock();
      goto EXIT_LABEL;
      break;
    }
    else
    {
      ets_printf("x");
      ets_delay_us(1000000);
    }
  }
  ets_printf("\r\n");

  ets_intr_unlock();

  // Would be lovely to flash the status LED at this point - would require
	// - reading status_led info from eeprom (which we already do)
	// - some WS2812B comms (which really isn't very hard - see otb_led_neo_update
	//   in otb_led.c)
  
  // If gpio14 is zero we have looped 15 times ...
  if (!gpio_state)
  {
    do_factory_reset();
    ets_delay_us(2000000);
    reset();
  }

EXIT_LABEL:

  return;
}

#ifdef BOOT_NO_ASM

// small stub method to ensure minimum stack space used
void call_user_start() {
	uint32 addr;
	stage2a *loader;
	
	start_otb_boot();
	read_eeprom();
	factory_reset();
	addr = find_image();
	if (addr != 0) {
		loader = (stage2a*)entry_addr;
		loader(addr);
	}
}

#else

// assembler stub uses no stack space
// works with gcc
void call_user_start() {
	__asm volatile (
		"mov a15, a0\n"          // store return addr, hope nobody wanted a15!
		"call0 start_otb_boot\n" // output some otb-iot info
		"mov a0, a15\n"          // restore return addr
		"call0 read_eeprom\n"  // See whether to reset to factory defaults
		"mov a0, a15\n"          // restore return addr
		"call0 factory_reset\n"  // See whether to reset to factory defaults
		"mov a0, a15\n"          // restore return addr
		"call0 find_image\n"     // find a good rom to boot
		"mov a0, a15\n"          // restore return addr
		"bnez a2, 1f\n"          // ?success
		"ret\n"                  // no, return
		"1:\n"                   // yes...
		"movi a3, entry_addr\n"  // actually gives us a pointer to entry_addr
		"l32i a3, a3, 0\n"       // now really load entry_addr
		"jx a3\n"                // now jump to it
	);
}

#endif
