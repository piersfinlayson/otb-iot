/*
 *
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
 * 
 */

#include <Arduino.h>
#include <osapi.h>
#include "otb.h"
#include "temp_mqtt.h"
#include "mqtt.h"

extern void *getTask(int);
extern void *getSr(int);

void *oneWire_handle;
void *sensors_handle;
void (*log_fn)(char *);

char log_s[MAX_LOG_LENGTH];
uint8_t ds18b20Reads = 2;
uint8_t disconnectedCounter = 0;

DeviceAddressString ds18b20Addresses[MAX_DS18B20S];
uint8_t ds18b20Count = 0;

//void ICACHE_FLASH_ATTR c_loop(void)
void ICACHE_FLASH_ATTR ds18b20Callback(os_event_t event)
{
  char temp_s[MAX_DS18B20S][7];
  int chars;

  // Read all the sensors.
  for (int ii = 0; ii < ds18b20Count; ii++)
  {
    int loop_times = ds18b20Reads;
    bool loop = TRUE;
    while (loop && (loop_times > 0))
    {
      loop = FALSE;
      request_temps(sensors_handle);
      get_temp_string(sensors_handle, ii, temp_s[ii]);
    
      if (!strcmp(temp_s[ii], "85.0")) 
      {
        loop = TRUE;
        delay(1000);
      }
      LOG("DS18B20: sensor: %d temp: %s", ii+1, temp_s[ii]);
      loop_times--;
    }
  }
  // Only read up to 2 times the first time through this function - we do this
  // to allow the sensors time to initialize.
  ds18b20Reads = 1;

  if (mqttClient.connState == MQTT_DATA)
  {
    LOG("DS18B20: Log sensor data");
    // Could send (but may choose not to), so reset disconnectedCounter
    disconnectedCounter = 0;

    for (int ii; ii < ds18b20Count; ii++)
    {
      chars = strlen(temp_s[ii]);
      if (strcmp(temp_s[ii], "-127.0") && strcmp(temp_s[ii], "85.0"))
      {
	// qos = 0, retain = 1
	sprintf(topic_s,
		"/%s/%s/%s/%s/%s/%s",
		OTB_ROOT,
		LOCATION_1,
		LOCATION_2,
		LOCATION_3,
                ds18b20Addresses[ii],
		TEMPERATURE);
	MQTT_Publish(&mqttClient, topic_s, temp_s[ii], chars, 0, 1);
	sprintf(topic_s,
		"/%s/%s/%s/%s/%s/%s/%s",
		OTB_ROOT,
		LOCATION_1,
		LOCATION_2,
		LOCATION_3,
		LOCATION_4_OPT,
                ds18b20Addresses[ii],
		TEMPERATURE);
	MQTT_Publish(&mqttClient, topic_s, temp_s[ii], chars, 0, 1);
	sprintf(topic_s,
		"/%s/%s/%s/%s/%s/%s/%s",
		OTB_ROOT,
		LOCATION_1,
		LOCATION_2,
		LOCATION_3,
		OTB_CHIPID,
                ds18b20Addresses[ii],
		TEMPERATURE);
	MQTT_Publish(&mqttClient, topic_s, temp_s[ii], chars, 0, 1);
	sprintf(topic_s,
		"/%s/%s/%s/%s/%s/%s/%s/%s",
		OTB_ROOT,
		LOCATION_1,
		LOCATION_2,
		LOCATION_3,
		OTB_CHIPID,
		LOCATION_4_OPT,
                ds18b20Addresses[ii],
		TEMPERATURE);
	MQTT_Publish(&mqttClient, topic_s, temp_s[ii], chars, 0, 1);
	sprintf(topic_s,
		"/%s/%s/%s/%s/%s/%s/%s/%s",
		OTB_ROOT,
		LOCATION_1,
		LOCATION_2,
		LOCATION_3,
		LOCATION_4_OPT,
		OTB_CHIPID,
                ds18b20Addresses[ii],
		TEMPERATURE);
	MQTT_Publish(&mqttClient, topic_s, temp_s[ii], chars, 0, 1);
      }
    }
  }
  else
  {
    LOG("DS18B20: MQTT not connected, so not sending");
    disconnectedCounter += 1;
  }

  if ((disconnectedCounter*REPORT_INTERVAL) >= DISCONNECTED_REBOOT_INTERVAL)
  {
    LOG("DS18B20: MQTT disconnected %d ms so resetting", disconnectedCounter*REPORT_INTERVAL);
    reset();
  }

  //LOG("DS18B20: callback exit");
}


void ICACHE_FLASH_ATTR c_setup(void(*log)(char *))
{
  void *vTask;
  log_fn = log;

  LOG("DS18B20: c_setup entry");

  LOG("MQTT: initialize mqtt ...");
  mqtt_init(MQTT_SERVER, 1880, 0, "otb_iot", "user", "pass", 120);
  // brief pause to allow this to settle down
  delay(1000);

  // Query one wire bus for DS18B20 devices.  We have to do this before we
  // start up MQTT, as MQTT scheduling screws up with OneWire timing.
  oneWire_handle = (void *)0;
  sensors_handle = (void *)0;

  LOG("DS18B20: Initialize one wire bus (GPIO pin %d) ...", ONE_WIRE_BUS);
  initialize_temp(ONE_WIRE_BUS, &oneWire_handle, &sensors_handle);
  delay(1000);
  LOG("DS18B20: One Wire bus initialized");
  ds18b20Count = get_device_count(sensors_handle);

  LOG("DS18B20: Device count %u", ds18b20Count);
  if (ds18b20Count > MAX_DS18B20S)
  {
    ds18b20Count = MAX_DS18B20S;
    LOG("DS18B20: Device count reset to max %u", MAX_DS18B20S);
  }
  int jj = 0;
  for (int ii = 0; (ii < ds18b20Count); ii++)
  {
    // Use jj to actually write in array, and only increment if device address
    // is read successfully.
    LOG("DS18B20: Read device # %u", ii+1);
    bool rc;
    // Note any failures are logged in get_device_address
    rc = get_device_address(sensors_handle, ii, ds18b20Addresses+jj);
    if (rc)
    {
      jj++;
    }
  }
  // Reset the counter to successfully read addresses
  ds18b20Count = jj;

  // Schedule task to read DS18B20 values every REPORT_INTERVAL, forever,
  // starting ASAP.
  vTask = getTask(2);
  // Interval between being scheduled, and iterations (-1 = forever)
  repeatTask(vTask, REPORT_INTERVAL, -1);
  fake_system_os_post(0, 0, 0, &ds18b20Callback, vTask, 0);

  LOG("DS18B20: c_setup exit");
}
