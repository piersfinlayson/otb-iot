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
#include <time.h>
#include "otb_hwinfo_types.h"

#define OTB_EEPROM_C
#include "otb_eeprom.h"

#include "otb_hwinfo.h"
#include "otb_board_info.h"

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

  // Set up hardware information based on type of board
  // XXX Currently not type of board!
  hwinfo.board_info = &otb_hwinfo_main_board_otbiot_v0_4_board_info;

  // Process the args - copy info into temporary structures
  argp_rc = argp_parse(&otb_hwinfo_argp, argc, argv, 0, 0, NULL);
  if (argp_rc)
  {
    printf("Failed to parse command line args\n");
    rc = -1;
    goto EXIT_LABEL;
  }
  
  // Set up the hardware info buffer to be written to the file
  otb_hwinfo_setup();

  // Output the file
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
      hwinfo.eeprom_size = argu32;
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
          hwinfo.serial[ii] = ' ';
          ii++;
          len++;
        }
        strcpy(hwinfo.serial, arg);
      }
      break;
      
    case 'c':
      // code
      iarg = strtol(arg, NULL, 16);
      argu32 = (uint32)iarg;
      hwinfo.code = argu32;
      break;
      
    case 's':
      // subcode
      iarg = strtol(arg, NULL, 16);
      argu32 = (uint32)iarg;
      hwinfo.subcode = argu32;
      break;
      
    case 'i':
      // chipid
      iarg = strtol(arg, NULL, 16);
      hwinfo.chipid[0] = (iarg >> 16) & 0xff;
      hwinfo.chipid[1] = (iarg >> 8) & 0xff;
      hwinfo.chipid[2] = (iarg) & 0xff;
      break;
      
    case '1':
      // mac1
      iarg = strtol(arg, NULL, 16);
      hwinfo.mac1[0] = (iarg >> 16) & 0xff;
      hwinfo.mac1[1] = (iarg >> 8) & 0xff;
      hwinfo.mac1[2] = (iarg) & 0xff;
      // Fill in last 3 bytes later based on chipid
      break;
      
    case '2':
      // mac2
      iarg = strtol(arg, NULL, 16);
      hwinfo.mac2[0] = (iarg >> 16) & 0xff;
      hwinfo.mac2[1] = (iarg >> 8) & 0xff;
      hwinfo.mac2[2] = (iarg) & 0xff;
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
      hwinfo.esp_module = argu32;
      break;
      
    case 'f':
      // flash_size
      iarg = strtol(arg, NULL, 10);
      argu32 = iarg * 1024;
      if (iarg != 4096)
      {
        printf("flash_size unexpected value %li\n", iarg);
      }
      hwinfo.flash_size_bytes = argu32;
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
      hwinfo.i2c_adc = argu32;
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
      hwinfo.internal_adc_type = argu32;
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

uint32 sdk_size;

