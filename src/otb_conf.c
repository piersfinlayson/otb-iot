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
  
    if (os_memcmp(conf->pad1, pad, sizeof(*(conf->pad1))))
    {
      WARN("CONF: Pad 1 wasn't 0");
      os_memset(conf->pad1, 0, 3); 
      modified = TRUE;
    }
    
    if ((conf->ds18b20s < 0) || (conf->ds18b20s > OTB_DS18B20_MAX_DS18B20S))
    {
      WARN("CONF: Invalid number of DS18B20s");
      conf->ds18b20s = 0;
      os_memset(conf->ds18b20, 0, OTB_DS18B20_MAX_DS18B20S * sizeof(otb_conf_ds18b20));
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
  }
  
  if (!rc)
  {
    // Something serious was wrong with the config we loaded, so recreate.  The precise
    // error has already been logged by otb_conf_verify
    otb_conf_init_config(otb_conf); 
    rc = otb_conf_save(otb_conf);
    if (!rc)
    {
      otb_reset_error(otb_conf_load_error_string);
      rc = TRUE;
    }
  }
  
  // Log loaded config
  otb_conf_log(otb_conf);
  
  // Now process the config
  if (otb_conf->keep_ap_active)
  {
    otb_wifi_ap_keep_alive();
  }

  DEBUG("CONF: otb_conf_load exit");

  return(rc);
}

