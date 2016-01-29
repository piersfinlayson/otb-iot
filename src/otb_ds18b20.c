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

/*
 * Adaptation of Paul Stoffregen's One wire library to the ESP8266 and
 * Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu>
 * 
 * Paul's original library site:
 *   http://www.pjrc.com/teensy/td_libs_OneWire.html
 *   LGPL v2.1 
 *
 * Necromant's gihub site:
 *   https://github.com/nekromant/esp8266-frankenstein
 *   License ?
 * 
 * See also http://playground.arduino.cc/Learning/OneWire
 * 
 */

#define OTB_DS18B20_C
#include "otb.h"

void ICACHE_FLASH_ATTR otb_ds18b20_initialize(uint8_t bus)
{
  bool rc;
  void *vTask;

  DEBUG("DS18B20: otb_ds18b20_initialize entry");

  DEBUG("DS18B20: Initialize one wire bus on GPIO pin %d", bus);

  otb_ds18b20_init(bus);

  INFO("DS18B20: One Wire bus initialized");
  
  rc = otb_ds18b20_get_devices();

  INFO("DS18B20: DS18B20 device count %u", otb_ds18b20_count);
  if (rc)
  {
    WARN("DS18B20: More than %d DS18B20 devices - only reading from first %d",
         OTB_DS18B20_MAX_DS18B20S,
         OTB_DS18B20_MAX_DS18B20S);
  }
  
  if (otb_ds18b20_count > 0)
  {
#if 0
    // Schedule task to read DS18B20 values every REPORT_INTERVAL, forever,
    // starting ASAP.
    // Only do this is we have some DS18B20s attached.
    vTask = otb_sched_get_task(OTB_SCHED_OW_TASK);
    // Interval between being scheduled, and iterations (-1 = forever)
    otb_sched_repeat_task(vTask, OTB_DS18B20_REPORT_INTERVAL, -1);
    otb_sched_system_os_post(0, 0, 0, &otb_ds18b20_callback, vTask, 0);
#endif
    os_timer_disarm(&otb_ds18b20_timer);
    os_timer_setfn(&otb_ds18b20_timer, (os_timer_func_t *)otb_ds18b20_callback, NULL);
    os_timer_arm(&otb_ds18b20_timer, OTB_DS18B20_REPORT_INTERVAL, 1);
  }
  
  DEBUG("DS18B20: otb_ds18b20_initialize entry");
  
  return;
}

// Returns TRUE if more than OTB_DS18B20_MAX_DS18B20S devices
bool ICACHE_FLASH_ATTR otb_ds18b20_get_devices(void)
{
  int rc;
  char ds18b20[OTB_DS18B20_DEVICE_ADDRESS_LENGTH];
  char crc;
  
  otb_ds18b20_count = 0;
  
  do
  {
    rc = ds_search(ds18b20);
    if (rc)
    {
      // I want the address format in the same format as debian/raspbian
      // Which reverses the order of all but the first byte, and drops the CRC8
      // byte at the end (which we'll check).
      os_memcpy(otb_ds18b20_addresses[otb_ds18b20_count].addr,
                ds18b20, 
                OTB_DS18B20_DEVICE_ADDRESS_LENGTH);
      snprintf((char*)otb_ds18b20_addresses[otb_ds18b20_count].friendly,
               OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH,
               "%02x-%02x%02x%02x%02x%02x%02x",
               ds18b20[0],
               ds18b20[6],
               ds18b20[5],
               ds18b20[4],
               ds18b20[3],
               ds18b20[2],
               ds18b20[1]);
      crc = crc8(ds18b20, 7);
			if(crc != ds18b20[7])
			{
				WARN("DS18B20: CRC error: %s, crc=%xd",
				     otb_ds18b20_addresses[otb_ds18b20_count].friendly,
				     crc);
			}
      otb_ds18b20_count++;
      DEBUG("DS18B20: Successfully read device address %s",
            otb_ds18b20_addresses[otb_ds18b20_count].friendly);
    }
  } while (rc && (otb_ds18b20_count < OTB_DS18B20_MAX_DS18B20S));

  if (rc)
  {
    rc = ds_search(ds18b20);
  }
    
  return(rc);
}

