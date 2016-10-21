//////////////////////////////////////////////////
// rBoot open source boot loader for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
//////////////////////////////////////////////////

#ifdef RBOOT_INTEGRATION
#include <rboot-integration.h>
#endif

#include "rboot-private.h"
#include <rboot-hex2a.h>
#include "otb_flash.h"

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

static uint32 get_gpio14() {
  uint32 x;
  uint32 *pin_in = (uint32 *)0x60000318;
#if 0  
  uint32 *pin_dir_input = (uint32 *)0x60000314;
  uint32 *pin_out_set = (uint32 *)0x60000304;
  uint32 *pin_out = (uint32 *)0x60000300;
#endif  
#define GPIO14_MASK  (1 << 14)

  //ets_printf("BOOT: Reading GPIO14 ... ");
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
#if 0  
  // uint32 x = (READ_PERI_REG(GPIO14) & 1); 
  ets_printf("BOOT: GPIO14 via macro: 0x%08x\n", READ_PERI_REG(GPIO14));

  ets_printf("BOOT: before all pins output: 0x%08x input: 0x%08x\n", *pin_out, *pin_in);
  *pin_out_set = GPIO14_MASK;
  *pin_dir_input = GPIO14_MASK;
#endif  
  x = (*pin_in & GPIO14_MASK) >> 14;
#if 0
  ets_printf("BOOT:  after all pins output: 0x%08x input: 0x%08x\n", *pin_out, *pin_in);
#endif
  //ets_printf("%d\r\n", x);
	
	return x;
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

void NOINLINE start_otb_boot(void)
{
	// delay to slow boot (help see messages when debugging)
	ets_delay_us(2000000);
	
	ets_printf("\r\nBOOT: OTA-BOOT v0.1\r\n");
	ets_printf("BOOT: OTA-Boot based on rBoot v1.2.1 - https://github.com/raburton/rboot\r\n");

  return;
}

// otb-iot factory reset function
#define FACTORY_RESET_LEN  15  // seconds
#define OTB_BOOT_ROM_0_LEN            0xfe000
#define OTB_BOOT_ROM_1_LEN            0xfe000
#define OTB_BOOT_ROM_2_LEN            0xfa000
#define OTB_BOOT_ROM_0_LOCATION        0x2000  // length 0xFE000 = 896KB
#define OTB_BOOT_ROM_1_LOCATION      0x202000  // length 0xFE000 = 896KB
#define OTB_BOOT_ROM_2_LOCATION      0x302000  // length 0xFA000 = 880KB

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
  ets_printf("BOOT: Config cleared\r\n");

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

  return;
}

void NOINLINE factory_reset(void)
{
  uint32 ii;
  uint32 gpio14;
  
  // Check if GPIO14 is depressed
	ets_printf("BOOT: Checking GPIO14 ");
  for (ii = 0; ii <= FACTORY_RESET_LEN; ii++)
  {
    gpio14 = get_gpio14();
    if (gpio14)
    {
      ets_printf("o");
      break;
    }
    else
    {
      ets_printf("x");
      ets_delay_us(1000000);
    }
  }
  ets_printf("\r\n");
  
  // Would be lovely to flash the status LED at this point, but that's beyond our
  // abilities - as we would need to speak I2C to do so!
  
  // If gpio14 is zero we have looped 15 times ...
  if (!gpio14)
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
