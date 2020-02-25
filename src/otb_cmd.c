/*
 *
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
 * 
 */

#define OTB_CMD_C
#include "otb.h"
#include <stdarg.h>

MLOG("CMD");

otb_cmd_control ICACHE_FLASH_ATTR *otb_cmd_load_cmd_control_from_flash(otb_cmd_control *ctrl)
{
  otb_cmd_control *rc = (otb_cmd_control *)&otb_cmd_control_flash_buf;
  int ii;

  ENTRY;

  // This is a bit inefficient - we always read in whole of buf size even though
  // cmd_control structure is unlikely to be that long.  <Shrugs> - it's easier
  // than figuring out the size at runtime
  for (ii = 0; ii < OTB_CMD_CONTROL_BUF_SIZE/4; ii++)
  {
    // load in 4 bytes at a time or will throw an exception
    *(((uint32_t*)(&otb_cmd_control_flash_buf))+ii) = *(((uint32_t*)(ctrl))+ii);
  }

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_cmd_mqtt_receive(uint32_t *client,
                                            const char* topic,
                                            uint32_t topic_len,
                                            const char *msg,
                                            uint32_t msg_len,
                                            char *buf,
                                            uint16_t buf_len)
{
  unsigned char *cur_match = NULL;
  otb_cmd_control *cur_control = NULL;
  unsigned char *cur_cmd = NULL;
  unsigned char *prev_cmd;
  unsigned char *match_cmd;
  uint8_t depth;
  bool rc = FALSE;
  bool match = TRUE;
  int ii;
  int jj;

  ENTRY;
  
  MDEBUG("topic len %d", topic_len);
  MDEBUG("msg len %d", msg_len);
  
  // Zero out response
  otb_cmd_rsp_clear();

  // Process incoming message - break this up and populate otb_cmd_incoming with lower
  // case
  rc = otb_cmd_populate_all(topic, topic_len, msg, msg_len);
  if (!rc)
  {
    // otb_cmd_populate fills in any error message
    goto EXIT_LABEL;
  }
  rc = FALSE;
  
  // Start at the top
  cur_control = otb_cmd_load_cmd_control_from_flash(otb_cmd_control_topic_top);
  
  // Now run through otb_cmd_incoming according to the rules defined within the
  // otb_cmd_control_top_level tree.
  // Note no need to use str_n_ style commands as we were diligent in
  // otb_cmd_populate_all
  depth = 0;
  cur_cmd = otb_cmd_incoming_cmd[0];
  
  // While cur_cmd is not a null terminated string
  while ((cur_cmd[0] != 0) && (depth < OTB_CMD_MAX_CMDS))
  {
    MDEBUG("Current cmd %s", cur_cmd);
    match = FALSE;
    
    for (ii = 0; !match; ii++)
    {
      MDEBUG("cur_cmd: %s", cur_cmd);
      // perform match
      if (cur_control[ii].match_cmd != NULL)
      {
        // Do string_match
        if (depth == 0)
        {
          // Override topic - e.g. can be espi or otb-iot
          match_cmd = otb_mqtt_root;
        }
        else
        {
          match_cmd = cur_control[ii].match_cmd;
        }
        if (!os_strcmp(match_cmd, cur_cmd))
        {
          MDEBUG("Match");
          match = TRUE;
          break;
        }
      }
      else if (cur_control[ii].match_fn != NULL)
      {
        match = cur_control[ii].match_fn(cur_cmd);
        MDEBUG("match func result %d", match);
        if (match)
        {
          break;
        }
      }
      else
      {
        // No match and have been through this whole command struct
        break;
      }
    }
    
    if (match)
    {
      // Actually do this step.  That's either moving onto the next step - if
      // sub_cmd_control is set, or the handler_fn if not.
      if (cur_control[ii].sub_cmd_control != NULL)
      {
        cur_control = otb_cmd_load_cmd_control_from_flash(cur_control[ii].sub_cmd_control);
        depth++;
        if (depth < OTB_CMD_MAX_CMDS)
        {
          cur_cmd = otb_cmd_incoming_cmd[depth];
        }
        else
        {
          break;
        }
      }
      else if (cur_control[ii].handler_fn != NULL)
      {
        depth++;
        if (depth < OTB_CMD_MAX_CMDS)
        {
          cur_cmd = otb_cmd_incoming_cmd[depth];
          prev_cmd = NULL;
          if (depth > 0)
          {
            prev_cmd = otb_cmd_incoming_cmd[depth-1];
          }
          // This function sets the response
          rc = cur_control[ii].handler_fn(cur_cmd,
                                          cur_control[ii].arg,
                                          prev_cmd);
          if (!rc)
          {
            goto EXIT_LABEL;
          }
        }
        else
        {
          otb_cmd_rsp_append("MQTT topic/message too long");
        }
        // Break out of processing once we have hander function
        break;
      }
      else
      {
        // This means there is no sub cmd control and no handler function which isn't valid
        OTB_ASSERT(FALSE);
        otb_cmd_rsp_append("Internal error");
        goto EXIT_LABEL;
      }
      // Move onto the next step
      if (depth >= OTB_CMD_MAX_CMDS)
      {
        break;
      }
    }
    else
    {
      // No match - invalid structure
      otb_cmd_rsp_append("Invalid MQTT topic/message");
      goto EXIT_LABEL;
    }
  }
  
  // If we get here we're done!  We don't set rc to TRUE - it _should_ have been set by
  // match_fn
  rc = TRUE;
  
EXIT_LABEL:  

  // XXX Need to fill some indication of what we're responding to.  How best to do
  // this?
  otb_mqtt_send_status_or_buf(rc ? OTB_MQTT_STATUS_OK : OTB_MQTT_STATUS_ERROR,
                              otb_cmd_rsp_get(),
                              "",
                              "",
                              buf,
                              buf_len);
  
  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_cmd_rsp_clear(void)
{

  ENTRY;
  
  // Clear it out - no point doing a memset
  otb_cmd_rsp_next = 0;
  otb_cmd_rsp[0] = 0;
  
  EXIT;
  
  return;
}

void ICACHE_FLASH_ATTR otb_cmd_rsp_append(char *format, ...)
{
  int written;
  va_list args;

  ENTRY;
  
  // If something already in string, add a / delimiter
  if (otb_cmd_rsp_next > 0)
  {
    if (otb_cmd_rsp_next < (OTB_CMD_RSP_MAX_LEN - 1))
    {
      otb_cmd_rsp[otb_cmd_rsp_next] = '/';
      otb_cmd_rsp_next++;
    }
  }
  
  va_start(args, format);
  written = os_vsnprintf(otb_cmd_rsp + otb_cmd_rsp_next,
                         OTB_CMD_RSP_MAX_LEN - otb_cmd_rsp_next - 1,
                         format,
                         args);
                         
  otb_cmd_rsp_next += written;
  
  MDEBUG("rsp_append now %s", otb_cmd_rsp);

  // Assert to check we didn't write over the end of the available space  
  OTB_ASSERT(otb_cmd_rsp_next < OTB_CMD_RSP_MAX_LEN);

  EXIT;
  
  return;
}

unsigned char ICACHE_FLASH_ATTR *otb_cmd_rsp_get(void)
{

  ENTRY;
  
  EXIT;
  
  return otb_cmd_rsp;
}

bool ICACHE_FLASH_ATTR otb_cmd_populate_all(const char *topic,
                                            uint32_t topic_len,
                                            const char *msg,
                                            uint32_t msg_len)
{
  bool rc = FALSE;
  int ii, jj;
  unsigned char *cur;
  unsigned char *next;
  int len;
  uint8_t written;

  ENTRY;
  
  // Two things to populate - topics and cmds.  Do both, into same array.
  
  os_memset(otb_cmd_incoming_cmd, 0, sizeof(otb_cmd_incoming_cmd));

  rc = otb_cmd_populate_one(otb_cmd_incoming_cmd,
                            OTB_CMD_MAX_CMDS,
                            OTB_CMD_MAX_CMD_LEN,
                            topic,
                            topic_len,
                            &written);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  // We don't do this as the topic and messages gets run together by the MQTT stack
  // so the above handles messages as well  
  rc = otb_cmd_populate_one(otb_cmd_incoming_cmd + written,
                            OTB_CMD_MAX_CMDS - written,
                            OTB_CMD_MAX_CMD_LEN,
                            msg,
                            msg_len,
                            &written);
                            
  // Strictly we don't need the rest of the code before the EXIT_LABEL, but it keeps it
  // consistent                            
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = TRUE;

EXIT_LABEL:

  EXIT;
  
  return rc;
}

//
// This function takes the input_str, which may or may not be null terminated and will
// process precisely input_str_len of it, with all slashes stripped out, and used
// to put in the store array.  It will only put up to store_num in the array
// and the store_str_len indicates the maximum string len allowed in each on (including
// NULL terminator.  Note that this function will not stop if it finds a NULL terminator
// in input_str - it is assumed input_str_len is accurate.
// Returns FALSE if in some way invalid (i.e. too many bits to store, or a string 
// being too long.  It will also use otb_cmd_rsp_append to add an error in this case.
//
bool ICACHE_FLASH_ATTR otb_cmd_populate_one(unsigned char store[][OTB_CMD_MAX_CMD_LEN],
                                            uint8_t store_num,
                                            uint8_t store_str_len,
                                            const char *input_str,
                                            uint32_t input_str_len,
                                            uint8_t *written)
{
  int input_index;
  int store_string_index;
  int store_index;
  bool started_storing_segment;
  bool rc = FALSE;
  int ii;
  
  ENTRY;

  MDEBUG("populate_one store 0x%08x", store);
  MDEBUG("populate_one store_num %d", store_num);
  MDEBUG("populate_one store_str_len %d", store_str_len);
  MDEBUG("populate_one input_str 0x%08x", input_str);
  MDEBUG("populate_one input_str_len %d", input_str_len);
  MDEBUG("populate_one written 0x%08x", written);

  input_index = 0;
  store_index = 0;
  started_storing_segment = FALSE;
  store_string_index = 0;
  *written = 0;
  while (input_index < input_str_len)
  {
    if (store_index >= store_num)
    {
      MDEBUG("store_index >= store_num");
      otb_cmd_rsp_append("Invalid MQTT command (too many segments)");
      goto EXIT_LABEL;
    }
    
    if (input_str[input_index] == '/')
    {
      MDEBUG("Skip a /");
      if (started_storing_segment)
      {
        // Finish off last segment
        MDEBUG("Terminate segment");
        store[store_index][store_string_index] = 0;
        started_storing_segment = FALSE;
        store_index++;
        store_string_index = 0;
        (*written)++;
      }
    }
    else
    {
      started_storing_segment = TRUE;
      if (store_string_index >= (store_str_len-1))
      {
        MDEBUG("store_string_index >= store_str_len-1");
        otb_cmd_rsp_append("Invalid MQTT command (segment too long)");
        goto EXIT_LABEL;
      }
      else
      {
        MDEBUG("Store char %c", input_str[input_index]);
        store[store_index][store_string_index] = input_str[input_index];
      }
      store_string_index++;
    }
    input_index++;
  }  
  
  if (started_storing_segment)
  {
    // Finish off the last one
    MDEBUG("Terminate segment");
    store[store_index][store_string_index] = 0;
    (*written)++;
  }
  
  // Set return values
  rc = TRUE;

EXIT_LABEL:

  // Log result
  if (rc)
  {
    MDEBUG("Populated: %d", *written);
    for (ii = 0; ii < *written; ii++)
    {
      MDEBUG("          %s", store[ii]);
    }
  }
  
  EXIT;

  return rc;
}

unsigned char *otb_cmd_get_next_cmd(unsigned char *cmd)
{
  unsigned char *next_cmd = NULL;
  int depth;
  unsigned char *cur_cmd;
  bool next = FALSE;

  ENTRY;
  
  // Go through the command stack looking for a match and return the next
  depth = 0;
  cur_cmd = otb_cmd_incoming_cmd[0];
  
  // While cur_cmd is not a null terminated string
  while ((cur_cmd[0] != 0) && (depth < OTB_CMD_MAX_CMDS))
  {
    if (cur_cmd == cmd)
    {
      next = TRUE;
    }
    depth++;
    cur_cmd = otb_cmd_incoming_cmd[depth];
    if (next)
    {
      next_cmd = cur_cmd;
      break;
    }
  }
  
  EXIT;

  return next_cmd;
}

bool ICACHE_FLASH_ATTR otb_cmd_match_chipid(unsigned char *to_match)
{
  bool rc = FALSE;
  
  ENTRY;

  if (!os_strcmp(OTB_MAIN_CHIPID, to_match))
  {
    rc = TRUE;
  }
  else if (!os_strcmp(OTB_MQTT_MATCH_ALL, to_match))
  {
    rc = TRUE;
  }

  EXIT;

  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_cmd_get_string(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;        

  otb_cmd_rsp_append((unsigned char *)arg);
  rc = TRUE;
  
  EXIT;        

  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_cmd_get_boot_slot(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;
  
  otb_cmd_rsp_append("%d", otb_rboot_get_slot(FALSE));
  rc = TRUE;
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_rssi(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;
  
  otb_cmd_rsp_append("%d", otb_wifi_get_rssi());
  rc = TRUE;
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_heap_size(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;
  
  otb_cmd_rsp_append("%d", system_get_free_heap_size());
  rc = TRUE;
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_vdd33(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  uint16 vdd33;
  double voltage;
  int voltage_int;
  int voltage_thou;
  bool rc = FALSE;
  
  ENTRY;
  
  rc = otb_util_get_vdd33(&vdd33);
  if (rc)
  {
    //vdd33 = system_adc_read();
    voltage = vdd33/1024;
    voltage_int = voltage/1;
    voltage_thou = (int)(vdd33*1000/1024)%1000;
    otb_cmd_rsp_append("0x%04x/%d.%03dV", vdd33, voltage_int, voltage_thou);
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_logs_ram(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int ii;
  char *error;
  
  ENTRY;

  if ((next_cmd != NULL) && (next_cmd[0] != 0))
  {
    ii = atoi(next_cmd);
    if ((ii <= 0xff) && (ii >= 0))
    {
      error = otb_util_get_log_ram((uint8)ii);
      if (error != NULL)
      {
        otb_cmd_rsp_append(error);
        rc = TRUE;
      }
    }
  }
  
  if (!rc)
  {
    otb_cmd_rsp_append("invalid index");
  }

  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_reason_reboot(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *reason;
  char ALIGN4 flash_reason[48];
  
  ENTRY;
  
  reason = "error getting reason: failed to read from flash";
  rc = otb_util_flash_read(OTB_BOOT_LAST_REBOOT_REASON, (uint32 *)flash_reason, 48);
  // NULL terminate just in case ...
  flash_reason[47] = 0;
  if (rc)
  {
    otb_cmd_rsp_append(flash_reason);
  }
  else
  {
    otb_cmd_rsp_append("error getting reason");
  }
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_test_led_fn(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32 type;
  unsigned char *error;
  
  ENTRY;
  
  type = (uint32)arg;
  OTB_ASSERT(type < OTB_CMD_TRIGGER_TEST_LED_TYPES);
  
  switch(type)
  {
    case OTB_CMD_TRIGGER_TEST_LED_ONCE:
      rc = otb_led_test(next_cmd, FALSE, &error);
      break;

    case OTB_CMD_TRIGGER_TEST_LED_GO:
      rc = otb_led_test(next_cmd, TRUE, &error);
      break;

    case OTB_CMD_TRIGGER_TEST_LED_STOP:
      rc = otb_led_test_stop(next_cmd, &error);
      break;
  }
  
  otb_cmd_rsp_append(error);
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_set_boot_slot(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *error;
  
  ENTRY;
  
  rc = otb_rboot_update_slot(next_cmd, &error);
  otb_cmd_rsp_append(error);
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_assert(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;

  ENTRY;

  OTB_ASSERT(FALSE);
  rc = TRUE;

  EXIT;
  
  return rc;
}

char ALIGN4 otb_cmd_wipe_error_string[] = "Config wipe";
bool ICACHE_FLASH_ATTR otb_cmd_trigger_wipe(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;

  otb_conf_init_config(otb_conf);
  rc = otb_conf_save(otb_conf);
  if (!rc)
  {
    otb_cmd_rsp_append("failed to store new config");
    rc = FALSE;
  }
  else
  {
    otb_reset_schedule(1000, otb_cmd_wipe_error_string, FALSE);
    rc = TRUE;
  }

  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_update(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *next_cmd2 = NULL;
  unsigned char *next_cmd3 = NULL;
  unsigned char *error = "invalid upgrade source";
  
  ENTRY;
  
  next_cmd2 = otb_cmd_get_next_cmd(next_cmd);
  if (next_cmd2 != NULL)
  {
    next_cmd3 = otb_cmd_get_next_cmd(next_cmd2);
  }
  rc = otb_rboot_update(next_cmd, next_cmd2, next_cmd3, &error);
  otb_cmd_rsp_append(error);
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_control_get_sensor_temp_ds18b20_num(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;

  otb_cmd_rsp_append("%d", otb_ds18b20_count);
  rc = TRUE;
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_control_get_sensor_temp_ds18b20_addr(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int index;
  
  ENTRY;

  index = otb_ds18b20_valid_index(next_cmd);
  if (index >= 0)
  {
    otb_cmd_rsp_append(otb_ds18b20_addresses[index].friendly);
    rc = TRUE;
  }
  else
  {
    otb_cmd_rsp_append("invalid index");
  }
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_control_get_sensor_temp_ds18b20_value(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int index;
  
  ENTRY;

  index = otb_ds18b20_valid_index(next_cmd);
  if (index >= 0)
  {
    otb_cmd_rsp_append(otb_ds18b20_last_temp_s[index]);
    rc = TRUE;
  }
  else
  {
    otb_cmd_rsp_append("invalid index");
  }
  
  EXIT;
  
  return rc;

}

char ALIGN4 otb_cmd_reset_error_string[] = "MQTT triggered reset/reboot";
bool ICACHE_FLASH_ATTR otb_cmd_trigger_reset(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;
  
  otb_reset(otb_cmd_reset_error_string);
  otb_cmd_rsp_append("ok");
  rc = TRUE;

  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_ping(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;
  
  otb_cmd_rsp_append("pong");
  rc = TRUE;
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_config_all(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int jj;
  unsigned char kk = 0;
  
  ENTRY;
  
  //
  // Output format of this is:
  // ok:len.xxx (xxx is decimal length of config)
  // data:yy/yy/yy/.. (yy is a hex byte of data, with 32 bytes per response)
  // end
  //

  // We use otb_mqtt_publish direct:
  // - for the data, to avoid sending ok in front
  // - the the len as we're sending data qos 0 which gets through sooner than 
  //   the standard 2 - so send len as 0 well so it gets through first!  
  unsigned char *conf_struct = (unsigned char *)otb_conf;
  otb_cmd_rsp_append("len.%d", sizeof(otb_conf_struct));
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_TOPIC_STATUS,
                   "",
                   "ok",
                   otb_cmd_rsp_get(),
                   0,
                   0,
                   NULL,
                   0);  
  otb_cmd_rsp_clear();
  for (jj = 0; jj < sizeof(otb_conf_struct); jj++)
  {
    otb_cmd_rsp_append("%02x", conf_struct[jj]);
    if (kk >= 31)
    {
      kk = 0;
      otb_mqtt_publish(&otb_mqtt_client,
                       OTB_MQTT_TOPIC_STATUS,
                       "",
                       "data",
                       otb_cmd_rsp_get(),
                       0,
                       0,
                       NULL,
                       0);  
      otb_cmd_rsp_clear();
    }
    else
    {
      kk++;
    }
  }
  
  if (kk > 0)
  {
      otb_mqtt_publish(&otb_mqtt_client,
                       OTB_MQTT_TOPIC_STATUS,
                       "",
                       "data",
                       otb_cmd_rsp_get(),
                       0,
                       0,
                       NULL,
                       0);  
    otb_cmd_rsp_clear();
  }
  otb_cmd_rsp_append("end");
   
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_sensor_adc_ads(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  ENTRY;
  
  // XXX not implemented
  otb_cmd_rsp_append("not implemented");
  
  EXIT;
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_ip_info(unsigned char *next_cmd,
                                           void *arg,
                                           unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  struct ip_info ip_inf;
  ip_addr_t dns;
  bool ip_info_rc;
  char addr_s[OTB_WIFI_MAX_IPV4_STRING_LEN];
  int dns_num;
    
  ENTRY;
  
  // Double check what we're being asked to do is valid
  cmd = (int)arg;
  OTB_ASSERT((cmd >= OTB_CMD_IP_MIN) && (cmd < OTB_CMD_IP_TOTAL));
  
  switch(cmd)
  {
    case OTB_CMD_IP_IP:
    case OTB_CMD_IP_MASK:
    case OTB_CMD_IP_GATEWAY:
      ip_info_rc = wifi_get_ip_info(STATION_IF, &ip_inf);
      if (!ip_info_rc)
      {
        rc = FALSE;
        otb_cmd_rsp_append("Internal error");
        goto EXIT_LABEL;
      }
      if (cmd == OTB_CMD_IP_IP)
      {
        otb_wifi_get_ip_string(ip_inf.ip.addr, addr_s);
      }
      else if (cmd == OTB_CMD_IP_MASK)
      {
        otb_wifi_get_ip_string(ip_inf.netmask.addr, addr_s);
      }
      else // Gateway
      {
        otb_wifi_get_ip_string(ip_inf.gw.addr, addr_s);
      }
      otb_cmd_rsp_append(addr_s);
      rc = TRUE;
      break;

    case OTB_CMD_IP_DNS1:
    case OTB_CMD_IP_DNS2:
      OTB_ASSERT(OTB_CMD_IP_DNS1 == (OTB_CMD_IP_DNS2 - 1));
      dns_num = cmd - OTB_CMD_IP_DNS1;
      dns = espconn_dns_getserver(dns_num);
      otb_wifi_get_ip_string(dns.addr, addr_s);
      otb_cmd_rsp_append(addr_s);
      rc = TRUE;
      break;

    case OTB_CMD_IP_DHCP:
      if (otb_conf->ip.manual == OTB_IP_DHCP_DHCP)
      {
        otb_cmd_rsp_append("DHCP");
      }
      else
      {
        otb_cmd_rsp_append("manual");
      }
      rc = TRUE;
      break;

    case OTB_CMD_IP_DOMAIN:
      otb_cmd_rsp_append(otb_conf->ip.domain_name);
      rc = TRUE;
      break;

    default:
      // Shouldn't get here
      OTB_ASSERT(FALSE);
      rc = FALSE;
      otb_cmd_rsp_append("Invalid command");
      goto EXIT_LABEL;
      break;
  }

EXIT_LABEL:

  EXIT;
  
  return rc;
}

// XXX Dummy
bool ICACHE_FLASH_ATTR xxx(unsigned char *next_cmd, void *arg)
{
  bool rc = FALSE;
  
  ENTRY;
  
  otb_cmd_rsp_append("%d", system_get_free_heap_size());
  rc = TRUE;
  
  EXIT;
  
  return rc;

}
