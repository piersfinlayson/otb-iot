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

// Addresses of eeproms/boards
#define OTB_EEPROM_MAIN_BOARD_ADDR   0x57
#define OTB_EEPROM_MIN_ADDR          0x50
#define OTB_EEPROM_MAX_ADDR          0x57

// Error codes - may be ORed together
#define OTB_EEPROM_ERR_OK        0
#define OTB_EEPROM_ERR           0x01000000   // Internal error
#define OTB_EEPROM_ERR_I2C       0x1    // Error contacting eeprom over I2C
#define OTB_EEPROM_ERR_MAGIC     0x2    // Magic value error
#define OTB_EEPROM_ERR_VERSION   0x4    // Unsupported version
#define OTB_EEPROM_ERR_CHECKSUM  0x8    // Invalid checksum
#define OTB_EEPROM_ERR_LENGTH    0x10   // Invalid Length
#define OTB_EEPROM_ERR_BUF_LEN_MAIN   0x20   // Buffer provided not big enough to read structure (ignoring any other components)
#define OTB_EEPROM_ERR_BUF_LEN_COMP   0x40   // Buffer provided not big enough to read structure plus all other components
#define OTB_EEPROM_ERR_NOT_FOUND      0x80   // Not found on eeprom
#define OTB_EEPROM_ERR_STRUCT_SIZE    0x100  // struct_size wrong
#define OTB_EEPROM_ERR_MAIN_COMP_TOO_BIG 0x200  // Main component bigger than OTB_EEPROM_MAX_MAIN_COMP_LENGTH

// No main component can be longer than this
#define OTB_EEPROM_MAX_MAIN_COMP_LENGTH 1024

#define OTB_EEPROM_COMP_0_OR_MORE  0
#define OTB_EEPROM_COMP_1          1
#define OTB_EEPROM_COMP_1_OR_MORE  2

#define OTB_EEPROM_MAX_MODULES    4

/*
 * The structure of an otbiot eeprom is as follows:
 *
 * otb_eeprom_info at location 0 which contains an array of structures pointing out to
 * different pieces of information (components).
 *
 * Those components are located as indicated in the structures.
 *
 * Hierarchical eeprom structure as follows:
 *
 * - otb_eeprom_info (all eeprom types)
 *   - otb_eeprom_hdr
 *   - otb_eeprom_info_comp[]
 *
 * - otb_eeprom_main_board (main boards)
 *   - otb_eeprom_hw_common
 *     - otb_eeprom_hdr
 *
 * - otb_eeprom_main_board_sdk_init_data (main boards)
 *   - otb_eeprom_hdr
 *   - sdk_init_data[]
 *
 * - otb_eeprom_main_board_gpio_pins (main boards)
 *   - otb_eeprom_hdr
 *   - otb_eeprom_pin_info[]
 *
 * - otb_eeprom_main_board_module (main boards - may be multiple of)
 *   - otb_eeprom_hdr
 *   - otb_eeprom_pin_info[]
 *
 * - otb_eeprom_main_module (module boards)
 *   - otb_eeprom_hw_common
 *     - otb_eeprom_hdr
 *
 * - otb_eeprom_main_module_pins (module boards)
 *   - otb_eeprom_hdr
 *   - otb_eeprom_pin_info[]
 *
 * A main board may have, for example:
 *
 * 0x0000 - otb_eeprom_info, pointing to:
 * 0x0800 - otb_eeprom_main_board
 * 0x1000 - otb_eeprom_main_board_sdk_init_data
 * 0x1800 - otb_eeprom_main_board_gpio_pins
 * 0x2000 - otb_eeprom_main_board_module (1)
 * 0x2800 - otb_eeprom_main_board_module (2)
 * 0x3000 - otb_eeprom_main_board_module (3)
 *
 * A module board may have:
 *
 * 0x0000 - otb_eeprom_info, pointing to:
 * 0x0800 - otb_eeprom_main_module
 * 0x1000 - otb_eeprom_main_module_pins
 *
 */ 

#define OTB_EEPROM_INFO_TYPE_INFO                0
#define OTB_EEPROM_INFO_TYPE_MAIN_BOARD          1
#define OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE   2
#define OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA       3
#define OTB_EEPROM_INFO_TYPE_GPIO_PINS           4
#define OTB_EEPROM_INFO_TYPE_NUM                 5

typedef struct otb_eeprom_main_comp_type
{
  // Type not included - is index into array of structs
  // uint32 type;

  unsigned char *name;

  uint32_t magic;

  uint32_t struct_size_min;

  uint32_t struct_size_max;

  uint32_t length_total;

  uint32_t version_min;

  uint32_t version_max;

  uint32_t default_loc;

  uint32_t max_num;

  uint32_t quantity;

  void **global;

} otb_eeprom_main_comp_type;

