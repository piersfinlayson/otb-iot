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
 
#define OTB_HWINFO_C

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <assert.h>
#include "otb_hwinfo_types.h"
#include "otb_eeprom.h"
#include "otb_hwinfo.h"

int main(int argc, char **argv)
{
  int rc = 0;
  bool bool_rc;
  error_t argp_rc;
  
  bool_rc = otb_hwinfo_test_sizes();
  if (!bool_rc)
  {
    printf("Failed size tests\n");
    rc = -1;
    goto EXIT_LABEL;
  }
  
  // Figure out host and target endianness
  otb_hwinfo_setup_endian();

  // Set up the hardware info structures
  otb_hwinfo_setup();

  // Process the args - copy straight into structures
  argp_rc = argp_parse(&otb_hwinfo_argp, argc, argv, 0, 0, NULL);
  if (argp_rc)
  {
    printf("Failed to parse command line args\n");
    rc = -1;
    goto EXIT_LABEL;
  }
  
  otb_hwinfo_postprocess();
  otb_hwinfo_checksums();
  otb_hwinfo_output();

  // Store the structures off into files
  bool_rc = otb_hwinfo_store();
  if (!bool_rc)
  {
    printf("Failed to store off structures into files\n");
    rc = -1;
    goto EXIT_LABEL;
  }

  rc = 0;

EXIT_LABEL:

  return rc;
}

