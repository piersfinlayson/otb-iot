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

#define OTB_I2C_C
#include "otb.h"

char OTB_FLASH_ATTR *otb_str_i2c_invalid_cmd = "invalid I2C command";    
char OTB_FLASH_ATTR *otb_str_i2c_ads_invalid_cmd = "invalid ADS command";    
char OTB_FLASH_ATTR *otb_str_i2c_ok = "OK";    
char OTB_FLASH_ATTR *otb_str_i2c_init_failed = "init failed";
char OTB_FLASH_ATTR *otb_str_i2c_read_failed_fmt = "read failed 0x%x";
char OTB_FLASH_ATTR *otb_str_i2c_bad_addr = "bad address";
char OTB_FLASH_ATTR *otb_str_i2c_no_addr = "no address";
char OTB_FLASH_ATTR *otb_str_i2c_no_scratch = "ran out of error scratch space";

// Store off whether I2C already initialized
bool otb_i2c_initialized = FALSE;

// timer routines for ADS devices
volatile os_timer_t otb_ads_timer[OTB_CONF_ADS_MAX_ADSS];

// Whether ADS running;
bool otb_i2c_ads_state[OTB_CONF_ADS_MAX_ADSS];

void ICACHE_FLASH_ATTR otb_ads_build_msb_lsb_conf(otb_conf_ads *ads, uint8 *msb, uint8 *lsb)
{
  
  DEBUG("I2C: otb_ads_build_msb_lsb_conf entry");

  // MSB
  // Bit 15        1 = begin conversion
  // Bits 14:12  000 = AINp = AIN0 and AINn = AIN1
  // Bits 11:9   010 = +- 2.048V (001=+-4.096V)
  // Bit 8         0 = Continuous conversion mode
  *msb = 0b10000000 | (ads->mux << 4)| (ads->gain << 1) | ads->cont;

  // LSB
  // Bits 7:5    100 = 128SPS (111=860SPS)
  // Bit 4         0 = Traditiona; comparator
  // Bit 3         0 = Comparator active low
  // Bit 2         0 = Non latching comparator
  // Bits 1:0     11 = Disable comparator
  *lsb = (ads->rate << 5) | (0b00000) | (0b0000) | (0b000) | (0b11);
  
  DEBUG("I2C: msb 0x%02x lsb 0x%02x", *msb, *lsb);

  DEBUG("I2C: otb_ads_build_msb_lsb_conf exit");
  
  return;
}
    
