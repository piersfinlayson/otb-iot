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

void ICACHE_FLASH_ATTR otb_conf_ads_init_one(otb_conf_ads *ads, char ii)
{

  DEBUG("CONF: otb_conf_ads_init_one entry");

  os_memset(ads, 0, sizeof(otb_conf_ads));
  ads->index = ii;
  ads->mux = 0b000;
  ads->gain = 0b010;
  ads->rate = 0b100;

  DEBUG("CONF: otb_conf_ads_init_one exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_conf_ads_init(otb_conf_struct *conf)
{
  int ii;
  
  DEBUG("CONF: otb_conf_ads_init entry");
  
  conf->adss = 0;
  
  // Belt and braces:
  os_memset(conf->ads, 0, OTB_CONF_ADS_MAX_ADSS * sizeof(otb_conf_ads));
  
  // Now reset each ADS individually
  for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
  {
    otb_conf_ads_init_one(&(conf->ads[ii]), ii);
  }

  DEBUG("CONF: otb_conf_ads_init exit");
  
  return;
}      

bool ICACHE_FLASH_ATTR otb_conf_verify(otb_conf_struct *conf)
{
  bool rc = FALSE;
  int ii;
  char pad[3] = {0, 0, 0};
  bool modified = FALSE;
  int valid_adss = 0;
  
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
    
    // DS18B20 config checking
    
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
    
    // ADS config checking
    
    if ((conf->adss < 0) || (conf->adss > OTB_CONF_ADS_MAX_ADSS))
    {
      WARN("CONF: Invalid number of ADSs");
      otb_conf_ads_init(conf);
      modified = TRUE;
    }
    

    for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
    {
      if (conf->ads[ii].addr != 0x00)
      {
        valid_adss++;
      }
    }
    if (valid_adss != otb_conf->adss)
    {
      WARN("CONF: Wrong overall ADS number set - clearing"); 
      otb_conf_ads_init(conf);
      modified = TRUE;
    }
    
    for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
    {
      // Check DS18B20 id is right length (or 0), and location is null terminated
      if ((os_strnlen(conf->ads[ii].loc, OTB_CONF_ADS_LOCATION_MAX_LEN) >=
                                                         OTB_CONF_ADS_LOCATION_MAX_LEN) ||
          ((conf->ads[ii].addr != 0x00) &&
           (conf->ads[ii].addr != 0x48) &&
           (conf->ads[ii].addr != 0x49) &&
           (conf->ads[ii].addr != 0x4a) &&
           (conf->ads[ii].addr != 0x4b)) ||
          (conf->ads[ii].index != ii) ||
          (conf->ads[ii].pad1[0] != 0) ||
          ((conf->ads[ii].mux < 0) || (conf->ads[ii].mux > 7)) ||
          ((conf->ads[ii].gain < 0) || (conf->ads[ii].gain > 7)) ||
          ((conf->ads[ii].rate < 0) || (conf->ads[ii].rate > 7)) ||
          ((conf->ads[ii].cont < 0) || (conf->ads[ii].cont > 1)) ||
          ((conf->ads[ii].rms < 0) || (conf->ads[ii].rms > 1)) ||
          ((conf->ads[ii].period < 0) || (conf->ads[ii].period >= 0xffff)) ||
          ((conf->ads[ii].samples < 0) || (conf->ads[ii].samples >= 1024)))
      {
        WARN("CONF: ADS index %d something invalid", ii);
        INFO("CONF: ADS %d index:     %d", ii, conf->ads[ii].index);
        INFO("CONF: ADS %d address:   0x%02x", ii, conf->ads[ii].addr);
        INFO("CONF: ADS %d location:  %s", ii, conf->ads[ii].loc);
        INFO("CONF: ADS %d pad1:      0x%x", ii, conf->ads[ii].pad1[0]);
        INFO("CONF: ADS %d mux:       0x%x", ii, conf->ads[ii].mux);
        INFO("CONF: ADS %d gain:      0x%x", ii, conf->ads[ii].gain);
        INFO("CONF: ADS %d rate:      0x%x", ii, conf->ads[ii].rate);
        INFO("CONF: ADS %d cont:      0x%x", ii, conf->ads[ii].cont);
        INFO("CONF: ADS %d rms:       0x%x", ii, conf->ads[ii].rms);
        INFO("CONF: ADS %d period:    %ds", ii, conf->ads[ii].period);
        INFO("CONF: ADS %d samples:   %d", ii, conf->ads[ii].samples);
        otb_conf_ads_init_one(&(conf->ads[ii]), ii);
        modified = TRUE;
      }
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
    // Need to update rather than save so new checksum is calculated
    rc = otb_conf_update(conf);
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
  
  os_memset(conf, 0, sizeof(otb_conf_struct));
  conf->magic = OTB_CONF_MAGIC;
  conf->version = OTB_CONF_VERSION_CURRENT;
  conf->mqtt.port = OTB_MQTT_DEFAULT_PORT;
  
  // Do this last!
  conf->checksum = otb_conf_calc_checksum(conf);
  OTB_ASSERT(otb_conf_verify_checksum(conf));

  DEBUG("CONF: otb_conf_init_config exit");

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
  INFO("CONF: MQTT server: %s", conf->mqtt.svr);
  INFO("CONF: MQTT port:   %d", conf->mqtt.port);
  INFO("CONF: MQTT user:   %s", conf->mqtt.user);
  INFO("CONF: MQTT pass:   %s", conf->mqtt.pass);
  INFO("CONF: location 1: %s", conf->loc.loc1);
  INFO("CONF: location 2: %s", conf->loc.loc2);
  INFO("CONF: location 3: %s", conf->loc.loc3);
  INFO("CONF: DS18B20s:  %d", conf->ds18b20s);
  for (ii = 0; ii < conf->ds18b20s; ii++)
  {
    INFO("CONF: DS18B20 #%d address:  %s", ii, conf->ds18b20[ii].id);
    INFO("CONF: DS18B20 #%d location: %s", ii, conf->ds18b20[ii].loc);
  }
  INFO("CONF: ADSs:  %d", conf->adss);
  for (ii = 0; ii < conf->adss; ii++)
  {
    INFO("CONF: ADS %d address:   0x%02x", ii, conf->ads[ii].addr);
    INFO("CONF: ADS %d location:  %s", ii, conf->ads[ii].loc);
    INFO("CONF: ADS %d mux:       0x%x", ii, conf->ads[ii].mux);
    INFO("CONF: ADS %d gain:      0x%x", ii, conf->ads[ii].gain);
    INFO("CONF: ADS %d rate:      0x%x", ii, conf->ads[ii].rate);
    INFO("CONF: ADS %d cont:      0x%x", ii, conf->ads[ii].cont);
    INFO("CONF: ADS %d rms:       0x%x", ii, conf->ads[ii].rms);
    INFO("CONF: ADS %d period:    %ds", ii, conf->ads[ii].period);
    INFO("CONF: ADS %d samples:   %d", ii, conf->ads[ii].samples);
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

uint8 ICACHE_FLASH_ATTR otb_conf_store_sta_conf(char *ssid, char *password, bool commit)
{
  uint8 rc = OTB_CONF_RC_NOT_CHANGED;
  otb_conf_struct *conf = &otb_conf_private;
  bool update_rc;

  DEBUG("CONF: otb_conf_store_sta_conf entry");
  
  if (os_strncmp(conf->ssid, ssid, OTB_CONF_WIFI_SSID_MAX_LEN) ||
      os_strncmp(conf->password, password, OTB_CONF_WIFI_PASSWORD_MAX_LEN))
  {
    INFO("CONF: New config: ssid, password");
    rc = OTB_CONF_RC_CHANGED;
    os_strncpy(conf->ssid, ssid, OTB_CONF_WIFI_SSID_MAX_LEN);
    os_strncpy(conf->password, password, OTB_CONF_WIFI_PASSWORD_MAX_LEN);
    if (commit)
    {
      INFO("CONF: Committing new config: ssid, password");
      update_rc = otb_conf_update(conf);
      if (!update_rc)
      {
        ERROR("CONF: Failed to update config");
        rc = OTB_CONF_RC_ERROR;
      }
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

void ICACHE_FLASH_ATTR otb_conf_mqtt_conf(char *cmd1, char *cmd2, char *cmd3, char *cmd4, char *cmd5)
{
  uint8 ii;
  uint8 field = OTB_MQTT_CONFIG_INVALID_;
  bool rc = FALSE;
  char *value;
  int loc_num;
  char response[8];
  char *ok_error;

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
        
      case OTB_MQTT_CONFIG_DS18B20_:
        otb_ds18b20_conf_set(cmd3, cmd4);
        break;
        
      case OTB_MQTT_CONFIG_ADS_:
        otb_i2c_ads_conf_set(cmd3, cmd4, cmd5);
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
        value = otb_conf->keep_ap_active ? OTB_MQTT_TRUE : OTB_MQTT_FALSE;
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_OK,
                             value);
        break;
        
      case OTB_MQTT_CONFIG_LOC1_:
      case OTB_MQTT_CONFIG_LOC2_:
      case OTB_MQTT_CONFIG_LOC3_:
        rc = FALSE;
        ok_error = OTB_MQTT_STATUS_ERROR;
        loc_num = atoi(cmd2+3);
        value = "Invalid location";
        if ((loc_num >= 1) && (loc_num <= 3))
        {
          ok_error = OTB_MQTT_STATUS_OK;
          switch (loc_num)
          {
            case 1:
              value = otb_conf->loc.loc1;
              rc = TRUE;
              break;
              
            case 2:
              value = otb_conf->loc.loc2;
              rc = TRUE;
              break;
              
            case 3:
              value = otb_conf->loc.loc3;
              rc = TRUE;
              break;
              
            default:
              OTB_ASSERT(FALSE);
              rc = FALSE;
              ok_error = OTB_MQTT_STATUS_OK;
              value = "Internal error";
              break;
          }
        }
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                             OTB_MQTT_CMD_GET,
                             ok_error,
                             value);
        goto EXIT_LABEL;
        break;
        
      case OTB_MQTT_CONFIG_DS18B20_:
        otb_ds18b20_conf_get(cmd3, cmd4);
        break;
        
      case OTB_MQTT_CONFIG_ADS_:
        otb_i2c_ads_conf_get(cmd3, cmd4, cmd5);
        break;
        
      case OTB_MQTT_CONFIG_DS18B20S_:
        os_snprintf(response, 8, "%d", otb_conf->ds18b20s);
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_OK,
                             response);
        break;
        
      default:
        INFO("CONF: Invalid config field");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_ERROR,
                             "Invalid field");
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