void ICACHE_FLASH_ATTR otb_conf_log(otb_conf_struct *conf)
{
  int ii;

  DEBUG("CONF: otb_conf_log entry");
  
  INFO("CONF: version:  %d", conf->version);
  INFO("CONF: ssid:     %s", conf->ssid);
  INFO("CONF: password: ********");
  INFO("CONF: keep_ap_active: %d", conf->keep_ap_active);
  INFO("CONF: location1: %s", conf->loc.loc1);
  INFO("CONF: location2: %s", conf->loc.loc2);
  INFO("CONF: location3: %s", conf->loc.loc3);
  INFO("CONF: DS18B20s:  %d", conf->ds18b20s);
  for (ii = 0; ii < conf->ds18b20s; ii++)
  {
    INFO("CONF: DS18B20 #%d address:  ", ii, conf->ds18b20[ii].id);
    INFO("CONF: DS18B20 #%d location: ", ii, conf->ds18b20[ii].loc);
  }
  
  DEBUG("CONF: otb_conf_log exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_conf_save(otb_conf_struct *conf)
{
  bool rc;

  DEBUG("CONF: otb_conf_save entry");

  spi_flash_erase_sector(OTB_CONF_LOCATION/0x1000);
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

bool ICACHE_FLASH_ATTR otb_conf_store_sta_conf(char *ssid, char *password)
{
  bool rc = TRUE;
  otb_conf_struct *conf = &otb_conf_private;
  bool changed = FALSE;

  DEBUG("CONF: otb_conf_store_sta_conf entry");
  
  if (os_strncmp(conf->ssid, ssid, OTB_CONF_WIFI_SSID_MAX_LEN) ||
      os_strncmp(conf->password, password, OTB_CONF_WIFI_PASSWORD_MAX_LEN))
  {
    INFO("CONF: Updating config: ssid, password");
    os_strncpy(conf->ssid, ssid, OTB_CONF_WIFI_SSID_MAX_LEN);
    os_strncpy(conf->password, password, OTB_CONF_WIFI_PASSWORD_MAX_LEN);
    rc = otb_conf_update(conf);
    if (!rc)
    {
      ERROR("CONF: Failed to update config");
    }
  }
  
  DEBUG("CONF: otb_conf_store_sta_conf exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_conf_store_ap_enabled(bool enabled)
{
  bool rc = TRUE;
  otb_conf_struct *conf = &otb_conf_private;
  
  DEBUG("CONF: otb_conf_store_ap_enabled entry");
  
  if (conf->keep_ap_active != enabled)
  {
    INFO("CONF: Updating config: keep_ap_active");
    conf->keep_ap_active = enabled;
    rc = otb_conf_update(conf);
    if (!rc)
    {
      ERROR("CONF: Failed to update config");
    }
  }
  
  DEBUG("CONF: otb_conf_store_ap_enabled exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_conf_update(otb_conf_struct *conf)
{
  bool rc;
  
  DEBUG("CONF: otb_conf_update entry");

  conf->checksum = otb_conf_calc_checksum(conf);
  rc = otb_conf_save(conf);

  DEBUG("CONF: otb_conf_update exit");

  return(rc);
}

void ICACHE_FLASH_ATTR otb_conf_update_loc(char *loc, char *val)
{
  int len;
  int loc_num;
  bool rc = FALSE;
  otb_conf_struct *conf;

  DEBUG("CONF: otb_conf_update entry");
  
  conf = &otb_conf_private;
  
  if (loc != NULL)
  {
    loc_num = atoi(loc+3);
    if ((loc_num < 1) || (loc_num > 3))
    {
      WARN("CONF: Invalid location %d", loc_num);
      rc = FALSE;
      goto EXIT_LABEL;
    }
  
    if (val == NULL)
    {
      val = OTB_MQTT_EMPTY;
    }
    
    len = os_strlen(val);
    if (len > (OTB_CONF_LOCATION_MAX_LEN))
    {
      WARN("CONF: Invalid location string %s", loc);
      rc = FALSE;
      goto EXIT_LABEL;
    }
  
    switch (loc_num)
    {
      case 1:
        os_strncpy(conf->loc.loc1, val, OTB_CONF_LOCATION_MAX_LEN);
        break;
    
      case 2:
        os_strncpy(conf->loc.loc2, val, OTB_CONF_LOCATION_MAX_LEN);
        break;
    
      case 3:
        os_strncpy(conf->loc.loc3, val, OTB_CONF_LOCATION_MAX_LEN);
        break;
    
      default:
        // Only get here if sloppy coding as checked loc_num above!
        OTB_ASSERT(FALSE);
        break;
    }
  }
  else
  {
    INFO("CONF: No value provided for location");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  rc = otb_conf_update(conf);
  if (!rc)
  {
    ERROR("CONF: Failed to update config");
  }
  
EXIT_LABEL:

  if (rc)
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_SET,
                         OTB_MQTT_STATUS_OK,
                         "");
  }
  else
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_SET,
                         OTB_MQTT_STATUS_ERROR,
                         loc);
  }
  
  DEBUG("CONF: otb_conf_update exit");
 
  return; 
}

void ICACHE_FLASH_ATTR otb_conf_mqtt(char *cmd1, char *cmd2, char *cmd3)
{
  uint8 ii;
  uint8 field = OTB_MQTT_CONFIG_INVALID_;
  bool rc = FALSE;
  char *value;
  int loc_num;

  DEBUG("CONF: otb_conf_mqtt entry");
  
  // Note that cmd1 and cmd2 are colon terminated
  // Supported commands:
  // cmd1 = get/set
  // cmd2 = field
  // cmd3 = value (set only)
  
  if ((cmd1 == NULL) || (cmd2 == NULL))
  {
    INFO("CONF: Invalid config command");
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_STATUS_ERROR,
                         "Invalid command",
                         "");
    goto EXIT_LABEL;
  }
  
  if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_SET))
  {
    for (ii = OTB_MQTT_CONFIG_FIRST_; ii <= OTB_MQTT_CONFIG_LAST_; ii++)
    {
      if (otb_mqtt_match(cmd2, otb_mqtt_config_fields[ii]))
      {
        field = ii;
        break;
      }
    }
    switch (field)
    {
      case OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_:
        if (cmd3 == NULL)
        {
          INFO("CONF: Invalid config command - no value");
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                               OTB_MQTT_CMD_SET,
                               OTB_MQTT_STATUS_ERROR,
                               "No value");
          goto EXIT_LABEL;
        }
        if (!os_strcmp(cmd3, OTB_MQTT_CONFIG_TRUE) ||
            !os_strcmp(cmd3, OTB_MQTT_CONFIG_YES))
        {
          if (otb_wifi_ap_enable())
          {
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                 OTB_MQTT_CMD_SET,
                                 OTB_MQTT_STATUS_OK,
                                 OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_);
          }
          else
          {
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                 OTB_MQTT_CMD_SET,
                                 OTB_MQTT_STATUS_ERROR,
                                 OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_);
          }
        }
        else if (!os_strcmp(cmd3, OTB_MQTT_CONFIG_FALSE) ||
                 !os_strcmp(cmd3, OTB_MQTT_CONFIG_NO))
        {
          if (otb_wifi_ap_disable())
          {
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                 OTB_MQTT_CMD_SET,
                                 OTB_MQTT_STATUS_OK,
                                 OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_);
          }
          else
          {
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                 OTB_MQTT_CMD_SET,
                                 OTB_MQTT_STATUS_ERROR,
                                 OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_);
          }
        }
        else
        {
          INFO("CONF: Invalid value for keep_ap_active");
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                               OTB_MQTT_CMD_SET,
                               OTB_MQTT_STATUS_ERROR,
                               "Invalid value");
          goto EXIT_LABEL;
        }
        break;
        
      case OTB_MQTT_CONFIG_LOC1_:
      case OTB_MQTT_CONFIG_LOC2_:
      case OTB_MQTT_CONFIG_LOC3_:
        otb_conf_update_loc(cmd2, cmd3);
        break;
        
      default:
        INFO("CONF: Invalid config field");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                             OTB_MQTT_CMD_SET,
                             OTB_MQTT_STATUS_ERROR,
                             "Invalid field");
        goto EXIT_LABEL;
        break;
    }
  }
  else if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_GET))
  {
    for (ii = OTB_MQTT_CONFIG_FIRST_; ii++; ii <= OTB_MQTT_CONFIG_LAST_)
    {
      if (otb_mqtt_match(cmd2, otb_mqtt_config_fields[ii]))
      {
        field = ii;
        break;
      }
    }
    switch (field)
    {
      case OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_:
        value = otb_conf->keep_ap_active ? OTB_MQTT_TRUE : OTB_MQTT_FALSE;
        break;
        
      case OTB_MQTT_CONFIG_LOC1_:
      case OTB_MQTT_CONFIG_LOC2_:
      case OTB_MQTT_CONFIG_LOC3_:
        loc_num = atoi(cmd2+3);
        if ((loc_num >= 1) && (loc_num <= 3))
        {
          switch (loc_num)
          {
            case 1:
              value = otb_conf->loc.loc1;
              break;
              
            case 2:
              value = otb_conf->loc.loc2;
              break;
              
            case 3:
              value = otb_conf->loc.loc3;
              break;
              
            default:
              OTB_ASSERT(FALSE);
              otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                   OTB_MQTT_CMD_GET,
                                   OTB_MQTT_STATUS_ERROR,
                                   "Internal error");
              goto EXIT_LABEL;
              break;
          }
        }
        else
        {
          INFO("CONF: Invalid location %d", loc_num);
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                               OTB_MQTT_CMD_GET,
                               OTB_MQTT_STATUS_ERROR,
                               "Invalid location");
          
          goto EXIT_LABEL;
        }
        break;
        
      default:
        INFO("CONF: Invalid config field");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_ERROR,
                             "Invalid location");
        goto EXIT_LABEL;
        break;
    }
  }
  else
  {
    INFO("CONF: Unknown config command");
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_STATUS_ERROR,
                         "Invalid command",
                         "");
    goto EXIT_LABEL;
  }
  
EXIT_LABEL:
  
  if (!rc)
  {
    // Nothing to do - we send an error above
  }  
  
  DEBUG("CONF: otb_conf_mqtt exit");
  
  return;
}
