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

#define OTB_RELAY_C
#include "otb.h"

int8_t otb_relay_id;

bool ICACHE_FLASH_ATTR otb_relay_valid_id(unsigned char *to_match)
{
  bool rc = FALSE;
  int8_t value;

  DEBUG("RELAY: otb_relay_valid_id entry");
    
  value = otb_relay_get_index(to_match);
  if (value < 0)
  {
    rc = FALSE;
    otb_cmd_rsp_append("Invalid relay module ID %s", to_match);
    goto EXIT_LABEL;
  }
  
  rc = otb_relay_check_index(value);
  if (!rc)
  {
    otb_cmd_rsp_append("Invalid relay module ID %s", to_match);
    goto EXIT_LABEL;
  }
  
  if (rc)
  {
    otb_relay_id = value;
  }
  
EXIT_LABEL:

  DEBUG("RELAY: otb_relay_valid_id exit");

  return rc;
  
}

int8_t ICACHE_FLASH_ATTR otb_relay_get_index(unsigned char *text)
{
  int8_t value;
  
  DEBUG("RELAY: otb_relay_conf_get_index entry");

  if (text == NULL)
  {
    value = -1;
    goto EXIT_LABEL;
  }
  
  value = atoi(text);
  if ((value == 0) && (text[0] != 0))
  {
    value = -1;
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  DEBUG("RELAY: otb_relay_conf_get_index exit");

  return value;

}

bool ICACHE_FLASH_ATTR otb_relay_check_index(int8_t index)
{
  bool rc = FALSE;

  DEBUG("RELAY: otb_relay_check_index entry");
  
  if ((index > 0) && (index <= OTB_CONF_RELAY_MAX_MODULES))
  {
    rc = TRUE;
  }
  
EXIT_LABEL:
  
  DEBUG("RELAY: otb_relay_check_index exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_relay_trigger(unsigned char *next_cmd,
                                         void *arg,
                                         unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  otb_conf_relay *relay = NULL;
  int8_t index;
  uint8_t id;
  uint8_t value;
  int ivalue;
  uint8_t bvalue;
  uint8_t type;
  unsigned char *next_next_cmd;
    
  DEBUG("RELAY: otb_relay_trigger entry");
  
  // No need to check whether the pin is valid or reserved - this has already been done
  index = otb_relay_id;
  OTB_ASSERT((index > 0) && (index <= OTB_CONF_RELAY_MAX_MODULES));
  id = index-1;
  
  if (next_cmd == NULL)
  {
    rc = FALSE;
    otb_cmd_rsp_append("No value to trigger");
    goto EXIT_LABEL;
  }
  
  relay = &(otb_conf->relay[id]);
  type = relay->type;
  if (type != OTB_CONF_RELAY_TYPE_OTB_0_4)
  {
    rc = FALSE;
    otb_cmd_rsp_append("Unconfigured relay or supported relay type");
  }
  
  // next_cmd will either be "all" to send in a string of binary values for each of the 
  // relays, or <num> which identifies which relay to trigger, followed by a 1 or 0
  if (!os_strcmp("all", next_cmd))
  {
    rc = FALSE;
    otb_cmd_rsp_append("all is currently unsupported");
    goto EXIT_LABEL;
  }
  else
  {
    ivalue = atoi(next_cmd);
    if ((ivalue == 0) && (next_cmd[0] != 0))
    {
      rc = FALSE;
      otb_cmd_rsp_append("Invalid trigger command");
      goto EXIT_LABEL;
    }

    switch(type)
    {
      case OTB_CONF_RELAY_TYPE_OTB_0_4:
        if ((ivalue < 1) || (ivalue > 8))
        {
          rc = FALSE;
          otb_cmd_rsp_append("Invalid relay (1-8)");
          goto EXIT_LABEL;
        }
        
        next_next_cmd = otb_cmd_get_next_cmd(next_cmd);
        if (next_next_cmd == NULL)
        {
          rc = FALSE;
          otb_cmd_rsp_append("No value provided");
          goto EXIT_LABEL;
        }
        
        rc = otb_i2c_ads_get_binary_val(next_next_cmd, &bvalue);
        if ((!rc) || (bvalue > 1))
        {
          rc = FALSE;
          otb_cmd_rsp_append("Invalid relay state");
          goto EXIT_LABEL;
        }
        
        // Phew - now have a relay module (relay), relay itself (ivalue), and desired
        // binary state (bvalue)
        // Note otb-relay has pins 1 to 8 reversed - hence 9-ivalue!
        rc = otb_relay_trigger_relay(relay, (uint8_t)(9-ivalue), bvalue);
        if (!rc)
        {
          rc = FALSE;
          otb_cmd_rsp_append("Failed to trigger relay");
          goto EXIT_LABEL;
        }
        rc = TRUE;
        break;
        
      default:
        OTB_ASSERT(FALSE);
        otb_cmd_rsp_append("Internal error");
        rc = FALSE;
        goto EXIT_LABEL;
        break;
    }    
  }
  
EXIT_LABEL:

  DEBUG("RELAY: otb_relay_trigger exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_relay_trigger_relay(otb_conf_relay *relay, uint8_t num, uint8_t state)
{
  bool rc;
  uint8_t brzo_rc;
  uint8_t i2c_addr;
  uint8_t bytes[5];
  
  DEBUG("RELAY: otb_relay_trigger_relay entry");
  

  i2c_addr = OTB_I2C_PCA9685_BASE_ADDR + relay->addr;

  INFO("RELAY: Trigger otb-relay PCA9685 address 0x%2x num: %d to status: %d", i2c_addr, num, state);

  bytes[0] = OTB_I2C_PCA9685_REG_IO0_ON_L + ((num-1) * 4);
  bytes[1] = 0b0;
  bytes[2] = state ? 0b00010000 : 0;
  bytes[3] = 0b0;
  bytes[4] = state ? 0 : 0b00010000;
  brzo_i2c_start_transaction(i2c_addr, 100);
  brzo_i2c_write(bytes, 5, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (brzo_rc)
  {
    INFO("RELAY: Failed to put otb-relay PCA9685 address 0x%2x num: %d to status: %d, rc: %d", i2c_addr, num, state, brzo_rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  DEBUG("RELAY: otb_relay_trigger_relay exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_relay_conf_set(unsigned char *next_cmd,
                                          void *arg,
                                          unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  otb_conf_relay *relay = NULL;
  int8_t index;
  uint8_t id;
  uint8_t value;
  int ivalue;
  bool store_conf = FALSE;
  uint8_t type;
    
  DEBUG("RELAY: otb_relay_conf_set entry");
  
  // Double check what we're being asked to do is valid
  cmd = (int)arg;
  OTB_ASSERT((cmd >= OTB_CMD_RELAY_MIN) && (cmd < OTB_CMD_RELAY_TOTAL));
  
  // No need to check whether the pin is valid or reserved - this has already been done
  index = otb_relay_id;
  OTB_ASSERT((index > 0) && (index <= OTB_CONF_RELAY_MAX_MODULES));
  id = index-1;
  
  if (next_cmd == NULL)
  {
    rc = FALSE;
    otb_cmd_rsp_append("No value to set");
    goto EXIT_LABEL;
  }
  
  type = otb_conf->relay[id].type;
  
  switch(cmd)
  {
    case OTB_CMD_RELAY_LOC:
      if (os_strnlen(next_cmd, OTB_CONF_RELAY_LOC_LEN) >= OTB_CONF_RELAY_LOC_LEN)
      {
        rc = FALSE;
        otb_cmd_rsp_append("Location string too long");
        goto EXIT_LABEL;
      }
      os_memcpy(otb_conf->relay[id].loc, next_cmd, os_strnlen(next_cmd, OTB_CONF_RELAY_LOC_LEN));
      otb_conf->relay[id].loc[OTB_CONF_RELAY_LOC_LEN] = 0; // Defensive
      rc = TRUE;
      store_conf = TRUE;
      break;
      
    case OTB_CMD_RELAY_TYPE:
      if (!os_strcmp(OTB_CONF_RELAY_TYPE_OTB_0_4_STR, next_cmd))
      {
        otb_conf->relay[id].type = OTB_CONF_RELAY_TYPE_OTB_0_4;
        rc = TRUE;
      }
      else if (!os_strcmp(OTB_CONF_RELAY_TYPE_PCA_STR, next_cmd))
      {
        otb_cmd_rsp_append("Unsupported relay type");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      else if (!os_strcmp(OTB_CONF_RELAY_TYPE_PCF_STR, next_cmd))
      {
        otb_cmd_rsp_append("Unsupported relay type");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      else if (!os_strcmp(OTB_CONF_RELAY_TYPE_MCP_STR, next_cmd))
      {
        otb_cmd_rsp_append("Unsupported relay type");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      else
      {
        otb_cmd_rsp_append("Unsupported relay type");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      
      // Update type now
      type = otb_conf->relay[id].type;
      rc = TRUE;
      store_conf = TRUE;
      break;
      
    case OTB_CMD_RELAY_ADDR:
      if (type == OTB_CONF_RELAY_TYPE_OTB_0_4)
      {
        // Convert string from binary to integer
        rc = otb_i2c_ads_get_binary_val(next_cmd, &value);
        if (!rc)
        {
          otb_cmd_rsp_append("Invalid otb-relay address %s", next_cmd);
          rc = FALSE;
          goto EXIT_LABEL;
        }
        
        // Check it
        if (value > 8)
        {
          otb_cmd_rsp_append("Invalid otb-relay address %s (should be 000-111)", next_cmd);
          rc = FALSE;
          goto EXIT_LABEL;
        }
        
        // Now store it
        otb_conf->relay[id].addr = value;
        rc = TRUE;
        store_conf = TRUE;
      }
      else
      {
        otb_cmd_rsp_append("Set relay type before setting address");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      break;
      
    case OTB_CMD_RELAY_NUM:
      ivalue = atoi(next_cmd);
      if ((ivalue == 0) && (next_cmd[0] != 0))
      {
        rc = FALSE;
        otb_cmd_rsp_append("Invalid number of relays %s", next_cmd);
        goto EXIT_LABEL;
      }
      
      if ((type == OTB_CONF_RELAY_TYPE_OTB_0_4) && ((ivalue < 1) || (ivalue > 8)))
      {
        rc = FALSE;
        otb_cmd_rsp_append("Invalid number of relays %s (1-8) for otb-relay", next_cmd);
        goto EXIT_LABEL;
      }
      
      otb_conf->relay[id].num_relays = ivalue;
      rc = TRUE;
      store_conf = TRUE;
      break;
      
    case OTB_CMD_RELAY_STATUS:
      rc = FALSE;
      otb_cmd_rsp_append("Status invalid for otb-relay");
      goto EXIT_LABEL;
      break;
      
    case OTB_CMD_RELAY_PWR_ON:
      rc = FALSE;
      otb_cmd_rsp_append("Not implemented");
      goto EXIT_LABEL;
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

  // If successful store off new config
  if (store_conf)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }

  DEBUG("RELAY: otb_relay_conf_set exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_relay_configured(void)
{
  bool rc = FALSE;
  int ii;

  DEBUG("RELAY: otb_relay_configured entry");

  for (ii = 0; ii < OTB_CONF_RELAY_MAX_MODULES; ii++)
  {
    if (otb_conf->relay[ii].type != OTB_CONF_RELAY_TYPE_NONE)
    {
      rc = TRUE;
      break;
    }
  }
  
  INFO("RELAY: Have one or more relay modules configured");

  DEBUG("RELAY: otb_relay_configured exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_relay_init(void)
{
  int ii, jj;
  bool dupe;
  
  DEBUG("RELAY: otb_relay_init entry");

  otb_relay_num = 0;
    
  // Initialize internal relay module state
  for (ii = 0; ii < OTB_CONF_RELAY_MAX_MODULES; ii++)
  {
    otb_relay_status[ii].index = -1;
    otb_relay_status[ii].connected = FALSE;
  }

  // Now set up internal relay module state based on config
  for (ii = 0; ii < OTB_CONF_RELAY_MAX_MODULES; ii++)
  {
    dupe = FALSE;
    if (otb_conf->relay[ii].type == OTB_CONF_RELAY_TYPE_OTB_0_4)
    {
      // Check we don't already have this
      for (jj = 0; jj < OTB_CONF_RELAY_MAX_MODULES; jj++)
      {
        if (otb_relay_status[jj].index >= 0)
        {
          if (otb_conf->relay[ii].addr == otb_conf->relay[otb_relay_status[jj].index].addr)
          {
            // Duplicate address
            dupe = TRUE;
            INFO("RELAY: Found dupe relay %d %d %d", ii, jj, otb_relay_status[jj].index);
            break;
          }
        }
      }
      
      if (!dupe)
      {
        DEBUG("RELAY: Setting timer for module %d", ii);
        otb_relay_status[otb_relay_num].index = ii;
        os_timer_disarm((os_timer_t*)&(otb_relay_status[otb_relay_num].timer));
        os_timer_setfn((os_timer_t*)&(otb_relay_status[otb_relay_num].timer),
                       (os_timer_func_t *)otb_relay_on_timer,
                       &(otb_relay_status[otb_relay_num]));
        os_timer_arm((os_timer_t*)&(otb_relay_status[otb_relay_num].timer), 1000, 0);
        otb_relay_num++;
      }
    }
    else
    {
      // Otherwise ignore
    }
  }
  
  if (otb_relay_num > 0)
  {
    INFO("RELAY: Set up %d relay module(s)", otb_relay_num);
  }
  
  DEBUG("RELAY: otb_relay_init exit");
  
  return;
  
}

void ICACHE_FLASH_ATTR otb_relay_on_timer(void *arg)
{
  bool rc = FALSE;
  uint8_t brzo_rc;
  uint8_t bytes[5];
  uint8_t i2c_addr;
  
  otb_relay *relay;
  otb_conf_relay *relay_conf;

  DEBUG("RELAY: otb_relay_on_timer entry");
  
  // This is the initialization routine for relays
  // I2C bus can be assumed to have been initialized
  
  relay = (otb_relay *)arg;
  OTB_ASSERT(relay != NULL);
  OTB_ASSERT(relay->index >= 0);
  OTB_ASSERT(relay->index < OTB_CONF_RELAY_MAX_MODULES);
  relay_conf = &(otb_conf->relay[relay->index]);
  
  os_timer_disarm((os_timer_t*)&(relay->timer));

  if (!relay->connected)
  {
    DEBUG("RELAY: Connect to relay module %d", relay->index);
    
    switch(relay_conf->type)
    {
      case OTB_CONF_RELAY_TYPE_OTB_0_4:
        // Figure out I2C address
        i2c_addr = OTB_I2C_PCA9685_BASE_ADDR + relay_conf->addr;
      
        // Set the mode
        bytes[0] = 0x00; // MODE1 register
        bytes[1] = 0b00100001; // reset = 1, AI = 1, sleep = 0, allcall = 1
        brzo_i2c_start_transaction(i2c_addr, 100);
        brzo_i2c_write(bytes, 2, FALSE);
        brzo_rc = brzo_i2c_end_transaction();
        if (brzo_rc)
        {
          INFO("RELAY: Failed to set otb-relay PCA9685 mode: %d", rc);
          rc = FALSE;
          goto EXIT_LABEL;
        }

        // Set everything to off - should replace this with power on function
        bytes[0] = 0xfa;
        bytes[1] = 0b0;
        bytes[2] = 0b0;
        bytes[3] = 0b0;
        bytes[4] = 0b0;
        brzo_i2c_start_transaction(i2c_addr, 100);
        brzo_i2c_write(bytes, 5, FALSE);
        brzo_rc = brzo_i2c_end_transaction();
        if (brzo_rc)
        {
          INFO("RELAY: Failed to turn on otb-relay PCA9685 status led: %d", rc);
          rc = FALSE;
          goto EXIT_LABEL;
        }

        // Now set status LED to on (was turned off with previous step
        bytes[0] = OTB_I2C_PCA9685_REG_IO0_ON_L + (OTB_RELAY_STATUS_LED_OTB_0_4 * 4);
        bytes[2] = 0b00010000;
        brzo_i2c_start_transaction(i2c_addr, 100);
        brzo_i2c_write(bytes, 5, FALSE);
        brzo_rc = brzo_i2c_end_transaction();
        if (brzo_rc)
        {
          INFO("RELAY: Failed to turn on otb-relay PCA9685 status led: %d", rc);
          rc = FALSE;
          goto EXIT_LABEL;
        }

        relay->connected = TRUE;
        rc = TRUE;
        goto EXIT_LABEL;
        break;
    
      default:
        OTB_ASSERT(FALSE);
        break;
    }
  }
  else
  {
    rc = TRUE;
  }
  
EXIT_LABEL:

  // If failed, try again in 1s
  if (!rc)
  {
    INFO("RELAY: Failed to connect to relay module %d", relay->index);
    os_timer_setfn((os_timer_t*)&(relay->timer),
                   (os_timer_func_t *)otb_relay_on_timer,
                   relay);
    os_timer_arm((os_timer_t*)&(relay->timer), 1000, 0);
  }
  DEBUG("RELAY: otb_relay_on_timer exit");
  
  return;
}