void ICACHE_FLASH_ATTR otb_ds18b20_callback(void *arg)
{
  int chars;
  bool rc;
  
  DEBUG("DS18B20: otb_ds18b20_callback entry");

  // Read all the sensors.
  otb_ds18b20_prepare_to_read();
  for (int ii = 0; ii < otb_ds18b20_count; ii++)
  {
    rc = otb_ds18b20_request_temp(otb_ds18b20_addresses[ii].addr,
                                  otb_ds18b20_last_temp_s[ii]);
    if (!rc)
    {
      os_strcpy(otb_ds18b20_last_temp_s[ii], OTB_DS18B20_INTERNAL_ERROR_TEMP);
    }
    
    INFO("DS18B20: Device: %s temp: %s", otb_ds18b20_addresses[ii].friendly, otb_ds18b20_last_temp_s[ii]);
  }

  if (otb_mqtt_client.connState == MQTT_DATA)
  {
    DEBUG("DS18B20: Log sensor data");

    // Could send (but may choose not to), so reset disconnectedCounter
    otb_ds18b20_mqtt_disconnected_counter = 0;

    for (int ii; ii < otb_ds18b20_count; ii++)
    {
      if (strcmp(otb_ds18b20_last_temp_s[ii], "-127.00") &&
          strcmp(otb_ds18b20_last_temp_s[ii], OTB_DS18B20_INTERNAL_ERROR_TEMP) &&
          strcmp(otb_ds18b20_last_temp_s[ii], "85.00"))
      {
        // Decided on reporting using a single format to reduce require MQTT buffer
        // size.
        // Setting qos = 0 (don't care if gets lost), retain = 1 (always retain last
        // publish)
        // XXX Should replace with otb_mqtt_publish call
        snprintf(otb_mqtt_topic_s,
                 OTB_MQTT_MAX_TOPIC_LENGTH,
                 "/%s/%s/%s/%s/%s/%s/%s/%s",
                 OTB_MQTT_ROOT,
                 OTB_MQTT_LOCATION_1,
                 OTB_MQTT_LOCATION_2,
                 OTB_MQTT_LOCATION_3,
                 OTB_MAIN_CHIPID,
                 OTB_MQTT_LOCATION_4_OPT,
                 otb_ds18b20_addresses[ii].friendly,
                 OTB_MQTT_TEMPERATURE);
        DEBUG("DS18B20: Publish topic: %s", otb_mqtt_topic_s);
        DEBUG("DS18B20:       message: %s", otb_ds18b20_last_temp_s[ii]);
        chars = strlen(otb_ds18b20_last_temp_s[ii]);
        MQTT_Publish(&otb_mqtt_client, otb_mqtt_topic_s, otb_ds18b20_last_temp_s[ii], chars, 0, 1);
      }
    }
  }
  else
  {
    WARN("DS18B20: MQTT not connected, so not sending");
    otb_ds18b20_mqtt_disconnected_counter += 1;
  }

  if ((otb_ds18b20_mqtt_disconnected_counter * OTB_DS18B20_REPORT_INTERVAL) >=
                                                    OTB_MQTT_DISCONNECTED_REBOOT_INTERVAL)
  {
    ERROR("DS18B20: MQTT disconnected %d ms so resetting", 
    otb_ds18b20_mqtt_disconnected_counter * OTB_DS18B20_REPORT_INTERVAL);
    otb_reset();
  }

  DEBUG("DS18B20: otb_ds18b20_callback exit");

  return;
}