static error_t otb_hwinfo_parse_opt(int key, char *arg, struct argp_state *state)
{
  error_t rc = 0;
  long int iarg;
  uint32 argu32;
  signed char argc;
  size_t len;
  int ii;

  switch (key)
  {
    case 'e':
      // eeprom_size
      iarg = strtol(arg, NULL, 10);
      if (iarg != 128)
      {
        printf("eeprom_size unexpected value: %li\n", iarg);
      }
      // size is value in kbit
      argu32 = iarg * 1024 / 8;
      OTB_HWINFO_STORE(argu32, otb_eeprom_glob.eeprom_size);
      break;
      
    case 'h':
      // hw_loc
      iarg = strtol(arg, NULL, 10);
      argu32 = (uint32)iarg;
      OTB_HWINFO_STORE(argu32, otb_eeprom_glob.loc_hw_struct);
      break;
      
    case 'g':
      // sign (unsupported)
      rc = ARGP_ERR_UNKNOWN;
      break;
      
    case 'a':
      // sign_alg (unsupported)
      rc = ARGP_ERR_UNKNOWN;
      break;
      
    case 'l':
      // sign_len (unsupported)
      rc = ARGP_ERR_UNKNOWN;
      break;
      
    case 'k':
      // sign_key (unsupported)
      rc = ARGP_ERR_UNKNOWN;
      break;
      
    case 'j':
      // sign_loc (unsupported)
      rc = ARGP_ERR_UNKNOWN;
      break;
      
    case 'z':
      // serial
      len = strnlen(arg, 16);
      if ((len <= 0) || (len >= 16))
      {
        rc = EINVAL;
        printf("Serial string too long: %s", arg);
      }
      else
      {
        ii = 0;
        while (len < 15)
        {
          otb_eeprom_hw.serial[ii] = ' ';
          ii++;
          len++;
        }
        strcpy(otb_eeprom_hw.serial+ii, arg);
      }
      break;
      
    case 'c':
      // code
      iarg = strtol(arg, NULL, 16);
      argu32 = (uint32)iarg;
      OTB_HWINFO_STORE(argu32, otb_eeprom_hw.code);
      break;
      
    case 's':
      // subcode
      iarg = strtol(arg, NULL, 16);
      argu32 = (uint32)iarg;
      OTB_HWINFO_STORE(argu32, otb_eeprom_hw.subcode);
      break;
      
    case 'i':
      // chipid
      iarg = strtol(arg, NULL, 16);
      otb_eeprom_hw.chipid[0] = (iarg >> 16) & 0xff;
      otb_eeprom_hw.chipid[1] = (iarg >> 8) & 0xff;
      otb_eeprom_hw.chipid[2] = (iarg) & 0xff;
      break;
      
    case '1':
      // mac1
      iarg = strtol(arg, NULL, 16);
      otb_eeprom_hw.mac1[0] = (iarg >> 16) & 0xff;
      otb_eeprom_hw.mac1[1] = (iarg >> 8) & 0xff;
      otb_eeprom_hw.mac1[2] = (iarg) & 0xff;
      // Fill in last 3 bytes later based on chipid
      break;
      
    case '2':
      // mac2
      iarg = strtol(arg, NULL, 16);
      otb_eeprom_hw.mac2[0] = (iarg >> 16) & 0xff;
      otb_eeprom_hw.mac2[1] = (iarg >> 8) & 0xff;
      otb_eeprom_hw.mac2[2] = (iarg) & 0xff;
      // Fill in last 3 bytes later based on chipid
      break;
      
    case 'm':
      iarg = strtol(arg, NULL, 10);
      argu32 = (uint32)iarg;
      // esp_module
      if ((argu32 != OTB_EEPROM_HW_ESP12) && (argu32 != OTB_EEPROM_HW_ESP07S))
      {
        printf("esp_module unknown value %d\n", argu32);
      }
      OTB_HWINFO_STORE(argu32, otb_eeprom_hw.esp_module);
      break;
      
    case 'f':
      // flash_size
      iarg = strtol(arg, NULL, 10);
      argu32 = iarg * 1024;
      if (iarg != 4096)
      {
        printf("flash_size unexpected value %li\n", iarg);
      }
      OTB_HWINFO_STORE(argu32, otb_eeprom_hw.flash_size_bytes);
      break;
      
    case 'd':
      // adc_type
      iarg = strtol(arg, NULL, 10);
      argu32 = (uint32)iarg;
      if ((argu32 != OTB_EEPROM_HW_I2C_ADC_NONE) && 
          (argu32 != OTB_EEPROM_HW_I2C_ADC_ADS1115_I2C))
      {
        printf("adc_type unexpected value %li\n", iarg);
      }
      OTB_HWINFO_STORE(argu32, otb_eeprom_hw.i2c_adc);
      break;
      
    case 't':
      // adc_config
      iarg = strtol(arg, NULL, 10);
      argu32 = (uint32)iarg;
      if ((argu32 != OTB_EEPROM_HW_INT_ADC_NONE) && 
          (argu32 != OTB_EEPROM_HW_INT_ADC_3V3_10K_2K49))
      {
        printf("adc_config unexpected value %li\n", iarg);
      }
      OTB_HWINFO_STORE(argu32, otb_eeprom_hw.internal_adc_type);
      break;
      
    case 'A':
      // sda_int
      iarg = strtol(arg, NULL, 10);
      argc = (signed char)iarg;
      if ((argc < -1) || (argc > 16))
      {
        printf("pin unexpected value %d\n", argc);
      }
      OTB_HWINFO_STORE(argc, otb_eeprom_hw.i2c_int_sda_pin);
      break;
      
    case 'L':
      // scl_int
      iarg = strtol(arg, NULL, 10);
      argc = (signed char)iarg;
      if ((argc < -1) || (argc > 16))
      {
        printf("pin unexpected value %d\n", argc);
      }
      OTB_HWINFO_STORE(argc, otb_eeprom_hw.i2c_int_scl_pin);
      break;
      
    case 'B':
      // sda_ext
      iarg = strtol(arg, NULL, 10);
      argc = (signed char)iarg;
      if ((argc < -1) || (argc > 16))
      {
        printf("pin unexpected value %d\n", argc);
      }
      OTB_HWINFO_STORE(argc, otb_eeprom_hw.i2c_ext_sda_pin);
      break;
      
    case 'M':
      // scl_ext
      iarg = strtol(arg, NULL, 10);
      argc = (signed char)iarg;
      if ((argc < -1) || (argc > 16))
      {
        printf("pin unexpected value %d\n", argc);
      }
      OTB_HWINFO_STORE(argc, otb_eeprom_hw.i2c_ext_scl_pin);
      break;
      
    case 'v':
      // verbose
      otb_hwinfo_verbose = TRUE;
      break;
      
    case ARGP_KEY_END:
      // No-op
      break;
      
    default:
      rc = ARGP_ERR_UNKNOWN;
      break;
  }

  return rc;
}

