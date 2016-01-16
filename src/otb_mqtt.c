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

#include "otb.h"

MQTT_Client otb_mqtt_client;

char otb_mqtt_topic_s[OTB_MQTT_MAX_TOPIC_LENGTH];
char otb_mqtt_msg_s[OTB_MQTT_MAX_MSG_LENGTH];

void ICACHE_FLASH_ATTR otb_mqtt_publish(MQTT_Client *mqtt_client,
                                        char *subtopic,
                                        char *extra_subtopic,
                                        char *message,
                                        char *extra_message,
                                        uint8_t qos,
                                        bool retain)
{
  int chars;

  DEBUG("MQTT: otb_mqtt_publish entry");

  if (extra_message[0] == 0)
  {
    chars = snprintf(otb_mqtt_msg_s, OTB_MQTT_MAX_MSG_LENGTH, "%s", message);
  }
  else
  {
    chars = snprintf(otb_mqtt_msg_s,
                     OTB_MQTT_MAX_MSG_LENGTH,
                     "%s/%s",
                     message,
                     extra_message);
  }
  
  if (extra_subtopic[0] == 0)
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_LOCATION_1,
             OTB_MQTT_LOCATION_2,
             OTB_MQTT_LOCATION_3,
             OTB_MAIN_CHIPID,
             subtopic);
  }
  else
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_LOCATION_1,
             OTB_MQTT_LOCATION_2,
             OTB_MQTT_LOCATION_3,
             OTB_MAIN_CHIPID,
             subtopic,
             extra_subtopic);
  }
  
  DEBUG("MQTT: Publish topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:       message: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:           qos: %d retain: %d", qos, retain);
  MQTT_Publish(mqtt_client, otb_mqtt_topic_s, otb_mqtt_msg_s, chars, qos, retain);

  DEBUG("MQTT: otb_mqtt_publish exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_subscribe(MQTT_Client *mqtt_client,
                                          char *subtopic,
                                          char *extra_subtopic,
                                          uint8_t qos)
{

  DEBUG("MQTT: otb_mqtt_subscribe entry");

  if (extra_subtopic[0] == 0)
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_LOCATION_1,
             OTB_MQTT_LOCATION_2,
             OTB_MQTT_LOCATION_3,
             OTB_MAIN_CHIPID,
             subtopic);
  }
  else
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_LOCATION_1,
             OTB_MQTT_LOCATION_2,
             OTB_MQTT_LOCATION_3,
             OTB_MAIN_CHIPID,
             subtopic,
             extra_subtopic);
  }
  
  DEBUG("MQTT: Subscribe topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:             qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);

  // Also subscribe to system commands for ALL devices at this location
  if (extra_subtopic[0] == 0)
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_LOCATION_1,
             OTB_MQTT_LOCATION_2,
             OTB_MQTT_LOCATION_3,
             OTB_MQTT_ALL,
             subtopic);
  }
  else
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_LOCATION_1,
             OTB_MQTT_LOCATION_2,
             OTB_MQTT_LOCATION_3,
             OTB_MQTT_ALL,
             subtopic,
             extra_subtopic);
  }
  
  DEBUG("MQTT: Subscribe topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:             qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);

  // Also subscribe to system commands for ALL devices at this location
  if (extra_subtopic[0] == 0)
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_ALL,
             subtopic);
  }
  else
  {
    snprintf(otb_mqtt_topic_s,
             OTB_MQTT_MAX_TOPIC_LENGTH,
             "/%s/%s/%s/%s",
             OTB_MQTT_ROOT,
             OTB_MQTT_ALL,
             subtopic,
             extra_subtopic);
  }
  
  DEBUG("MQTT: Subscribe topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:             qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);
  
  DEBUG("MQTT: otb_mqtt_subscribe exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_on_connected(uint32_t *client)
{
  int chars;
	MQTT_Client* mqtt_client;
	
  DEBUG("MQTT: otb_mqtt_on_connected entry");

  mqtt_client = (MQTT_Client*)client;

  // Now publish status.  First off booted.  Don't need to retain this.
  otb_mqtt_publish(mqtt_client,
                   OTB_MQTT_PUB_STATUS,
                   OTB_MQTT_STATUS_BOOTED,
                   "",
                   "",
                   2,
                   0);
                   
  // Now version ID.  Need to retain this.                 
  otb_mqtt_publish(mqtt_client,
                   OTB_MQTT_PUB_STATUS,
                   OTB_MQTT_STATUS_VERSION,
                   OTB_MAIN_VERSION_ID,
                   "",
                   2,
                   1);

  // Now Chip ID.  Need to retain this.                 
  otb_mqtt_publish(mqtt_client,
                   OTB_MQTT_PUB_STATUS,
                   OTB_MQTT_STATUS_CHIPID,
                   OTB_MAIN_CHIPID,
                   "",
                   2,
                   1);

  // Now subscribe for system topic, qos = 1 to ensure we get at least 1 of every command
  otb_mqtt_subscribe(mqtt_client,
                     OTB_MQTT_CMD_SYSTEM,
                     "",
                     1);

  DEBUG("MQTT: otb_mqtt_on_connected exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_on_disconnected(uint32_t *client)
{
	MQTT_Client* mqtt_client;
	
	DEBUG("MQTT: otb_mqtt_on_disconnected entry");
	
  mqtt_client = (MQTT_Client*)client;
	
	DEBUG("MQTT: otb_mqtt_on_disconnected exit");
}

void ICACHE_FLASH_ATTR otb_mqtt_on_published(uint32_t *client)
{
	MQTT_Client* mqtt_client;
	
	DEBUG("MQTT: otb_mqtt_on_published entry");
	
  mqtt_client = (MQTT_Client*)client;
	
	DEBUG("MQTT: otb_mqtt_on_published exit");
}

void ICACHE_FLASH_ATTR otb_mqtt_on_receive_publish(uint32_t *client,
                                                   const char* topic,
                                                   uint32_t topic_len,
                                                   const char *msg,
                                                   uint32_t msg_len)
{
  MQTT_Client* mqtt_client = (MQTT_Client*)client;

  DEBUG("MQTT: otb_mqtt_on_receive_publish entry");

  // Check can actually handle received topic and msg  
  if (topic_len > (OTB_MQTT_MAX_TOPIC_LENGTH - 1))
  {
    WARN("MQTT: Received topic length too long: %d", topic_len);
    topic_len = OTB_MQTT_MAX_TOPIC_LENGTH - 1;
  }
  if (msg_len > (OTB_MQTT_MAX_MSG_LENGTH - 1))
  {
    WARN("MQTT: Received msg length too long: %d", msg_len);
    msg_len = OTB_MQTT_MAX_MSG_LENGTH - 1;
  }

  // Copy received topic and msg into our string buffers
  os_memcpy(otb_mqtt_topic_s, topic, topic_len);
  otb_mqtt_topic_s[topic_len] = 0;
  os_memcpy(otb_mqtt_msg_s, msg, msg_len);
  otb_mqtt_msg_s[msg_len] = 0;

  INFO("MQTT: Received publish topic: %s", otb_mqtt_topic_s);
  INFO("MQTT:                message: %s ", otb_mqtt_msg_s);

  // Don't bother parsing topic - we know what we subscribed to (system)

  if ((!strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_RESET)) ||
      (!strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_REBOOT)))
  {
    INFO("MQTT: system command Reset/Reboot");
    otb_reset();
  }
  else if (!strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_UPGRADE))
  {
    INFO("MQTT: system command upgrade");
    // XXX To be implemented
  }
  else
  {
    INFO("MQTT: Unknown system command: %s", otb_mqtt_msg_s);
  }
  
  DEBUG("MQTT: otb_mqtt_on_receive_publish exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_initialize(char *hostname,
                                           int port,
                                           int security,
                                           char *device_id,
                                           char *mqtt_username,
                                           char *mqtt_password,
                                           uint16_t keepalive)
{
  MQTT_Client *mqtt_client;

	DEBUG("MQTT: otb_mqtt_initialize entry");

  // Initialize library
  mqtt_client = &(otb_mqtt_client);
	MQTT_InitConnection(mqtt_client, hostname, port, 0);
	MQTT_InitClient(mqtt_client, device_id, mqtt_username, mqtt_password, keepalive, TRUE);
	
  // Set up LWT (last will and testament)
  sprintf(otb_mqtt_topic_s,
          "/lwt/%s/%s/%s/%s/%s/%s",
          OTB_MQTT_ROOT,
          OTB_MQTT_LOCATION_1,
          OTB_MQTT_LOCATION_2,
          OTB_MQTT_LOCATION_3,
          OTB_MAIN_CHIPID,
          OTB_MQTT_PUB_STATUS);
	MQTT_InitLWT(mqtt_client, otb_mqtt_topic_s, "offline", 0, 0);
	
	// Set up callbacks
	MQTT_OnConnected(mqtt_client, otb_mqtt_on_connected);
	MQTT_OnDisconnected(mqtt_client, otb_mqtt_on_disconnected);
	MQTT_OnPublished(mqtt_client, otb_mqtt_on_published);
	MQTT_OnData(mqtt_client, otb_mqtt_on_receive_publish);
	
	// Connect to server
	MQTT_Connect(mqtt_client);

	DEBUG("MQTT: otb_mqtt_initialize exit");

}
