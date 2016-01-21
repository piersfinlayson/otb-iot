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

#define OTB_OW_C
#include "otb.h"

void ICACHE_FLASH_ATTR otb_ow_initialize(uint8_t bus)
{
  int ii;
  int jj;
  bool rc;
  void *vTask;

  DEBUG("OW: otb_ow_initialize entry");

  DEBUG("OW: Initialize one wire bus on GPIO pin %d", bus);

  otb_ow_onewire_handle = (void *)0;
  otb_ow_sensors_handle = (void *)0;

  otb_temp_initialize_temp(bus, &otb_ow_onewire_handle, &otb_ow_sensors_handle);
  otb_util_delay_ms(1000);
  INFO("OW: One Wire bus initialized");

  otb_ow_ds18b20_count = otb_temp_get_device_count(otb_ow_sensors_handle);
  INFO("OW: DS18B20 device count %u", otb_ow_ds18b20_count);
  if (otb_ow_ds18b20_count > OTB_OW_MAX_DS18B20S)
  {
    otb_ow_ds18b20_count = OTB_OW_MAX_DS18B20S;
    WARN("OW: DS18B20 device count too big, reseting to max %u", OTB_OW_MAX_DS18B20S);
  }
  
  for (ii = 0, jj = 0; (ii < otb_ow_ds18b20_count); ii++)
  {
    // Use jj to actually write in array, and only increment if device address
    // is read successfully.
    DEBUG("OW: Read DS18B20 device # %u", ii+1);
    // Note any failures are logged in get_device_address
    rc = otb_temp_get_device_address(otb_ow_sensors_handle,
                                     ii,
                                     otb_ow_ds18b20_addresses + jj);
    if (rc)
    {
      DEBUG("OW: Successfully read device address %s", otb_ow_ds18b20_addresses + jj);
      jj++;
    }
  }
  // Reset the counter to successfully read addresses
  otb_ow_ds18b20_count = jj;

  if (otb_ow_ds18b20_count > 0)
  {
    // Schedule task to read DS18B20 values every REPORT_INTERVAL, forever,
    // starting ASAP.
    // Only do this is we have some DS18B20s attached.
    vTask = otb_sched_get_task(OTB_SCHED_OW_TASK);
    // Interval between being scheduled, and iterations (-1 = forever)
    otb_sched_repeat_task(vTask, OTB_OW_REPORT_INTERVAL, -1);
    otb_sched_system_os_post(0, 0, 0, &otb_ow_ds18b20_callback, vTask, 0);
  }
  
  DEBUG("OW: otb_ow_initialize entry");
  
  return;
}

void ICACHE_FLASH_ATTR otb_ow_ds18b20_callback(os_event_t event)
{
  int chars;
  
  DEBUG("OW: otb_ow_ds18b20_callback entry");

  // Read all the sensors.
  for (int ii = 0; ii < otb_ow_ds18b20_count; ii++)
  {
    int loop_times = otb_ow_ds18b20_reads;
    bool loop = TRUE;
    while (loop && (loop_times > 0))
    {
      loop = FALSE;
      otb_temp_request_temps(otb_ow_sensors_handle);
      otb_temp_get_temp_string(otb_ow_sensors_handle, ii, otb_ow_last_temp_s[ii]);
    
      if (!strcmp(otb_ow_last_temp_s[ii], "85.0")) 
      {
        loop = TRUE;
        delay(1000);
      }
      INFO("OW: sensor: %d temp: %s", ii+1, otb_ow_last_temp_s[ii]);
      loop_times--;
    }
  }
  // Only read up to 2 times the first time through this function - we do this
  // to allow the sensors time to initialize.
  otb_ow_ds18b20_reads = 1;

  if (otb_mqtt_client.connState == MQTT_DATA)
  {
    DEBUG("DS18B20: Log sensor data");
    // Could send (but may choose not to), so reset disconnectedCounter
    otb_ow_mqtt_disconnected_counter = 0;

    for (int ii; ii < otb_ow_ds18b20_count; ii++)
    {
      chars = strlen(otb_ow_last_temp_s[ii]);
      if (strcmp(otb_ow_last_temp_s[ii], "-127.0") &&
          strcmp(otb_ow_last_temp_s[ii], "85.0"))
      {
        // Decided on reporting using a single format to reduce require MQTT buffer
        // size.
        // Setting qos = 0 (don't care if gets lost), retain = 1 (always retain last
        // publish)
        // XXX Should replace with otb_mqtt_publish call
        snprintf(otb_mqtt_topic_s,
                 OTB_MQTT_MAX_TOPIC_LENGTH,
                 "/%s/%s/%s/%s/%s/%s/%s/%s",
                 OTB_MQTT_ROOT,
                 OTB_MQTT_LOCATION_1,
                 OTB_MQTT_LOCATION_2,
                 OTB_MQTT_LOCATION_3,
                 OTB_MAIN_CHIPID,
                 OTB_MQTT_LOCATION_4_OPT,
                 otb_ow_ds18b20_addresses[ii],
                 OTB_MQTT_TEMPERATURE);
        DEBUG("OW: Publish topic: %s", otb_mqtt_topic_s);
        DEBUG("OW:       message: %s", otb_ow_last_temp_s[ii]);
        MQTT_Publish(&otb_mqtt_client, otb_mqtt_topic_s, otb_ow_last_temp_s[ii], chars, 0, 1);
      }
    }
  }
  else
  {
    WARN("OW: MQTT not connected, so not sending");
    otb_ow_mqtt_disconnected_counter += 1;
  }

  if ((otb_ow_mqtt_disconnected_counter * OTB_OW_REPORT_INTERVAL) >=
                                                    OTB_MQTT_DISCONNECTED_REBOOT_INTERVAL)
  {
    ERROR("OW: MQTT disconnected %d ms so resetting", 
    otb_ow_mqtt_disconnected_counter * OTB_OW_REPORT_INTERVAL);
    otb_reset();
  }

  DEBUG("OW: otb_ow_ds18b20_callback exit");

  return;
}


