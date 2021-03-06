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

/*
 * Adaptation of Paul Stoffregen's One wire library to the ESP8266 and
 * Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu>
 * 
 * Paul's original library site:
 *   http://www.pjrc.com/teensy/td_libs_OneWire.html
 *   LGPL v2.1 
 *
 * Necromant's gihub site:
 *   https://github.com/nekromant/esp8266-frankenstein
 *   License ?
 * 
 * See also http://playground.arduino.cc/Learning/OneWire
 * 
 */

#define OTB_DS18B20_C
#include "otb.h"

MLOG("DS18B20");

void ICACHE_FLASH_ATTR otb_ds18b20_initialize(uint8_t bus)
{
  bool rc;
  int ii;

  ENTRY;

  MDETAIL("Initialize one wire bus on GPIO pin %d", bus);

  otb_ds18b20_init(bus);

  INFO("OTB: One wire bus initialized");
  
  // Initialize last temp
  for (ii = 0; ii < OTB_DS18B20_MAX_DS18B20S; ii++)
  {
    os_strncpy(otb_ds18b20_last_temp_s[ii], OTB_DS18B20_INTERNAL_ERROR_TEMP, OTB_DS18B20_MAX_TEMP_LEN);
    otb_ds18b20_last_temp_s[ii][OTB_DS18B20_MAX_TEMP_LEN-1] - 0;
  }

  // Get devices
  otb_ds18b20_count = 0;
  otb_ds18b20_device_callback(NULL);  

  // Set timer to periodically rescan bus for more devices
  os_timer_disarm((os_timer_t*)&otb_ds18b20_device_timer);
  os_timer_setfn((os_timer_t*)&otb_ds18b20_device_timer, (os_timer_func_t *)otb_ds18b20_device_callback, NULL);
  os_timer_arm((os_timer_t*)&otb_ds18b20_device_timer, OTB_DS18B20_REFRESH_DEVICE_INTERVAL, 1);

  EXIT;
  
  return;
}

int ICACHE_FLASH_ATTR otb_ds18b20_valid_index(unsigned char *next_cmd)
{
  int index = -1;

  ENTRY;

  if (next_cmd != NULL)
  {
    index = atoi(next_cmd);
    if (!((index >= 0) && (index < OTB_DS18B20_MAX_DS18B20S) && (index < otb_ds18b20_count)))
    {
      index = -1;
    }
  }

  EXIT;

  return index;
}

void ICACHE_FLASH_ATTR otb_ds18b20_device_callback(void *arg)
{
  bool rc;
  uint8_t prev_count;
  int ii;
  uint32_t timer_int;
  
  ENTRY;

  prev_count = otb_ds18b20_count;

  MDEBUG("Checking for new DS18B20s");

  rc = otb_ds18b20_get_devices();
  if (rc)
  {
    MWARN("More than %d DS18B20 devices - only reading from first %d",
         OTB_DS18B20_MAX_DS18B20S,
         OTB_DS18B20_MAX_DS18B20S);
  }

  if (otb_ds18b20_count > prev_count)
  {
    MDETAIL("DS18B20 device count changed was %u now %u", prev_count, otb_ds18b20_count);
    for (ii = 0; ii < otb_ds18b20_count; ii++)
    {
      // Stagger timer for each temperature sensor - and do any we already had as well
      MDETAIL("Index %d Address %s", ii, otb_ds18b20_addresses[ii].friendly);
      timer_int = OTB_DS18B20_REPORT_INTERVAL * (ii + 1) / otb_ds18b20_count;
      otb_ds18b20_addresses[ii].timer_int = timer_int;
      os_timer_disarm((os_timer_t*)(otb_ds18b20_timer + ii));
      os_timer_setfn((os_timer_t*)(otb_ds18b20_timer + ii), (os_timer_func_t *)otb_ds18b20_callback, otb_ds18b20_addresses + ii);
      if (timer_int != OTB_DS18B20_REPORT_INTERVAL)
      {
        os_timer_arm((os_timer_t*)(otb_ds18b20_timer + ii), timer_int, 0);
      }
      else
      {
        os_timer_arm((os_timer_t*)(otb_ds18b20_timer + ii), timer_int, 1);
      }
    }
  }

  EXIT;

  return;
}