void otb_hwinfo_setup(void)
{
  uint32 magic;
  uint32 hw_size;
  uint32 glob_size;
  uint32 version;

#if 0
  // Actually unnecessary as these are globals so inited to 0 anyway
  memset(&otb_eeprom_glob, 0, sizeof(otb_eeprom_glob));
  memset(&otb_eeprom_hw, 0, sizeof(otb_eeprom_hw));
#endif
  
  //  Global config header
  magic = OTB_EEPROM_GLOB_MAGIC;
  glob_size = sizeof(otb_eeprom_glob);
  version = OTB_EEPROM_GLOB_VERSION_1;
  OTB_HWINFO_STORE(magic, otb_eeprom_glob.hdr.magic);
  OTB_HWINFO_STORE(glob_size, otb_eeprom_glob.hdr.struct_size);
  OTB_HWINFO_STORE(version, otb_eeprom_glob.hdr.version);
  
  //  Hardware config header
  magic = OTB_EEPROM_HW_MAGIC;
  hw_size = sizeof(otb_eeprom_hw);
  version = OTB_EEPROM_HW_VERSION_1;
  OTB_HWINFO_STORE(magic, otb_eeprom_hw.hdr.magic);
  OTB_HWINFO_STORE(hw_size, otb_eeprom_hw.hdr.struct_size);
  OTB_HWINFO_STORE(version, otb_eeprom_hw.hdr.version)

  return;
}

void otb_hwinfo_postprocess(void)
{
  int ii;
  uint32 size;

  // If hw_loc is set, set hw_len
  if (otb_eeprom_glob.loc_hw_struct != 0)
  {
    size = sizeof(otb_eeprom_hw);
    OTB_HWINFO_STORE(size, otb_eeprom_glob.loc_hw_struct_len);
  }

  // Fill in last 3 bytes of MAC addresses as chip ID
  for (ii = 3; ii < 6; ii++)
  {
    otb_eeprom_hw.mac1[ii] = otb_eeprom_hw.chipid[ii-3];
    otb_eeprom_hw.mac2[ii] = otb_eeprom_hw.chipid[ii-3];
  }
  
  return;
}

void otb_hwinfo_checksums(void)
{
  OTB_HWINFO_CHECKSUM_DO(otb_eeprom_hw);
  OTB_HWINFO_CHECKSUM_DO(otb_eeprom_glob);

  return;
}

void otb_hwinfo_checksum_store(void *data, size_t total_size, int checksum_loc, size_t checksum_size)
{
  uint32 checksum;
  uint32 checksum2;
  uint32 *checksum_store;
  int ii;
  char *datac;
  
  // Checks
  assert(checksum_size == sizeof(checksum));
  assert(checksum_size == sizeof(*checksum_store));
  
  // Set up point to where we need to store the checksum
  datac = (char *)data;
  checksum_store = (uint32*)(datac + checksum_loc);
  checksum = OTB_EEPROM_CHECKSUM_INITIAL;
  for (ii = 0; ii < total_size; ii++)
  {
    if ((ii < checksum_loc) || (ii >= (checksum_loc + checksum_size)))
    {
      checksum += (*(datac+ii) << ((ii%4) * 8));
    }
  }  
  
  OTB_HWINFO_STORE(checksum, *checksum_store);

  // Double check it!
  datac = (char *)data;
  checksum_store = (uint32*)(datac + checksum_loc);
  checksum2 = OTB_EEPROM_CHECKSUM_INITIAL;
  for (ii = 0; ii < total_size; ii++)
  {
    if ((ii < checksum_loc) || (ii >= (checksum_loc + checksum_size)))
    {
      checksum2 += (*(datac+ii) << ((ii%4) * 8));
    }
  }  
  assert(checksum == checksum2);
  
  return;
}

void otb_hw_info_checksum_calc(void *data, size_t total_size, int checksum_loc, size_t checksum_size)
{
}

