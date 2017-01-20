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

#ifndef OTB_EEPROM_H_INCLUDED
#define OTB_EEPROM_H_INCLUDED

// Size of 24LC128 EEPROM (default) in bytes (128KBit, 16KByte, 0x4000)
#define OTB_EEPROM_SIZE_128          (128*1024/8)
#define OTB_EEPROM_ADDR              0x57

// Header structure for eeprom contents - mustn't be changed
typedef struct otb_eeprom_hdr_conf
{
  // Magic number
  uint32 magic;
  
  // Struct size
  uint32 struct_size;
  
  // Starts at 1 x(0x00000001)
  uint32 version;
  
  // Checksum of the entire structure (which includes but is not limited to this header,
  // ignoring the checksum itself.
#define OTB_EEPROM_CHECKSUM_INITIAL  0x12345678
  uint32 checksum;
  
} otb_eeprom_hdr_conf;

#define OTB_EEPROM_GLOB_CONFIG_LOC  0x0000  // Always located at address 0x0 on eeprom
typedef struct otb_eeprom_glob_conf
{
  // Version 1 fields
#define OTB_EEPROM_GLOB_MAGIC  0xd108b915
#define OTB_EEPROM_GLOB_VERSION_1  1
  otb_eeprom_hdr_conf hdr;
  
  // Size of installed eeprom in bytes (not bits)
  uint32 eeprom_size;
  
  // Location of hardware structure in bytes from start of eeprom
#define OTB_EEPROM_HW_STRUCT_LOC_DEFAULT 0x400 // Located at 1K
#define OTB_EEPROM_HW_STRUCT_MAX_LEN     0x400 // 1K
  uint32 loc_hw_struct;
  uint32 loc_hw_struct_len;
  
  // Location of hardware structure signature in bytes from start of eeprom
#define OTB_EEPROM_HW_STRUCT_SIGN_LOC_DEFAULT 0x800 // Located at 2K
#define OTB_EEPROM_HW_STRUCT_SIGN_MAX_LEN     0x400 // 1K - note this is max len - actual len is stored in loc_hw_struct_sign_len below
  uint32 loc_hw_struct_sign;
  uint32 loc_hw_struct_sign_len;

  // Location of esp_init_data_default.bin.  This is used to reset the SDK
  // data on factory reset.  Location is in bytes from start of eeprom
#define OTB_EEPROM_SDK_INIT_DATA_LOC_DEFAULT  0xc00 // Located at 3K
#define OTB_EEPROM_SDK_INIT_DATA_MAX_LEN      0x400 // 1K - note this is max len - actual len is stored in loc_sdk_init_data_len below
  uint32 loc_sdk_init_data;
  uint32 loc_sdk_init_data_len;
  
  // Hashing, padding and signing algorithm for hw structure signing process
#define OTB_EEPROM_SIGN_ALGORITHM_NONE  0 // No signed structure provided
#define OTB_EEPROM_SIGN_ALGORITHM_1     1 // Sample algorithm
  uint32 hash_algorithm;
  
  // Signing key length in bytes
#define OTB_EEPROM_SIGN_KEY_LEN_BITS  (4096/8)
  uint32 sign_key_len_bytes;

  // Version 2 fields follow

} otb_eeprom_glob_conf;

