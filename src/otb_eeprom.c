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

#define OTB_EEPROM_C
#include "otb.h"
#include "brzo_i2c.h"

#ifdef OTB_RBOOT_BOOTLOADER
#undef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

char ICACHE_FLASH_ATTR otb_eeprom_init(void)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_init entry");
  
  // Initialize the I2C stack
  otb_brzo_i2c_setup(0);
  
  // Connect to the eeprom
  rc = otb_i2c_24xxyy_init(otb_eeprom_addr);
  if (rc)
  {
    DEBUG("EEPROM: Connected to eeprom at 0x%02x successfully", otb_eeprom_addr);
  }
  else
  {
    WARN("EEPROM: Failed to connect to eeprom at 0x%02x %d", otb_eeprom_addr, rc);
    goto EXIT_LABEL;
  }
  
  rc = 1;
  
EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_init exit");

  return rc;
}

// Reads global data and hw info.  Doesn't read hw signature or sdk data -
// these are done separately.
char ICACHE_FLASH_ATTR otb_eeprom_read_all(void)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_read_all entry");

  rc = otb_eeprom_read_glob_conf(&otb_eeprom_glob);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  rc = otb_eeprom_read_hw_conf(&otb_eeprom_glob, &otb_eeprom_hw);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = 1;
    
EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_read_all exit");
  
  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_read_sdk_init_data(otb_eeprom_glob_conf *glob_conf, unsigned char *buf, uint32 buf_len)
{
  char rc = 0;
  uint32 sdk_end;
  otb_eeprom_sdk_init_data *sdk;
  int ii;

  DEBUG("EEPROM: otb_eeprom_read_sdk_init_data entry");

  // Check we can safely access the sdk init data
  sdk_end = glob_conf->loc_sdk_init_data + glob_conf->loc_sdk_init_data_len;
  if ((sdk_end > OTB_EEPROM_SIZE_128) || (sdk_end > otb_eeprom_size))
  {
    WARN("EEPROM: Invalid SDK init data location on eeprom: %u %u", glob_conf->loc_sdk_init_data, glob_conf->loc_sdk_init_data_len);
    goto EXIT_LABEL;
  }

  if (glob_conf->loc_sdk_init_data_len > buf_len)
  {
    WARN("EEPROM: Buffer not big enough for sdk data: %u %u:", glob_conf->loc_sdk_init_data_len, buf_len);
    goto EXIT_LABEL;
  }
  
  // Read the data
  rc = otb_i2c_24xx128_read_data(otb_eeprom_addr, glob_conf->loc_sdk_init_data, glob_conf->loc_sdk_init_data_len, (uint8_t *)buf);
if (rc)
  {
    DEBUG("EEPROM: Read %d bytes from eeprom successfully (sdk init data) from 0x%x", glob_conf->loc_sdk_init_data_len, glob_conf->loc_sdk_init_data);
  }
  else
  {
    WARN("EEPROM: Failed to read %d bytes from eeprom (sdk init data) %d", glob_conf->loc_sdk_init_data_len, rc);
    goto EXIT_LABEL;
  }

  sdk = (otb_eeprom_sdk_init_data *)buf;
  // Check magic number - can't continue if this is wrong
  if (sdk->hdr.magic != OTB_EEPROM_SDK_INIT_DATA_MAGIC)
  {
    WARN("EEPROM: Invalid magic value in sdk init data: 0x%08x", sdk->hdr.magic);
    rc = 0;
    goto EXIT_LABEL;
  }

  INFO("EEPROM: SDK Init data format:   V%d", sdk->hdr.version);
  INFO("EEPROM: SDK Init data checksum: 0x%08x", sdk->hdr.checksum);

  rc = otb_eeprom_check_checksum((char*)sdk,
                                 glob_conf->loc_sdk_init_data_len,
                                 (char*)&(sdk->hdr.checksum) - (char*)sdk,
                                 sizeof(sdk->hdr.checksum));
  if (!rc)
  {
    WARN("EEPROM: SDK Init data checksum: Invalid");
    goto EXIT_LABEL;
  }
  else
  {
    INFO("EEPROM: SDK Init data checksum: Valid");
  }

  // We've loaded the SDK init data now - the actual data should succeed this
  // structure. Finally check len is loc_sdk_init_data_len - the structure
  if (sdk->hdr.struct_size != glob_conf->loc_sdk_init_data_len)
  {
    WARN("EEPROM: Invalid SDK Init data len: %u %u", sdk->hdr.struct_size, glob_conf->loc_sdk_init_data_len);
    goto EXIT_LABEL;
  }

  // Done.  Copy the data to the start of buf.  Better do this a byte at a time
  // as memcpy doesn't work on overlapping ranges, and I can't remember if
  // memmove might try and allocate memory.
  for (ii = 0; ii < (glob_conf->loc_sdk_init_data_len); ii++)
  {
    buf[ii] = buf[sizeof(*sdk) + ii];
  } 
  rc = 1;

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_read_sdk_init_data exit");
  
  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_read_glob_conf(otb_eeprom_glob_conf *glob_conf)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_read_glob_conf entry");
  
  rc = otb_i2c_24xx128_read_data(otb_eeprom_addr, OTB_EEPROM_GLOB_CONFIG_LOC, sizeof(otb_eeprom_glob_conf), (uint8_t *)glob_conf);
  if (rc)
  {
    DEBUG("EEPROM: Read %d bytes from eeprom successfully (glob conf)", sizeof(otb_eeprom_glob_conf));
  }
  else
  {
    WARN("EEPROM: Failed to read %d bytes from eeprom (glob conf) %d", sizeof(otb_eeprom_glob_conf), rc);
    goto EXIT_LABEL;
  }
  
  // Check magic number - can't continue if this is wrong
  if (glob_conf->hdr.magic != OTB_EEPROM_GLOB_MAGIC)
  {
    WARN("EEPROM: Invalid magic value in glob conf: 0x%08x", glob_conf->hdr.magic);
    rc = 0;
    goto EXIT_LABEL;
  }
  
  INFO("EEPROM: Eeprom size:            %d bytes", glob_conf->eeprom_size);
  INFO("EEPROM: Global info format:     V%d", glob_conf->hdr.version);
  INFO("EEPROM: Global checksum:        0x%08x", glob_conf->hdr.checksum);
  rc = otb_eeprom_check_checksum((char*)glob_conf,
                                 sizeof(*glob_conf),
                                 (char*)&(glob_conf->hdr.checksum) - (char*)glob_conf,
                                 sizeof(glob_conf->hdr.checksum));
  if (!rc)
  {
    WARN("EEPROM: Global checksum:        Invalid");
    goto EXIT_LABEL;
  }
  else
  {
    INFO("EEPROM: Global checksum:        Valid");
  }
  
  otb_eeprom_size = glob_conf->eeprom_size;
  
EXIT_LABEL:  
  
  DEBUG("EEPROM: otb_eeprom_read_glob_conf exit");
  
  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_check_checksum(char *data, int size, int checksum_loc, int checksum_size)
{
  char rc = 0;
  int ii;
  uint32 *checksum;
  uint32 calc_check;
  
  DEBUG("EEPROM: otb_eeprom_check_checksum entry");
  
  OTB_ASSERT(checksum_size == sizeof(uint32));
  
  checksum = (uint32*)(data + checksum_loc);
  DEBUG("EEPROM: Stored checksum: 0x%08x", *checksum);
  
  calc_check = OTB_EEPROM_CHECKSUM_INITIAL;
  for (ii = 0; ii < size; ii++)
  {
    if ((ii < checksum_loc) || (ii >= (checksum_loc + checksum_size)))
    {
      calc_check += (*(data+ii) << ((ii%4) * 8));
    }
  }  
  
  DEBUG("EEPROM: Calculated checksum: 0x%08x", calc_check);

  rc = (*checksum == calc_check);
  
  DEBUG("EEPROM: otb_eeprom_check_checksum exit");
  
  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_read_hw_conf(otb_eeprom_glob_conf *glob_conf, otb_eeprom_hw_conf *hw_conf)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_read_hw_conf entry");
  
  uint32 hw_end;
  unsigned char serial[OTB_EEPROM_HW_SERIAL_LEN+2];
  
  // Check we can actually safely access the hardware information location
  hw_end = glob_conf->loc_hw_struct + glob_conf->loc_hw_struct_len;
  if ((hw_end > OTB_EEPROM_SIZE_128) || (hw_end > otb_eeprom_size))
  {
    // Invalid hardware info location
    WARN("EEPROM: Invalid hardware info location on eeprom: %u %u", glob_conf->loc_hw_struct, glob_conf->loc_hw_struct_len);
    goto EXIT_LABEL;
  }
  
  if (glob_conf->loc_hw_struct_len < sizeof(otb_eeprom_hw_conf))
  {
    // Invalid hardware info length
    WARN("EEPROM: Invalid hardware info location length on eeprom: %u %u", glob_conf->loc_hw_struct, glob_conf->loc_hw_struct_len);
    goto EXIT_LABEL;
  }

  rc = otb_i2c_24xx128_read_data(otb_eeprom_addr, glob_conf->loc_hw_struct, sizeof(otb_eeprom_hw_conf), (uint8_t *)hw_conf);
  if (rc)
  {
    DEBUG("EEPROM: Read %d bytes from eeprom successfully (hw structure)", sizeof(otb_eeprom_hw_conf));
  }
  else
  {
    WARN("EEPROM: Failed to read %d bytes from eeprom (hw structure) %d", sizeof(otb_eeprom_hw_conf), rc);
    goto EXIT_LABEL;
  }
  
  // Check magic number - can't continue if this is wrong
  if (hw_conf->hdr.magic != OTB_EEPROM_HW_MAGIC)
  {
    WARN("EEPROM: Invalid magic value in hw conf: 0x%08x", hw_conf->hdr.magic);
    rc = 0;
    goto EXIT_LABEL;
  }
  
  INFO("EEPROM: Hardware info format:   V%d", hw_conf->hdr.version);
  INFO("EEPROM: Hardware checksum:      0x%08x", hw_conf->hdr.checksum);

  rc = otb_eeprom_check_checksum((char*)hw_conf,
                                 sizeof(*hw_conf),
                                 (char*)&(hw_conf->hdr.checksum) - (char*)hw_conf,
                                 sizeof(hw_conf->hdr.checksum));
  if (!rc)
  {
    WARN("EEPROM: Hardware checksum:      Invalid");
    goto EXIT_LABEL;
  }
  else
  {
    INFO("EEPROM: Hardware checksum:      Valid");
  }

  // We copy serial number out in order to ensure it's NULL terminated
  os_memcpy(serial, hw_conf->serial, OTB_EEPROM_HW_SERIAL_LEN+1);
  serial[OTB_EEPROM_HW_SERIAL_LEN+1] = 0;
  // Also null terminate within the struct just in case ...
  hw_conf->serial[OTB_EEPROM_HW_SERIAL_LEN] = 0;
  INFO("EEPROM: Device serial:          %s", serial);
  INFO("EEPROM: Hardware code/sub code: %08x:%08x", hw_conf->code, hw_conf->subcode);
  INFO("EEPROM: Chip ID:                %02x%02x%02x", hw_conf->chipid[0], hw_conf->chipid[1], hw_conf->chipid[2]);
  INFO("EEPROM: MAC 1:                  %02x:%02x:%02x:%02x:%02x:%02x", hw_conf->mac1[0], hw_conf->mac1[1], hw_conf->mac1[2], hw_conf->mac1[3], hw_conf->mac1[4], hw_conf->mac1[5]);
  INFO("EEPROM: MAC 2:                  %02x:%02x:%02x:%02x:%02x:%02x", hw_conf->mac2[0], hw_conf->mac2[1], hw_conf->mac2[2], hw_conf->mac2[3], hw_conf->mac2[4], hw_conf->mac2[5]);
  INFO("EEPROM: ESP module type:        %d", hw_conf->esp_module);
  INFO("EEPROM: Flash Size:             %d bytes", hw_conf->flash_size_bytes);
  INFO("EEPROM: ADC Type(s):            %d", hw_conf->i2c_adc);
  INFO("EEPROM: ADC Config(s):          %d", hw_conf->internal_adc_type);
  INFO("EEPROM: Internal SDA pin:       %d", hw_conf->i2c_int_sda_pin);
  INFO("EEPROM: Internal SCL pin:       %d", hw_conf->i2c_int_scl_pin);
  INFO("EEPROM: External SDA pin:       %d", hw_conf->i2c_ext_sda_pin);
  INFO("EEPROM: External SCL pin:       %d", hw_conf->i2c_ext_scl_pin);
    
  rc = 1;
  
EXIT_LABEL:  
  
  DEBUG("EEPROM: otb_eeprom_read_hw_conf exit");
  
  return rc;
}