// Header structure for eeprom contents - mustn't be changed
typedef struct otb_eeprom_hdr
{
  // Magic number
  uint32 magic;
  
  // Header struct size
  uint32 struct_size;
  
  // Starts at 1 x(0x00000001)
  uint32 version;
  
  // Length of the entire structure this hdr is embedded within (and used to calculate
  // checksum)
  uint32 length;
  
  // Checksum of the entire structure (which includes but is not limited to this header,
  // ignoring the checksum itself.
#define OTB_EEPROM_CHECKSUM_INITIAL  0x12345678
  uint32 checksum;
  
} otb_eeprom_hdr;

// Structure containing details about information on the eeprom
typedef struct otb_eeprom_info_comp
{
  // Type of information - OTB_EEPROM_INFO_TYPE_
  uint32 type;
  
  // Offset from start of eeprom of the information
  uint32 location;
  
  // Length of the information
  uint32 length;

} otb_eeprom_info_comp;

typedef struct otb_eeprom_info
{
#define OTB_EEPROM_INFO_MAGIC    0xbc13ee7a
  otb_eeprom_hdr hdr;
  
  // Size of installed eeprom in bytes (not bits)
  uint32 eeprom_size;
  
  // Number of component structures which follow
#define OTB_EEPROM_INFO_MAX_COMP  16
  uint32 comp_num;
  
  // Eeprom data write date, encoded 0xYYYYMMDD (YYYY in hex, MM in hex, DD in hex)
  uint32 write_date;
  
  // Beginning of a list of structures pointing to information components.  This
  // component (otb_eeprom_info) is not pointed to.
  otb_eeprom_info_comp comp[];

} otb_eeprom_info;

// Structure containing sdk_init_data - only used by bootloader
typedef struct otb_eeprom_main_board_sdk_init_data
{
#define OTB_EEPROM_MAIN_BOARD_SDK_INIT_DATA_MAGIC    0xb3b8d62c
  otb_eeprom_hdr hdr;

  uint32 data_len;

  // The sdk_init_data
  unsigned char sdk_init_data[];

} otb_eeprom_main_board_sdk_init_data;

// Structure containing information common to multiple eeprom info types
typedef struct otb_eeprom_hw_common
{
 // Version 1 fields
  otb_eeprom_hdr hdr;
  
  // Serial number of this unit, as a string, NULL terminated
#define OTB_EEPROM_HW_SERIAL_LEN   15
  unsigned char serial[OTB_EEPROM_HW_SERIAL_LEN+1];
  
  // otb-iot hardware code
#define OTB_EEPROM_HW_CODE_GENERIC      0x0 // Use for any off the shelf or non OTB-IOT ESP8266 module
#define OTB_EEPROM_HW_CODE_MAIN_BOARD   0x001 // otb-iot main board
#define OTB_EEPROM_HW_CODE_MAIN_MODULE  0x101 // otb-iot main module
  uint32 code;
  
  // otb-iot hardware subcode
#define OTB_EEPROM_HW_SUBCODE_MAIN_BOARD_NONE         0x0
#define OTB_EEPROM_HW_SUBCODE_MAIN_BOARD_OTB_IOT_0_4  0x001 // otb-iot v0.4
#define OTB_EEPROM_HW_SUBCODE_MAIN_BOARD_OTB_IOT_0_5  0x002 // otb-iot v0.5 April 2017
#define OTB_EEPROM_HW_SUBCODE_MAIN_MODULE_MEZZ        0x101
#define OTB_EEPROM_HW_SUBCODE_MAIN_MODULE_PROG        0x201
  uint32 subcode;

} otb_eeprom_hw_common;

// Structure of data on the eeprom containing main board configuration
typedef struct otb_eeprom_main_board
{
  // Info common to multiple eeprom info types
#define OTB_EEPROM_MAIN_BOARD_MAGIC    0xa140c5ad
  otb_eeprom_hw_common common;

  // The ESP8266 chip ID
  unsigned char chipid[3];
  char pad1[1];
  
  // The ESP8266 MAC addresses
  unsigned char mac1[6];
  unsigned char mac2[6];

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
  
  // Number of modules
  uint32 num_modules;
  
} otb_eeprom_main_board;

