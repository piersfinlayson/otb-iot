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
  char *loc1, *loc2, *loc3, *loc1_, *loc2_, *loc3_;
  int chars;

  DEBUG("MQTT: otb_mqtt_publish entry");

  otb_mqtt_handle_loc(&loc1, &loc1_, &loc2, &loc2_, &loc3, &loc3_);
  
  if (extra_message[0] == 0)
  {
    chars = os_snprintf(otb_mqtt_msg_s, OTB_MQTT_MAX_MSG_LENGTH, "%s", message);
  }
  else
  {
    chars = os_snprintf(otb_mqtt_msg_s,
                        OTB_MQTT_MAX_MSG_LENGTH,
                        "%s/%s",
                        message,
                        extra_message);
  }
  
  if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s",
                OTB_MQTT_ROOT,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MAIN_CHIPID,
                subtopic);
  }
  else
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s/%s",
                OTB_MQTT_ROOT,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MAIN_CHIPID,
                subtopic,
                extra_subtopic);
  }
  
  INFO("MQTT: Publish topic: %s", otb_mqtt_topic_s);
  INFO("MQTT:       message: %s", otb_mqtt_msg_s);
  INFO("MQTT:           qos: %d retain: %d", qos, retain);
  MQTT_Publish(mqtt_client, otb_mqtt_topic_s, otb_mqtt_msg_s, chars, qos, retain);

  DEBUG("MQTT: otb_mqtt_publish exit");

  return;
}

