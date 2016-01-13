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

/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <Arduino.h>
#include "ets_sys.h"
//#include "driver/uart.h"
#include "osapi.h"
#include "lwip/ip.h"
#include "mqtt.h"
//#include "wifi.h"
//#include "config.h"
#include "debug.h"
//#include "gpio.h"
#include "mqtt_user_config.h"
#include "mem.h"
#include "otb.h"

MQTT_Client mqttClient;

#if 0
void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}
#endif 

char topic_s[MAX_TOPIC_LENGTH];
char msg_s[MAX_MSG_LENGTH];

void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected");

        // Register for all OTB IOT commands

        // Register for OTB_SYSTEM TOPIC
        int chars;
        chars = sprintf(msg_s, "%s/%s", BOOTED_STATUS, OTB_VERSION_ID);
        sprintf(topic_s, "/%s/%s/%s/%s/%s", OTB_ROOT, LOCATION_1, LOCATION_2, LOCATION_3, SYSTEM_CMD);
        MQTT_Subscribe(client, topic_s, 1); // 1 = receive >= 1 times
        MQTT_Publish(client, topic_s, msg_s, chars, 0, 0); // qos=0, retain=0
        sprintf(topic_s, "/%s/%s/%s/%s/%s/%s", OTB_ROOT, LOCATION_1, LOCATION_2, LOCATION_3, OTB_CHIPID, SYSTEM_CMD);
        MQTT_Subscribe(client, topic_s, 1); // 1 = receive >= 1 times
        MQTT_Publish(client, topic_s, msg_s, chars, 0, 0); // qos=0, retain=0
        sprintf(topic_s, "/%s/%s/%s/%s/%s/%s", OTB_ROOT, LOCATION_1, LOCATION_2, LOCATION_3, LOCATION_4_OPT, SYSTEM_CMD);
        MQTT_Subscribe(client, topic_s, 1); // 1 = receive >= 1 times
        MQTT_Publish(client, topic_s, msg_s, chars, 0, 0); // qos=0, retain=0
        sprintf(topic_s, "/%s/%s/%s/%s/%s/%s/%s", OTB_ROOT, LOCATION_1, LOCATION_2, LOCATION_3, OTB_CHIPID, LOCATION_4_OPT, SYSTEM_CMD);
        MQTT_Subscribe(client, topic_s, 1); // 1 = receive >= 1 times
        MQTT_Publish(client, topic_s, msg_s, chars, 0, 0); // qos=0, retain=0
        sprintf(topic_s, "/%s/%s/%s/%s/%s/%s/%s", OTB_ROOT, LOCATION_1, LOCATION_2, LOCATION_3, LOCATION_4_OPT, OTB_CHIPID, SYSTEM_CMD);
        MQTT_Subscribe(client, topic_s, 1); // 1 = receive >= 1 times
        MQTT_Publish(client, topic_s, msg_s, chars, 0, 0); // qos=0, retain=0
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published");
}

char topicBuf_s[MAX_TOPIC_LENGTH];
char msgBuf_s[MAX_MSG_LENGTH];
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
        char *topicBuf = (char *)&topicBuf_s;
        char *dataBuf = (char *)&msgBuf_s;

        MQTT_Client* client = (MQTT_Client*)args;

        if (topic_len > (MAX_TOPIC_LENGTH - 1))
        {
          INFO("MQTT: Error, received topic length too long: %d", topic_len);
          topic_len = MAX_TOPIC_LENGTH - 1;
        }
        if (data_len > (MAX_MSG_LENGTH - 1))
        {
          INFO("MQTT: Error, received msg length too long: %d", data_len);
          data_len = MAX_MSG_LENGTH - 1;
        }
        os_memcpy(topicBuf, topic, topic_len);
        topicBuf[topic_len] = 0;

        os_memcpy(dataBuf, data, data_len);
        dataBuf[data_len] = 0;

        INFO("Receive topic: %s, data: %s ", topicBuf, dataBuf);

  char *topicf;
#if 0
  // No need to parse topic - assume it's what we subscribed too
  char *topicf_tmp = NULL;
  char *topic_tmp = (char *)topicBuf;
  char *saveptr;
  do
  {
    // find the last topic string (NULL if none)
    topicf = topicf_tmp;
    topicf_tmp = strtok_r(topic_tmp, "/", &saveptr);
    topic_tmp = NULL;
  } while (topicf_tmp != NULL);
#else
  topicf = dataBuf;
#endif
  if (!topicf)
  {
    LOG("SYSTEM: No topic");
  }
  else if ((!strcmp(topicf, RESET_CMD)) || (!strcmp(topicf, REBOOT_CMD)))
  {
    LOG("SYSTEM: Reset/Reboot");
    reset();
  }
  else if (!strcmp(topicf, UPGRADE_CMD))
  {
    LOG("SYSTEM: Upgrade");
    // XXX To be implemented
  }
  else
  {
    LOG("SYSTEM: %s", dataBuf);
  }
}

void mqtt_init(char *host, int port, int security, char *device_id, char *mqtt_username, char *mqtt_password, int keepalive)
{
	LOG("mqtt_init entry");
	MQTT_InitConnection(&mqttClient, host, port, 0);
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

	MQTT_InitClient(&mqttClient, device_id, mqtt_username, mqtt_password, keepalive, 1);
	//MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);

        sprintf(topic_s, "/lwt/%s/%s/%s/%s/%s/%s", OTB_ROOT, LOCATION_1, LOCATION_2, LOCATION_3, OTB_CHIPID, LOCATION_4_OPT);
	MQTT_InitLWT(&mqttClient, topic_s, "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
	LOG("MQTT: connect to mqtt server %s", MQTT_SERVER);
	MQTT_Connect(&mqttClient);
	LOG("MQTT: done");
	//delay(1000);


	//WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);	
	LOG("mqtt_init exit");

}