// Information about individual pins
typedef struct otb_eeprom_pin_info
{
  // Pin number  for main_board this is gpio num, for modules this is pin num
  // within the header - which is next
  uint32 num;

  // Header number (not valid when included from otb_eeprom_main_board_gpio_pins)
  // (LSB = header 1, next LSB = header 2, etc)
#define OTB_EEPROM_PIN_HEADER_NONE  0x00000000
#define OTB_EEPROM_PIN_HEADER_1     0x00000001
#define OTB_EEPROM_PIN_HEADER_2     0x00000002
  uint32 header_num;

  // What purpose pin is used for on the hardware (status led, general gpio, etc)
#define OTB_EEPROM_PIN_USE_NC          0
#define OTB_EEPROM_PIN_USE_RESERVED    1
#define OTB_EEPROM_PIN_USE_GND         2
#define OTB_EEPROM_PIN_USE_V33         3
#define OTB_EEPROM_PIN_USE_V5          4
#define OTB_EEPROM_PIN_USE_GPIO        5
#define OTB_EEPROM_PIN_USE_STATUS_LED  6
#define OTB_EEPROM_PIN_USE_RESET_HARD  7
#define OTB_EEPROM_PIN_USE_RESET_SOFT  8
#define OTB_EEPROM_PIN_USE_INT_SDA     9
#define OTB_EEPROM_PIN_USE_INT_SCL     10
#define OTB_EEPROM_PIN_USE_TOUT        11
#define OTB_EEPROM_PIN_USE_TX          13
#define OTB_EEPROM_PIN_USE_RX          14
#define OTB_EEPROM_PIN_USE_D_PLUS      15
#define OTB_EEPROM_PIN_USE_D_MINUS     16
#define OTB_EEPROM_PIN_USE_WP          17
#define OTB_EEPROM_PIN_USE_ADDR_0      18
#define OTB_EEPROM_PIN_USE_ADDR_1      19
#define OTB_EEPROM_PIN_USE_ADDR_2      20
#define OTB_EEPROM_PIN_USE_JACK_1      21
#define OTB_EEPROM_PIN_USE_JACK_2      22
#define OTB_EEPROM_PIN_USE_JACK_3      23
  uint32 use;
  
  // For main boards what modules this pin is exposed to (LSB = module 1,
  // next LSB = module 2, etc)
#define OTB_EEPROM_PIN_MODULE_NONE  0x00000000
#define OTB_EEPROM_PIN_MODULE_1     0x00000001
#define OTB_EEPROM_PIN_MODULE_2     0x00000002
#define OTB_EEPROM_PIN_MODULE_3     0x00000004
  uint32 module;
  
  // Further information about this pin:
  // - For otb_eeprom_main_board_gpio_pins, any extra information about pin - for status led the type
  // - For otb_eeprom_main_board_module_info, when GPIO pin, which GPIO (as in that case pin number is pin of the header!)

#define OTB_EEPROM_PIN_FINFO_NONE             0

// When use = OTB_EEPROM_PIN_USE_STATUS_LED
#define OTB_EEPROM_PIN_FINFO_LED_TYPE_NEO     1

// When use = OTB_EEPROM_PIN_USE_INT_SDA and OTB_EEPROM_PIN_USE_INT_SCL
#define OTB_EEPROM_PIN_FINFO_I2C_V33          1
#define OTB_EEPROM_PIN_FINFO_I2C_V5           2

#define OTB_EEPROM_PIN_FINFO_RSVD_FLASH       1

#define OTB_EEPROM_PIN_FINFO_JACK_TYPE_3_5MM  1
#define OTB_EEPROM_PIN_FINFO_JACK_TYPE_RJ11   2

#define OTB_EEPROM_PIN_FINFO_FUSE_100MA      1

#define OTB_EEPROM_PIN_FINFO_GPIO_0          0x100
#define OTB_EEPROM_PIN_FINFO_GPIO_1          0x101
#define OTB_EEPROM_PIN_FINFO_GPIO_2          0x102
#define OTB_EEPROM_PIN_FINFO_GPIO_3          0x103
#define OTB_EEPROM_PIN_FINFO_GPIO_4          0x104
#define OTB_EEPROM_PIN_FINFO_GPIO_5          0x105
#define OTB_EEPROM_PIN_FINFO_GPIO_6          0x106
#define OTB_EEPROM_PIN_FINFO_GPIO_7          0x107
#define OTB_EEPROM_PIN_FINFO_GPIO_8          0x108
#define OTB_EEPROM_PIN_FINFO_GPIO_9          0x109
#define OTB_EEPROM_PIN_FINFO_GPIO_10         0x10a
#define OTB_EEPROM_PIN_FINFO_GPIO_11         0x10b
#define OTB_EEPROM_PIN_FINFO_GPIO_12         0x10c
#define OTB_EEPROM_PIN_FINFO_GPIO_13         0x10d
#define OTB_EEPROM_PIN_FINFO_GPIO_14         0x10e
#define OTB_EEPROM_PIN_FINFO_GPIO_15         0x10f
#define OTB_EEPROM_PIN_FINFO_GPIO_16         0x110

#define OTB_EEPROM_PIN_FINFO_WP_MODULE       1
#define OTB_EEPROM_PIN_FINFO_WP_MAIN_BOARD   2

  uint32 further_info;

  // Pulled to what voltage
#define OTB_EEPROM_PIN_PULLED_NA     0
#define OTB_EEPROM_PIN_PULLED_FLOAT  1
#define OTB_EEPROM_PIN_PULLED_V0     2
#define OTB_EEPROM_PIN_PULLED_V33    3
#define OTB_EEPROM_PIN_PULLED_V5    4
  uint8 pulled;

  uint8 pad1[3];
  
} otb_eeprom_pin_info;

