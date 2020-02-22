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

#define OTB_MQTT_C
#include "otb.h"

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
  int ii;

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
                        "%s:%s",
                        message,
                        extra_message);
  }
  otb_util_convert_char_to_char(otb_mqtt_msg_s, ' ', '_');
  for (ii = 0; ii < os_strlen(otb_mqtt_msg_s); ii++)
  {
    otb_mqtt_msg_s[ii] = tolower(otb_mqtt_msg_s[ii]);
  }
  
  if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s",
                otb_mqtt_root,
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
                otb_mqtt_root,
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
  
  // We don't "INFO" this as can be retrieved from MQTT broker
  DEBUG("MQTT: Publish: %s %s qos: %d retain: %d",
       otb_mqtt_topic_s,
       otb_mqtt_msg_s,
       qos,
       retain);
  MQTT_Publish(mqtt_client, otb_mqtt_topic_s, otb_mqtt_msg_s, chars, qos, retain);

  DEBUG("MQTT: otb_mqtt_publish exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_handle_loc(char **loc1,
                                           char **loc1_,
                                           char **loc2,
                                           char **loc2_,
                                           char **loc3,
                                           char **loc3_)
{

  DEBUG("MQTT: otb_mqtt_handle_loc entry");
  
  // Initialize
  *loc1 = OTB_MQTT_EMPTY;
  *loc1_ = OTB_MQTT_EMPTY;
  *loc2 = OTB_MQTT_EMPTY;
  *loc2_ = OTB_MQTT_EMPTY;
  *loc3 = OTB_MQTT_EMPTY;
  *loc3_ = OTB_MQTT_EMPTY;

  // Just omit a location if empty
  if (OTB_MQTT_LOCATION_1[0] != 0)
  {
    *loc1_ = OTB_MQTT_SLASH;
    *loc1 = OTB_MQTT_LOCATION_1;
  }
  if (OTB_MQTT_LOCATION_2[0] != 0)
  {
    *loc2_ = OTB_MQTT_SLASH;
    *loc2 = OTB_MQTT_LOCATION_2;
  }
  if (OTB_MQTT_LOCATION_3[0] != 0)
  {
    *loc3_ = OTB_MQTT_SLASH;
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

  if (subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s",
                otb_mqtt_root,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MAIN_CHIPID);
  }
  else if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s",
                otb_mqtt_root,
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
                otb_mqtt_root,
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
  
  DETAIL("MQTT: Subscribe: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:       qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);

  // Also subscribe to system commands for ALL devices at this location
  if (subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s",
                otb_mqtt_root,
                loc1_,
                loc1,
                loc2_,
                loc2,
                loc3_,
                loc3,
                OTB_MQTT_ALL);
  }
  else if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s%s%s%s%s%s%s/%s/%s",
                otb_mqtt_root,
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
                otb_mqtt_root,
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
  
  DETAIL("MQTT: Subscribe: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:       qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);

  // Also subscribe to system commands for ALL devices at this location
  if (subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s",
                otb_mqtt_root,
                OTB_MQTT_ALL);
  }
  else if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s",
                otb_mqtt_root,
                OTB_MQTT_ALL,
                subtopic);
  }
  else
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s/%s",
                otb_mqtt_root,
                OTB_MQTT_ALL,
                subtopic,
                extra_subtopic);
  }
  
  DETAIL("MQTT: Subscribe: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:       qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);
  
  // Also subscribe to system commands for this devices which no location
  // Note if location isn't set this could cause us to sub twice for this topic.  Shrugs.
  if (subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s",
                otb_mqtt_root,
                OTB_MAIN_CHIPID);
  }
  else if (extra_subtopic[0] == 0)
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s",
                otb_mqtt_root,
                OTB_MAIN_CHIPID,
                subtopic);
  }
  else
  {
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s/%s",
                otb_mqtt_root,
                OTB_MAIN_CHIPID,
                subtopic,
                extra_subtopic);
  }
  
  DETAIL("MQTT: Subscribe: %s", otb_mqtt_topic_s);
  DEBUG("MQTT:       qos: %d", qos);
  MQTT_Subscribe(mqtt_client, otb_mqtt_topic_s, qos);
  
  DEBUG("MQTT: otb_mqtt_subscribe exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_on_connected(uint32_t *client)
{
  int chars;
  uint8_t slot;
  char slot_s[4];
	MQTT_Client* mqtt_client;
	
  DEBUG("MQTT: otb_mqtt_on_connected entry");
  
  INFO("OTB: MQTT connected");

  otb_mqtt_connected = TRUE;
  otb_util_timer_cancel(&otb_mqtt_connected_timer);

  mqtt_client = (MQTT_Client*)client;

  // Now publish status.  First off booted.  Don't need to retain this.

  slot = rboot_get_current_rom();  
  os_snprintf(slot_s, 4, "%d", slot);
  otb_mqtt_send_status(OTB_MQTT_STATUS_BOOTED, "slot", slot_s, "");
  
  otb_led_wifi_update(OTB_LED_NEO_COLOUR_GREEN, TRUE);
  otb_led_wifi_blink(OTB_MQTT_LED_WIFI_BLINK_TIMES_ON_CONNECTED);
  
                   
  // Now subscribe for system topic, qos = 1 to ensure we get at least 1 of every command
  otb_mqtt_subscribe(mqtt_client,
                     "",
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
  otb_mqtt_connected = FALSE;
	
	DEBUG("MQTT: otb_mqtt_on_disconnected exit");
}

void ICACHE_FLASH_ATTR otb_mqtt_on_published(uint32_t *client)
{
	MQTT_Client* mqtt_client;
	
	DEBUG("MQTT: otb_mqtt_on_published entry");
	
  mqtt_client = (MQTT_Client*)client;
  otb_led_wifi_blink(OTB_MQTT_LED_WIFI_BLINK_TIMES_ON_PUBLISHED);
	
	DEBUG("MQTT: otb_mqtt_on_published exit");
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
  char *loc1, *loc2, *loc3, *loc1_, *loc2_, *loc3_;
  int chars;
  int ii;

  DEBUG("MQTT: otb_mqtt_initialize entry");

  INFO("OTB: MQTT connecting");
  DETAIL("MQTT: Create MQTT connection to server %s port:%d username:%s", hostname, port, mqtt_username);

  otb_mqtt_connected = FALSE;
  
  otb_util_timer_set((os_timer_t *)&otb_mqtt_connected_timer,
                     (os_timer_func_t *)otb_mqtt_connected_timerfunc,
                     NULL,
                     OTB_MQTT_INITIAL_CONNECT_TIMER,
                     0);

  // Initialize library
  mqtt_client = &(otb_mqtt_client);
	MQTT_InitConnection(mqtt_client, hostname, port, 0);
	MQTT_InitClient(mqtt_client, device_id, mqtt_username, mqtt_password, keepalive, TRUE);
	
  // Set up LWT (last will and testament)
  otb_mqtt_handle_loc(&loc1, &loc1_, &loc2, &loc2_, &loc3, &loc3_);
  os_snprintf(otb_mqtt_topic_s,
              OTB_MQTT_MAX_TOPIC_LENGTH,
              "/%s%s%s%s%s%s%s/%s/%s",
              otb_mqtt_root,
              loc1,
              loc1_,
              loc2,
              loc2_,
              loc3,
              loc3_,
              OTB_MAIN_CHIPID,
              OTB_MQTT_TOPIC_STATUS);
	MQTT_InitLWT(mqtt_client, otb_mqtt_topic_s, "offline", 0, 0);
	
	// Set up callbacks
	MQTT_OnConnected(mqtt_client, otb_mqtt_on_connected);
	MQTT_OnDisconnected(mqtt_client, otb_mqtt_on_disconnected);
	MQTT_OnPublished(mqtt_client, otb_mqtt_on_published);
	//MQTT_OnData(mqtt_client, otb_mqtt_on_receive_publish);
	MQTT_OnData(mqtt_client, otb_cmd_mqtt_receive);
	
	// Connect to server
	MQTT_Connect(mqtt_client);

	DEBUG("MQTT: otb_mqtt_initialize exit");

}

void ICACHE_FLASH_ATTR otb_mqtt_connected_timerfunc(void *arg)
{
  
  DEBUG("MQTT: otb_mqtt_connected_timerfunc entry");
  
  if (otb_mqtt_connected)
  {
    // Phew - window condition, nothing to do.
    goto EXIT_LABEL;
  }

  INFO("MQTT: Failed to connect, so starting AP to allow manual recovery");
  // If can't connect over MQTT better kick off the wireless AP
  if (!otb_wifi_ap_running)
  {
    otb_wifi_ap_quick_start();
  }
  otb_mqtt_wifi_timeout_set_timer();
  
  DEBUG("MQTT: otb_mqtt_connected_timerfunc exit");

EXIT_LABEL:

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_wifi_timeout_set_timer(void)
{

  DEBUG("WIFI: otb_wifi_set_timeout_timer entry");

  // No args (NULL arg) and don't repeat this (the 0 arg)  
  otb_util_timer_set((os_timer_t*)&otb_mqtt_wifi_timeout_timer,
                     otb_mqtt_wifi_timeout_timerfunc,
                     NULL,
                     OTB_WIFI_DEFAULT_DISCONNECTED_TIMEOUT, // reuse wifi timer (5 mins)
                     0);
  
  DEBUG("WIFI: otb_wifi_set_timeout_timer exit");
  
  return;
}

char ALIGN4 otb_mqtt_wifi_timeout_timerfunc_string[] = "MQTT: Failed to connect timeout";
void ICACHE_FLASH_ATTR otb_mqtt_wifi_timeout_timerfunc(void *arg)
{

  DEBUG("WIFI: otb_wifi_timeout_timerfunc entry");
  
  if (!otb_mqtt_connected)
  {
    otb_reset(otb_mqtt_wifi_timeout_timerfunc_string);
  }
  
  DEBUG("WIFI: otb_wifi_timeout_timerfunc exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_report_error(char *cmd, char *error)
{

  DEBUG("MQTT: otb_mqtt_report_error entry");
  
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_TOPIC_ERROR,
                   "",
                   cmd,
                   error,
                   2,
                   0);  

  DEBUG("MQTT: otb_mqtt_report_error exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mqtt_send_status(char *val1,
                                            char *val2,
                                            char *val3,
                                            char *val4)
{
  char *new_val2 = OTB_MQTT_EMPTY;
  int max_len;
  int len = 0;

  DEBUG("MQTT: otb_mqtt_send_status entry");
  
  if ((val2 != NULL) && (os_strlen(val2) > 0))
  {
    otb_util_convert_char_to_char(val2, ' ', '_');
    otb_util_convert_char_to_char(val2, ':', '.');
    len = os_snprintf(otb_mqtt_scratch,
                      max_len,
                      "%s",
                      val2);
    max_len = OTB_MQTT_MAX_MSG_LENGTH - len;
    if ((val3 != NULL) && (os_strlen(val3) > 0))
    {
      otb_util_convert_char_to_char(val3, ' ', '_');
      otb_util_convert_char_to_char(val3, ':', '.');
      len += os_snprintf(otb_mqtt_scratch + len,
                         max_len,
                         ":%s",
                         val3);
      max_len = OTB_MQTT_MAX_MSG_LENGTH - len;
      if ((val4 != NULL) && (os_strlen(val4) > 0))
      {
        otb_util_convert_char_to_char(val4, ' ', '_');
        otb_util_convert_char_to_char(val4, ':', '.');
        len += os_snprintf(otb_mqtt_scratch + len,
                           max_len,
                           ":%s",
                           val4);
        max_len = OTB_MQTT_MAX_MSG_LENGTH - len;
      }
    }
    new_val2 = otb_mqtt_scratch;
  }
  
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_TOPIC_STATUS,
                   "",
                   val1,
                   new_val2,
                   2,
                   0);  

  DEBUG("MQTT: otb_mqtt_send_status exit");

  return;
}



// This function handles publishes to the OTB-IOT device.  So this implements the 
// handling of all support MQTT system commands
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
  char response[8];
  uint8 topic_id;
  uint8 command;
  char *sub_cmd[OTB_MQTT_MAX_CMDS-1];
  int ii;
  char *cmd;
  char *error;

  DEBUG("MQTT: otb_mqtt_on_receive_publish entry");

  // This function is currently written to expect 5 SUB commands (6 commands)
  OTB_ASSERT(OTB_MQTT_MAX_CMDS == 6);

  otb_led_wifi_blink(OTB_MQTT_LED_WIFI_BLINK_TIMES_ON_PUBLISH);

  // Check can actually handle received topic and msg  
  if ((topic_len > (OTB_MQTT_MAX_TOPIC_LENGTH - 1)) ||
      (msg_len > (OTB_MQTT_MAX_MSG_LENGTH - 1)))
  {
    DETAIL("MQTT: Received topic of msg length too long: %d", topic_len);
    otb_mqtt_send_status(OTB_MQTT_STATUS_ERROR,
                         "Topic or message too long",
                         "",
                         "");
    goto EXIT_LABEL;
  }

  // Copy received topic and msg into our string buffers
  os_memcpy(otb_mqtt_topic_s, topic, topic_len);
  otb_mqtt_topic_s[topic_len] = 0;
  os_memcpy(otb_mqtt_msg_s, msg, msg_len);
  otb_mqtt_msg_s[msg_len] = 0;
  for (ii = 0; ii < os_strlen(otb_mqtt_topic_s); ii++)
  {
    otb_mqtt_topic_s[ii] = tolower(otb_mqtt_topic_s[ii]);
  }
  for (ii = 0; ii < os_strlen(otb_mqtt_msg_s); ii++)
  {
    otb_mqtt_msg_s[ii] = tolower(otb_mqtt_msg_s[ii]);
  }

  DEBUG("MQTT: Received publish: %s %s", otb_mqtt_topic_s, otb_mqtt_msg_s);

  topic_id = otb_mqtt_pub_get_topic(otb_mqtt_topic_s);
  if (topic_id != OTB_MQTT_TOPIC_SYSTEM_)
  {
    DETAIL("MQTT: Received unsupported topic");
    otb_mqtt_send_status(OTB_MQTT_STATUS_ERROR, "Unsupported topic", "", "");
    goto EXIT_LABEL;
  }
  
  // Get sub commands
  command = otb_mqtt_pub_get_command(otb_mqtt_msg_s, sub_cmd);
  
  // Set up cmd variable
  cmd = os_strstr(otb_mqtt_msg_s, OTB_MQTT_COLON);
  if (cmd != NULL)
  {
    *cmd = 0;
  }
  cmd = otb_mqtt_msg_s;
  
  // Now do stuff based on command
  switch (command)
  {
    case OTB_MQTT_SYSTEM_CONFIG_:
      otb_conf_mqtt_conf(sub_cmd[0], sub_cmd[1], sub_cmd[2], sub_cmd[3], sub_cmd[4]);
      break;

    case OTB_MQTT_SYSTEM_GPIO_:
      otb_gpio_mqtt(sub_cmd[0], sub_cmd[1], sub_cmd[2]);
      break;

    case OTB_MQTT_SYSTEM_I2C_:
      otb_i2c_mqtt(cmd, sub_cmd);
      break;

    case OTB_MQTT_SYSTEM_ADS_:
      otb_i2c_ads_mqtt(cmd, sub_cmd);
      break;

    default:
      DEBUG("MQTT: Unknown command");
      otb_mqtt_send_status(OTB_MQTT_STATUS_ERROR, "Unsupported message type", "", "");
      goto EXIT_LABEL;
  }

EXIT_LABEL:  
  
  DEBUG("MQTT: otb_mqtt_on_receive_publish exit");
  
  return;
}

uint8 ICACHE_FLASH_ATTR otb_mqtt_pub_get_topic(char *topic)
{
  char *sub_topic;
  char *next_sub_topic;
  uint8 topic_id = OTB_MQTT_TOPIC_INVALID_;

  DEBUG("MQTT: otb_mqtt_pub_get_topic entry");
  
  // This function finds the last "sub-topic", and expects there to be more than 1 (i.e.
  // it won't match on the first
  
  // Find topic
  next_sub_topic = os_strstr(otb_mqtt_topic_s, OTB_MQTT_SLASH);
  while (next_sub_topic != NULL)
  {
    sub_topic = next_sub_topic;
    next_sub_topic++;
    next_sub_topic = os_strstr(next_sub_topic, OTB_MQTT_SLASH);
  }
  
  if (sub_topic != NULL)
  {
    // This ++ should be safe - as it's a string worse case scenario is this takes us to 
    // a null terminator, but should take us past the /
    sub_topic++;
    
    if (!os_memcmp(sub_topic, OTB_MQTT_TOPIC_SYSTEM, strlen(OTB_MQTT_TOPIC_SYSTEM)))
    {
      topic_id = OTB_MQTT_TOPIC_SYSTEM_;
    }
  }  

  DEBUG("MQTT: otb_mqtt_pub_get_topic exit");
  
  return(topic_id);
}  

int ICACHE_FLASH_ATTR otb_mqtt_get_cmd_len(char *cmd)
{
  int len;
  
  DEBUG("MQTT: otb_mqtt_get_cmd_len entry");

  for (len = 0; ((cmd[len] != ':') && (cmd[len] != 0)); len++);
   
  DEBUG("MQTT: otb_mqtt_get_cmd_len exit");

  return(len);
}

bool ICACHE_FLASH_ATTR otb_mqtt_match(char *msg, char *cmd)
{
  bool match = FALSE;
  int len;

  DEBUG("MQTT: otb_mqtt_match entry");
  
  len = otb_mqtt_get_cmd_len(msg);

  DEBUG("MQTT: match msg %s len %d", msg, len);
  DEBUG("MQTT: match cmd %s", cmd);
  
  if (os_strlen(cmd) == len)
  {
    match = os_memcmp(msg, cmd, len);

    // Flip match as os_strncmp says 0 if matched
    match = match ? FALSE : TRUE;
  }
  else
  {
    match = FALSE;
  }

  DEBUG("MQTT: match %s vs %s = %d", msg, cmd, match);
  
  DEBUG("MQTT: otb_mqtt_match exit");
  
  return match;
}

uint8 ICACHE_FLASH_ATTR otb_mqtt_pub_get_command(char *msg, char *val[])
{
  uint8 ii;
  uint8 cmd_id = OTB_MQTT_SYSTEM_INVALID_;
  char *temp;

  DEBUG("MQTT: otb_mqtt_pub_get_command entry");
  
  for (ii = 0; ii < (OTB_MQTT_MAX_CMDS - 1); ii++)
  {
    val[ii] = NULL;
  }

  // First get pointers to the various sub bits of the command
  ii = 0;
  temp = os_strstr(msg, OTB_MQTT_COLON);
  while ((temp != NULL) && (ii < (OTB_MQTT_MAX_CMDS - 1)))
  {
    temp++;
    val[ii] = temp;
    ii++;
    temp = os_strstr(temp, OTB_MQTT_COLON);
  }
  
  // Now try to match the command.
  for (ii = OTB_MQTT_SYSTEM_CMD_FIRST_; ii <= OTB_MQTT_SYSTEM_CMD_LAST_; ii++)
  {
    if (otb_mqtt_match(msg, otb_mqtt_system_cmds[ii]))
    {
      cmd_id = ii;
      break;
    }
  }  
  
  DEBUG("MQTT: otb_mqtt_pub_get_command exit");
  
  return(cmd_id);
}

uint8 ICACHE_FLASH_ATTR otb_mqtt_get_reason(char *msg)
{
  uint8 ii;
  uint8 reason_id = OTB_MQTT_REASON_INVALID_;

  DEBUG("MQTT: otb_mqtt_get_reason entry");
  
  for (ii = OTB_MQTT_REASON_FIRST_; ii <= OTB_MQTT_REASON_LAST_; ii++)
  {
    if (otb_mqtt_match(msg, otb_mqtt_reasons[ii]))
    {
      reason_id = ii;
      break;
    }
  }
    
  DEBUG("MQTT: otb_mqtt_get_reason exit");
  
  return(reason_id);
}

uint8 ICACHE_FLASH_ATTR otb_mqtt_set_svr(char *svr, char *port, bool commit)
{
  uint8 rc = OTB_CONF_RC_NOT_CHANGED;
  uint8 bytes;
  char *ptr;
  int byte;
  int port_int;
  bool conf_rc;

  DEBUG("MQTT: otb_mqtt_set_svr entry");

  // First of all check the validity of the stuff passed in.
  // Don't bother checking the address as can be a DN, FQDN or IP address
  
  if (port != NULL)
  {
    port_int = atoi(port);
    if ((port_int < 0) || (port_int > 0xffff))
    {
      DETAIL("MQTT: Invalid MQTT port");
      rc = OTB_CONF_RC_ERROR;
      goto EXIT_LABEL;
    }
  }
  
  // Now see if it's actually changed
  
  if (svr != NULL)
  {
    if (os_strcmp(otb_conf->mqtt.svr, svr))
    {
      DETAIL("MQTT: server changed");
      rc = OTB_CONF_RC_CHANGED;
    }
  }
  
  if (port != NULL)
  {
    if (port_int != otb_conf->mqtt.port)
    {
      DETAIL("MQTT: port changed");
      rc = OTB_CONF_RC_CHANGED;
    }
  }
  
EXIT_LABEL:  
  
  // Valid looking IP address - store it off
  if (rc == OTB_CONF_RC_CHANGED)
  {
    if (svr != NULL)
    {
      os_strncpy(otb_conf->mqtt.svr, svr, OTB_MQTT_MAX_SVR_LEN);
      otb_conf->mqtt.svr[OTB_MQTT_MAX_SVR_LEN-1] = 0;
    }
    if (port != NULL)
    {
      otb_conf->mqtt.port = port_int;
    }

    if (commit)
    {
      conf_rc = otb_conf_update(otb_conf);
      if (!conf_rc)
      {
        ERROR("CONF: Failed to update config");
        rc = OTB_CONF_RC_ERROR;
      }
    }
  }
  
  DEBUG("MQTT: otb_mqtt_set_svr exit");
  
  return(rc);
}

uint8 ICACHE_FLASH_ATTR otb_mqtt_set_user(char *user, char *pass, bool commit)
{
  uint8 rc = OTB_CONF_RC_NOT_CHANGED;
  bool conf_rc;

  DEBUG("MQTT: otb_mqtt_set_user enty");

  // First of all check validity of what's passed in - nothing to do here
  
  // Now see if anything's changed
  
  if (user != NULL)
  {
    if (os_strcmp(otb_conf->mqtt.user, user))
    {
      rc = OTB_CONF_RC_CHANGED;
    }
  }
  
  if (pass != NULL)
  {
    if (os_strcmp(otb_conf->mqtt.pass, pass))
    {
      rc = OTB_CONF_RC_CHANGED;
    }
  }

EXIT_LABEL:
  
  if (rc == OTB_CONF_RC_CHANGED)
  {
    os_strncpy(otb_conf->mqtt.user, user, OTB_MQTT_MAX_USER_LEN);
    otb_conf->mqtt.user[OTB_MQTT_MAX_USER_LEN-1] = 0;
    os_strncpy(otb_conf->mqtt.pass, pass, OTB_MQTT_MAX_PASS_LEN);
    otb_conf->mqtt.user[OTB_MQTT_MAX_PASS_LEN-1] = 0;

    if (commit)
    {
      conf_rc = otb_conf_update(otb_conf);
      if (!conf_rc)
      {
        rc = OTB_CONF_RC_ERROR;
        ERROR("CONF: Failed to update config");
      }
    }
  }

  DEBUG("MQTT: otb_mqtt_set_user exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_mqtt_config_handler(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32_t cmd = (uint32_t)arg;
  char *username, *password, *svr, *port;
  char port_exist[8];
  uint8 mqtt_rc;
  
  DEBUG("CMD: otb_mqtt_config_handler entry");

  OTB_ASSERT(((cmd & 0xff) >= OTB_MQTT_CONFIG_CMD_MIN) &&
             ((cmd & 0xff) <= OTB_MQTT_CONFIG_CMD_MAX));
  OTB_ASSERT((cmd & OTB_MQTT_CFG_CMD_GET) || (cmd & OTB_MQTT_CFG_CMD_SET));

  if (cmd & OTB_MQTT_CFG_CMD_GET)
  {
    switch (cmd & 0xff)
    {
      case OTB_MQTT_CONFIG_CMD_SERVER:
        otb_cmd_rsp_append("%s", otb_conf->mqtt.svr);
        rc = TRUE;
        break;

      case OTB_MQTT_CONFIG_CMD_PORT:
        otb_cmd_rsp_append("%d", otb_conf->mqtt.port);
        rc = TRUE;
        break;

      case OTB_MQTT_CONFIG_CMD_USERNAME:
        otb_cmd_rsp_append("%s", otb_conf->mqtt.user);
        rc = TRUE;
        break;

      case OTB_MQTT_CONFIG_CMD_PASSWORD:
        otb_cmd_rsp_append("%s", otb_conf->mqtt.pass);
        rc = TRUE;
        break;

      default:
        otb_cmd_rsp_append("internal error");
        rc = FALSE;
        goto EXIT_LABEL;
        break;
    }
  }
  else
  {
    if (next_cmd == NULL)
    {
        otb_cmd_rsp_append("no value provided");
        rc = FALSE;
        goto EXIT_LABEL;
    }
    switch (cmd & 0xff)
    {
      case OTB_MQTT_CONFIG_CMD_SERVER:
      case OTB_MQTT_CONFIG_CMD_PORT:
        // Blank _password_ and _username_ OK
        if (next_cmd[0] == 0)
        {
            otb_cmd_rsp_append("no value provided");
            rc = FALSE;
            goto EXIT_LABEL;
        }
        svr = otb_conf->mqtt.svr;
        os_snprintf(port_exist, 7, "%d", otb_conf->mqtt.port);
        port = port_exist;
        if ((cmd & 0xff) == OTB_MQTT_CONFIG_CMD_SERVER)
        {
          svr = next_cmd;
        }
        else
        {
          port = next_cmd;
        }
        DETAIL("MQTT: Change svr from %s to %s port from %d to %s", otb_conf->mqtt.svr, svr, otb_conf->mqtt.port, port);
        mqtt_rc = otb_mqtt_set_svr(svr, port, TRUE);
        if (mqtt_rc == OTB_CONF_RC_NOT_CHANGED)
        {
          otb_cmd_rsp_append("unchanged");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        else if (mqtt_rc == OTB_CONF_RC_ERROR)
        {
          rc = FALSE;
          goto EXIT_LABEL;
        }
        rc = TRUE;
        break;

      case OTB_MQTT_CONFIG_CMD_USERNAME:
      case OTB_MQTT_CONFIG_CMD_PASSWORD:
        username = otb_conf->mqtt.user;
        password = otb_conf->mqtt.pass;
        if ((cmd & 0xff) == OTB_MQTT_CONFIG_CMD_USERNAME)
        {
          username = next_cmd;
        }
        else
        {
          password = next_cmd;
        }
        DETAIL("MQTT: Change username from %s to %s password from %s to %s", otb_conf->mqtt.user, username, otb_conf->mqtt.pass, password);
        mqtt_rc = otb_mqtt_set_user(username, password, TRUE);
        if (mqtt_rc == OTB_CONF_RC_NOT_CHANGED)
        {
          otb_cmd_rsp_append("unchanged");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        else if (mqtt_rc == OTB_CONF_RC_ERROR)
        {
          rc = FALSE;
          goto EXIT_LABEL;
        }
        rc = TRUE;
        break;

      default:
        otb_cmd_rsp_append("internal error");
        rc = FALSE;
        goto EXIT_LABEL;
        break;
    }
  }
    
  rc = TRUE;

EXIT_LABEL:
  
  DEBUG("CMD: otb_mqtt_config_handler exit");
  
  return rc;

}

