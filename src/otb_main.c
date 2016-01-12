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
bool ds18b20Firsttime = TRUE;
uint8_t disconnectedCounter = 0;

//void ICACHE_FLASH_ATTR c_loop(void)
void ICACHE_FLASH_ATTR ds18b20Callback(os_event_t event)
{
  float temp_f;
  int temp_i;
  char temp_s[8];
  int chars;
  bool repeat=TRUE;

  //LOG("DS18B20: callback entry");
  while (repeat)
  {
    repeat = FALSE;
    request_temps(sensors_handle);
    temp_f = get_temp(sensors_handle, 0);
    temp_i = (int)temp_f;
    
    if ((temp_i == 85) && (ds18b20Firsttime))
    {
      // Try one more time if the first time and got 85 degrees (uninitialized)
      ds18b20Firsttime = FALSE;
      repeat = TRUE;
      delay(1000);
    }
  } 
  ds18b20Firsttime = FALSE;


  LOG("DS18B20: temp: %d", temp_i);

  if (mqttClient.connState == MQTT_DATA)
  {
    // Could send (but may choose not to), so reset disconnectedCounter
    disconnectedCounter = 0;

    // Need to send MQTT message
    // XXX
    chars = sprintf(temp_s, "%d", temp_i);
    
    if ((temp_i > -127) && (temp_i < 85))
    {
      // qos = 0, retain = 1
      sprintf(topic_s,
	      "/%s/%s/%s/%s/%s",
	      OTB_ROOT,
	      LOCATION_1,
	      LOCATION_2,
	      LOCATION_3,
	      TEMPERATURE);
      MQTT_Publish(&mqttClient, topic_s, temp_s, chars, 0, 1);
      sprintf(topic_s,
	      "/%s/%s/%s/%s/%s/%s",
	      OTB_ROOT,
	      LOCATION_1,
	      LOCATION_2,
	      LOCATION_3,
	      LOCATION_4_OPT,
	      TEMPERATURE);
      MQTT_Publish(&mqttClient, topic_s, temp_s, chars, 0, 1);
      sprintf(topic_s,
	      "/%s/%s/%s/%s/%s/%s",
	      OTB_ROOT,
	      LOCATION_1,
	      LOCATION_2,
	      LOCATION_3,
	      OTB_CHIPID,
	      TEMPERATURE);
      MQTT_Publish(&mqttClient, topic_s, temp_s, chars, 0, 1);
      sprintf(topic_s,
	      "/%s/%s/%s/%s/%s/%s/%s",
	      OTB_ROOT,
	      LOCATION_1,
	      LOCATION_2,
	      LOCATION_3,
	      OTB_CHIPID,
	      LOCATION_4_OPT,
	      TEMPERATURE);
      MQTT_Publish(&mqttClient, topic_s, temp_s, chars, 0, 1);
      sprintf(topic_s,
	      "/%s/%s/%s/%s/%s/%s/%s",
	      OTB_ROOT,
	      LOCATION_1,
	      LOCATION_2,
	      LOCATION_3,
	      LOCATION_4_OPT,
	      OTB_CHIPID,
	      TEMPERATURE);
      MQTT_Publish(&mqttClient, topic_s, temp_s, chars, 0, 1);
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
  oneWire_handle = (void *)0;
  sensors_handle = (void *)0;

  LOG("MQTT: initialize mqtt ...");
  mqtt_init(MQTT_SERVER, 1880, 0, "otb_iot", "user", "pass", 120);
  delay(1000);

  LOG("DS18B20: Initialize one wire bus (GPIO pin %d) ...", ONE_WIRE_BUS);
  initialize_temp(ONE_WIRE_BUS, &oneWire_handle, &sensors_handle);
  LOG("\r\nDS18B20: One Wire bus initialized");
  vTask = getTask(2);
  // Interval between being scheduled, and iterations (-1 = forever)
  repeatTask(vTask, REPORT_INTERVAL, -1);
  fake_system_os_post(0, 0, 0, &ds18b20Callback, vTask, 0);
  //system_os_post(0, 0, 0);
  //delay(1000);


  LOG("DS18B20: c_setup exit");
}