// Information on main board gpio pins
typedef struct otb_eeprom_main_board_gpio_pins
{
 // Version 1 fields
#define OTB_EEPROM_GPIO_PIN_INFO_MAGIC      0x8a60ebe8
#define OTB_EEPROM_GPIO_PIN_INFO_VERSION_1  1
  otb_eeprom_hdr hdr;
  
  // Number of GPIO pins
  uint32 num_pins;

  // Information about the pins.  Index into array is not informational.
  otb_eeprom_pin_info pin_info[];
  
} otb_eeprom_main_board_gpio_pins;

typedef struct otb_eeprom_main_board_module
{
#define OTB_EEPROM_MAIN_BOARD_MODULE_MAGIC  0x498af75e
  otb_eeprom_hdr hdr;

  // Module port number (no need to start at 1, or to be in order but must be unique)
  uint32 num;
  
  // Module type
#define OTB_EEPROM_MODULE_TYPE_MEZZ              1
#define OTB_EEPROM_MODULE_TYPE_PROG              2
#define OTB_EEPROM_MODULE_TYPE_DOUBLE_MEZZ       3  // Used by otb_eeprom_main_module only
#define OTB_EEPROM_MODULE_TYPE_DOUBLE_MEZZ_PROG  4  // Used by otb_eeprom_main_module only
  uint32 socket_type;
  
  // Number of headers
  uint32 num_headers;
  
  // Total number of pins exposed on this module port (across all headers)
  uint32 num_pins;

  // Module's address;
  uint8 address;
  uint8 pad1[3];
  
  // Information about all pins
  otb_eeprom_pin_info pin_info[];
  
} otb_eeprom_main_board_module;

// Structure of data on the eeprom containing main module configuration
typedef struct otb_eeprom_main_module
{
  // Info common to multiple eeprom info types
#define OTB_EEPROM_HW_MAIN_MODULE_MAGIC      0xeb6438dc

  otb_eeprom_hw_common common;

  // Type of this module
#define OTB_EEPROM_MODULE_TYPE_PROG_V0_1  0x00000001  // CP2104 based programmer
#define OTB_EEPROM_MODULE_TYPE_ADC_V0_1   0x00000101  // ADS1115 board
#define OTB_EEPROM_MODULE_TYPE_LL_V0_1    0x00000201  // Logic level shifter 3.3V-5V
  uint32 module_type;
  
  // Socket type required by this module (see otb_eeprom_main_board_module_info)
  uint32 socket_type;

  // Whether jack connected to this module is used
  uint8 jack_used;
  uint8 pad1[3];
  
} otb_eeprom_main_module;

// Information on module pins
typedef struct otb_eeprom_main_module_pins
{
 // Version 1 fields
#define OTB_EEPROM_MAIN_MODULE_PINS_MAGIC      0x9f99eb64
  otb_eeprom_hdr hdr;
  
  // Number of headers
  uint32 num_headers;
  
  // Total number of pins exposed on this module (across all headers)
  uint32 num_pins;
  
  // Information about the pins.  Index into array is not informational.
  otb_eeprom_pin_info pin_info[];
  
} otb_eeprom_main_module_pins;

// Global pointers to eeprom structures (and info_comps)
// NULL if not yet read from eeprom, or couldn't be read from eeprom.
#ifndef OTB_EEPROM_C

extern otb_eeprom_info *otb_eeprom_info_g;
extern otb_eeprom_main_board *otb_eeprom_main_board_g;
extern otb_eeprom_main_board_module *otb_eeprom_main_board_module_g;
extern otb_eeprom_main_board_gpio_pins *otb_eeprom_main_board_gpio_pins_g[OTB_EEPROM_MAX_MODULES];
#else // OTB_EEPROM_C

