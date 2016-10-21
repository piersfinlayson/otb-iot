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
 *
 * Necromant's gihub site:
 *   https://github.com/nekromant/esp8266-frankenstein
 * 
 * See also http://playground.arduino.cc/Learning/OneWire
 * 
 */

typedef struct otbDs18b20DeviceAddress
{
  // Index in array
  uint8_t index;

  // Timer interval
  uint32_t timer_int;

  // Actual one wire address
  char addr[OTB_DS18B20_DEVICE_ADDRESS_LENGTH];
  
  // Friendly formatted address length
  char friendly[OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH];
} otbDs18b20DeviceAddress;

extern struct otbDs18b20DeviceAddress otb_ds18b20_addresses[OTB_DS18B20_MAX_DS18B20S];
extern uint8_t otb_ds18b20_count;

#ifdef OTB_DS18B20_C
// Globals
uint8_t otb_ds18b20_mqtt_disconnected_counter = 0;
struct otbDs18b20DeviceAddress otb_ds18b20_addresses[OTB_DS18B20_MAX_DS18B20S];
uint8_t otb_ds18b20_count = 0;
char otb_ds18b20_last_temp_s[OTB_DS18B20_MAX_DS18B20S][OTB_DS18B20_MAX_TEMP_LEN];
static volatile os_timer_t otb_ds18b20_timer[OTB_DS18B20_MAX_DS18B20S];
#endif

extern void otb_ds18b20_initialize(uint8_t bus);
extern void otb_ds18b20_callback(void *arg);
void otb_ds18b20_cmd(char *cmd0, char *cmd1, char *cmd2);
extern bool otb_ds18b20_get_devices(void);
extern char *otb_ds18b20_get_sensor_name(char *addr, otb_conf_ds18b20 **ds);
extern char *otb_ds18b20_get_addr(char *name, otb_conf_ds18b20 **ds);
bool otb_ds18b20_check_addr_format(char *addr);
bool otb_ds18b20_conf_set(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
extern void otb_ds18b20_conf_get(char *sensor, char *index);
extern void otb_ds18b20_prepare_to_read(void);
extern bool otb_ds18b20_request_temp(char *addr, char *temp_s);
void otb_ds18b20_init(int gpio);
static int ds_search( uint8_t *addr );
static void reset_search();
static void write_bit( int v );
static void write( uint8_t v, int parasitePower );
static inline int read_bit(void);
static inline void write_bit( int v );
static void select(const uint8_t rom[8]);
void ds_init( int gpio );
static uint8_t reset(void);
static uint8_t read();
static uint8_t crc8(const uint8_t *addr, uint8_t len);

#define DS1820_WRITE_SCRATCHPAD	0x4E
#define DS1820_READ_SCRATCHPAD	0xBE
#define DS1820_COPY_SCRATCHPAD	0x48
#define DS1820_READ_EEPROM	  0xB8
#define DS1820_READ_PWRSUPPLY	  0xB4
#define DS1820_SEARCHROM		    0xF0
#define DS1820_SKIP_ROM			    0xCC
#define DS1820_READROM			    0x33
#define DS1820_MATCHROM			    0x55
#define DS1820_ALARMSEARCH		  0xEC
#define DS1820_CONVERT_T		    0x44

// DS18x20 family codes
#define DS18S20		              0x10
#define DS18B20 	              0x28