bool ICACHE_FLASH_ATTR otb_ads_configure(otb_conf_ads *ads)
{
  bool rc;
  uint8 msb;
  uint8 lsb;

  DEBUG("I2C: otb_ads_configure entry");
  
  otb_ads_build_msb_lsb_conf(ads, &msb, &lsb);
  rc = otb_i2c_ads_set_cont_mode(ads->addr, msb, lsb);

  DEBUG("I2C: otb_ads_configure exit");
  
  return rc;
}
// Called from main initialization serialization
void ICACHE_FLASH_ATTR otb_ads_initialize(void)
{
  bool rc;
  int ii;
  otb_conf_ads *ads;

  DEBUG("I2C: otb_ads_initialize entry");

  os_memset(otb_i2c_ads_state, 0, sizeof(bool) * OTB_CONF_ADS_MAX_ADSS);

  if (otb_conf->adss > 0)
  {
    // Initialize the I2C bus
    rc = otb_i2c_init();
    if (!rc)
    {
      goto EXIT_LABEL;
    }

    // Configure each ADS    
    for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
    {
      ads = &(otb_conf->ads[ii]);
      if (ads->addr > 0)
      {
        // Configure with appropriate settings
        otb_i2c_ads_state[ii] = otb_ads_configure(ads);
        if (otb_i2c_ads_state[ii])
        {
          // Set ADS into conversion mode
          otb_i2c_ads_state[ii] = otb_i2c_ads_set_read_mode(ads->addr, 0b00000000);
          if (otb_i2c_ads_state[ii])
          {
            // Set up timer function to activate every period
            if (ads->period > 0)
            {
              os_timer_disarm((os_timer_t*)&otb_ads_timer);
              os_timer_setfn((os_timer_t*)&otb_ads_timer, (os_timer_func_t *)otb_i2c_ads_on_timer, ads);
              // Set this timer up to run repeatedly, rather than reschedule each time
              os_timer_arm((os_timer_t*)&otb_ads_timer, ads->period*1000, 1);  
              INFO("I2C: ADS 0x%02x sampling scheduled", ads->addr);
            }
          }
          else
          {
            WARN("I2C: Failed to activate ADS #%d, address 0x%02x", ii, ads->addr);
          }
        }
        else
        {
          WARN("I2C: Failed to configure ADS #%d, address 0x%02x", ii, ads->addr);
        }
      }
      else
      {
        otb_i2c_ads_state[ii] = FALSE;
      }
    }
  }
  else
  {
    INFO("I2C: No ADS/I2C initialization required (no devices)");
    rc = TRUE;
  }
  
EXIT_LABEL:
  if (!rc)
  {
    WARN("I2C: Failed to initialize I2C/ADS");
  }

  DEBUG("I2C: otb_ads_initialize exit");

  return;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_on_timer(void *arg)
{
  bool rc;
  int time_taken;
  otb_conf_ads *ads;
  int16_t val = 0;
#define OTB_I2C_ADS_ON_TIMER_MSG_LEN 64
  char message[OTB_I2C_ADS_ON_TIMER_MSG_LEN];
  char message_mains[OTB_I2C_ADS_ON_TIMER_MSG_LEN];
  int chars=0;
  int chars_mains=0;
  double voltage;
  int voltage_int;
  int voltage_hundreth;
  double current;
  int current_int;
  int current_thou;
  double current_sens;
  int current_sens_int;
  int current_sens_thou;
  int power;
#define OTB_I2C_ADS_ON_TIMER_RESISTOR_VALUE 22.14 // measured with isotech dmm
#define OTB_I2C_ADS_ON_TIMER_TRANSFORMER_TURNS 2000
#define OTB_I2C_ADS_ON_TIMER_MAINS_VOLTAGE 245 // measured with isotech dmm, but will vary

  DEBUG("I2C: otb_i2c_ads_on_timer entry");
  
  // Null term the strings
  message[0] = 0;
  message_mains[0] = 0;
  
  OTB_ASSERT(arg != NULL);
  ads = (otb_conf_ads *)arg;
  
  // Do samples
  if (ads->rms)
  {
    rc = otb_i2c_ads_rms_range(ads->addr, ads->samples, &val, &time_taken);
    chars = os_snprintf(message, OTB_I2C_ADS_ON_TIMER_MSG_LEN, "0x%04x", val);
  }
  else
  {
    rc = otb_i2c_ads_range(ads->addr, ads->samples, &val, &time_taken);
    chars = os_snprintf(message,
                        OTB_I2C_ADS_ON_TIMER_MSG_LEN,
                        "%s0x%04x",
                        (val<0)?"-":"",
                        (val<0)?-val:val);
  }
  
  if (rc)
  {    
    // Calculate the voltage
    OTB_ASSERT(ads->gain < OTB_I2C_ADC_GAIN_VALUES);
    voltage = otb_i2c_ads_gain_to_v[ads->gain] * val * 1000/ 0x8000; // 0x8000 as in differential (+ & -ve mode)
    voltage_int = voltage/1;
    voltage_hundreth = (int)(voltage*100)%100;
    
    // Calculate the current through sensor circuit
    current_sens = voltage / OTB_I2C_ADS_ON_TIMER_RESISTOR_VALUE;
    current_sens_int = current_sens/1;
    current_sens_thou = (int)(current_sens*1000)%1000;

    chars += os_snprintf(message+chars, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars, ":%d.%03dmA", current_sens_int, current_sens_thou);
    chars += os_snprintf(message+chars, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars, ":%d.%02dmV", voltage_int, voltage_hundreth); 
    chars += os_snprintf(message+chars, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars, ":%duS", time_taken); 
    
    // Calculate voltage of mains supply
    // No op at the moment
    
    // Calculate the current flowing through the transformer
    // Note hack - this should be configured
    current = voltage * OTB_I2C_ADS_ON_TIMER_TRANSFORMER_TURNS / OTB_I2C_ADS_ON_TIMER_RESISTOR_VALUE / 1000; // 1000 as mV
    current_int = current/1;
    current_thou = (int)(current*1000)%1000;
    
    // Calculate power P = IV
    // Hack - should detect voltage
    power = OTB_I2C_ADS_ON_TIMER_MAINS_VOLTAGE * current;

    // Build up power output
    chars_mains += os_snprintf(message_mains+chars_mains, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars_mains, "%dW", power);
    chars_mains += os_snprintf(message_mains+chars_mains, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars_mains, ":%d.%03dA", current_int, current_thou);
    chars_mains += os_snprintf(message_mains+chars_mains, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars_mains, ":%d.%02dV", OTB_I2C_ADS_ON_TIMER_MAINS_VOLTAGE, 0);
  
    // ADS ADC measurements (reading, time taken, mV)
    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s/%s/%s/%s/%s",
                OTB_MQTT_ROOT,
                OTB_MQTT_LOCATION_1,
                OTB_MQTT_LOCATION_2,
                OTB_MQTT_LOCATION_3,
                OTB_MAIN_CHIPID,
                OTB_MQTT_ADC,
                ads->loc);
    DEBUG("I2C: Publish topic: %s", otb_mqtt_topic_s);
    DEBUG("I2C:       message: %s", message);
    MQTT_Publish(&otb_mqtt_client, otb_mqtt_topic_s, message, chars, 0, 0);

    os_snprintf(otb_mqtt_topic_s,
                OTB_MQTT_MAX_TOPIC_LENGTH,
                "/%s/%s/%s/%s/%s/%s/%s",
                OTB_MQTT_ROOT,
                OTB_MQTT_LOCATION_1,
                OTB_MQTT_LOCATION_2,
                OTB_MQTT_LOCATION_3,
                OTB_MAIN_CHIPID,
                OTB_MQTT_POWER,
                ads->loc);
    DEBUG("I2C: Publish topic: %s", otb_mqtt_topic_s);
    DEBUG("I2C:       message: %s", message_mains);
    MQTT_Publish(&otb_mqtt_client, otb_mqtt_topic_s, message_mains, chars_mains, 0, 0);
  }
  else
  {
    // ERROR will cause MQTT message to be sent (if connected)
    ERROR("I2C: Failed to collect samples from ADS %d 0x%02x", ads->index, ads->addr);
  }
    
  DEBUG("I2C: otb_i2c_ads_on_timer exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_init()
{
  bool rc = FALSE;

  DEBUG("I2C: otb_i2c_init entry");

  i2c_master_gpio_init();
  rc = TRUE;
  INFO("I2C: I2C bus initialized");

  DEBUG("I2C: otb_i2c_init exit");

  return(rc);
}

char ICACHE_FLASH_ATTR *otb_i2c_mqtt_error_write(char *error)
{
  int len = 0;
  //char buffer[128];
  
  // DEBUG("I2C: otb_i2c_mqtt_error_write entry");
  
  //otb_util_copy_flash_to_ram(buffer, error, 128);
  
  //len = os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%s", buffer);
  otb_i2c_mqtt_error[0] = 0;
  len = os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%s", error);
  
  // DEBUG("I2C: otb_i2c_mqtt_error_write exit");
  
  return(otb_i2c_mqtt_error);
}

bool ICACHE_FLASH_ATTR otb_i2c_mqtt_get_addr(char *byte, uint8 *addr)
{
  uint8 digits[2];
  int ii;
  bool rc = FALSE;

  DEBUG("I2C: otb_i2c_mqtt_get_addr entry");
  
  if (byte != NULL)
  {
    if ((byte[0] != 0) && (byte[1] != 0))
    {
      for (ii = 0; ii < 2; ii++)
      {
        switch(tolower(byte[ii]))
        {
          case '0':
            digits[ii] = 0;
            break;
          case '1':
            digits[ii] = 1;
            break;
          case '2':
            digits[ii] = 2;
            break;
          case '3':
            digits[ii] = 3;
            break;
          case '4':
            digits[ii] = 4;
            break;
          case '5':
            digits[ii] = 5;
            break;
          case '6':
            digits[ii] = 6;
            break;
          case '7':
            digits[ii] = 7;
            break;
          case '8':
            digits[ii] = 8;
            break;
          case '9':
            digits[ii] = 9;
            break;
          case 'a':
            digits[ii] = 0xa;
            break;
          case 'b':
            digits[ii] = 0xb;
            break;
          case 'c':
            digits[ii] = 0xc;
            break;
          case 'd':
            digits[ii] = 0xd;
            break;
          case 'e':
            digits[ii] = 0xe;
            break;
          case 'f':
            digits[ii] = 0xf;
            break;
          default:
            goto EXIT_LABEL;
            break;
        }
      }
    }
    *addr = digits[0];
    *addr = *addr << 4;
    *addr = *addr | digits[1];
    rc = TRUE;
  }
  
EXIT_LABEL:  

  DEBUG("I2C: address 0x%02x", *addr);

  DEBUG("I2C: otb_i2c_mqtt_get_addr exit");

  return(rc);      
}

bool ICACHE_FLASH_ATTR otb_i2c_mqtt_get_num(char *byte, int *num)
{
  uint8 digit;
  int ii;
  bool rc = TRUE;

  DEBUG("I2C: otb_i2c_mqtt_get_num entry");
  
  *num=0;
  if (byte != NULL)
  {
    for (ii = 0; ii < 10; ii++)
    {
      digit = 0;
      if (byte[ii] != 0)
      {
        switch(tolower(byte[ii]))
        {
          case '0':
            digit = 0;
            break;
          case '1':
            digit = 1;
            break;
          case '2':
            digit = 2;
            break;
          case '3':
            digit = 3;
            break;
          case '4':
            digit = 4;
            break;
          case '5':
            digit = 5;
            break;
          case '6':
            digit = 6;
            break;
          case '7':
            digit = 7;
            break;
          case '8':
            digit = 8;
            break;
          case '9':
            digit = 9;
            break;
          default:
            rc = FALSE;
            goto EXIT_LABEL;
            break;
        }
        *num = (*num*10)+digit;
      }
      else
      {
        break;
      }
    }
    rc = TRUE;
  }
  
EXIT_LABEL:  

  DEBUG("I2C: num %d", *num);

  DEBUG("I2C: otb_i2c_mqtt_get_num exit");

  return(rc);      
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_get_binary_val(char *byte, uint8_t *num)
{
  uint8 digit;
  int ii;
  bool rc = TRUE;

  DEBUG("I2C: otb_i2c_mqtt_get_binary_val entry");
  
  *num=0;
  if (byte != NULL)
  {
    for (ii = 0; ii < 8; ii++)
    {
      digit = 0;
      if (byte[ii] != 0)
      {
        switch(tolower(byte[ii]))
        {
          case '0':
            digit = 0;
            break;
          case '1':
            digit = 1;
            break;
          default:
            rc = FALSE;
            goto EXIT_LABEL;
            break;
        }
        *num = (*num*2)+digit;
      }
      else
      {
        break;
      }
    }
    rc = TRUE;
  }
  
EXIT_LABEL:  

  DEBUG("I2C: num %d", *num);

  DEBUG("I2C: otb_i2c_mqtt_get_binary_val exit");

  return(rc);      
}

void ICACHE_FLASH_ATTR otb_i2c_mqtt(char *cmd, char **sub_cmd)
{
  bool rc;
  char *error;
  rc = FALSE;
  uint8 addr;
  
  DEBUG("I2C: otb_i2c_mqtt entry");
  
  if (sub_cmd[0] == NULL)
  {
    rc = FALSE;
    error = otb_i2c_mqtt_error_write(otb_str_i2c_invalid_cmd);
  }
  else if (otb_mqtt_match(sub_cmd[0], OTB_MQTT_CMD_INIT))
  {
    if (otb_i2c_initialized)
    {
      // Don't bother
      rc = TRUE;
    }
    else
    {
      rc = otb_i2c_init();
    }
    if (rc)
    {
      error = "";
    }
    else
    {
      rc = FALSE;
      error = otb_i2c_mqtt_error_write(otb_str_i2c_init_failed);
    }
  }
  else if (otb_mqtt_match(sub_cmd[0], OTB_MQTT_CMD_SCAN))
  {
    int jj;
    int string_len = 0;
    os_strcpy(otb_i2c_mqtt_error, "--");
    for (jj = 0; jj < 0x80; jj++)
    {
      rc = otb_i2c_test(jj);
      if (rc)
      {
        if (string_len > 0)
        {
          string_len += os_snprintf(otb_i2c_mqtt_error + string_len,
                                    OTB_I2C_MQTT_ERROR_LEN - string_len,
                                    ":");
        }
        string_len += os_snprintf(otb_i2c_mqtt_error + string_len,
                                  OTB_I2C_MQTT_ERROR_LEN - string_len,
                                  "0x%02x",
                                  jj);
      }
    }
    rc = TRUE;
    error = otb_i2c_mqtt_error;
  }
  else
  {
    rc = FALSE;
    error = otb_i2c_mqtt_error_write(otb_str_i2c_invalid_cmd);
  }
  
  if (rc)
  {
    otb_mqtt_send_status(cmd, OTB_MQTT_STATUS_OK, error, "");
  }
  else
  {
    otb_mqtt_send_status(cmd, OTB_MQTT_STATUS_ERROR, error, "");
  }

  DEBUG("I2C: otb_i2c_mqtt exit");

}

void ICACHE_FLASH_ATTR otb_i2c_ads_mqtt(char *cmd, char **sub_cmd)
{
  bool rc;
  char *error;
  rc = FALSE;
  uint8 addr;
  
  DEBUG("I2C: otb_i2c_ads_mqtt entry");
  
  error = "";
  if (sub_cmd[1] != NULL)
  {
    rc = otb_i2c_mqtt_get_addr(sub_cmd[0], &addr);
    if (rc)
    {
      if (otb_mqtt_match(sub_cmd[1], OTB_MQTT_CMD_SET))
      {
        if (sub_cmd[2] != NULL)
        {
          if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_CONT_MODE))
          {
            rc = otb_i2c_ads_set_cont_mode(addr, 0b10000010, 0b11100011);
          }
          else if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_READ_CONV_MODE))
          {
            rc = otb_i2c_ads_set_read_mode(addr, 0b00000000);
          }
          else if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_READ_CONF_MODE))
          {
            rc = otb_i2c_ads_set_read_mode(addr, 0b00000001);
          }
          else if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_READ_LO_MODE))
          {
            rc = otb_i2c_ads_set_read_mode(addr, 0b00000010);
          }
          else if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_READ_HIGH_MODE))
          {
            rc = otb_i2c_ads_set_read_mode(addr, 0b00000011);
          }
          else
          {
            rc = FALSE;
            error = otb_i2c_mqtt_error_write(otb_str_i2c_ads_invalid_cmd);
          }
        }
        else
        {
            rc = FALSE;
            error = otb_i2c_mqtt_error_write(otb_str_i2c_ads_invalid_cmd);
        }
      }
      else if (otb_mqtt_match(sub_cmd[1], OTB_MQTT_CMD_GET))
      {
        if (sub_cmd[2] != NULL)
        {
          if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_VALUE))
          {
            int16_t val;
            rc = otb_i2c_ads_read(addr, &val);
            if (rc)
            {
              os_snprintf(otb_i2c_mqtt_error,
                          OTB_I2C_MQTT_ERROR_LEN,
                          "%s0x%04x",
                          (val<0)?"-":"",
                          (val<0)?-val:val);
              error = otb_i2c_mqtt_error;
            }
          }
          else if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_RANGE))
          {
            if (sub_cmd[3] != NULL)
            {
              int num;
              rc = otb_i2c_mqtt_get_num(sub_cmd[3], &num);
              if (rc)
              {
                int16_t val;
                int time_taken;
                rc = otb_i2c_ads_range(addr, num, &val, &time_taken);
                if (rc)
                {
                  os_snprintf(otb_i2c_mqtt_error,
                              OTB_I2C_MQTT_ERROR_LEN,
                              "%s0x%04x",
                              (val<0)?"-":"",
                              (val<0)?-val:val);
                  error = otb_i2c_mqtt_error;
                }
              }
              else
              {
                error = otb_i2c_mqtt_error_write("invalid range");
              }
            }
            else
            {
              error = otb_i2c_mqtt_error_write("invalid range");
            }
          }
          else if (otb_mqtt_match(sub_cmd[2], OTB_MQTT_I2C_ADS_RMS_RANGE))
          {
            if (sub_cmd[3] != NULL)
            {
              int num;
              rc = otb_i2c_mqtt_get_num(sub_cmd[3], &num);
              if (rc)
              {
                int time_taken;
                int16_t val;
                rc = otb_i2c_ads_rms_range(addr, num, &val, &time_taken);
                if (rc)
                {
                  os_snprintf(otb_i2c_mqtt_error,
                              OTB_I2C_MQTT_ERROR_LEN,
                              "0x%04x.%d",
                              val,
                              time_taken);
                  error = otb_i2c_mqtt_error;
                }
              }
              else
              {
                error = otb_i2c_mqtt_error_write("invalid range");
              }
            }
            else
            {
              error = otb_i2c_mqtt_error_write("invalid range");
            }
          }
        }
        else
        {
          rc = FALSE;
          error = otb_i2c_mqtt_error_write(otb_str_i2c_ads_invalid_cmd);
        }
      }
      else
      {
        rc = FALSE;
        error = otb_i2c_mqtt_error_write(otb_str_i2c_ads_invalid_cmd);
      }
    }
    else
    {
      rc = FALSE;
      error = otb_i2c_mqtt_error_write(otb_str_i2c_bad_addr);
    }
  }
  else
  {
    rc = FALSE;
    error = otb_i2c_mqtt_error_write(otb_str_i2c_ads_invalid_cmd);
  }
  
  if (rc)
  {
    otb_mqtt_send_status(cmd, OTB_MQTT_STATUS_OK, error, "");
  }
  else
  {
    otb_mqtt_send_status(cmd, OTB_MQTT_STATUS_ERROR, error, "");
  }

  DEBUG("I2C: otb_i2c_ads_mqtt exit");

}