otb_eeprom_info *otb_eeprom_info_g;
uint8_t otb_eeprom_main_board_addr = OTB_EEPROM_MAIN_BOARD_ADDR;
uint32_t otb_eeprom_size = OTB_EEPROM_SIZE_128;
otb_eeprom_main_board *otb_eeprom_main_board_g;
otb_eeprom_main_board_module *otb_eeprom_main_board_module_g[OTB_EEPROM_MAX_MODULES];
otb_eeprom_main_board_gpio_pins *otb_eeprom_main_board_gpio_pins_g;
#endif // OTB_EEPROM_C

#ifndef OTB_EEPROM_C
otb_eeprom_main_comp_type otb_eeprom_main_comp_types[OTB_EEPROM_INFO_TYPE_NUM];
#else // OTB_EEPROM_C
otb_eeprom_main_comp_type otb_eeprom_main_comp_types[OTB_EEPROM_INFO_TYPE_NUM] =
{
  {"otb_eeprom_info",
   OTB_EEPROM_INFO_MAGIC,
   sizeof(otb_eeprom_info),
   sizeof(otb_eeprom_info),
   0,
   1,
   1,
   0x0,
   OTB_EEPROM_COMP_1,
   1,
   (void**)&otb_eeprom_info_g},
  {"otb_eeprom_main_board",
   OTB_EEPROM_MAIN_BOARD_MAGIC,
   sizeof(otb_eeprom_main_board),
   sizeof(otb_eeprom_main_board),
   0,
   1,
   1,
   0x400,
   OTB_EEPROM_COMP_1,
   1,
   (void**)&otb_eeprom_main_board_g},
  {"otb_eeprom_main_board_module",
   OTB_EEPROM_MAIN_BOARD_MODULE_MAGIC,
   sizeof(otb_eeprom_main_board_module),
   sizeof(otb_eeprom_main_board_module),
   0,
   1,
   1,
   0x800,
   OTB_EEPROM_COMP_1_OR_MORE,
   OTB_EEPROM_MAX_MODULES,
   (void**)&otb_eeprom_main_board_module_g},
  {"otb_eeprom_main_board_sdk_init_data",
   OTB_EEPROM_MAIN_BOARD_SDK_INIT_DATA_MAGIC,
   sizeof(otb_eeprom_main_board_sdk_init_data),
   sizeof(otb_eeprom_main_board_sdk_init_data),
   0,
   1,
   1,
   0x3000,
   OTB_EEPROM_COMP_0_OR_MORE,
   1,
   NULL},
  {"otb_eeprom_main_board_gpio_pins",
   OTB_EEPROM_GPIO_PIN_INFO_MAGIC,
   sizeof(otb_eeprom_main_board_gpio_pins),
   sizeof(otb_eeprom_main_board_gpio_pins),
   0,
   1,
   1,
   0xc00,
   OTB_EEPROM_COMP_1,
   1,
   (void**)&otb_eeprom_main_board_gpio_pins_g},
};
#endif // OTB_EEPROM_C

#ifndef OTB_HWINFO_C
void otb_eeprom_read(void);
char otb_eeprom_init(uint8_t addr, brzo_i2c_info *i2c_info);
void otb_eeprom_read_all(uint8_t addr,
                         brzo_i2c_info *i2c_info);
void otb_eeprom_process_all(void);
void otb_eeprom_read_main_types(uint8_t addr,
                                brzo_i2c_info *i2c_info,
                                otb_eeprom_info *eeprom_info,
                                uint32_t types[],
                                uint32_t types_num);
void *otb_eeprom_load_main_comp(uint8_t addr,
                                brzo_i2c_info *i2c_info,
                                otb_eeprom_info *eeprom_info,
                                uint32_t type,
                                void *buf,
                                uint32_t buf_len);
bool otb_eeprom_find_main_comp(otb_eeprom_info *eeprom_info,
                               uint32_t type, 
                               uint32_t *loc,
                               uint32_t *length);
bool otb_eeprom_process_hdr(otb_eeprom_hdr *hdr,
                            uint32_t type,
                            uint32_t buf_len,
                            uint32_t *rc);
uint32_t otb_eeprom_read_main_comp(uint8_t addr,
                                   brzo_i2c_info *i2c_info,
                                   otb_eeprom_info *eeprom_info,
                                   uint32_t type,
                                   void *read_buf,
                                   uint32_t buf_len);
char otb_eeprom_check_checksum(char *data,
                               int size,
                               int checksum_loc,
                               int checksum_size);
#endif // OTB_HWINFO_C

#endif // OTB_EEPROM_H_INCLUDED