bool otb_hwinfo_store(void)
{
  bool rc = TRUE;
  FILE *fp = NULL;
  int ii;
  char *write;
  int irc;
  
  fp = fopen(otb_hwinfo_fn_glob, "wb");
  if (fp == NULL)
  {
    printf("Failed to open %s for writing\n", otb_hwinfo_fn_glob);  
    rc = FALSE;
    goto EXIT_LABEL;
  }
  write = (char *)&otb_eeprom_glob;
  for (ii = 0; ii < sizeof(otb_eeprom_glob); ii++)
  {
    irc = fputc(*(write+ii), fp);
    if (irc == EOF)
    {
      rc = FALSE;
      goto EXIT_LABEL;
    }
  }
  fclose(fp);
  fp = NULL;
  
  fp = fopen(otb_hwinfo_fn_hw, "wb");
  if (fp == NULL)
  {
    printf("Failed to open %s for writing\n", otb_hwinfo_fn_hw);  
    rc = FALSE;
    goto EXIT_LABEL;
  }
  write = (char *)&otb_eeprom_hw;
  for (ii = 0; ii < sizeof(otb_eeprom_hw); ii++)
  {
    irc = fputc(*(write+ii), fp);
    if (irc == EOF)
    {
      rc = FALSE;
      goto EXIT_LABEL;
    }
  }
  fclose(fp);
  fp = NULL;
  
  fp = fopen(otb_hwinfo_fn_sign, "wb");
  if (fp == NULL)
  {
    printf("Failed to open %s for writing\n", otb_hwinfo_fn_sign);  
    rc = FALSE;
    goto EXIT_LABEL;
  }
  fclose(fp);
  fp = NULL;
  
EXIT_LABEL:

  if (fp != NULL)
  {
    fclose(fp);
    fp = NULL;
  }

  return rc;
}

bool otb_hwinfo_test_sizes(void)
{
  bool rc = TRUE;
  
  // If any of these tests fail then you need to change the size of the various
  // types to match those on the ESP8266.  
  if ((sizeof(uint32) != 4) || (sizeof(int32) != 4))
  {
    printf("Int32 missized: %lu %lu\n", sizeof(uint32), sizeof(int32));
    rc = FALSE;
  }

  if ((sizeof(uint16) != 2) || (sizeof(int16) != 2))
  {
    printf("Int16 missized: %lu %lu\n", sizeof(uint16), sizeof(int16));
    rc = FALSE;
  }

  if ((sizeof(uint8) != 1) || (sizeof(int8) != 1))
  {
    printf("Int8 missized: %lu %lu\n", sizeof(uint8), sizeof(int8));
    rc = FALSE;
  }

  if ((sizeof(unsigned char) != 1) || (sizeof(char) != 1))
  {
    printf("Char missized: %lu %lu\n", sizeof(unsigned char), sizeof(char));
    rc = FALSE;
  }

  return rc;
}

void otb_hwinfo_setup_endian(void)
{
  uint32 test = OTB_HWINFO_ENDIAN_TEST_UINT32;
  unsigned char *byte;
  
  //printf("Host endian: ");
  byte = (unsigned char *)&test;
  if (byte[0] == OTB_HWINFO_ENDIAN_TEST_BYTE0_LITTLE_ENDIAN)
  {
    otb_hwinfo_host_endian = OTB_HWINFO_ENDIAN_LITTLE;
    //printf("little");
  }
  else
  {
    assert(byte[0] == OTB_HWINFO_ENDIAN_TEST_BYTE0_BIG_ENDIAN);
    otb_hwinfo_host_endian = OTB_HWINFO_ENDIAN_BIG;
    //printf("big");
  }
  //printf("\n");
  
  // ESP
  //printf("Target endian: ");
  otb_hwinfo_target_endian = OTB_HWINFO_ENDIAN_ESP8266;
  //printf("big");
  //printf("\n");
  
  return;
}

void otb_hwinfo_store_field(char *s, char *d, uint8 b)
{
  int ii;
  
  assert((b == 1) || (b == 2) || (b == 4));

  for (ii = 0; ii < b;  ii++)
  {
    *(d+ii) = *(s+b-ii-1);
  }

  return;
}

void otb_hwinfo_check_endian_set(void)
{
  assert((otb_hwinfo_host_endian == OTB_HWINFO_ENDIAN_LITTLE) ||
         (otb_hwinfo_host_endian == OTB_HWINFO_ENDIAN_BIG));
  assert((otb_hwinfo_target_endian == OTB_HWINFO_ENDIAN_LITTLE) ||
         (otb_hwinfo_target_endian == OTB_HWINFO_ENDIAN_BIG));

  return;
}