bool ICACHE_FLASH_ATTR otb_i2c_test(uint8 addr)
{
  bool rc = FALSE;
  
  DEBUG("I2C: otb_i2c_test entry");
  
  i2c_master_start();
  i2c_master_writeByte((addr << 1) | 0b0); // Write
  rc = i2c_master_checkAck();
  i2c_master_stop();
  
  DEBUG("I2C: otb_i2c_test exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_set_cont_mode(uint8 addr, uint8 msb, uint8 lsb)
{
  bool rc = FALSE;

  DEBUG("I2C: otb_i2c_ads_set_cont_mode entry");

  i2c_master_start();
  
  // Write to config register
  i2c_master_writeByte((addr << 1) | 0b0); // Write
  if (!i2c_master_checkAck())
  {
    goto EXIT_LABEL;
    INFO("I2C: No ack");
  }
  i2c_master_writeByte(0b00000001); // Config register
  if (!i2c_master_checkAck())
  {
    goto EXIT_LABEL;
    INFO("I2C: No ack");
  }
  // MSB
  // Bit 15        1 = begin conversion
  // Bits 14:12  000 = AINp = AIN0 and AINn = AIN1
  // Bits 11:9   010 = +- 2.048V (001=+-4.096V)
  // Bit 8         0 = Continuous conversion mode
  i2c_master_writeByte(msb); // MSB 
  if (!i2c_master_checkAck())
  {
    goto EXIT_LABEL;
    INFO("I2C: No ack");
  }
  // LSB
  // Bits 7:5    100 = 128SPS (111=860SPS)
  // Bit 4         0 = Traditiona; comparator
  // Bit 3         0 = Comparator active low
  // Bit 2         0 = Non latching comparator
  // Bits 1:0     11 = Disable comparator
  i2c_master_writeByte(lsb); // LSB
  if (!i2c_master_checkAck())
  {
    goto EXIT_LABEL;
    INFO("I2C: No ack");
  }
  
  rc = TRUE;
  
EXIT_LABEL:  
  
  i2c_master_stop();

  DEBUG("I2C: otb_i2c_ads_set_cont_mode exit");

}

bool ICACHE_FLASH_ATTR otb_i2c_ads_set_read_mode(uint8 addr, uint8 mode)
{
  bool rc = FALSE;
  
  DEBUG("I2C: otb_i2c_ads_allow_reading entry");

  i2c_master_start();
  i2c_master_writeByte((addr << 1) | 0b0); // Write
  if (!i2c_master_checkAck())
  {
    INFO("I2C: No ack");
    goto EXIT_LABEL;
  }
  i2c_master_writeByte(mode);
  if (!i2c_master_checkAck())
  {
    INFO("I2C: No ack");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:

  i2c_master_stop();

  DEBUG("I2C: otb_i2c_ads_allow_reading exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_read(uint8 addr, int16_t *val)
{
  bool rc = FALSE;
  uint8_t val1, val2;
  
  DEBUG("I2C: otb_i2c_ads_read entry");

  // Write to pointer register
  i2c_master_start();
  i2c_master_writeByte((addr << 1) | 0b1); // Read
  if (!i2c_master_checkAck())
  {
    goto EXIT_LABEL;
    INFO("I2C: No ack");
  }
  val1 = i2c_master_readByte();
  i2c_master_send_ack();
  val2 = i2c_master_readByte();
  i2c_master_send_ack();
  *val = (val1 << 8) | val2;
  rc = TRUE;

EXIT_LABEL:

  i2c_master_stop();

  DEBUG("I2C: otb_i2c_ads_read exit");

  return rc;
}

// Came from http://www.codecodex.com/wiki/Calculate_an_integer_square_root
unsigned long isqrt(unsigned long x)  
{  
    register unsigned long op, res, one;  
  
    op = x;  
    res = 0;  
  
    /* "one" starts at the highest power of four <= than the argument. */  
    one = 1 << 30;  /* second-to-top bit set */  
    while (one > op) one >>= 2;  
  
    while (one != 0) {  
        if (op >= res + one) {  
            op -= res + one;  
            res += one << 1;  // <-- faster than 2 * one  
        }  
        res >>= 1;  
        one >>= 2;  
    }  
    return res;  
}  

// Output must be an int as this could be negative
bool ICACHE_FLASH_ATTR otb_i2c_ads_range(uint8 addr, int num, int16_t *result, int *time_taken)
{
  bool rc = FALSE;
  int ii;
  int16_t samples[1024];
  int sum;
  int working;
  uint32 start_time;
  uint32 end_time;
  
  DEBUG("I2C: otb_i2c_ads_range entry");
  
  DEBUG("I2C: Get range %d", num);
  
  if (num > 1024)
  {
    WARN("I2C: Too many samples requested - max is 1024");
    goto EXIT_LABEL;
  }
  
  start_time = system_get_time();
  
  for (ii = 0; ii < num; ii++)
  {
    rc = otb_i2c_ads_read(addr, &(samples[ii]));
    if (!rc)
    {
      WARN("I2C: Failed to read sample num %d", ii+1);
      goto EXIT_LABEL;
    }
    // Worked out experimentally base don 860SPS.
    os_delay_us(398);
  }
  
  end_time = system_get_time();
  
  *time_taken = end_time - start_time;
  
  sum = 0;
  for (ii = 0; ii < num; ii++)
  {
    sum += samples[ii];
  }
  
  working = sum/num;
  *result = (int16_t)working;
  INFO("I2C: Result %d working %d", *result, working);
  // Check we haven't lost any data!
  OTB_ASSERT(*result == working);
  rc = TRUE;

EXIT_LABEL:
  
  DEBUG("I2C: otb_i2c_ads_range exit");
  
  return rc;
}

// Output is a unsigned int as rms so must be positive
bool ICACHE_FLASH_ATTR otb_i2c_ads_rms_range(uint8 addr, int num, uint16_t *result, int *time_taken)
{
  bool rc = FALSE;
  int ii;
  uint32_t samples[1024];
  int16_t sample;
  unsigned long long sum;
  uint32_t working;
  uint32 start_time;
  uint32 end_time;
  
  DEBUG("I2C: otb_i2c_ads_rms_range entry");
  
  DEBUG("I2C: Get range %d", num);
  
  if (num > 1024)
  {
    WARN("I2C: Too many samples requested - max is 1024");
    goto EXIT_LABEL;
  }
  
  start_time = system_get_time();
  
  for (ii = 0; ii < num; ii++)
  {
    rc = otb_i2c_ads_read(addr, &sample);
    if (!rc)
    {
      WARN("I2C: Failed to read sample num %d", ii+1);
      goto EXIT_LABEL;
    }
    samples[ii] = sample * sample;
    // Worked out experimentally base don 860SPS.
    os_delay_us(398);
  }
  
  end_time = system_get_time();
  
  *time_taken = end_time - start_time;
  
  sum = 0;
  for (ii = 0; ii < num; ii++)
  {
    sum += samples[ii];
  }
  
  working = sum/num;
  working = isqrt(working);
  OTB_ASSERT(working < 0xffff);
  *result = (uint16_t)working;
  rc = TRUE;

EXIT_LABEL:
  
  DEBUG("I2C: otb_i2c_ads_rms_range exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_conf_get_addr(uint8_t addr, otb_conf_ads **ads)
{
  bool rc = FALSE;
  int ii;

  DEBUG("I2C: otb_i2c_ads_conf_get_addr entry");
  
  for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
  {
    if (otb_conf->ads[ii].addr == addr)
    {
      *ads = &(otb_conf->ads[ii]);
      rc = TRUE;
      break;
    }
  }
  
  DEBUG("I2C: otb_i2c_ads_conf_get_addr exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_conf_get_loc(char *loc, otb_conf_ads **ads)
{
  bool rc = FALSE;
  int ii;

  DEBUG("I2C: otb_i2c_ads_conf_get_loc entry");

  // Only match a non empty location
  if (loc[0] != 0)
  {
    for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
    {
      if (!(os_strncmp(otb_conf->ads[ii].loc, loc, OTB_CONF_ADS_LOCATION_MAX_LEN)))
      {
        *ads = &(otb_conf->ads[ii]);
        rc = TRUE;
        break;
      }
    }
  }
  
  DEBUG("I2C: otb_i2c_ads_conf_get_loc exit");
  
  return rc;
}

int8_t ICACHE_FLASH_ATTR otb_i2c_ads_conf_field_match(char *field)
{
  int8_t match = -1; 
  int ii;
  
  DEBUG("I2C: otb_i2c_ads_conf_field_match exit");

  for (ii = 0; ii < OTB_MQTT_I2C_ADS_FIELD_LAST_+1; ii++)
  {
    if (otb_mqtt_match(field, otb_mqtt_i2c_ads_fields[ii]))
    {
      match = ii;
      break;
    }
  }

  DEBUG("I2C: otb_i2c_ads_conf_field_match exit");

  return match;
}
  
void ICACHE_FLASH_ATTR otb_i2c_ads_conf_set(char *addr, char *field, char *value)
{
  otb_conf_ads *ads;
  char *response = OTB_MQTT_EMPTY;
  bool rc = FALSE;
  int ii, jj;
  bool ads_match;
  uint8_t addr_b;
  int8_t field_i;
  uint8_t val_b;
  int val_i;

  DEBUG("I2C: otb_i2c_ads_conf_set entry");

  if (addr == NULL)
  {
    rc = FALSE;
    response = "no addr";
    goto EXIT_LABEL;
  }

  // See if command is really to clear
  if (otb_mqtt_match(addr, OTB_MQTT_CMD_SET_CLEAR))
  {
    // Could just null first byte, but this is more privacy conscious, and we have to
    // completely erase and rewrite the flash segment anyway ...
    otb_conf_ads_init(otb_conf);
    rc = TRUE;
    goto EXIT_LABEL;
  }
  
  // Get address
  if (!otb_i2c_mqtt_get_addr(addr, &addr_b) ||
      (addr_b < 0x48) ||
      (addr_b > 0x4b))
  {
    rc = FALSE;
    response = "invalid address";
    goto EXIT_LABEL;
  }
  
  // First of all see if there's already an ADS of this address. If so replace with the
  // provided field/value.
  rc = otb_i2c_ads_conf_get_addr(addr_b, &ads);
  if (!rc)
  {
    // No match, so we need to find a free slot instead
    for (ii = 0, ads = otb_conf->ads; ii < OTB_CONF_ADS_MAX_ADSS; ads++, ii++)
    {
      if (ads->addr == 0)
      {
        INFO("I2C: Use empty slot %d", ii);
        ads->addr = addr_b;
        otb_conf->adss++;
        rc = TRUE;
        break;
      }
    }
  }  
  
  if (!rc)
  {
    // Going to have to reject
    response = "no free slots";
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  // If we've got to here then we've found a suitable ADS, so update the field
  
  // Blank field is actually OK - this will just have set up a slot if there wasn't
  // already so succeed, but indicate no field
  if (field == NULL)
  {
    rc = TRUE;
    response = "no field";
    goto EXIT_LABEL;
  }
  
  // If we get to here with no value then things aren't going to work, as we will need
  // to store something
  if (value == NULL)
  {
    rc = FALSE;
    response = "no value";
    goto EXIT_LABEL;
  }
  
  field_i = otb_i2c_ads_conf_field_match(field);
  DEBUG("CONF: ADS field %d", field_i);
  switch (field_i)
  {
    case OTB_MQTT_I2C_ADS_FIELD_MUX_:
      rc = otb_i2c_ads_get_binary_val(value, &val_b);
      if (!rc || (val_b < 0) || (val_b > 7))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->mux = val_b;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_GAIN_:
      rc = otb_i2c_ads_get_binary_val(value, &val_b);
      if (!rc || (val_b < 0) || (val_b > 7))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->gain = val_b;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_RATE_:
      rc = otb_i2c_ads_get_binary_val(value, &val_b);
      if (!rc || (val_b < 0) || (val_b > 7))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->rate = val_b;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_CONT_:
      rc = otb_i2c_ads_get_binary_val(value, &val_b);
      if (!rc || (val_b < 0) || (val_b > 1))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->cont = val_b;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_RMS_:
      rc = otb_i2c_ads_get_binary_val(value, &val_b);
      if (!rc || (val_b < 0) || (val_b > 1))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->rms = val_b;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_PERIOD_:
      rc = otb_i2c_mqtt_get_num(value, &val_i);
      if (!rc || (val_i < 0) || (val_i > 65535))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->period = val_i;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_SAMPLES_:
      rc = otb_i2c_mqtt_get_num(value, &val_i);
      if (!rc || (val_i < 0) || (val_i > 1024))
      {
        response = "invalid value";
        goto EXIT_LABEL;
      }
      ads->samples = val_i;
      break;

    case OTB_MQTT_I2C_ADS_FIELD_LOC_:
      os_strncpy(ads->loc, value, OTB_CONF_ADS_LOCATION_MAX_LEN);
      ads->loc[OTB_CONF_ADS_LOCATION_MAX_LEN-1] = 0;
      rc = TRUE;
      break;
    
    default:
      rc = FALSE;
      response = "invalid field";
      goto EXIT_LABEL;
      break;
  }

EXIT_LABEL:

  // If successful store off new config
  if (rc)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      response = "failed to store new config";
    }
  }
  
  // Send appropriate response
  if (rc)
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_SET,
                         OTB_MQTT_STATUS_OK,
                         "");
  
  }  
  else
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_SET,
                         OTB_MQTT_STATUS_ERROR,
                         response);
    INFO("CONF: Sent status");
  }
  
  DEBUG("DS18B20: otb_i2c_ads_conf_set exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_conf_get(char *addr, char *field, char *field2)
{
  char *match = NULL;
  int slot_num;
  char *response = NULL;
  bool rc;
  otb_conf_ads *ads = NULL;
  uint8_t addr_b;
  char *real_field = field;
  int8_t field_i;

  DEBUG("I2C: otb_i2c_ads_conf_get entry");
  
  // Check "addr" isn't really an index
  if (addr == NULL)
  {
    match = NULL;
    response = "argument not provided";
    goto EXIT_LABEL;
  }
  
  if (!os_strncmp(addr, OTB_MQTT_CMD_GET_INDEX, os_strlen(OTB_MQTT_CMD_GET_INDEX)))
  {
    if (field == NULL)
    {
      match = NULL;
      response = "no index provided";
      goto EXIT_LABEL;
    }
    // Get slot number
    slot_num = atoi(field);
    DEBUG("I2C: index %d", slot_num);
    if ((slot_num >= 0) && (slot_num < otb_conf->adss))
    {
      ads = otb_conf->ads + slot_num;
      real_field = field2;
    } 
    else
    {
      match = NULL;
      response = "invalid index";
      goto EXIT_LABEL; 
    }
  }
  
  // Not a slot, so sensor may be addr or friendly name - so return the other
  if (!(ads ||
       (otb_i2c_mqtt_get_addr(addr, &addr_b) &&
        (otb_i2c_ads_conf_get_addr(addr_b, &ads) ||
        (otb_i2c_ads_conf_get_loc(addr, &ads))))))
  {
    match = NULL;
    response = "invalid address or loc";
    goto EXIT_LABEL; 
  }
  
  // No field is OK, but just ack the ADS (which we must have found to get here)
  if (real_field == NULL)
  {
    match = "";
    goto EXIT_LABEL;
  }
  
  // If got here have an ADS so can return the info!
  field_i = otb_i2c_ads_conf_field_match(field);
  DEBUG("CONF: ADS field %d", field_i);
  switch (field_i)
  {
    case OTB_MQTT_I2C_ADS_FIELD_MUX_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "0x%x", ads->mux);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_GAIN_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "0x%x", ads->gain);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_RATE_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "0x%x", ads->rate);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_CONT_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "0x%x", ads->cont);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_RMS_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "0x%x", ads->rms);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_PERIOD_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%d", ads->period);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_SAMPLES_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%d", ads->samples);
      break;

    case OTB_MQTT_I2C_ADS_FIELD_LOC_:
      os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%s", ads->loc);
      break;
    
    default:
      rc = FALSE;
      match = "invalid field";
      goto EXIT_LABEL;
      break;
  }
  match = otb_i2c_mqtt_error;
  rc = TRUE;

EXIT_LABEL:
  
  if (match != NULL)
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_GET,
                         OTB_MQTT_STATUS_OK,
                         match);
  }
  else
  {
    if (response == NULL)
    {
      response = "sensor not found";
    }
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_CONFIG,
                         OTB_MQTT_CMD_GET,
                         OTB_MQTT_STATUS_ERROR,
                         response);
  }
  
  DEBUG("I2C: otb_i2c_ads_conf_get exit");
  
  return;
}