// Returns TRUE if have already got this device
bool ICACHE_FLASH_ATTR otb_ds18b20_check_existing_device(char *ds18b20)
{
  int ii;
  bool rc = FALSE;

  ENTRY;

  for (ii = 0; ii < otb_ds18b20_count; ii++)
  {
    // Don't check the CRC byte
    if (!os_memcmp(otb_ds18b20_addresses[ii].addr,
                   ds18b20, 
                   OTB_DS18B20_DEVICE_ADDRESS_LENGTH-1))
    {
      rc = TRUE;
      goto EXIT_LABEL;
    }
  }

EXIT_LABEL:

  EXIT;

  return (rc);
}

// Returns TRUE if more than OTB_DS18B20_MAX_DS18B20S devices
bool ICACHE_FLASH_ATTR otb_ds18b20_get_devices(void)
{
  int rc = TRUE;
  char ds18b20[OTB_DS18B20_DEVICE_ADDRESS_LENGTH];
  char crc;
  
  MDEBUG("Get devices ...");
  
  reset_search();
  while (rc && (otb_ds18b20_count < OTB_DS18B20_MAX_DS18B20S))
  {
    rc = ds_search(ds18b20);
    if (rc)
    {
      // Only progress if crc is good
      crc = crc8(ds18b20, 7);
      if (crc == ds18b20[7])
      {
        // Only actually add this device if we haven't already added it when
        // previously scanning the bus
        if (!otb_ds18b20_check_existing_device(ds18b20))
        {
          os_memcpy(otb_ds18b20_addresses[otb_ds18b20_count].addr,
                    ds18b20, 
                    OTB_DS18B20_DEVICE_ADDRESS_LENGTH);
          // I want the address format in the same format as debian/raspbian
          // Which reverses the order of all but the first byte, and drops the CRC8
          // byte at the end (which we'll check).
          os_snprintf((char*)otb_ds18b20_addresses[otb_ds18b20_count].friendly,
                      OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH,
                      "%02x-%02x%02x%02x%02x%02x%02x",
                      ds18b20[0],
                      ds18b20[6],
                      ds18b20[5],
                      ds18b20[4],
                      ds18b20[3],
                      ds18b20[2],
                      ds18b20[1]);
          otb_ds18b20_addresses[otb_ds18b20_count].timer_int = 0;
          otb_ds18b20_addresses[otb_ds18b20_count].index = otb_ds18b20_count;
          otb_ds18b20_count++;
          MDEBUG("Successfully added device %s",
                otb_ds18b20_addresses[otb_ds18b20_count].friendly);
        }
      }
      else
      {
        MWARN("CRC error: %02x-%02x%02x%02x%02x%02x%02x, crc=0x%x",
             ds18b20[0],
             ds18b20[6],
             ds18b20[5],
             ds18b20[4],
             ds18b20[3],
             ds18b20[2],
             ds18b20[1],
             crc);
      }
    }
  }

  if (rc)
  {
    rc = ds_search(ds18b20);
  }
    
  return(rc);
}