void otb_hwinfo_output(void)
{
  char *output;

  if (!otb_hwinfo_verbose)
  {
    goto EXIT_LABEL;    
  }
  
  printf("%s\n", argp_program_version);
  printf("  Endianness\n");
  output = (otb_hwinfo_host_endian == OTB_HWINFO_ENDIAN_BIG ? "Big" : "Little");
  printf("    Host: %s\n", output);
  output = (otb_hwinfo_target_endian == OTB_HWINFO_ENDIAN_BIG ? "Big" : "Little");
  printf("    Target: %s\n", output);
  if (otb_hwinfo_host_endian != otb_hwinfo_target_endian)
  {
    printf("    Integers below will appear in opposite byte order\n");
  }

  printf("  Global\n");
  printf("    Header\n");
  printf("      Magic: 0x%08x\n", otb_eeprom_glob.hdr.magic);
  printf("      Struct_size: 0x%08x\n", otb_eeprom_glob.hdr.struct_size);
  printf("      Version: 0x%08x\n", otb_eeprom_glob.hdr.version);
  printf("      Checksum: 0x%08x\n", otb_eeprom_glob.hdr.checksum);
  printf("    Eeprom_size: 0x%08x\n", otb_eeprom_glob.eeprom_size);
  printf("    Hw loc: 0x%08x\n", otb_eeprom_glob.loc_hw_struct);
  printf("    Hw len: 0x%08x\n", otb_eeprom_glob.loc_hw_struct_len);
  printf("    Hw sign loc: 0x%08x\n", otb_eeprom_glob.loc_hw_struct_sign);
  printf("    Hw sign len: 0x%08x\n", otb_eeprom_glob.loc_hw_struct_sign_len);
  printf("    Hash algorithm: 0x%08x\n", otb_eeprom_glob.hash_algorithm);
  printf("    Sign key len: 0x%08x\n", otb_eeprom_glob.sign_key_len_bytes);
  
  printf("  Hardware\n");
  printf("    Header\n");
  printf("      Magic: 0x%08x\n", otb_eeprom_hw.hdr.magic);
  printf("      Struct_size: 0x%08x\n", otb_eeprom_hw.hdr.struct_size);
  printf("      Version: 0x%08x\n", otb_eeprom_hw.hdr.version);
  printf("      Checksum: 0x%08x\n", otb_eeprom_hw.hdr.checksum);
  printf("    Serial: %s\n", otb_eeprom_hw.serial);
  printf("    Code: 0x%08x\n", otb_eeprom_hw.code);
  printf("    Subcode: 0x%08x\n", otb_eeprom_hw.subcode);
  printf("    Chip ID: %02x%02x%02x\n", otb_eeprom_hw.chipid[0], otb_eeprom_hw.chipid[1], otb_eeprom_hw.chipid[2]);
  printf("    MAC 1: %02x:%02x:%02x:%02x:%02x:%02x\n", otb_eeprom_hw.mac1[0], otb_eeprom_hw.mac1[1], otb_eeprom_hw.mac1[2], otb_eeprom_hw.mac1[3], otb_eeprom_hw.mac1[4], otb_eeprom_hw.mac1[5]);
  printf("    MAC 2: %02x:%02x:%02x:%02x:%02x:%02x\n", otb_eeprom_hw.mac2[0], otb_eeprom_hw.mac2[1], otb_eeprom_hw.mac2[2], otb_eeprom_hw.mac2[3], otb_eeprom_hw.mac2[4], otb_eeprom_hw.mac2[5]);
  printf("    ESP module: 0x%08x\n", otb_eeprom_hw.esp_module);
  printf("    Flash size: 0x%08x\n", otb_eeprom_hw.flash_size_bytes);
  printf("    ADC Type: 0x%08x\n", otb_eeprom_hw.i2c_adc);
  printf("    ADC Config: 0x%08x\n", otb_eeprom_hw.internal_adc_type);
  printf("    Internal SDA pin: 0x%02x\n", otb_eeprom_hw.i2c_int_sda_pin);
  printf("    Internal SCL pin: 0x%02x\n", otb_eeprom_hw.i2c_int_scl_pin);
  printf("    External SDA pin: 0x%02x\n", otb_eeprom_hw.i2c_ext_sda_pin);
  printf("    External SCL pin: 0x%02x\n", otb_eeprom_hw.i2c_ext_scl_pin);
  
EXIT_LABEL:

  return;
}