// Need to statically assign, rather than from stack
char otb_mqtt_string_empty[] = "";
char otb_mqtt_string_slash[] = "/";
void ICACHE_FLASH_ATTR otb_mqtt_handle_loc(char **loc1,
                                           char **loc1_,
                                           char **loc2,
                                           char **loc2_,
                                           char **loc3,
                                           char **loc3_)
{

  DEBUG("MQTT: otb_mqtt_handle_loc entry");
  
  // Initialize
  *loc1 = otb_mqtt_string_empty;
  *loc1_ = otb_mqtt_string_empty;
  *loc2 = otb_mqtt_string_empty;
  *loc2_ = otb_mqtt_string_empty;
  *loc3 = otb_mqtt_string_empty;
  *loc3_ = otb_mqtt_string_empty;

  // Just omit a location if empty
  if (OTB_MQTT_LOCATION_1[0] != 0)
  {
    *loc1_ = otb_mqtt_string_slash;
    *loc1 = OTB_MQTT_LOCATION_1;
  }
  if (OTB_MQTT_LOCATION_2[0] != 0)
  {
    *loc2_ = otb_mqtt_string_slash;
    *loc2 = OTB_MQTT_LOCATION_2;
  }
  if (OTB_MQTT_LOCATION_3[0] != 0)
  {
    *loc3_ = otb_mqtt_string_slash;
    *loc3 = OTB_MQTT_LOCATION_3;
  }

  DEBUG("MQTT: otb_mqtt_handle_loc exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_subscribe(MQTT_Client *mqtt_client,
                                          char *subtopic,
                                          char *extra_subtopic,
                                          uint8_t qos)
{
  char *loc1, *loc2, *loc3, *loc1_, *loc2_, *loc3_;

  DEBUG("MQTT: otb_mqtt_subscribe entry");

  otb_mqtt_handle_loc(&loc1, &loc1_, &loc2, &loc2_, &loc3, &loc3_);
  
  if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s",
                OTB_MQTT_ROOT,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MAIN_CHIPID,
                subtopic);
  }
  else
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s/%s",
                OTB_MQTT_ROOT,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MAIN_CHIPID,
                subtopic,
                extra_subtopic);
  }
  
  INFO("MQTT: Subscribe topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:             qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);

  // Also subscribe to system commands for ALL devices at this location
  if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s",
                OTB_MQTT_ROOT,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MQTT_ALL,
                subtopic);
  }
  else
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s/%s",
                OTB_MQTT_ROOT,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MQTT_ALL,
                subtopic,
                extra_subtopic);
  }
  
  INFO("MQTT: Subscribe topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:             qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);

  // Also subscribe to system commands for ALL devices at this location
  if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s",
                OTB_MQTT_ROOT,
                OTB_MQTT_ALL,
                subtopic);
  }
  else
  {
    os_snprintf(otb_mqtt_topic_s,
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

  // Now subscribe for gpio topic, qos = 2 to ensure we get at only 1 of every command
  otb_mqtt_subscribe(mqtt_client,
                     OTB_MQTT_CMD_GPIO,
                     "",
                     2);

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

char ALIGN4 otb_mqtt_reset_error_string[] = "MQTT: System command reset/reboot";
void ICACHE_FLASH_ATTR otb_mqtt_on_receive_publish(uint32_t *client,
                                                   const char* topic,
                                                   uint32_t topic_len,
                                                   const char *msg,
                                                   uint32_t msg_len)
{
  MQTT_Client* mqtt_client = (MQTT_Client*)client;
  char *sub_topic = NULL;
  char *next_sub_topic = NULL;
  char *sub_msg = NULL;
  int no1;
  int no2;
  bool rc;

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

  DEBUG("MQTT: Received publish topic: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:                message: %s ", otb_mqtt_msg_s);
 
  // Find topic
  next_sub_topic = os_strstr(otb_mqtt_topic_s, "/");
  while (next_sub_topic != NULL)
  {
    sub_topic = next_sub_topic;
    next_sub_topic++;
    next_sub_topic = os_strstr(next_sub_topic, "/");
  }
  
  if (sub_topic != NULL)
  {
    // This ++ should be safe - as its a string worse case scenario is this takes us to a
    // null terminator
    sub_topic++;
  
    if (!strcmp(sub_topic, OTB_MQTT_CMD_SYSTEM))
    {
      INFO("MQTT: system command %s", otb_mqtt_msg_s);
      if ((!strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_RESET)) ||
          (!strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_REBOOT)))
      {
        otb_reset(otb_mqtt_reset_error_string);
      }
      else if (!memcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_UPDATE, strlen(OTB_MQTT_CMD_UPDATE)-1))
      {
        // Call upgrage function with pointer to end of update and one char for the colon!
        otb_rboot_update(otb_mqtt_msg_s + strlen(OTB_MQTT_CMD_UPDATE));
      }
      else if (!memcmp(otb_mqtt_msg_s, "set_boot_slot", 13))
      {
        otb_rboot_update_slot(otb_mqtt_msg_s);
      }
      else if (!memcmp(otb_mqtt_msg_s, "get_boot_slot", 13))
      {
        otb_rboot_get_slot(TRUE);
      }
      else if (!strncmp(otb_mqtt_msg_s, OTB_MQTT_CMD_HEAP, strlen(OTB_MQTT_CMD_HEAP)))
      {
        otb_util_get_heap_size();
      }
      else if (!strncmp(otb_mqtt_msg_s,
                        OTB_MQTT_CMD_AP_ENABLE,
                        strlen(OTB_MQTT_CMD_AP_ENABLE)))
      {
        otb_wifi_ap_enable();
      }
      else if (!strncmp(otb_mqtt_msg_s,
                        OTB_MQTT_CMD_AP_DISABLE,
                        strlen(OTB_MQTT_CMD_AP_DISABLE)))
      {
        otb_wifi_ap_disable();
      }
      else if ((!strncmp(otb_mqtt_msg_s, "loc1", 4)) ||
               (!strncmp(otb_mqtt_msg_s, "loc2", 4)) ||
               (!strncmp(otb_mqtt_msg_s, "loc3", 4)))
      {
        otb_conf_update_loc(otb_mqtt_msg_s);
      }
      else if (!os_strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_WIFI_STRENGTH))
      {
        otb_wifi_mqtt_do_rssi(otb_mqtt_msg_s);
      }
      else if (!os_strcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_PING))
      {
        otb_mqtt_report_status(OTB_MQTT_STATUS_PONG, "");
      }
      else
      {
        INFO("ERROR: Unknown system command: %s", otb_mqtt_msg_s);
      }
    }
    else if (!strcmp(sub_topic, OTB_MQTT_CMD_GPIO))
    {
      INFO("MQTT: GPIO topic received");
      if (!memcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_GPIO_GET, strlen(OTB_MQTT_CMD_GPIO_GET)))
      {
        INFO("MQTT: Get msg received");
        sub_msg = os_strstr(otb_mqtt_msg_s, ":");
        if (sub_msg)
        {
          sub_msg++;
          no1 = atoi(sub_msg);
          otb_gpio_get(no1);
        }
        else
        {
          ERROR("MQTT: Missing pin number");
        }
      }
      else if (!memcmp(otb_mqtt_msg_s, OTB_MQTT_CMD_GPIO_SET, strlen(OTB_MQTT_CMD_GPIO_SET)))
      {
        INFO("MQTT: Set msg received");
        rc = FALSE;
        sub_msg = os_strstr(otb_mqtt_msg_s, ":");
        if (sub_msg)
        {
          sub_msg++;
          no1 = atoi(sub_msg);
          sub_msg = os_strstr(sub_msg, ":");
          if (sub_msg)
          {
            sub_msg++;
            no2 = atoi(sub_msg);
            otb_gpio_set(no1, no2); 
            rc = TRUE;
          }
        }
        if (!rc)
        {
          ERROR("MQTT: Missing pin number or value");
        }
      }
      else
      {
        INFO("MQTT: Unknown GPIO command: %s", otb_mqtt_msg_s);
      }
    }
    else
    {
      ERROR("MQTT: Unknown sub-topic: %s", sub_topic);
    }
  }
  else
  {
    ERROR("MQTT: Unknown topic: %s", topic);
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
  os_snprintf(otb_mqtt_topic_s,
              OTB_MQTT_MAX_TOPIC_LENGTH,
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

void ICACHE_FLASH_ATTR otb_mqtt_report_error(char *cmd, char *error)
{

  DEBUG("MQTT: otb_mqtt_report_error entry");
  
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_PUB_ERROR,
                   "",
                   cmd,
                   error,
                   2,
                   0);  

  DEBUG("MQTT: otb_mqtt_report_error exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_report_status(char *cmd, char *status)
{

  DEBUG("MQTT: otb_mqtt_report_status entry");
  
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_PUB_STATUS,
                   "",
                   cmd,
                   status,
                   2,
                   0);  

  DEBUG("MQTT: otb_mqtt_report_status exit");

  return;
}