void ICACHE_FLASH_ATTR otb_ds18b20_prepare_to_read(void)
{
  DEBUG("DS18B20: otb_ds18b20_prepare_to_read entry");

  reset();
  write( DS1820_SKIP_ROM, 1 );
  write( DS1820_CONVERT_T, 1 );
  //os_delay_us( 750*1000 );
  otb_util_delay_ms(750);

  DEBUG("DS18B20: otb_ds18b20_prepare_to_read exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_ds18b20_request_temp(char *addr, char *temp_s)
{
  int ii;
  bool rc = FALSE;
	uint8_t data[12];
  uint16_t tVal, tFract;
  char tSign[2];
  
  DEBUG("DS18B20: otb_ds18b20_request_temp entry");

  reset();
  select(addr);
  write( DS1820_READ_SCRATCHPAD, 0 );
  for(ii = 0; ii< 9; ii++)
  {
    data[ii] = read();
  }

  // float arithmetic isn't really necessary, tVal and tFract are in 1/10 °C
  tVal = (data[1] << 8) | data[0];
  if (tVal & 0x8000)
  {
    tVal = (tVal ^ 0xffff) + 1;				// 2's complement
    tSign[0] = '-';
    tSign[1] = 0;
  } 
  else
  {
    tSign[0] = 0;
  }
  
  // datasize differs between DS18S20 and DS18B20 - 9bit vs 12bit
  if (addr[0] == DS18S20) {
    tFract = (tVal & 0x01) ? 50 : 0;		// 1bit Fract for DS18S20
    tVal >>= 1;
  }
  else 
  {
    tFract = (tVal & 0x0f) * 100 / 16;		// 4bit Fract for DS18B20
    tVal >>= 4;
  }
  
  snprintf(temp_s,
           OTB_DS18B20_MAX_TEMP_LEN,
           "%s%d.%02d",
           tSign, 
           tVal, 
           tFract);

  rc = TRUE;

  DEBUG("DS18B20: otb_ds18b20_request_temp exit");

  return(rc);
}

// global search state
static unsigned char ROM_NO[8];
static uint8_t LastDiscrepancy;
static uint8_t LastFamilyDiscrepancy;
static uint8_t LastDeviceFlag;
static int gpioPin;

void ICACHE_FLASH_ATTR otb_ds18b20_init(int gpio)
{
	gpioPin = gpio;

	PIN_FUNC_SELECT(pin_mux[gpioPin], pin_func[gpioPin]);
	//PIN_PULLDWN_DIS(pin_mux[gpioPin]);
  // Enable pullup resistor for this GPIO (should obviate the need for external resistor)
	PIN_PULLUP_EN(pin_mux[gpioPin]);  
	  
	GPIO_DIS_OUTPUT(gpioPin);

}

static void reset_search()
{
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;
	for(int i = 7; ; i--) {
		ROM_NO[i] = 0;
		if ( i == 0) break;
	}
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
static uint8_t reset(void)
{
	//	IO_REG_TYPE mask = bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
	uint8_t retries = 125;

	// noInterrupts();
	// DIRECT_MODE_INPUT(reg, mask);
	GPIO_DIS_OUTPUT( gpioPin );
	
	// interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		os_delay_us(2);
	} while ( !GPIO_INPUT_GET( gpioPin ));

	// noInterrupts();
	GPIO_OUTPUT_SET( gpioPin, 0 );
	// DIRECT_WRITE_LOW(reg, mask);
	// DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	// interrupts();
	os_delay_us(480);
	// noInterrupts();
	GPIO_DIS_OUTPUT( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	os_delay_us(70);
	// r = !DIRECT_READ(reg, mask);
	r = !GPIO_INPUT_GET( gpioPin );
	// interrupts();
	os_delay_us(410);

	return r;
}

/* pass array of 8 bytes in */
static int ds_search( uint8_t *newAddr )
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number;
	uint8_t id_bit, cmp_id_bit;
	int search_result;

	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		if (!reset())
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}

		// issue the search command
		write(DS1820_SEARCHROM, 0);

		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = read_bit();
			cmp_id_bit = read_bit();
	 
			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;
			else
			{
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit)
					search_direction = id_bit;  // bit write value for search
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					else
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					// if 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

				// serial number search direction write bit
				write_bit(search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		}
		while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}
	for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
	return search_result;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
static void write( uint8_t v, int power ) {
	uint8_t bitMask;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		write_bit( (bitMask & v)?1:0);
	}
	if ( !power) {
		// noInterrupts();
		GPIO_DIS_OUTPUT( gpioPin );
		GPIO_OUTPUT_SET( gpioPin, 0 );
		// DIRECT_MODE_INPUT(baseReg, bitmask);
		// DIRECT_WRITE_LOW(baseReg, bitmask);
		// interrupts();
	}
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline void write_bit( int v )
{
	// IO_REG_TYPE mask=bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	GPIO_OUTPUT_SET( gpioPin, 0 );
	if( v ) {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(10);
		GPIO_OUTPUT_SET( gpioPin, 1 );
		// DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		// interrupts();
		os_delay_us(55);
	} else {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(65);
		GPIO_OUTPUT_SET( gpioPin, 1 );
		//	DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		//		interrupts();
		os_delay_us(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline int read_bit(void)
{
	//IO_REG_TYPE mask=bitmask;
	//volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
  
	// noInterrupts();
	GPIO_OUTPUT_SET( gpioPin, 0 );
	// DIRECT_MODE_OUTPUT(reg, mask);
	// DIRECT_WRITE_LOW(reg, mask);
	os_delay_us(3);
	GPIO_DIS_OUTPUT( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	os_delay_us(10);
	// r = DIRECT_READ(reg, mask);
	r = GPIO_INPUT_GET( gpioPin );
	// interrupts();
	os_delay_us(53);

	return r;
}

//
// Do a ROM select
//
static void select(const uint8_t *rom)
{
	uint8_t i;

	write(DS1820_MATCHROM, 0);           // Choose ROM

	for (i = 0; i < 8; i++) write(rom[i], 0);
}

//
// Read a byte
//
static uint8_t read() {
	uint8_t bitMask;
	uint8_t r = 0;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		if ( read_bit()) r |= bitMask;
	}
	return r;
}

//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
static uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
