/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2020 Piers Finlayson
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

MLOG("CONF");

void ICACHE_FLASH_ATTR otb_conf_init(void)
{
  ENTRY;

  OTB_ASSERT(OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH == OTB_CONF_DS18B20_MAX_ID_LEN);
  OTB_ASSERT(OTB_CONF_MAX_CONF_SIZE >= sizeof(otb_conf_struct));

  os_memset((void *)&otb_conf_private, 0, sizeof(otb_conf));
  otb_conf = &otb_conf_private;

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_conf_ads_init_one(otb_conf_ads *ads, char ii)
{

  ENTRY;

  os_memset(ads, 0, sizeof(otb_conf_ads));
  ads->index = ii;
  ads->mux = 0b000;
  ads->gain = 0b010;
  ads->rate = 0b100;

  EXIT;
  
  return;
}

void ICACHE_FLASH_ATTR otb_conf_ads_init(otb_conf_struct *conf)
{
  int ii;
  
  ENTRY;
  
  conf->adss = 0;
  
  // Belt and braces:
  os_memset(conf->ads, 0, OTB_CONF_ADS_MAX_ADSS * sizeof(otb_conf_ads));
  
  // Now reset each ADS individually
  for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
  {
    otb_conf_ads_init_one(&(conf->ads[ii]), ii);
  }

  EXIT;
  
  return;
}      

bool ICACHE_FLASH_ATTR otb_conf_verify(otb_conf_struct *conf)
{
  bool rc = FALSE;
  int ii;
  char pad[3] = {0, 0, 0};
  bool modified = FALSE;
  int valid_adss = 0;
  
  ENTRY;

  // Test magic value
  if (otb_conf->magic != OTB_CONF_MAGIC)
  {
    MERROR("Bad magic value - invalid config file");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  // Test version valid
  if (otb_conf->version > OTB_CONF_VERSION_CURRENT)
  {
    MERROR("Bad magic value - invalid config file");
    rc = FALSE;
    goto EXIT_LABEL;
  }

  // Test checksum
  rc = otb_conf_verify_checksum(conf, sizeof(*conf));
  if (!rc)
  {
    // Checksum test failed.  This either means
    // - config is corrupt, in which case we'll wipe and start again
    // - the otb-iot didn't know about the ip, mqtt_httpd and pad4 fields
    //   in which case we'll let the failed check slide - and clear out
    //   the ip, mqtt_httpd and pad4 fields
    if (otb_conf_verify_checksum(conf, (sizeof(*conf) - sizeof(conf->ip) - 4)))
    {
      os_memset((void *)&(conf->ip), 0, sizeof(conf->ip));
      conf->mqtt_httpd = OTB_CONF_MQTT_HTTPD_DISABLED;
      os_memset((void *)(conf->pad4), 0, 3);
      modified = TRUE;
      MWARN("IP info not in config - correcting");
    }
    else
    {
      MERROR("Bad checksum - invalid config file");
      goto EXIT_LABEL;
    }
  }
  
  if (otb_conf->version >= 1)
  {
    if ((os_strnlen(conf->ssid, OTB_CONF_WIFI_SSID_MAX_LEN) >=
                                                            OTB_CONF_WIFI_SSID_MAX_LEN) ||
        (os_strnlen(conf->password, OTB_CONF_WIFI_PASSWORD_MAX_LEN) >=
                                                          OTB_CONF_WIFI_PASSWORD_MAX_LEN))
    {
      // Reset _both_ ssid and password if either got corrupted
      MWARN("SSID or Password wasn't null terminated");
      os_memset(conf->ssid, 0, OTB_CONF_WIFI_SSID_MAX_LEN);
      os_memset(conf->password, 0, OTB_CONF_WIFI_PASSWORD_MAX_LEN);
      modified = TRUE;
    }

    OTB_ASSERT((OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL >= OTB_CONF_STATUS_LED_BEHAVIOUR_MIN) &&
               (OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL <= OTB_CONF_STATUS_LED_BEHAVIOUR_MAX));
    if ((conf->status_led < OTB_CONF_STATUS_LED_BEHAVIOUR_MIN) ||
        (conf->status_led > OTB_CONF_STATUS_LED_BEHAVIOUR_MAX))
    {
      MWARN("status_led value invalid %d", conf->status_led);
      conf->status_led = OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL;
      modified = TRUE;
    }
  
    // DS18B20 config checking
    
    if ((conf->ds18b20s < 0) || (conf->ds18b20s > OTB_DS18B20_MAX_DS18B20S))
    {
      MWARN("Invalid number of DS18B20s");
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
        MWARN("DS18B20 index %d id or location invalid", ii);
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
      MWARN("Location invalid");
      os_memset(conf->loc.loc1, 0, OTB_CONF_LOCATION_MAX_LEN);
      os_memset(conf->loc.loc2, 0, OTB_CONF_LOCATION_MAX_LEN);
      os_memset(conf->loc.loc3, 0, OTB_CONF_LOCATION_MAX_LEN);
      modified = TRUE;
    }
    
    // ADS config checking
    
    if ((conf->adss < 0) || (conf->adss > OTB_CONF_ADS_MAX_ADSS))
    {
      MWARN("Invalid number of ADSs");
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
      MWARN("Wrong overall ADS number set - clearing"); 
      otb_conf_ads_init(conf);
      modified = TRUE;
    }

    for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
    {
      // Check ADS id is right length (or 0), and location is null terminated
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
        MWARN("ADS index %d something invalid", ii);
        conf->ads[ii].loc[OTB_CONF_ADS_LOCATION_MAX_LEN-1] = 0; // Null terminate just in case!
        MDETAIL("ADS %d index:     %d", ii, conf->ads[ii].index);
        MDETAIL("ADS %d address:   0x%02x", ii, conf->ads[ii].addr);
        MDETAIL("ADS %d location:  %s", ii, conf->ads[ii].loc);
        MDETAIL("ADS %d pad1:      0x%x", ii, conf->ads[ii].pad1[0]);
        MDETAIL("ADS %d mux:       0x%x", ii, conf->ads[ii].mux);
        MDETAIL("ADS %d gain:      0x%x", ii, conf->ads[ii].gain);
        MDETAIL("ADS %d rate:      0x%x", ii, conf->ads[ii].rate);
        MDETAIL("ADS %d cont:      0x%x", ii, conf->ads[ii].cont);
        MDETAIL("ADS %d rms:       0x%x", ii, conf->ads[ii].rms);
        MDETAIL("ADS %d period:    %ds", ii, conf->ads[ii].period);
        MDETAIL("ADS %d samples:   %d", ii, conf->ads[ii].samples);
        otb_conf_ads_init_one(&(conf->ads[ii]), ii);
        modified = TRUE;
      }
    }
    
    for (ii = 0; ii < 17; ii++)
    {
      if ((conf->gpio_boot_state[ii] < 0) || (conf->gpio_boot_state[ii] > 1))
      {
        MWARN("gpio_boot_state %d invalid 0x2.2x", ii, conf->gpio_boot_state[ii]);
        conf->gpio_boot_state[ii] = 0;
#ifdef OTB_IOT_ESP12
        // For ESP12 modules set GPIO2 to 1 on boot to turn on board LED off
        conf->gpio_boot_state[2] = 1;
#endif
        modified = TRUE;
      } 
    }
    
    for (ii = 0; ii < 3; ii++)
    {
      if (conf->pad3[ii] != 0)
      {
        MWARN("pad3 %d invalid 0x%2.2x", ii, conf->pad3[0]);
        conf->pad3[ii] = 0;
        modified = TRUE;
      }
    }
    
    for (ii = 0; ii < OTB_CONF_RELAY_MAX_MODULES; ii++)
    {
      if ((os_strnlen(conf->relay[ii].loc, OTB_CONF_RELAY_LOC_LEN) >=
                                                                OTB_CONF_RELAY_LOC_LEN) ||
          (conf->relay[ii].index != ii) ||
          ((conf->relay[ii].type < OTB_CONF_RELAY_TYPE_MIN) || (conf->relay[ii].type >= OTB_CONF_RELAY_TYPE_NUM)) ||
          ((conf->relay[ii].num_relays < 0) || (conf->relay[ii].num_relays > OTB_CONF_RELAY_MAX_PER_MODULE)) ||
          (conf->relay[ii].pad1[0] != 0) ||
          (conf->relay[ii].pad1[1] != 0) ||
          (conf->relay[ii].pad1[2] != 0) ||
          ((conf->relay[ii].type == OTB_CONF_RELAY_TYPE_OTB_0_4) &&
           ((conf->relay[ii].addr < 0) || (conf->relay[ii].addr > 7)) ||
           (conf->relay[ii].num_relays > 8) ||
           (conf->relay[ii].status_led != 0) ||
           (conf->relay[ii].relay_pwr_on[0] != 0)) ||
          ((conf->relay[ii].type == OTB_CONF_RELAY_TYPE_PCA) &&
           ((conf->relay[ii].addr < 0x40) || (conf->relay[ii].addr > 0x3f)) ||
           (conf->relay[ii].num_relays > 16) ||
           ((conf->relay[ii].status_led < -1) || (conf->relay[ii].status_led > 16))))
      {
        MWARN("Relay index %d something invalid", ii);
        conf->relay[ii].loc[OTB_CONF_RELAY_LOC_LEN-1] = 0; // Null terminate just in case!
        MDETAIL("Relay %d location:    %s", ii, conf->relay[ii].loc);
        MDETAIL("Relay %d index:       %d", ii, conf->relay[ii].index);
        MDETAIL("Relay %d type:        %d", ii, conf->relay[ii].type);
        MDETAIL("Relay %d addr:        0x%02x", ii, conf->relay[ii].addr);
        MDETAIL("Relay %d num_relays:  0x%02x", ii, conf->relay[ii].num_relays);
        MDETAIL("Relay %d status_led:  0x%02x", ii, conf->relay[ii].status_led);
        MDETAIL("Relay %d pad1[0]:     0x%02x", ii, conf->relay[ii].pad1[0]);
        MDETAIL("Relay %d pad1[1]:     0x%02x", ii, conf->relay[ii].pad1[1]);
        MDETAIL("Relay %d pad1[2]:     0x%02x", ii, conf->relay[ii].pad1[2]);
        os_memset(&(conf->relay[ii]), 0, sizeof(otb_conf_relay));
        conf->relay[ii].index = ii;
        modified = TRUE;
      }
    }    

    if (otb_conf_verify_ip(conf))
    {
      modified = TRUE;
    }

    if ((conf->mqtt_httpd != OTB_CONF_MQTT_HTTPD_DISABLED) &&
        (conf->mqtt_httpd != OTB_CONF_MQTT_HTTPD_ENABLED))
    {
      MWARN("MQTT HTTPD invalid %d", conf->mqtt_httpd);
      conf->mqtt_httpd == OTB_CONF_MQTT_HTTPD_DISABLED;
      modified = TRUE;
    }
    
    if ((conf->pad4[0] != 0) || (conf->pad4[1] != 0) || (conf->pad4[2] != 0))
    {
      MWARN("pad4 invalid 0x%2.2x%2.2x%2.2x", conf->pad4[0], conf->pad4[1], conf->pad4[2]);
      os_memset((void *)(conf->pad4), 0, 3);
      modified = TRUE;
    }

  } // if (otb_conf->version >= 1)
  
  if (otb_conf->version > 1)
  {
    // When increment version number need to add code here to handle checking new
    // fields, and also upgrading back level versions
    MERROR("No versioning code provided for config version: %d", otb_conf->version);
    OTB_ASSERT(FALSE);
  }
  
  if (modified)
  {
    // Need to update rather than save so new checksum is calculated
    rc = otb_conf_update(conf);
    if (!rc)
    {
      MERROR("Failed to save corrected config");
      goto EXIT_LABEL;
    }
  }
  
  // If we get this far everything was fine, or we fixed it
  rc = TRUE;
  
EXIT_LABEL:

  EXIT;

  return(rc);
}

bool ICACHE_FLASH_ATTR otb_conf_verify_manual_ip(otb_conf_struct *conf)
{
  bool rc = TRUE;

  ENTRY;

  // Can't cope with an invalid IP address - will set back to DHCP
  if (otb_util_ip_is_all_val(conf->ip.ipv4, 0) ||
      otb_util_ip_is_all_val(conf->ip.ipv4, 0xff))
  {
    MDETAIL("IP IPv4 address invalid");
    rc = FALSE;
    goto EXIT_LABEL;
  }

  // Check subnet mask is non zero
  if (!otb_util_ip_is_subnet_valid(conf->ip.ipv4_subnet))
  {
    MDETAIL("IP IPv4 subnet mask invalid");
    rc = FALSE;
    goto EXIT_LABEL;
  }

  // DNS servers of 0.0.0.0 are OK (unconfigured) but 255.255.255.255 is not
  if (otb_util_ip_is_all_val(conf->ip.dns1, 0xff) ||
      otb_util_ip_is_all_val(conf->ip.dns2, 0xff))
  {
    MDETAIL("IP DNS server invalid");
    rc = FALSE;
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_conf_verify_ip(otb_conf_struct *conf)
{
  bool invalid = FALSE;
  int ii;
  uint32_t subnet_mask;

  ENTRY;

  if ((conf->ip.manual != OTB_IP_DHCP_DHCP) && 
      (conf->ip.manual != OTB_IP_DHCP_MANUAL))
  {
    MDETAIL("IP manual/dhcp configuration invalid");
    invalid = TRUE;
    goto EXIT_LABEL;
  }

  if ((conf->ip.pad[0] != 0) ||
      (conf->ip.pad[1] != 0) ||
      (conf->ip.pad[2] != 0))
  {
    MDETAIL("IP pad something invalid");
    invalid = TRUE;
    goto EXIT_LABEL;
  }

  // Only check other information if manual IP addressing selected
  if (conf->ip.manual == OTB_IP_DHCP_MANUAL)
  {
    if (!otb_conf_verify_manual_ip(conf))
    {
      invalid = TRUE;
    }
  }

  for (ii = 0; ii < OTB_IP_MAX_DOMAIN_NAME_LEN; ii++)
  {
    if (conf->ip.domain_name[ii] == 0)
    {
      break;
    }
  }
  if (ii >= OTB_IP_MAX_DOMAIN_NAME_LEN)
  {
    MDETAIL("IP domain name invalid");
    invalid = TRUE;
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  if (invalid)
  {
    MWARN("IP configuration invalid");
    otb_conf_log_ip(conf, TRUE);
    os_memset((void *)&(conf->ip), 0, sizeof(conf->ip));
  }

  EXIT;

  return(invalid);
}

void ICACHE_FLASH_ATTR otb_conf_init_config(otb_conf_struct *conf)
{
  int ii;

  ENTRY;
  
  os_memset(conf, 0, sizeof(otb_conf_struct));
#ifdef OTB_IOT_ESP12
  // For ESP12 modules set GPIO2 to 1 on boot to turn on board LED off
  conf->gpio_boot_state[2] = 1;
#endif
  conf->magic = OTB_CONF_MAGIC;
  conf->version = OTB_CONF_VERSION_CURRENT;
  conf->mqtt.port = OTB_MQTT_DEFAULT_PORT;
  for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
  {
    otb_conf_ads_init_one(&(conf->ads[ii]), ii);
  }
  for (ii = 0; ii < OTB_CONF_RELAY_MAX_MODULES; ii++)
  {
    os_memset(&(conf->relay[ii]), 0, sizeof(otb_conf_relay));
    conf->relay[ii].index = ii;
  }
  
  // Do this last!
  conf->checksum = otb_conf_calc_checksum(conf, sizeof(*conf));
  OTB_ASSERT(otb_conf_verify_checksum(conf, sizeof(*conf)));

  EXIT;

  return;
}

char ALIGN4 otb_conf_load_error_string[] = "CONF: Failed to save new config";
char ALIGN4 otb_conf_load_error_reboot_string[] = "CONF: New config stored, rebooting";
bool ICACHE_FLASH_ATTR otb_conf_load(void)
{
  bool rc;

  ENTRY;

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
    else
    {
      otb_reset(otb_conf_load_error_reboot_string);
      rc = TRUE;
    }
  }
  
  // Log loaded config
  otb_conf_log(otb_conf);
  
  // Now process the config
  if (otb_conf->keep_ap_active || otb_conf->mqtt_httpd)
  {
    otb_wifi_ap_keep_alive();
  }

  EXIT;

  return(rc);
}

void ICACHE_FLASH_ATTR otb_conf_log(otb_conf_struct *conf)
{
  int ii;

  ENTRY;
  
  MDETAIL("version:  %d", conf->version);
  MDETAIL("ssid:     %s", conf->ssid);
  MDETAIL("password: ********");
  MDETAIL("keep_ap_active: %d", conf->keep_ap_active);
  MDETAIL("MQTT server: %s", conf->mqtt.svr);
  MDETAIL("MQTT port:   %d", conf->mqtt.port);
  MDETAIL("MQTT user:   %s", conf->mqtt.user);
  MDETAIL("MQTT pass:   %s", conf->mqtt.pass);
  MDETAIL("location 1: %s", conf->loc.loc1);
  MDETAIL("location 2: %s", conf->loc.loc2);
  MDETAIL("location 3: %s", conf->loc.loc3);
  MDETAIL("DS18B20s:  %d", conf->ds18b20s);
  for (ii = 0; ii < conf->ds18b20s; ii++)
  {
    MDETAIL("DS18B20 #%d address:  %s", ii, conf->ds18b20[ii].id);
    MDETAIL("DS18B20 #%d location: %s", ii, conf->ds18b20[ii].loc);
  }
  MDETAIL("Status LED behaviour: %d", conf->status_led);
  MDETAIL("ADSs:  %d", conf->adss);
  for (ii = 0; ii < conf->adss; ii++)
  {
    MDETAIL("ADS %d address:   0x%02x", ii, conf->ads[ii].addr);
    MDETAIL("ADS %d location:  %s", ii, conf->ads[ii].loc);
    MDETAIL("ADS %d mux:       0x%x", ii, conf->ads[ii].mux);
    MDETAIL("ADS %d gain:      0x%x", ii, conf->ads[ii].gain);
    MDETAIL("ADS %d rate:      0x%x", ii, conf->ads[ii].rate);
    MDETAIL("ADS %d cont:      0x%x", ii, conf->ads[ii].cont);
    MDETAIL("ADS %d rms:       0x%x", ii, conf->ads[ii].rms);
    MDETAIL("ADS %d period:    %ds", ii, conf->ads[ii].period);
    MDETAIL("ADS %d samples:   %d", ii, conf->ads[ii].samples);
  }
  otb_conf_log_ip(conf, FALSE);
  MDETAIL("MQTT HTTPD enabled: %d", conf->mqtt_httpd);
  
  EXIT;
  
  return;
}

void ICACHE_FLASH_ATTR otb_conf_log_ip(otb_conf_struct *conf, bool detail)
{
  ENTRY;

  MDETAIL("IP domain name:  %s", conf->ip.domain_name);
  if (conf->ip.manual == OTB_IP_DHCP_DHCP)
  {
    MDETAIL("IP addressing: DHCP");
  }
  else if (conf->ip.manual == OTB_IP_DHCP_MANUAL)
  {
    MDETAIL("IP addressing: Manual");
  }
  else
  {
    MDETAIL("IP Manual: %d", conf->ip.manual);
  }
  if (detail)
  {
    MDETAIL("IP pad[0]: 0x%02x", conf->ip.pad[0]);
    MDETAIL("IP pad[1]: 0x%02x", conf->ip.pad[1]);
    MDETAIL("IP pad[2]: 0x%02x", conf->ip.pad[2]);
  }
  if ((conf->ip.manual == OTB_IP_DHCP_MANUAL) || detail)
  {
    MDETAIL("IP IPv4:   %d.%d.%d.%d",
            conf->ip.ipv4[0],
            conf->ip.ipv4[1],
            conf->ip.ipv4[2],
            conf->ip.ipv4[3]);
    MDETAIL("IP IPv4 subnet: %d.%d.%d.%d",
            conf->ip.ipv4_subnet[0],
            conf->ip.ipv4_subnet[1],
            conf->ip.ipv4_subnet[2],
            conf->ip.ipv4_subnet[3]);
    MDETAIL("IP IPv4 gateway: %d.%d.%d.%d",
            conf->ip.gateway[0],
            conf->ip.gateway[1],
            conf->ip.gateway[2],
            conf->ip.gateway[3]);
    MDETAIL("IP DNS 1:  %d.%d.%d.%d",
            conf->ip.dns1[0],
            conf->ip.dns1[1],
            conf->ip.dns1[2],
            conf->ip.dns1[3]);
    MDETAIL("IP DNS 2:  %d.%d.%d.%d",
            conf->ip.dns2[0],
            conf->ip.dns2[1],
            conf->ip.dns2[2],
            conf->ip.dns2[3]);
  }

  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_conf_save(otb_conf_struct *conf)
{
  bool rc;

  ENTRY;

  spi_flash_erase_sector(OTB_CONF_LOCATION/0x1000);
  rc = otb_util_flash_write((uint32)OTB_CONF_LOCATION,
                            (uint32 *)conf,
                            sizeof(otb_conf_struct));

  EXIT;

  return rc;
}

uint16_t ICACHE_FLASH_ATTR otb_conf_calc_checksum(otb_conf_struct *conf, size_t size)
{
  uint16_t calc_sum = 0;
  uint8_t *conf_data;
  uint16_t ii;

  ENTRY;

  // Check 16-bit ii is sufficient
  OTB_ASSERT(size <= 2^16);
  conf_data = (uint8_t *)conf;
  
  // Very simple checksum algorithm
  for (ii = 0; ii < size; ii++)
  {
    calc_sum += conf_data[ii];
  }
  
  // Now take off the checksum field - this is not part of the checksum check
  calc_sum -= conf_data[OTB_CONF_CHECKSUM_BYTE1-1];
  calc_sum -= conf_data[OTB_CONF_CHECKSUM_BYTE2-1];
  
  EXIT;

  return calc_sum;
}

bool ICACHE_FLASH_ATTR otb_conf_verify_checksum(otb_conf_struct *conf, size_t size)
{
  bool rc = FALSE;
  uint16_t calc_sum;

  ENTRY;

  calc_sum = otb_conf_calc_checksum(conf, size);
  if (calc_sum == conf->checksum)
  {
    MDEBUG("Checksums match");
    rc = TRUE;
  }

  MDETAIL("Provided checksum: 0x%04x Calculated checksum: 0x%04x",
       conf->checksum,
       calc_sum);

  EXIT;

  return rc;
}

uint8 ICACHE_FLASH_ATTR otb_conf_store_sta_conf(char *ssid, char *password, bool commit)
{
  uint8 rc = OTB_CONF_RC_NOT_CHANGED;
  otb_conf_struct *conf = &otb_conf_private;
  bool update_rc;

  ENTRY;
  
  if (os_strncmp(conf->ssid, ssid, OTB_CONF_WIFI_SSID_MAX_LEN) ||
      os_strncmp(conf->password, password, OTB_CONF_WIFI_PASSWORD_MAX_LEN))
  {
    MDETAIL("New config: ssid, password");
    rc = OTB_CONF_RC_CHANGED;
    os_strncpy(conf->ssid, ssid, OTB_CONF_WIFI_SSID_MAX_LEN);
    os_strncpy(conf->password, password, OTB_CONF_WIFI_PASSWORD_MAX_LEN);
    if (commit)
    {
      MDETAIL("Committing new config: ssid, password");
      update_rc = otb_conf_update(conf);
      if (!update_rc)
      {
        MERROR("Failed to update config");
        rc = OTB_CONF_RC_ERROR;
      }
    }
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_conf_store_ap_enabled(bool enabled)
{
  bool rc = TRUE;
  otb_conf_struct *conf = &otb_conf_private;
  
  ENTRY;
  
  if (conf->keep_ap_active != enabled)
  {
    MDETAIL("Updating config: keep_ap_active");
    conf->keep_ap_active = enabled;
    rc = otb_conf_update(conf);
    if (!rc)
    {
      MERROR("Failed to update config");
    }
  }
  
  EXIT;
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_conf_update(otb_conf_struct *conf)
{
  bool rc;
  
  ENTRY;

  conf->checksum = otb_conf_calc_checksum(conf, sizeof(*conf));
  rc = otb_conf_save(conf);

  EXIT;

  return(rc);
}

bool ICACHE_FLASH_ATTR otb_conf_update_loc(int loc, char *val)
{
  int len;
  bool rc = FALSE;
  otb_conf_struct *conf;

  ENTRY;
  
  conf = &otb_conf_private;
  
  OTB_ASSERT((loc >= 1) && (loc <= 3));
  
  len = os_strlen(val);
  if (len > (OTB_CONF_LOCATION_MAX_LEN))
  {
    otb_cmd_rsp_append("location string too long", val);
    goto EXIT_LABEL;
  }
  
  switch (loc)
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
      OTB_ASSERT(FALSE);
      goto EXIT_LABEL;
      break;
  }

  rc = otb_conf_update(conf);
  if (!rc)
  {
    MERROR("Failed to update config");
    otb_cmd_rsp_append("internal error");
  }
  
EXIT_LABEL:

  EXIT;
 
  return rc; 
}

bool ICACHE_FLASH_ATTR otb_conf_set_status_led(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32_t behaviour;
  uint8_t old;

  ENTRY;
  
  behaviour = (uint32_t)arg;

  OTB_ASSERT((behaviour == OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL) ||
             (behaviour == OTB_CONF_STATUS_LED_BEHAVIOUR_OFF) ||
             (behaviour == OTB_CONF_STATUS_LED_BEHAVIOUR_WARN));

  if (behaviour != otb_conf->status_led)
  {
    // Turn off LED before we actually update the configuration otherwise
    // LED update code won't run
    if ((behaviour == OTB_CONF_STATUS_LED_BEHAVIOUR_OFF) ||
        ((behaviour == OTB_CONF_STATUS_LED_BEHAVIOUR_WARN) &&
         (otb_led_wifi_colour == OTB_LED_NEO_COLOUR_GREEN)))
    {
      otb_led_wifi_update(OTB_LED_NEO_COLOUR_OFF, TRUE);
    }

    // Now update the config
    old = otb_conf->status_led;
    otb_conf->status_led = (uint8_t)behaviour;
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      MERROR("Failed to update config");
      otb_cmd_rsp_append("internal error");
      otb_conf->status_led = old;
      goto EXIT_LABEL;
    }
    otb_cmd_rsp_append("amended to %d", behaviour);
  }
  else
  {
    otb_cmd_rsp_append("no change");
  }

  rc = TRUE;

EXIT_LABEL:

  EXIT;
  
  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_conf_set_keep_ap_active(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  bool active;

  ENTRY;
  
  active = (bool)arg;
  
  OTB_ASSERT((active == FALSE) || (active == TRUE));
  
  if (active)
  {
    if (otb_wifi_ap_enable())
    {
      rc = TRUE;
    }
  }
  else
  {
    if (otb_wifi_ap_disable())
    {
      rc = TRUE;
    }
  }
  
  EXIT;
  
  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_conf_set_loc(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;
  int loc;

  ENTRY;

  // Loc is checked in otb_conf_update_loc (well, asserted)  
  loc = (int)arg;
  
  rc = otb_conf_update_loc(loc, next_cmd);

  EXIT;
  
  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_conf_delete_loc(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int loc;
  int ii;

  ENTRY;

  loc = (int)arg;
  if (loc == 0)
  {
    // This means all
    for (ii = 0; ii < 3; ii++)
    {
      rc = otb_conf_update_loc(ii+1, "");
      if (!rc)
      {
        otb_cmd_rsp_append("failed on loc %d, aborting", ii+1);
        goto EXIT_LABEL;
      }
    }
  }
  else
  {  
    rc = otb_conf_update_loc(loc, "");
  }

EXIT_LABEL:

  EXIT;
  
  return rc;
  
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

  ENTRY;
  
  // Note that cmd1 and cmd2 are colon terminated
  // Supported commands:
  // cmd1 = get/set
  // cmd2 = field
  // cmd3 = value (set only)
  
  if ((cmd1 == NULL) || (cmd2 == NULL))
  {
    MDETAIL("Invalid config command");
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
#if 0
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
#endif        
      default:
        MDETAIL("Invalid config field");
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
        
      case OTB_MQTT_CONFIG_ALL_:
        {
          unsigned char *conf_struct = (unsigned char *)otb_conf;
          unsigned char scratch[129];
          unsigned char kk = 0;
          os_snprintf(scratch, 128, "len:%d", sizeof(otb_conf_struct));
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                               OTB_MQTT_CMD_GET,
                               OTB_MQTT_STATUS_OK,
                               scratch);
          for (int jj = 0; jj < sizeof(otb_conf_struct); jj++)
          {
            os_snprintf(scratch+(kk*2), (128-(kk*2)), "%02x", conf_struct[jj]);
            if (kk >= 63)
            {
              // Send message of 64 bytes (128 byte string null terminated)
              scratch[128] = 0;
              otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                   OTB_MQTT_CMD_GET,
                                   OTB_MQTT_STATUS_OK,
                                   scratch);
              kk = 0;
            }
            else
            {
              kk++;
            }
          }
          if (kk != 0)
          {
            // Send message of k bytes
            scratch[kk*2] = 0;
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                                 OTB_MQTT_CMD_GET,
                                 OTB_MQTT_STATUS_OK,
                                 scratch);
          }
        }
        break;
        
      default:
        MDETAIL("Invalid config field");
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
    MDETAIL("Unknown config command");
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
  
  EXIT;
  
  return;
}
