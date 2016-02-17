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

#define OTB_CONF_C
#include "otb.h"

void ICACHE_FLASH_ATTR otb_conf_init(void)
{
  DEBUG("CONF: otb_conf_init entry");

  OTB_ASSERT(OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH == OTB_CONF_DS18B20_MAX_ID_LEN);
  OTB_ASSERT(OTB_CONF_MAX_CONF_SIZE >= sizeof(otb_conf_struct));

  os_memset((void *)&otb_conf_private, 0, sizeof(otb_conf));
  otb_conf = &otb_conf_private;

  DEBUG("CONF: otb_conf_init entry");

  return;
}

bool ICACHE_FLASH_ATTR otb_conf_verify(otb_conf_struct *conf)
{
  bool rc = FALSE;
  int ii;
  char pad[3] = {0, 0, 0};
  bool modified = FALSE;
  
  DEBUG("CONF: otb_conf_verify entry");

  // Test magic value
  if (otb_conf->magic != OTB_CONF_MAGIC)
  {
    ERROR("CONF: Bad magic value - invalid config file");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  // Test version valid
  if (otb_conf->version > OTB_CONF_VERSION_CURRENT)
  {
    ERROR("CONF: Bad magic value - invalid config file");
    rc = FALSE;
    goto EXIT_LABEL;
  }

  // Test checksum
  rc = otb_conf_verify_checksum(conf);
  if (!rc)
  {
    ERROR("CONF: Bad checksum - invalid config file");
    goto EXIT_LABEL;
  }
  
  if (otb_conf->version >= 1)
  {
    if ((os_strnlen(conf->ssid, OTB_CONF_WIFI_SSID_MAX_LEN) >=
                                                            OTB_CONF_WIFI_SSID_MAX_LEN) ||
        (os_strnlen(conf->password, OTB_CONF_WIFI_PASSWORD_MAX_LEN) >=
                                                          OTB_CONF_WIFI_PASSWORD_MAX_LEN))
    {
      // Reset _both_ ssid and password if either got corrupted
      WARN("CONF: SSID or Password wasn't null terminated");
      os_memset(conf->ssid, 0, OTB_CONF_WIFI_SSID_MAX_LEN);
      os_memset(conf->password, 0, OTB_CONF_WIFI_PASSWORD_MAX_LEN);
      modified = TRUE;
    }
  
    if (os_memcmp(conf->pad1, pad, 3))
    {
      WARN("CONF: Pad 1 wasn't 0");
      os_memset(conf->pad1, 0, 3); 
      modified = TRUE;
    }
  
    for (ii = 0; ii < OTB_DS18B20_MAX_DS18B20S; ii++)
    {
      // Check DS18B20 id is right length (or 0), and location is null terminated
      if (((os_strnlen(conf->ds18b20[ii].id, OTB_CONF_DS18B20_MAX_ID_LEN) !=
                                                         OTB_CONF_DS18B20_MAX_ID_LEN-1) &&
           (conf->ds18b20[ii].id[0] != 0)) ||
          (os_strnlen(conf->ds18b20[ii].loc, OTB_CONF_DS18B20_LOCATION_MAX_LEN) >=
                                                       OTB_CONF_DS18B20_LOCATION_MAX_LEN))
      {
        WARN("CONF: DS18B20 index %d id or location invalid", ii);
        os_memset(conf->ds18b20[ii].id, 0, OTB_CONF_DS18B20_MAX_ID_LEN);
        os_memset(conf->ds18b20[ii].loc, 0, OTB_CONF_DS18B20_LOCATION_MAX_LEN);
        modified = TRUE;
      }
    }
  
    if ((os_strnlen(conf->loc.loc1, OTB_CONF_LOCATION_MAX_LEN) >=
                                                             OTB_CONF_LOCATION_MAX_LEN) ||
        (os_strnlen(conf->loc.loc2, OTB_CONF_LOCATION_MAX_LEN) >=
                                                             OTB_CONF_LOCATION_MAX_LEN) ||
        (os_strnlen(conf->loc.loc3, OTB_CONF_LOCATION_MAX_LEN) >=
                                                               OTB_CONF_LOCATION_MAX_LEN))

    {
      WARN("CONF: Location invalid");
      os_memset(conf->loc.loc1, 0, OTB_CONF_LOCATION_MAX_LEN);
      os_memset(conf->loc.loc2, 0, OTB_CONF_LOCATION_MAX_LEN);
      os_memset(conf->loc.loc3, 0, OTB_CONF_LOCATION_MAX_LEN);
      modified = TRUE;
    }
  }
  
  if (otb_conf->version > 1)
  {
    // When increment version number need to add code here to handle checking new
    // fields, and also upgrading back level versions
    ERROR("CONF: No versioning code provided for config version: %d", otb_conf->version);
    OTB_ASSERT(FALSE);
  }
  
  if (modified)
  {
    rc = otb_conf_save(conf);
    if (!rc)
    {
      ERROR("CONF: Failed to save corrected config");
      goto EXIT_LABEL;
    }
  }
  
  // If we get this far everything was fine, or we fixed it
  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("CONF: otb_conf_verify exit");

  return(rc);
}

void ICACHE_FLASH_ATTR otb_conf_init_config(otb_conf_struct *conf)
{
  DEBUG("CONF: otb_conf_init_config entry");
  DEBUG("CONF: otb_conf_init_config exit");
  
  os_memset(conf, 0, sizeof(otb_conf_struct));
  conf->magic = OTB_CONF_MAGIC;
  conf->version = OTB_CONF_VERSION_CURRENT;
  
  // Do this last!
  conf->checksum = otb_conf_calc_checksum(conf);
  OTB_ASSERT(otb_conf_verify_checksum(conf));

  return;
}

char ALIGN4 otb_conf_load_error_string[] = "CONF: Failed to save new config";
bool ICACHE_FLASH_ATTR otb_conf_load(void)
{
  bool rc;

  DEBUG("CONF: otb_conf_load entry");

  rc = otb_util_flash_read(OTB_CONF_LOCATION, (uint32 *)otb_conf, sizeof(otb_conf_struct));
  if (rc)
  {
    // Check it's OK
    rc = otb_conf_verify(otb_conf);
    if (!rc)
    {
      // Something serious was wrong with the config we loaded, so recreate.  The precise
      // error has already been logged by otb_conf_verify
      otb_conf_init_config(otb_conf); 
      rc = otb_conf_save(otb_conf);
      if (!rc)
      {
        otb_reset_error(otb_conf_load_error_string);
      }
    }
  }

  DEBUG("CONF: otb_conf_load exit");

  return(rc);
}

bool ICACHE_FLASH_ATTR otb_conf_save(otb_conf_struct *conf)
{
  bool rc = TRUE;
  uint8 tries = 0;
  uint8 spi_rc = SPI_FLASH_RESULT_ERR;

  DEBUG("CONF: otb_conf_save entry");

  rc = otb_util_flash_write((uint32)OTB_CONF_LOCATION,
                            (uint32 *)conf,
                            sizeof(otb_conf_struct));

  DEBUG("CONF: otb_conf_save exit");

  return rc;
}

uint16_t ICACHE_FLASH_ATTR otb_conf_calc_checksum(otb_conf_struct *conf)
{
  uint16_t calc_sum = 0;
  uint8_t *conf_data;
  uint16_t ii;

  DEBUG("CONF: otb_conf_calc_checksum entry");

  // Check 16-bit ii is sufficient
  OTB_ASSERT(sizeof(otb_conf_struct) <= 2^16);
  conf_data = (uint8_t *)conf;
  
  // Very simple checksum algorithm
  for (ii = 0; ii < sizeof(otb_conf_struct); ii++)
  {
    calc_sum += conf_data[ii];
  }
  
  // Now take off the checksum field - this is not part of the checksum check
  calc_sum -= conf_data[OTB_CONF_CHECKSUM_BYTE1-1];
  calc_sum -= conf_data[OTB_CONF_CHECKSUM_BYTE2-1];
  
  DEBUG("CONF: otb_conf_calc_checksum exit");

  return calc_sum;
}

bool ICACHE_FLASH_ATTR otb_conf_verify_checksum(otb_conf_struct *conf)
{
  bool rc = FALSE;
  uint16_t calc_sum;

  DEBUG("CONF: otb_conf_verify_checksum entry");

  calc_sum = otb_conf_calc_checksum(conf);
  if (calc_sum == conf->checksum)
  {
    DEBUG("CONF: Checksums match");
    rc = TRUE;
  }

  INFO("CONF: Provided checksum: 0x%04x Calculated checksum: 0x%04x",
       conf->checksum,
       calc_sum);

  DEBUG("CONF: otb_conf_verify_checksum exit");

  return rc;
}

