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

#define OTB_CMD_C
#include "otb.h"
#include <stdarg.h>

void ICACHE_FLASH_ATTR otb_cmd_mqtt_receive(uint32_t *client,
                                            const char* topic,
                                            uint32_t topic_len,
                                            const char *msg,
                                            uint32_t msg_len)
{
  unsigned char *cur_match = NULL;
  otb_cmd_control *cur_control = NULL;
  unsigned char *cur_cmd = NULL;
  unsigned char *prev_cmd;
  uint8_t depth;
  bool rc = FALSE;
  bool match = TRUE;
  int ii;

  DEBUG("CMD: otb_cmd_mqtt_received entry");
  
  DEBUG("CMD: topic len %d", topic_len);
  DEBUG("CMD: msg len %d", msg_len);
  
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
  cur_control = otb_cmd_control_topic_top;
  
  // Now run through otb_cmd_incoming according to the rules defined within the
  // otb_cmd_control_top_level tree.
  // Note no need to use str_n_ style commands as we were diligent in
  // otb_cmd_populate_all
  depth = 0;
  cur_cmd = otb_cmd_incoming_cmd[0];
  
  // While cur_cmd is not a null terminated string
  while ((cur_cmd[0] != 0) && (depth < OTB_CMD_MAX_CMDS))
  {
    DEBUG("CMD: Current cmd %s", cur_cmd);
    match = FALSE;
    
    for (ii = 0; !match; ii++)
    {
      DEBUG("CMD: cur_cmd: %s", cur_cmd);
      // perform match
      if (cur_control[ii].match_cmd != NULL)
      {
        // Do string_match
        if (!os_strcmp(cur_control[ii].match_cmd, cur_cmd))
        {
          DEBUG("CMD: Match");
          match = TRUE;
          break;
        }
      }
      else if (cur_control[ii].match_fn != NULL)
      {
        match = cur_control[ii].match_fn(cur_cmd);
        DEBUG("CMD: match func result %d", match);
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
        cur_control = cur_control[ii].sub_cmd_control;
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
  otb_mqtt_send_status(rc ? OTB_MQTT_STATUS_OK : OTB_MQTT_STATUS_ERROR,
                       otb_cmd_rsp_get(),
                       "",
                       "");
  
  DEBUG("CMD: otb_cmd_mqtt_received exit");

  return;
}

void ICACHE_FLASH_ATTR otb_cmd_rsp_clear(void)
{

  DEBUG("CMD: otb_cmd_rsp_clear entry");
  
  // Clear it out - no point doing a memset
  otb_cmd_rsp_next = 0;
  otb_cmd_rsp[0] = 0;
  
  DEBUG("CMD: otb_cmd_rsp_clear exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_cmd_rsp_append(char *format, ...)
{
  int written;
  va_list args;

  DEBUG("CMD: otb_cmd_rsp_append entry");
  
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
  
  DEBUG("CMD: rsp_append now %s", otb_cmd_rsp);

  // Assert to check we didn't write over the end of the available space  
  OTB_ASSERT(otb_cmd_rsp_next < OTB_CMD_RSP_MAX_LEN);

  DEBUG("CMD: otb_cmd_rsp_append exit");
  
  return;
}

unsigned char ICACHE_FLASH_ATTR *otb_cmd_rsp_get(void)
{

  DEBUG("CMD: otb_cmd_rsp_append entry");
  
  DEBUG("CMD: otb_cmd_rsp_append exit");
  
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

  DEBUG("I2C: otb_cmd_populate_all entry");
  
  // Two things to populate - topics and cmds.  Do both, into same array.

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

  DEBUG("I2C: otb_cmd_populate_all exit");
  
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
  
  DEBUG("CMD: otb_cmd_populate_one entry");

  DEBUG("CMD: populate_one store 0x%08x", store);
  DEBUG("CMD: populate_one store_num %d", store_num);
  DEBUG("CMD: populate_one store_str_len %d", store_str_len);
  DEBUG("CMD: populate_one input_str 0x%08x", input_str);
  DEBUG("CMD: populate_one input_str_len %d", input_str_len);
  DEBUG("CMD: populate_one written 0x%08x", written);

  input_index = 0;
  store_index = 0;
  started_storing_segment = FALSE;
  store_string_index = 0;
  *written = 0;
  while (input_index < input_str_len)
  {
    if (store_index >= store_num)
    {
      DEBUG("CMD: store_index >= store_num");
      otb_cmd_rsp_append("Invalid MQTT command (too many segments)");
      goto EXIT_LABEL;
    }
    
    if (input_str[input_index] == '/')
    {
      DEBUG("CMD: Skip a /");
      if (started_storing_segment)
      {
        // Finish off last segment
        DEBUG("CMD: Terminate segment");
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
        DEBUG("CMD: store_string_index >= store_str_len-1");
        otb_cmd_rsp_append("Invalid MQTT command (segment too long)");
        goto EXIT_LABEL;
      }
      else
      {
        DEBUG("CMD: Store char %c", input_str[input_index]);
        store[store_index][store_string_index] = input_str[input_index];
      }
      store_string_index++;
    }
    input_index++;
  }  
  
  if (started_storing_segment)
  {
    // Finish off the last one
    DEBUG("CMD: Terminate segment");
    store[store_index][store_string_index] = 0;
    (*written)++;
  }
  
  // Set return values
  rc = TRUE;

EXIT_LABEL:

  // Log result
  if (rc)
  {
    DEBUG("CMD: Populated: %d", *written);
    for (ii = 0; ii < *written; ii++)
    {
      DEBUG("CMD:           %s", store[ii]);
    }
  }
  
  DEBUG("CMD: otb_cmd_populate_one exit");

  return rc;
}

unsigned char *otb_cmd_get_next_cmd(unsigned char *cmd)
{
  unsigned char *next_cmd = NULL;
  int depth;
  unsigned char *cur_cmd;
  bool next = FALSE;

  DEBUG("CMD: otb_cmd_get_next_cmd entry");
  
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
  
  DEBUG("CMD: otb_cmd_get_next_cmd exit");

  return next_cmd;
}

bool ICACHE_FLASH_ATTR otb_cmd_match_chipid(unsigned char *to_match)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_match_chipid entry");

  if (!os_strcmp(OTB_MAIN_CHIPID, to_match))
  {
    rc = TRUE;
  }

  DEBUG("CMD: otb_cmd_match_chipid exit");

  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_cmd_get_string(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_get_string entry");        

  otb_cmd_rsp_append((unsigned char *)arg);
  rc = TRUE;
  
  DEBUG("CMD: otb_cmd_get_string exit");        

  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_cmd_get_boot_slot(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_get_boot_slot entry");
  
  otb_cmd_rsp_append("%d", otb_rboot_get_slot(FALSE));
  rc = TRUE;
  
  DEBUG("CMD: otb_cmd_get_boot_slot exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_rssi(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_get_rssi entry");
  
  otb_cmd_rsp_append("%d", otb_wifi_get_rssi());
  rc = TRUE;
  
  DEBUG("CMD: otb_cmd_get_rssi exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_heap_size(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_get_heap_size entry");
  
  otb_cmd_rsp_append("%d", system_get_free_heap_size());
  rc = TRUE;
  
  DEBUG("CMD: otb_cmd_get_heap_size exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_vdd33(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  uint16 vdd33;
  double voltage;
  int voltage_int;
  int voltage_thou;
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_get_vdd33 entry");
  
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
  
  DEBUG("CMD: otb_cmd_get_vdd33 exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_logs_ram(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int ii;
  char *error;
  
  DEBUG("CMD: otb_cmd_get_logs_ram entry");

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

  DEBUG("CMD: otb_cmd_get_logs_ram exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_reason_reboot(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *reason;
  char ALIGN4 flash_reason[48];
  
  DEBUG("CMD: otb_cmd_get_reason_reboot entry");
  
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
  
  DEBUG("CMD: otb_cmd_get_reason_reboot exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_test_led_fn(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32 type;
  unsigned char *error;
  
  DEBUG("CMD: otb_cmd_trigger_test_led_fn entry");
  
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
  
  DEBUG("CMD: otb_cmd_trigger_test_led_fn exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_set_boot_slot(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *error;
  
  DEBUG("CMD: otb_cmd_set_boot_slot entry");
  
  rc = otb_rboot_update_slot(next_cmd, &error);
  otb_cmd_rsp_append(error);
  
  DEBUG("CMD: otb_cmd_set_boot_slot exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_update(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char *next_cmd2 = NULL;
  unsigned char *next_cmd3 = NULL;
  unsigned char *error = "invalid upgrade source";
  
  DEBUG("CMD: otb_cmd_trigger_update entry");
  
  next_cmd2 = otb_cmd_get_next_cmd(next_cmd);
  if (next_cmd2 != NULL)
  {
    next_cmd3 = otb_cmd_get_next_cmd(next_cmd2);
  }
  rc = otb_rboot_update(next_cmd, next_cmd2, next_cmd3, &error);
  otb_cmd_rsp_append(error);
  
  DEBUG("CMD: otb_cmd_trigger_update exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_control_get_sensor_temp_ds18b20_num(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_control_get_sensor_temp_ds18b20_num entry");

  otb_cmd_rsp_append("%d", otb_ds18b20_count);
  rc = TRUE;
  
  DEBUG("CMD: otb_cmd_control_get_sensor_temp_ds18b20_num exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_control_get_sensor_temp_ds18b20_addr(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int ii;
  
  DEBUG("CMD: otb_cmd_control_get_sensor_temp_ds18b20_addr entry");

  if (next_cmd != NULL)
  {
    ii = atoi(next_cmd);
    if ((ii >= 0) && (ii < OTB_DS18B20_MAX_DS18B20S) && (ii < otb_ds18b20_count))
    {
      otb_cmd_rsp_append(otb_ds18b20_addresses[ii].friendly);
      rc = TRUE;
    }
    else
    {
      otb_cmd_rsp_append("invalid index");
    }
  }
  
  DEBUG("CMD: otb_cmd_control_get_sensor_temp_ds18b20_addr exit");
  
  return rc;

}

char ALIGN4 otb_cmd_reset_error_string[] = "MQTT triggered reset/reboot";
bool ICACHE_FLASH_ATTR otb_cmd_trigger_reset(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_trigger_reset entry");
  
  otb_reset(otb_cmd_reset_error_string);
  otb_cmd_rsp_append("ok");
  rc = TRUE;

  DEBUG("CMD: otb_cmd_trigger_reset exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_trigger_ping(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_trigger_ping entry");
  
  otb_cmd_rsp_append("pong");
  rc = TRUE;
  
  DEBUG("CMD: otb_cmd_trigger_ping exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_config_all(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int jj;
  unsigned char kk = 0;
  
  DEBUG("CMD: otb_cmd_get_config_all entry");
  
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
                       0);  
    otb_cmd_rsp_clear();
  }
  otb_cmd_rsp_append("end");
   
  DEBUG("CMD: otb_cmd_get_config_all exit");
  
  return rc;

}

bool ICACHE_FLASH_ATTR otb_cmd_get_sensor_adc_ads(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  
  DEBUG("CMD: otb_cmd_get_sensor_adc_ads entry");
  
  // XXX not implemented
  otb_cmd_rsp_append("not implemented");
  
  DEBUG("CMD: otb_cmd_get_sensor_adc_ads exit");
  
  return rc;

}

// XXX Dummy
bool ICACHE_FLASH_ATTR xxx(unsigned char *next_cmd, void *arg)
{
  bool rc = FALSE;
  
  DEBUG("CMD: xxx entry");
  
  otb_cmd_rsp_append("%d", system_get_free_heap_size());
  rc = TRUE;
  
  DEBUG("CMD: xxx exit");
  
  return rc;

}