// Structure of data on the eeprom containing hardware configuration
typedef struct otb_eeprom_hw_conf
{
 // Version 1 fields
#define OTB_EEPROM_HW_MAGIC    0xa140c5ad
#define OTB_EEPROM_HW_VERSION_1  1
  otb_eeprom_hdr_conf hdr;
  
  // Serial number of this unit, as a string, NULL terminated
#define OTB_EEPROM_HW_SERIAL_LEN   15
  unsigned char serial[OTB_EEPROM_HW_SERIAL_LEN+1];
  
  // otb-iot hardware code
#define OTB_EEPROM_HW_CODE_GENERIC  0x0 // Use for any off the shelf or non OTB-IOT ESP8266 module
#define OTB_EEPROM_HW_CODE_OTB_IOT  0x1 // otb-iot custom hardware
  uint32 code;
  
  // otb-iot hardware subcode
#define OTB_EEPROM_HW_SUBCODE_NONE         0x0
#define OTB_EEPROM_HW_SUBCODE_OTB_IOT_0_4  0x1 // otb-iot v0.4
  uint32 subcode;

  // The ESP8266 chip ID
  unsigned char chipid[3];
  char pad1[1];
  
  // The ESP8266 MAC addresses
  uint8 mac1[6];
  uint8 mac2[6];

  // What ESP module this unit is based on
#define OTB_EEPROM_HW_ESP12   1
#define OTB_EEPROM_HW_ESP07S  2
  uint32 esp_module;
  
  // Flash size in bytes - should match flash size encoded at beginning of esp8266 flash
#define OTB_EEPROM_HW_DEFAULT_FLASH_SIZE_BYTES  (4 * 1024 * 1024)
  uint32 flash_size_bytes;
  
  // I2C ADC included (values may be ORed together (support for up to 32 different
  // ADC type/addresses
#define OTB_EEPROM_HW_I2C_ADC_NONE          0x0
#define OTB_EEPROM_HW_I2C_ADC_ADS1115_I2C   0x1
  uint32 i2c_adc;
  
  // Internal ADC configuration
#define OTB_EEPROM_HW_INT_ADC_NONE          0  // Not connected
#define OTB_EEPROM_HW_INT_ADC_3V3_10K_2K49  1  // Connected to 3.3V supply voltage via 10K, 2.49K voltage divider
  uint32 internal_adc_type;
  
  // Internal I2C pin information - not supported on any current otb-iot hw designs
#define OTB_EEPROM_I2C_INT_SDA_PIN_NONE   -1
#define OTB_EEPROM_I2C_INT_SDA_PIN_GPIO4  4
#define OTB_EEPROM_I2C_INT_SDA_PIN_GPIO5  5
#define OTB_EEPROM_I2C_INT_SCL_PIN_NONE   -1
#define OTB_EEPROM_I2C_INT_SCL_PIN_GPIO5  5
#define OTB_EEPROM_I2C_INT_SCL_PIN_GPIO4  4
  signed char i2c_int_sda_pin;
  signed char i2c_int_scl_pin;
  
  // External I2C pin information
#define OTB_EEPROM_I2C_EXT_SDA_PIN_NONE   -1
#define OTB_EEPROM_I2C_EXT_SDA_PIN_GPIO4  4
#define OTB_EEPROM_I2C_EXT_SDA_PIN_GPIO5  5
#define OTB_EEPROM_I2C_EXT_SCL_PIN_NONE   -1
#define OTB_EEPROM_I2C_EXT_SCL_PIN_GPIO5  5
#define OTB_EEPROM_I2C_EXT_SCL_PIN_GPIO4  4
  signed char i2c_ext_sda_pin;
  signed char i2c_ext_scl_pin;
  
  // Version 2 fields follow

} otb_eeprom_hw_conf;

typedef struct otb_eeprom_sdk_init_data
{
 // Version 1 fields
#define OTB_EEPROM_SDK_INIT_DATA_MAGIC    0xab690a1f
#define OTB_EEPROM_SDK_INIT_DATA_VERSION_1  1
  otb_eeprom_hdr_conf hdr;

  // Note size in hdr is struct + data which follows

} otb_eeprom_sdk_init_data;

#ifndef OTB_EEPROM_C

extern otb_eeprom_glob_conf otb_eeprom_glob;
extern otb_eeprom_hw_conf otb_eeprom_hw;

#else

uint8_t otb_eeprom_addr = OTB_EEPROM_ADDR;
uint32_t otb_eeprom_size = OTB_EEPROM_SIZE_128;
  

otb_eeprom_glob_conf otb_eeprom_glob;
otb_eeprom_hw_conf otb_eeprom_hw;

#endif // OTB_EEPROM_C

char otb_eeprom_init(void);
char otb_eeprom_read_all(void);
char otb_eeprom_read_sdk_init_data(otb_eeprom_glob_conf *glob_conf, unsigned char *buf, uint32 buf_len);
char otb_eeprom_read_glob_conf(otb_eeprom_glob_conf *glob_conf);
char otb_eeprom_check_checksum(char *data, int size, int checksum_loc, int checksum_size);
char otb_eeprom_read_hw_conf(otb_eeprom_glob_conf *glob_conf, otb_eeprom_hw_conf *hw_conf);

#endif // OTB_EEPROM_H_INCLUDED