char ALIGN4 otb_ds18b20_callback_error_string[] = "DS18B20: MQTT disconnected timeout";
void ICACHE_FLASH_ATTR otb_ds18b20_callback(void *arg)
{
  int chars;
  bool rc = FALSE;
  otbDs18b20DeviceAddress *addr;
  char *sensor_loc;
  int tries;
  char output[32];
  char output2[32];
  
  ENTRY;

  addr = (otbDs18b20DeviceAddress *)arg;
  MDEBUG("   Device: %s", addr->friendly);
  MDEBUG("timer_int: %d", addr->timer_int);
  MDEBUG("    index: %d", addr->index);

  if (addr->timer_int != OTB_DS18B20_REPORT_INTERVAL)
  {
    // This must be the first time we were scheduled - we stagger first schedule.
    // So now reschedule on the right timer.
    addr->timer_int = OTB_DS18B20_REPORT_INTERVAL;
    os_timer_disarm((os_timer_t*)(otb_ds18b20_timer + addr->index));
    os_timer_setfn((os_timer_t*)(otb_ds18b20_timer + addr->index), (os_timer_func_t *)otb_ds18b20_callback, arg);
    // Set repeat to 1!
    os_timer_arm((os_timer_t*)(otb_ds18b20_timer + addr->index), OTB_DS18B20_REPORT_INTERVAL, 1);
    // Note this won't match the last of N DS18B20s - that will have this interval
    // already set, and when set that was tested and set to repeat.
  }

  // Read this sensor - try up to 4 times (in case of invalid CRC/data)
  for (tries = 0; (tries < 4) && !rc; tries++)
  {
    otb_ds18b20_prepare_to_read();
    rc = otb_ds18b20_request_temp(addr->addr,
                                  otb_ds18b20_last_temp_s[addr->index]);
  }
  if (!rc)
  {
    os_strcpy(otb_ds18b20_last_temp_s[addr->index], OTB_DS18B20_INTERNAL_ERROR_TEMP);
  }
    
  MDETAIL("Device: %s temp: %s", addr->friendly, otb_ds18b20_last_temp_s[addr->index]);

  if (otb_mqtt_client.connState == MQTT_DATA)
  {
    MDEBUG("Log sensor data");

    // Could send (but may choose not to), so reset disconnectedCounter
    otb_ds18b20_mqtt_disconnected_counter = 0;

    // Put friendly name for sensor in as well, if exists.
    sensor_loc = otb_ds18b20_get_sensor_name(addr->friendly, NULL);
    if (sensor_loc != NULL)
    {
      MDEBUG("Sensor location: %s", sensor_loc);
      os_snprintf(otb_mqtt_scratch,
                  OTB_MQTT_MAX_MSG_LENGTH,
                  "%s/%s",
                  sensor_loc,
                  addr->friendly);
      sensor_loc = otb_mqtt_scratch;            
    }
    else
    {
      sensor_loc = addr->friendly;
    }
      
    if (tries > 1)
    {
      // Record the fact that we had to retry
      os_snprintf(output, 31, "retries:%d/final:%s", tries-1, otb_ds18b20_last_temp_s[addr->index]);
      os_snprintf(output2, 31, "%s/%s", OTB_MQTT_STATUS_ERROR, OTB_MQTT_TEMPERATURE);
      otb_mqtt_publish(&otb_mqtt_client,
                       output2,
                       sensor_loc,
                       output,
                       "",
                       0,
                       1,
                       NULL,
                       0);
    }

    if (strcmp(otb_ds18b20_last_temp_s[addr->index], "-127.00") &&
        strcmp(otb_ds18b20_last_temp_s[addr->index], OTB_DS18B20_INTERNAL_ERROR_TEMP) &&
        strcmp(otb_ds18b20_last_temp_s[addr->index], "85.00"))
    {
      // Decided on reporting using a single format to reduce require MQTT buffer
      // size.
      // Setting qos = 0 (don't care if gets lost), retain = 1 (always retain last
      // publish)
      otb_mqtt_publish(&otb_mqtt_client,
                       OTB_MQTT_TEMPERATURE,
                       sensor_loc,
                       otb_ds18b20_last_temp_s[addr->index],
                       "",
                       0,
                       1,
                       NULL,
                       0);
    }
  }
  else
  {
    MWARN("MQTT not connected, so not sending");
    otb_ds18b20_mqtt_disconnected_counter += 1;
  }

  if ((otb_ds18b20_mqtt_disconnected_counter * OTB_DS18B20_REPORT_INTERVAL) >=
                                                    OTB_MQTT_DISCONNECTED_REBOOT_INTERVAL)
  {
    MERROR("MQTT disconnected %d ms so resetting", 
    otb_ds18b20_mqtt_disconnected_counter * OTB_DS18B20_REPORT_INTERVAL);
    otb_reset(otb_ds18b20_callback_error_string);
  }

  EXIT;

  return;
}

char ICACHE_FLASH_ATTR *otb_ds18b20_get_sensor_name(char *addr, otb_conf_ds18b20 **ds)
{
  char *match;
  otb_conf_ds18b20 *ds18b20;
  int uint8;
  int ii;
  
  ENTRY;
  
  MDEBUG("addr %s", addr);
  
  match = NULL;
  for (ii = 0, ds18b20 = otb_conf->ds18b20; ii < otb_conf->ds18b20s; ds18b20++, ii++)
  {
    MDEBUG("test against %s", ds18b20->id);
    if (otb_mqtt_match(addr, ds18b20->id))
    {
      match = ds18b20->loc;
      if (ds != NULL)
      {
        *ds = ds18b20;
      }
      MDEBUG("Match %d 0x%p", ii, match);
      if (ds18b20->loc[0] != 0)
      {
        // Don't break unless the match was non-NULL - there may be another!
        break;
      }
    }
  }
  
  EXIT;
  
  return(match);
}

