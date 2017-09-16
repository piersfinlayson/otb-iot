/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017 Piers Finlayson
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
 
#ifndef OTB_HWINFO_H_INCLUDED
#define OTB_HWINFO_H_INCLUDED

#include "otb_hwinfo_sdk_init_data.h"

// Endianess #defines
#define OTB_HWINFO_ENDIAN_TEST_UINT32               0x01020304
#define OTB_HWINFO_ENDIAN_TEST_BYTE0_LITTLE_ENDIAN  0x04
#define OTB_HWINFO_ENDIAN_TEST_BYTE0_BIG_ENDIAN     0x00

#define OTB_HWINFO_ENDIAN_LITTLE  1
#define OTB_HWINFO_ENDIAN_BIG     2

#define OTB_HWINFO_ENDIAN_ESP8266 OTB_HWINFO_ENDIAN_LITTLE

#define OTB_HWINFO_STORE(D, S)                                               \
{                                                                            \
  size_t ss = sizeof(S);                                                     \
  size_t sd = sizeof(D);                                                     \
  assert(ss == sd);                                                          \
  otb_hwinfo_check_endian_set();                                             \
  if (otb_hwinfo_host_endian != otb_hwinfo_target_endian)                    \
  {                                                                          \
    otb_hwinfo_store_field((char*)&S, (char*)&D, ss);                        \
  }                                                                          \
  else                                                                       \
  {                                                                          \
    D = S;                                                                   \
  }                                                                          \
}

#define OTB_HWINFO_CHECKSUM_DO(S)                                            \
{                                                                            \
  int checksum_loc;                                                          \
  size_t checksum_size;                                                      \
  size_t total_size;                                                         \
  checksum_size = sizeof(S->checksum);                                       \
  checksum_loc = (char *)&(S->checksum) - (char *)(S);                       \
  total_size = S->length;                                                    \
  otb_hwinfo_checksum_store(S, total_size, checksum_loc, checksum_size);     \
}

// Used by hwinfo to store information to write to flash
typedef struct otb_hwinfo_info
{
  uint8 chipid[3];
  uint8 mac1[3];  // Only the header
  uint8 mac2[3];  // Only the header
  uint32 code;
  uint32 subcode;
  uint8 serial[16];
  uint32 eeprom_size;
  uint32 esp_module;
  uint32 flash_size_bytes;
  uint32 i2c_adc;
  uint32 internal_adc_type;
  const otb_hwinfo_main_board_info *board_info;
  uint8 *output;
  uint32 output_len;
} otb_hwinfo_info;

// Globals
#ifdef OTB_HWINFO_C
uint8 otb_hwinfo_host_endian;
uint8 otb_hwinfo_target_endian;
bool otb_hwinfo_verbose;
char otb_hwinfo_fn[] = "hwinfo.out";
otb_hwinfo_info hwinfo;
#endif

// Function prototypes
int main(int argc, char **argv);
static error_t otb_hwinfo_parse_opt(int key, char *arg, struct argp_state *state);
void otb_hwinfo_setup(void);
void otb_hwinfo_checksum_store(void *data, size_t total_size, int checksum_loc, size_t checksum_size);
bool otb_hwinfo_store(void);
bool otb_hwinfo_test_sizes(void);
void otb_hwinfo_setup_endian(void);
void otb_hwinfo_store_field(char *s, char *d, uint8 b);
void otb_hwinfo_check_endian_set(void);
void otb_hwinfo_output_hdr(char *prefix, char* type, otb_eeprom_hdr *hdr);
otb_eeprom_hdr *otb_hwinfo_get_next_hdr(otb_eeprom_hdr *hdr);
void otb_hwinfo_output_eeprom_info(char *prefix, otb_eeprom_hdr *hdr);
void otb_hwinfo_output_pin_info(char *prefix, uint32 num_pins, otb_eeprom_pin_info *pin_info);
void otb_hwinfo_output_mac(char *prefix, char *name, unsigned char *mac);
void otb_hwinfo_output_comps(char *prefix, otb_eeprom_hdr *hdr);
void otb_hwinfo_output(void);

// Command line argument stuff
const char *argp_program_version = "otb-iot hwinfo v0.2\nCopyright (c) 2017 Piers Finlayson";
const char *argp_program_bug_address = "piers@piersandkatie.com";
static char otb_hwinfo_doc[] =
  "\n"
  "hwinfo is used to create binary files containing valid otb-iot eeprom configuration "
  "information.\n"
  "\n"
  "This eeprom configuration is written at the factory, and then read by the otb-iot "
  "software to detect various hardware capabilities and configuration options.  This "
  "allows the otb-iot software to automatically adapt its function and behaviour based "
  "on the available hardware.\n"
  "\n"
  "The data contained in the binary files produced by hwinfo must be written to the "
  "eeprom on board each otb-iot module.";
static char otb_hwinfo_args_doc[] = "";
  
static struct argp_option otb_hwinfo_options[] = 
{
  {"eeprom_size", 'e', "SIZE", 0, "SIZE in kbit, e.g. 128"},
  {"sign", 'g', 0, 0, "Whether to sign the hw struct - NOT SUPPORTED"},
  {"sign_alg", 'a', "ALGORITHM", 0, "Signing ALGORITHM to use, numeric ID - NOT SUPPORTED"},
  {"sign_len", 'l', "SIGN_LEN", 0, "Length of signature in bits e.g. 4096 - NOT SUPPORTED"},
  {"sign_key", 'k', "KEY_FILE", 0, "Private keyfile location to use for signature - NOT SUPPORTED"},
  {"sign_loc", 'j', "SIGNLOC", 0, "Signing location in bytes offset from 0, e.g. 1024 - NOT SUPPORTED"},
  {"serial", 'z', "SERIAL_NO", 0, "up to 15 character serial number, shorter serials will be left padded with spaces"},
  {"code", 'c', "CODE", 0, "Hardware code, 32 bit hex string e.g. 1234, 01abcd34"},
  {"subcode", 's', "SUB_CODE", 0, "Harware subcode, 32 bit hex string e.g. 1234, 01abcd34"},
  {"chipid", 'i', "CHIPID", 0, "CHIPID in the format 123abc"},
  {"mac1", '1', "MAC1", 0, "The first three bytes of MAC address 1 - STA (last three bytes are chip id) in format 123abc"},
  {"mac2", '2', "MAC2", 0, "The first three bytes of MAC address 2 - AP (last three bytes are chip id) in format 123abc"},
  {"esp_module", 'm', "TYPE", 0, "ESP Module type, 1=ESP12, 2=ESP07S"},
  {"flash_size", 'f', "SIZE", 0, "ESP flash size in Kbyte e.g. 512, 4096"},
  {"adc_type", 'd', "TYPE", 0, "I2C type supported by this module, 0=None, 1=ADS1115"},
  {"adc_config", 't', "TYPE", 0, "Internal ADC configuration, 0=None, 1=3V3_10K_2K49, 2=3V3_220K_100K"},
  {"board_type", 'b', "BOARD_TYPE", 0, "otbiot_v0_3, otbiot_v0_4, otbiot_v0_5, d1_mini are allowed values"},
  {"verbose", 'v', 0, 0, "Verbose output"},
  {0}
};
static struct argp otb_hwinfo_argp = {otb_hwinfo_options, otb_hwinfo_parse_opt, otb_hwinfo_args_doc, otb_hwinfo_doc};

#define OTB_SDK_INIT_DATA obj_hwinfo_sdk_init_data_bin
#define OTB_SDK_INIT_DATA_LEN obj_hwinfo_sdk_init_data_bin_len

#endif // OTB_HWINFO_H_INCLUDED