//
// otb_hwinfo_setup
//
// This function builds up a single buffer containing all of the contents
// of the eeprom to be written.
//
// It's operation is as follows:
// - Run through all of the info comps to be added figuring out the buffer
//   size required, and then allocate it.  At the same time figure out the
//   total number of info comps (and setting this value)
// - Rerun through all of the info comps actually writing relevant info
//   into the buffer at the appropriate place.  At the same time fill in
//   the otb_eeprom_info info comp (location, length) for that info comp
// - Calculating all of the checksums
void otb_hwinfo_setup(void)
{
  uint32 magic;
  uint32 hw_size;
  uint32 glob_size;
  uint32 version;
  int ii, jj, kk;
  uint8_t *ptr, *new_ptr;
  uint32_t struct_size;
  uint32_t len, working_len, advance, loc;
  otb_eeprom_hdr *hdr;
  uint32_t info_comp_count;
  otb_eeprom_pin_info pin;
  otb_eeprom_info_comp *info_comp_ptr;

  info_comp_count = 0;
  len = 0;
  for (ii = 0; ii < OTB_EEPROM_INFO_TYPE_NUM; ii++)
  {
    struct_size = otb_eeprom_main_comp_types[ii].struct_size_max;

    // Now set up length based on type
    switch (ii)
    {
      case OTB_EEPROM_INFO_TYPE_INFO:
        // Nothing to do here - otb_eeprom_info info comps calculated at end
        // of loop
        len += struct_size;
        break;

      case OTB_EEPROM_INFO_TYPE_MAIN_BOARD:
        // No info comps as part of main board so stick with length as is
        //
        // However, add one to info comp count (as that is for otb_eeprom_info info
        // comps!)
        info_comp_count++;
        len += struct_size;
        break;

      case OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE:
        hwinfo.board_info->mod_count;
        for (jj = 0; jj < hwinfo.board_info->mod_count; jj++)
        {
          // Add main struct size, and then enough for the pins
          len += struct_size;
          len += (*hwinfo.board_info->module_info)[jj].num_pins * sizeof(otb_eeprom_pin_info);
          info_comp_count++;
        }
        break;

      case OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA:
        // sdk init data
        len += struct_size + OTB_SDK_INIT_DATA_LEN;
        info_comp_count++;
        break;

      case OTB_EEPROM_INFO_TYPE_GPIO_PINS:
        // gpios
        len += struct_size;
        len += (hwinfo.board_info->pin_count * sizeof(otb_eeprom_pin_info));
        info_comp_count++;
        break;

       default:
         assert(FALSE);
    }
  }

  // Add on space for otb_eeprom_info info comps
  len += info_comp_count * sizeof(otb_eeprom_info_comp);

  // Allocate memory
  ptr = (uint8_t *)malloc(len);
  assert(ptr != NULL);
  memset(ptr, 0, len);  // Avoids worrying about pads
  new_ptr = ptr;
  hwinfo.output_len = len;

  for (ii = 0; ii < OTB_EEPROM_INFO_TYPE_NUM; ii++)
  {
    len = 0;
    hdr = (otb_eeprom_hdr *)new_ptr;
    struct_size = otb_eeprom_main_comp_types[ii].struct_size_max;
    assert(struct_size % 4 == 0);
    magic = otb_eeprom_main_comp_types[ii].magic;
    version = otb_eeprom_main_comp_types[ii].version_max;

    // Fill in the rest
    switch (ii)
    {
      case OTB_EEPROM_INFO_TYPE_INFO:
        // set up otb_eeprom_info
        // ; required as declaration can't follow label!
        ;
        otb_eeprom_info *eeprom_info = (otb_eeprom_info *)hdr;
        OTB_HWINFO_STORE(eeprom_info->eeprom_size, hwinfo.eeprom_size);
        OTB_HWINFO_STORE(eeprom_info->comp_num, info_comp_count);
        time_t tt = time(NULL);
        assert(tt != -1);
        struct tm *tm = localtime(&tt);
        assert(tm != NULL);
        uint32 date = ((tm->tm_year+1900)<<16) | ((tm->tm_mon+1)<<8) | ((tm->tm_mday));
        OTB_HWINFO_STORE(eeprom_info->write_date, date);
        len = struct_size;

        // Need to account for the otb_eeprom_info info comps here
        // Could do single alloc rather than for loop ...
        info_comp_ptr = eeprom_info->comp;
        len += info_comp_count * sizeof(otb_eeprom_info_comp);
        new_ptr += len;
        break;

      case OTB_EEPROM_INFO_TYPE_MAIN_BOARD:
        ;
        otb_eeprom_main_board *main_board = (otb_eeprom_main_board *)hdr;
        memcpy(main_board->common.serial, hwinfo.serial, OTB_EEPROM_HW_SERIAL_LEN+1);
        OTB_HWINFO_STORE(main_board->common.code, hwinfo.code);
        OTB_HWINFO_STORE(main_board->common.subcode, hwinfo.subcode);
        memcpy(main_board->chipid, hwinfo.chipid, 3);
        memcpy(main_board->mac1, hwinfo.mac1, 3);
        memcpy(main_board->mac1+3, hwinfo.chipid, 3);
        memcpy(main_board->mac2, hwinfo.mac2, 3);
        memcpy(main_board->mac2+3, hwinfo.chipid, 3);
        OTB_HWINFO_STORE(main_board->esp_module, hwinfo.esp_module);
        OTB_HWINFO_STORE(main_board->flash_size_bytes, hwinfo.flash_size_bytes);
        OTB_HWINFO_STORE(main_board->i2c_adc, hwinfo.i2c_adc);
        OTB_HWINFO_STORE(main_board->internal_adc_type, hwinfo.internal_adc_type);
        OTB_HWINFO_STORE(main_board->num_modules, hwinfo.board_info->mod_count);
        len = struct_size;
        
        // Set up otb_eeprom_info info comp
        OTB_HWINFO_STORE(info_comp_ptr->type, ii);
        loc = (uint32)((uint8*)hdr-ptr);
        OTB_HWINFO_STORE(info_comp_ptr->location, loc);
        OTB_HWINFO_STORE(info_comp_ptr->length, len);
        info_comp_ptr++;

        new_ptr += struct_size;
        break;

      case OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE:
        ;
        otb_eeprom_main_board_module *module = (otb_eeprom_main_board_module *)hdr;
        working_len = 0;
        for (jj = 0; jj < hwinfo.board_info->mod_count; jj++)
        {
          len = 0;
          OTB_HWINFO_STORE(module->num, (*hwinfo.board_info->module_info)[jj].num);
          OTB_HWINFO_STORE(module->socket_type, (*hwinfo.board_info->module_info)[jj].socket_type);
          OTB_HWINFO_STORE(module->num_headers, (*hwinfo.board_info->module_info)[jj].num_headers);
          OTB_HWINFO_STORE(module->num_pins, (*hwinfo.board_info->module_info)[jj].num_pins);
          OTB_HWINFO_STORE(module->address, (*hwinfo.board_info->module_info)[jj].address);
          len += otb_eeprom_main_comp_types[ii].struct_size_max;

          // Now do each of the pins
          otb_eeprom_pin_info *pin = module->pin_info;
          for (kk = 0; kk < (*hwinfo.board_info->module_info)[jj].num_pins; kk++)
          {
            OTB_HWINFO_STORE(pin[kk].num, (*(*hwinfo.board_info->module_info)[jj].pin_info)[kk].num)
            OTB_HWINFO_STORE(pin[kk].header_num, (*(*hwinfo.board_info->module_info)[jj].pin_info)[kk].header_num)
            OTB_HWINFO_STORE(pin[kk].use, (*(*hwinfo.board_info->module_info)[jj].pin_info)[kk].use)
            OTB_HWINFO_STORE(pin[kk].module, (*(*hwinfo.board_info->module_info)[jj].pin_info)[kk].module)
            OTB_HWINFO_STORE(pin[kk].further_info, (*(*hwinfo.board_info->module_info)[jj].pin_info)[kk].further_info)
            OTB_HWINFO_STORE(pin[kk].pulled, (*(*hwinfo.board_info->module_info)[jj].pin_info)[kk].pulled)
          }

          len += (*hwinfo.board_info->module_info)[jj].num_pins * sizeof(otb_eeprom_pin_info);
          working_len += len;

          // Set up otb_eeprom_info info comp
          OTB_HWINFO_STORE(info_comp_ptr->type, ii);
          loc = (uint32)((uint8*)module-ptr);
          OTB_HWINFO_STORE(info_comp_ptr->location, loc);
          OTB_HWINFO_STORE(info_comp_ptr->length, len);
          info_comp_ptr++;

          // Fill in all header fields except checksum (which is last)
          // Note hdr is updated to point to new module so use module->hdr
          OTB_HWINFO_STORE(module->hdr.magic, magic);
          OTB_HWINFO_STORE(module->hdr.type, ii);
          OTB_HWINFO_STORE(module->hdr.struct_size, struct_size);
          OTB_HWINFO_STORE(module->hdr.version, version);
          OTB_HWINFO_STORE(module->hdr.length, len);

          // Now do the checksums
          OTB_HWINFO_CHECKSUM_DO((&(module->hdr)));

          // Figure out where next module starts
          module = (otb_eeprom_main_board_module *)(((uint8 *)hdr) + working_len);
        }
        new_ptr += working_len;
        break;

      case OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA:
        assert(OTB_SDK_INIT_DATA_LEN % 4 == 0);
        memcpy(((otb_eeprom_main_board_sdk_init_data *)hdr)->sdk_init_data,
               OTB_SDK_INIT_DATA,
               OTB_SDK_INIT_DATA_LEN);
        OTB_HWINFO_STORE(((otb_eeprom_main_board_sdk_init_data *)hdr)->data_len, OTB_SDK_INIT_DATA_LEN);
        len = struct_size + OTB_SDK_INIT_DATA_LEN;

        // Set up otb_eeprom_info info comp
        OTB_HWINFO_STORE(info_comp_ptr->type, ii);
        loc = (uint32)((uint8*)hdr-ptr);
        OTB_HWINFO_STORE(info_comp_ptr->location, loc);
        OTB_HWINFO_STORE(info_comp_ptr->length, len);
        info_comp_ptr++;

        new_ptr += len;
        break;

      case OTB_EEPROM_INFO_TYPE_GPIO_PINS:
        // Copy in GPIO pin information
        ;
        otb_eeprom_main_board_gpio_pins *gpio_pins = (otb_eeprom_main_board_gpio_pins *)hdr;
        gpio_pins->num_pins = hwinfo.board_info->pin_count; 
        for (jj = 0; jj < hwinfo.board_info->pin_count; jj++)
        {
          OTB_HWINFO_STORE(gpio_pins->pin_info[jj].num, (*hwinfo.board_info->pin_info)[jj].num);
          OTB_HWINFO_STORE(gpio_pins->pin_info[jj].header_num, (*hwinfo.board_info->pin_info)[jj].header_num);
          OTB_HWINFO_STORE(gpio_pins->pin_info[jj].use, (*hwinfo.board_info->pin_info)[jj].use);
          OTB_HWINFO_STORE(gpio_pins->pin_info[jj].module, (*hwinfo.board_info->pin_info)[jj].module);
          OTB_HWINFO_STORE(gpio_pins->pin_info[jj].further_info, (*hwinfo.board_info->pin_info)[jj].further_info);
          OTB_HWINFO_STORE(gpio_pins->pin_info[jj].pulled, (*hwinfo.board_info->pin_info)[jj].pulled);
        }
        len = struct_size + (hwinfo.board_info->pin_count * sizeof(otb_eeprom_pin_info));

        // Set up otb_eeprom_info info comp
        OTB_HWINFO_STORE(info_comp_ptr->type, ii);
        loc = (uint32)((uint8*)hdr-ptr);
        OTB_HWINFO_STORE(info_comp_ptr->location, loc);
        OTB_HWINFO_STORE(info_comp_ptr->length, len);
        info_comp_ptr++;

        new_ptr += len;
        break;

       default:
         assert(FALSE);
    }

    if (ii != OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE)
    {
      // Fill in all header fields except checksum (which is last)
      OTB_HWINFO_STORE(hdr->magic, magic);
      OTB_HWINFO_STORE(hdr->type, ii);
      OTB_HWINFO_STORE(hdr->struct_size, struct_size);
      OTB_HWINFO_STORE(hdr->version, version);
      OTB_HWINFO_STORE(hdr->length, len);

      // Now do the checksums
      // Can't do for otb_eeprom_info yet, as need info comps first.
      if (ii != OTB_EEPROM_INFO_TYPE_INFO)
      {
        OTB_HWINFO_CHECKSUM_DO(hdr);
      }
    }
  }

  // Now do otb_eeprom_info checksum
  hdr = (otb_eeprom_hdr*)ptr;
  OTB_HWINFO_CHECKSUM_DO(hdr);

  hwinfo.output = ptr;

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
  
  OTB_HWINFO_STORE(*checksum_store, checksum);

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

bool otb_hwinfo_store(void)
{
  bool rc = TRUE;
  FILE *fp = NULL;
  int ii;
  char *write;
  int irc;
  
  // Just one file to write
  fp = fopen(otb_hwinfo_fn, "wb");
  if (fp == NULL)
  {
    printf("Failed to open %s for writing\n", otb_hwinfo_fn);  
    rc = FALSE;
    goto EXIT_LABEL;
  }
  write = (char *)hwinfo.output;
  for (ii = 0; ii < hwinfo.output_len; ii++)
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

void otb_hwinfo_output_hdr(char *prefix, char* type, otb_eeprom_hdr *hdr)
{
  assert(prefix != NULL);
  assert(hdr != NULL);

  if (type != NULL)
  {
    printf("%s%s\n", prefix, type);
  }
  printf("%s  hdr:\n", prefix);
  printf("%s    magic:       0x%08x\n", prefix, hdr->magic);
  printf("%s    struct_size: 0x%08x\n", prefix, hdr->struct_size);
  printf("%s    version:     0x%08x\n", prefix, hdr->version);
  printf("%s    length:      0x%08x\n", prefix, hdr->length);
  printf("%s    checksum:    0x%08x\n", prefix, hdr->checksum);
  return;
}

otb_eeprom_hdr *otb_hwinfo_get_next_hdr(otb_eeprom_hdr *hdr)
{
  uint32_t len;
  otb_eeprom_hdr *next_hdr;

  assert(hdr != NULL);

  len = hdr->length;
  next_hdr = (otb_eeprom_hdr *)((uint8_t *)hdr + len);

  return(next_hdr);
}

void otb_hwinfo_output_eeprom_info(char *prefix, otb_eeprom_hdr *hdr)
{
  otb_eeprom_info *eeprom_info = (otb_eeprom_info *)hdr;

  assert(prefix != NULL);
  assert(hdr != NULL);

  otb_hwinfo_output_hdr(prefix, "otb_eeprom_info", hdr);
  printf("%s  eeprom_size: 0x%08x\n", prefix, eeprom_info->eeprom_size);
  printf("%s  comp_num:    %d\n", prefix, eeprom_info->comp_num);
  printf("%s  write_date:  0x%08x\n", prefix, eeprom_info->write_date);

  return;
}

void otb_hwinfo_output_pin_info(char *prefix, uint32 num_pins, otb_eeprom_pin_info *pin_info)
{
  int ii;
  otb_eeprom_pin_info *next_pin_info;
  for (ii = 0, next_pin_info = pin_info;
       ii < num_pins;
       ii++, next_pin_info++)
  {
    printf("%s  pin_info #%d:\n", prefix, ii);
    printf("%s    num:          %d\n", prefix, next_pin_info->num);
    printf("%s    header_num:   %d\n", prefix, next_pin_info->header_num);
    printf("%s    use:          %d\n", prefix, next_pin_info->use);
    printf("%s    module:       %d\n", prefix, next_pin_info->module);
    printf("%s    further_info: 0x%04x\n", prefix, next_pin_info->further_info);
    printf("%s    pulled:       %d\n", prefix, next_pin_info->pulled);
  }

  return;
}

void otb_hwinfo_output_mac(char *prefix, char *name, unsigned char *mac)
{
  int ii;

  printf("%s  %s:        ", prefix, name);
  for (ii = 0; ii < 6; ii++)
  {
    printf("%02x", mac[ii]);
  }
  printf("\n");

  return;
}

void otb_hwinfo_output_comps(char *prefix, otb_eeprom_hdr *hdr)
{
  int ii, jj, kk;
  otb_eeprom_info *eeprom_info = (otb_eeprom_info *)hdr;
  otb_eeprom_hdr *new_hdr = hdr;
  char *comp_type_str;

  // Output otb_eeprom_info comps
  for (ii = 0; ii < eeprom_info->comp_num; ii++)
  {
    printf("%s  info_comp #%d:\n", prefix, ii);
    printf("%s    type:     %d (%s)\n", prefix, eeprom_info->comp[ii].type, otb_eeprom_main_comp_types[eeprom_info->comp[ii].type].name);
    printf("%s    location: 0x%08x\n", prefix, eeprom_info->comp[ii].location);
    printf("%s    length:   0x%08x\n", prefix, eeprom_info->comp[ii].length);
  }

  // Now output the comps themselves
  for (ii = 0, new_hdr = otb_hwinfo_get_next_hdr(hdr);
       ii < eeprom_info->comp_num;
       ii++, new_hdr = otb_hwinfo_get_next_hdr(new_hdr))
  {
    otb_hwinfo_output_hdr(prefix, otb_eeprom_main_comp_types[eeprom_info->comp[ii].type].name, new_hdr);

    switch (eeprom_info->comp[ii].type)
    {
      case OTB_EEPROM_INFO_TYPE_MAIN_BOARD:
        ;
        otb_eeprom_main_board *main_board = (otb_eeprom_main_board *)new_hdr;
        printf("%s  common:\n", prefix);
        printf("%s    serial:    %s\n", prefix, main_board->common.serial);
        printf("%s    code:      0x%08x\n", prefix, main_board->common.code);
        printf("%s    subcode:   0x%08x\n", prefix, main_board->common.subcode);
        printf("%s  chipid:      %02x%02x%02x\n", prefix, main_board->chipid[0], main_board->chipid[1], main_board->chipid[2]);
        otb_hwinfo_output_mac(prefix, "mac1", main_board->mac1);
        otb_hwinfo_output_mac(prefix, "mac2", main_board->mac2);
        printf("%s  esp_module:  %d\n", prefix, main_board->esp_module);
        printf("%s  flash_size:  0x%08x\n", prefix, main_board->flash_size_bytes);
        printf("%s  i2c_adc:     %d\n", prefix, main_board->i2c_adc);
        printf("%s  internal_adc_type: %d\n", prefix, main_board->internal_adc_type);
        printf("%s  num_modules: %d\n", prefix, main_board->num_modules);
        break;

      case OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE:
        ;
        otb_eeprom_main_board_module *module = (otb_eeprom_main_board_module *)new_hdr;
        printf("%s  num:         %d\n", prefix, module->num);
        printf("%s  socket_type: %d\n", prefix, module->socket_type);
        printf("%s  num_headers: %d\n", prefix, module->num_headers);
        printf("%s  num_pins:    %d\n", prefix, module->num_pins);
        printf("%s  address:     0x%02x\n", prefix, module->address);
        otb_hwinfo_output_pin_info(prefix, module->num_pins, module->pin_info);
        break;

      case OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA:
        // Not dumping out entire sdk_init_data here
        kk = 0;
        otb_eeprom_main_board_sdk_init_data *sdk_init_data = (otb_eeprom_main_board_sdk_init_data *)new_hdr;
        printf("%s  data_len:      %d\n", prefix, sdk_init_data->data_len);
        printf("%s  sdk_init_data: ", prefix);
        for (jj = 0; jj < sdk_init_data->data_len; jj++)
        {
          if (kk > 0)
          {
            if (kk % 8 == 0)
            {
              printf("\n%s                 ", prefix);
            }
            else if (kk % 4 == 0)
            {
              printf("  ");
            }
            else
            {
              printf(" ");
            }
          }
          kk++;
          printf("%02x", sdk_init_data->sdk_init_data[jj]);
        }
        printf("\n");
        break;

      case OTB_EEPROM_INFO_TYPE_GPIO_PINS:
        ;
        otb_eeprom_main_board_gpio_pins *gpio_pins = (otb_eeprom_main_board_gpio_pins *)new_hdr;
        printf("%s  num_pins:  %d\n", prefix, gpio_pins->num_pins);
        otb_hwinfo_output_pin_info(prefix, gpio_pins->num_pins, gpio_pins->pin_info);
        break;

      default:
        printf("%s  Unknown comp type\n", prefix);
        break;
    }
  }

  return;
}

void otb_hwinfo_output(void)
{
  char *output;
  otb_eeprom_hdr *hdr = (otb_eeprom_hdr *)hwinfo.output;

  if (!otb_hwinfo_verbose)
  {
    goto EXIT_LABEL;    
  }
  
  printf("%s\n", argp_program_version);
  printf("  Output filename: %s\n", otb_hwinfo_fn);
  printf("  Total length:    %d bytes\n", hwinfo.output_len);
  printf("  Endianness\n");
  output = (otb_hwinfo_host_endian == OTB_HWINFO_ENDIAN_BIG ? "Big" : "Little");
  printf("    Host:    %s\n", output);
  output = (otb_hwinfo_target_endian == OTB_HWINFO_ENDIAN_BIG ? "Big" : "Little");
  printf("    Target:  %s\n", output);
  if (otb_hwinfo_host_endian != otb_hwinfo_target_endian)
  {
    printf("Can't output fields due to differing endianness\n");
    goto EXIT_LABEL;
  }

  otb_hwinfo_output_eeprom_info("  ", hdr);
  otb_hwinfo_output_comps("  ", hdr);
  
EXIT_LABEL:

  return;
}