char ICACHE_FLASH_ATTR *otb_ds18b20_get_addr(char *name, otb_conf_ds18b20 **ds)
{
  char *match;
  otb_conf_ds18b20 *ds18b20;
  int ii;
  
  ENTRY;
  
  match = NULL;
  for (ii = 0, ds18b20 = otb_conf->ds18b20; ii < otb_conf->ds18b20s; ds18b20++, ii++)
  {
    if (otb_mqtt_match(name, ds18b20->loc))
    {
      match = (char *)&(ds18b20->id[0]);
      if (ds != NULL)
      {
        *ds = ds18b20;
      }
      if (ds18b20->loc[0] != 0)
      {
        break;
      }
    }
  }
  
  EXIT;
  
  return(match);
}

bool ICACHE_FLASH_ATTR otb_ds18b20_check_addr_format(char *addr)
{
  bool rc = FALSE;
  int ii;
  int len;

  ENTRY;
  
  len = otb_mqtt_get_cmd_len(addr);
  
  if (len != (OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH - 1))
  {
    MDEBUG("addr - wrong string length");
    goto EXIT_LABEL;
  }
  
  if ((addr[0] != '2') || (addr[1] != '8') || (addr[2] != '-'))
  {
    MDEBUG("addr - doesn't begin '28-'");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  for (ii = 0; ii < (OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH - 1); ii++)
  {
    if (ii != 2)
    {
      if ((addr[ii] < 0x30) ||
          ((addr[ii] > 0x39) &&
           ((addr[ii] < 0x60) || (addr[ii] > 0x66))))
      {
        // Not hex digit
        MDEBUG("addr - not hex digit found");
        rc = FALSE;
        goto EXIT_LABEL;
        break;
      }
    }
  }
  
EXIT_LABEL:  
  
  EXIT;
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_ds18b20_valid_addr(unsigned char *to_match)
{
  bool rc = FALSE;

  ENTRY;
  
  rc = otb_ds18b20_check_addr_format(to_match);
  if (!rc)
  {
    // Malformed DS18B20 sensor address
    MDETAIL("Sensor address malformed %s", to_match);
    otb_cmd_rsp_append("sensor address malformed");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
    
EXIT_LABEL:

  EXIT;

  return rc;

}

bool ICACHE_FLASH_ATTR otb_ds18b20_configured_addr(unsigned char *to_match)
{
  bool rc = FALSE;
  char *match;
  otb_conf_ds18b20 *ds18b20 = NULL;

  ENTRY;
  
  rc = otb_ds18b20_valid_addr(to_match);
  if (!rc)
  {
    // Response dealt with in sub-function
    goto EXIT_LABEL;
  }
  
  rc = FALSE;

  match = otb_ds18b20_get_sensor_name(to_match, &ds18b20);
  if (match == NULL)
  {
    otb_cmd_rsp_append("unconfigured ds18b20 address");
    goto EXIT_LABEL;
  }

  rc = TRUE;
    
EXIT_LABEL:

  EXIT;

  return rc;

}

bool ICACHE_FLASH_ATTR otb_ds18b20_conf_set(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *addr;
  char *match;
  otb_conf_ds18b20 *ds18b20;
  char *response = OTB_MQTT_EMPTY;
  int ii, jj;
  struct otbDs18b20DeviceAddress *ds;
  bool ds_match;
  
  ENTRY;
  
  // Have already checked the address is valid
  addr = prev_cmd;
  
  // First of all see if there's already a sensor of this address.  If so replace
  // with the provided name.
  match = otb_ds18b20_get_sensor_name(addr, &ds18b20);
  if (match != NULL)
  {
    MDEBUG("Found sensor, updating");
    os_strncpy(ds18b20->loc, next_cmd, OTB_CONF_DS18B20_LOCATION_MAX_LEN);
    rc = TRUE;
    goto EXIT_LABEL;
  }
  
  // If not, find a free slot to put the address in. 
  for (ii = 0,ds18b20 = otb_conf->ds18b20; ii < OTB_DS18B20_MAX_DS18B20S; ds18b20++, ii++)
  {
    if (ds18b20->id[0] == 0)
    {
      MDETAIL("Update empty slot %d %s %s", ii, addr, next_cmd);
      // Have found an empty slot - fill it
      os_strncpy(ds18b20->id, addr, OTB_CONF_DS18B20_MAX_ID_LEN);
      ds18b20->id[OTB_CONF_DS18B20_MAX_ID_LEN-1] = 0;
      os_strncpy(ds18b20->loc, next_cmd, OTB_CONF_DS18B20_LOCATION_MAX_LEN);
      if (otb_conf->ds18b20s != ii)
      {
        MDETAIL("Conf DS18B20s not correct %d", otb_conf->ds18b20s);
      }
      otb_conf->ds18b20s = (ii + 1);
      rc = TRUE;
      goto EXIT_LABEL;
      break;
    }
  }
  
  // No free slot.  This is a bit trickier.  If there's a slot with the address of a
  // sensor not currently present then replace that.  Otherwise reject (this only happens
  // if the maximum number of sensors is present and the user tries to configure a
  // different).
  if (ii == OTB_DS18B20_MAX_DS18B20S)
  {
    for (ii = 0, ds18b20 = otb_conf->ds18b20;
         ii < OTB_DS18B20_MAX_DS18B20S;
         ds18b20++, ii++)
    {
      for (jj = 0, ds = otb_ds18b20_addresses; ii < otb_ds18b20_count; jj++, ds++)
      {
        ds_match = FALSE;
        if (!os_strncmp(ds->friendly, ds18b20->id, OTB_DS18B20_MAX_ADDRESS_STRING_LENGTH))
        {
          ds_match = TRUE;
          break;
        }
      }
      if (!ds_match)
      {
        // Can use this slot
        MDETAIL("Found slot not being used");
        os_strncpy(ds18b20->id, addr, OTB_CONF_DS18B20_MAX_ID_LEN);
        os_strncpy(ds18b20->loc, next_cmd, OTB_CONF_DS18B20_LOCATION_MAX_LEN);
        rc = TRUE;
        goto EXIT_LABEL;
        break;
      }
    }
  }
  
  otb_cmd_rsp_append("no free slots");
  
EXIT_LABEL:
  
  // If successful store off new config
  if (rc)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }

  EXIT;
  
  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_ds18b20_conf_delete(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  char *match;
  char *addr;
  otb_conf_ds18b20 *ds18b20;
  
  ENTRY;
  
  cmd = (int)arg;

  if (cmd == OTB_CMD_DS18B20_ALL)
  {
    os_memset(otb_conf->ds18b20,
              0,
              sizeof(struct otbDs18b20DeviceAddress) * OTB_DS18B20_MAX_DS18B20S);
    otb_conf->ds18b20s = 0;
    rc = TRUE;
    goto EXIT_LABEL;
  }
  else if (cmd == OTB_CMD_DS18B20_ADDR)
  {
    // We've tested the address is configured already, so a bit of asserting
    addr = prev_cmd;
    match = otb_ds18b20_get_sensor_name(addr, &ds18b20);
    OTB_ASSERT(match != NULL);
    os_memset(ds18b20, 0, sizeof(otb_conf_ds18b20));
    otb_conf->ds18b20s--;
    rc = TRUE;
  }
  else
  {
    OTB_ASSERT(FALSE);
  }  
  
EXIT_LABEL:

  // If successful store off new config
  if (rc)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }
  
  EXIT;
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_ds18b20_conf_get(char *sensor, char *index)
{
  char *match = NULL;
  int slot_num;
  char *response = NULL;

  ENTRY;
  
  // Check "sensor" isn't really an index
  if (sensor == NULL)
  {
    match = NULL;
    response = "argument not provided";
    goto EXIT_LABEL;
  }
  
  if (!os_strncmp(sensor, OTB_MQTT_CMD_GET_INDEX, os_strlen(OTB_MQTT_CMD_GET_INDEX)))
  {
    if (index == NULL)
    {
      match = NULL;
      response = "no index provided";
      goto EXIT_LABEL;
    }
    // Get slot number
    slot_num = atoi(index);
    MDEBUG("index %d", slot_num);
    if ((slot_num >= 0) && (slot_num < otb_conf->ds18b20s))
    {
      match = otb_conf->ds18b20[slot_num].id;
    } 
    else
    {
      match = NULL;
      response = "invalid index";
    }
    goto EXIT_LABEL; 
  }
  
  // Not a slot, so sensor may be addr or friendly name - so return the other
  match = otb_ds18b20_get_sensor_name(sensor, NULL);
  if (match == NULL)
  {
    match = otb_ds18b20_get_addr(sensor, NULL);
    goto EXIT_LABEL; 
  }

EXIT_LABEL:
  
  if (match != NULL)
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_GET,
                         OTB_MQTT_STATUS_OK,
                         match);
  }
  else
  {
    if (response == NULL)
    {
      response = "sensor not found";
    }
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_GET,
                         OTB_MQTT_STATUS_ERROR,
                         response);
  }
  
  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_ds18b20_trigger_device_refresh(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = TRUE;
  unsigned char scratch[4];

  ENTRY;

  otb_ds18b20_device_callback(NULL);
  os_snprintf(scratch, 4, "%d", otb_ds18b20_count);
  scratch[3] = 0;
  otb_cmd_rsp_append(scratch);

  EXIT;

  return rc;
};

void ICACHE_FLASH_ATTR otb_ds18b20_prepare_to_read(void)
{
  ENTRY;

  reset();
  write( DS1820_SKIP_ROM, 1 );
  write( DS1820_CONVERT_T, 1 );
  //os_delay_us( 750*1000 );
  otb_util_delay_ms(750);

  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_ds18b20_request_temp(char *addr, char *temp_s)
{
  int ii;
  bool rc = FALSE;
	uint8_t data[12];
  uint16_t tdata, sign;
  uint16_t tVal, tFract;
  char tSign[2];
  uint16_t sign_bits;
  uint8_t crc;
  
  ENTRY;

  // Read scratchpad from DS18 device we're interested in
  // This is up to 9 bytes long, but 0 and 1 bytes contain LSB and MSB
  // of current temp
  reset();
  select(addr);
  write( DS1820_READ_SCRATCHPAD, 0 );
  for(ii = 0; ii< 9; ii++)
  {
    data[ii] = read();
  }
  crc = crc8(data, 8);
  if (crc != data[8])
  {
    MWARN("DS18B20 response %02x%02x%02x%02x%02x%02x%02x%02x%02x - invalid CRC: 0x%02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], crc);
    goto EXIT_LABEL;
  }
  else
  {
    MDETAIL("DS18B20 response %02x%02x%02x%02x%02x%02x%02x%02x%02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
  }

  tdata = (data[1] << 8) | data[0]; 

  // Top X bits are either all 0s or all ones
  if (addr[0] != DS18S20)
  {
    sign_bits = 0xf800;
  }
  else
  {
    sign_bits = 0xff00;
  }

  sign = tdata & sign_bits;
  tSign[0] = 0;
  if (sign)
  {
    if (sign != sign_bits)
    {
      // All bits must be set if any are
      MWARN("Invalid data received 0x%04x", tdata);
      goto EXIT_LABEL;
    }
    tSign[0] = '-';
    tSign[1] = 0;
  }

  // Discard sign info
  tdata &= ~sign_bits;
  if (sign)
  {
    tdata = (tdata ^ (~sign_bits)) + 1;  // Take 2s complement
  }

  if (addr[0] != DS18S20)
  {
    // DS18B20
    tVal = tdata >> 4;
    tFract = (tdata & 0x0F) * 100 / 16;
  }
  else
  {
    // DS18S20
    tVal = tdata >> 1;
    tFract = (tdata & 1) ? 50 : 0;
  }

  if ((sign && ((tVal > 55) ||
                ((tVal == 55) &&
                (tFract > 0)))) ||
      (!sign && ((tVal > 125) ||
                 ((tVal == 125) &&
                  (tFract > 0)))))
  {
    // Invalid value
    MWARN("Invalid value decoded: %s%d.%02d from 0x%04x", tSign, tVal, tFract, (data[1]<<8 | data[0]));
    goto EXIT_LABEL;
  }
  
  os_snprintf(temp_s,
              OTB_DS18B20_MAX_TEMP_LEN,
              "%s%d.%02d",
              tSign, 
              tVal,
              tFract);

  rc = TRUE;

EXIT_LABEL:

  EXIT;

  return(rc);
}

// global search state
static unsigned char ROM_NO[8];
static uint8_t LastDiscrepancy;
static uint8_t LastFamilyDiscrepancy;
static uint8_t LastDeviceFlag;
static int gpioPin;

void ICACHE_FLASH_ATTR otb_ds18b20_init(int gpio)
{
	gpioPin = gpio;

	PIN_FUNC_SELECT(pin_mux[gpioPin], pin_func[gpioPin]);
	//PIN_PULLDWN_DIS(pin_mux[gpioPin]);
  //CLEAR_PERI_REG_MASK(pin_mux[gpioPin], BIT6);  // PULLDWN_DIS replacement
	//PIN_PULLUP_DIS(pin_mux[gpioPin]);
  // Enable pullup resistor for this GPIO (should obviate the need for external resistor)
	//PIN_PULLUP_EN(pin_mux[gpioPin]);  
	  
	GPIO_DIS_OUTPUT(gpioPin);

}

static void reset_search()
{
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;
	for(int i = 7; ; i--) {
		ROM_NO[i] = 0;
		if ( i == 0) break;
	}
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
static uint8_t reset(void)
{
	//	IO_REG_TYPE mask = bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
	uint8_t retries = 125;

	// noInterrupts();
	// DIRECT_MODE_INPUT(reg, mask);
	GPIO_DIS_OUTPUT( gpioPin );
	
	// interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		os_delay_us(2);
	} while ( !GPIO_INPUT_GET( gpioPin ));

	// noInterrupts();
	GPIO_OUTPUT_SET( gpioPin, 0 );
	// DIRECT_WRITE_LOW(reg, mask);
	// DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	// interrupts();
	os_delay_us(480);
	// noInterrupts();
	GPIO_DIS_OUTPUT( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	os_delay_us(70);
	// r = !DIRECT_READ(reg, mask);
	r = !GPIO_INPUT_GET( gpioPin );
	// interrupts();
	os_delay_us(410);

	return r;
}

/* pass array of 8 bytes in */
static int ds_search( uint8_t *newAddr )
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number;
	uint8_t id_bit, cmp_id_bit;
	int search_result;

	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		if (!reset())
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;

			return FALSE;
		}

		// issue the search command
		write(DS1820_SEARCHROM, 0);

		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = read_bit();
			cmp_id_bit = read_bit();
	 
			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
			{
				break;
			}
			else
			{
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit)
					search_direction = id_bit;  // bit write value for search
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					else
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					// if 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

				// serial number search direction write bit
				write_bit(search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		}
		while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}
	for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
	return search_result;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
static void write( uint8_t v, int power ) {
	uint8_t bitMask;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		write_bit( (bitMask & v)?1:0);
	}
	if ( !power) {
		// noInterrupts();
		GPIO_DIS_OUTPUT( gpioPin );
		GPIO_OUTPUT_SET( gpioPin, 0 );
		// DIRECT_MODE_INPUT(baseReg, bitmask);
		// DIRECT_WRITE_LOW(baseReg, bitmask);
		// interrupts();
	}
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline void ICACHE_FLASH_ATTR  write_bit( int v )
{
	// IO_REG_TYPE mask=bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	GPIO_OUTPUT_SET( gpioPin, 0 );
	if( v ) {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(10);
		GPIO_OUTPUT_SET( gpioPin, 1 );
		// DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		// interrupts();
		os_delay_us(55);
	} else {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(65);
		GPIO_OUTPUT_SET( gpioPin, 1 );
		//	DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		//		interrupts();
		os_delay_us(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline int read_bit(void)
{
	//IO_REG_TYPE mask=bitmask;
	//volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
  
	// noInterrupts();
	GPIO_OUTPUT_SET( gpioPin, 0 );
	// DIRECT_MODE_OUTPUT(reg, mask);
	// DIRECT_WRITE_LOW(reg, mask);
	os_delay_us(3);
	GPIO_DIS_OUTPUT( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	os_delay_us(10);
	// r = DIRECT_READ(reg, mask);
	r = GPIO_INPUT_GET( gpioPin );
	// interrupts();
	os_delay_us(53);

	return r;
}

//
// Do a ROM select
//
static void select(const uint8_t *rom)
{
	uint8_t i;

	write(DS1820_MATCHROM, 0);           // Choose ROM

	for (i = 0; i < 8; i++) write(rom[i], 0);
}

//
// Read a byte
//
static uint8_t read() {
	uint8_t bitMask;
	uint8_t r = 0;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		if ( read_bit()) r |= bitMask;
	}
	return r;
}

//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
// The other option is to store the lookup table in flash and then load
// it into a stack variable when required - but likely to be fairly
// slow as well
static uint8_t ICACHE_FLASH_ATTR crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
