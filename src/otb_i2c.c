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

#define OTB_I2C_C
#include "otb.h"
#include "brzo_i2c.h"

MLOG("I2C");

#ifdef OTB_RBOOT_BOOTLOADER
#undef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif // OTB_RBOOT_BOOTLOADER

// Internal I2C bus
// Must be declared here so rboot.c doesn't include otb_i2c.h
brzo_i2c_info otb_i2c_bus_internal;
brzo_i2c_info otb_i2c_bus_internal_backup;

char OTB_FLASH_ATTR *otb_str_i2c_invalid_cmd = "invalid I2C command";    
char OTB_FLASH_ATTR *otb_str_i2c_ads_invalid_cmd = "invalid ADS command";    
char OTB_FLASH_ATTR *otb_str_i2c_ok = "OK";    
char OTB_FLASH_ATTR *otb_str_i2c_init_failed = "init failed";
char OTB_FLASH_ATTR *otb_str_i2c_read_failed_fmt = "read failed 0x%x";
char OTB_FLASH_ATTR *otb_str_i2c_bad_addr = "bad address";
char OTB_FLASH_ATTR *otb_str_i2c_no_addr = "no address";
char OTB_FLASH_ATTR *otb_str_i2c_no_scratch = "ran out of error scratch space";

// timer routines for ADS devices
volatile os_timer_t otb_ads_timer[OTB_CONF_ADS_MAX_ADSS];

// Whether ADS running;
bool otb_i2c_ads_state[OTB_CONF_ADS_MAX_ADSS];

otb_i2c_ads_samples *otb_i2c_ads_samples_array[OTB_CONF_ADS_MAX_ADSS] = { 0 };

void ICACHE_FLASH_ATTR otb_i2c_initialize_bus_internal()
{
  
  ENTRY;

  otb_i2c_initialize_bus(&otb_i2c_bus_internal,
                         OTB_I2C_BUS_INTERNAL_SDA_PIN,
                         OTB_I2C_BUS_INTERNAL_SCL_PIN);

  EXIT;
  
}

void ICACHE_FLASH_ATTR otb_i2c_initialize_bus(brzo_i2c_info *info,
                                              uint8_t sda_pin,
                                              uint8_t scl_pin)
{

  ENTRY;
  
  OTB_ASSERT(info != NULL);

  os_memset(info, 0, sizeof(*info));

  info->sda_pin = sda_pin;
  info->scl_pin = scl_pin;

#ifndef ARDUINO
  info->sda_pin_func = pin_func[sda_pin];
  info->scl_pin_func = pin_func[scl_pin];
  info->sda_pin_mux = pin_mux[sda_pin];
  info->scl_pin_mux = pin_mux[scl_pin];
#endif

  info->clock_stretch_time_out_usec = 0;

  brzo_i2c_setup_info(info);

  EXIT;

  return;
}

#ifndef OTB_RBOOT_BOOTLOADER

void ICACHE_FLASH_ATTR otb_i2c_ads_disable_all_timers(void)
{
  otb_i2c_ads_samples *samples;
  int ii;
  
  ENTRY;
  
  os_timer_disarm((os_timer_t*)&otb_ads_timer);
  for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
  {
    if (samples != NULL)
    {
      os_timer_disarm((os_timer_t*)&(samples->timer));
    }
  }
  
  
  EXIT;
}

void ICACHE_FLASH_ATTR otb_ads_build_msb_lsb_conf(otb_conf_ads *ads, uint8 *msb, uint8 *lsb)
{
  
  ENTRY;

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
  
  MDEBUG("msb 0x%02x lsb 0x%02x", *msb, *lsb);

  EXIT;
  
  return;
}
    
bool ICACHE_FLASH_ATTR otb_ads_configure(otb_conf_ads *ads)
{
  bool rc;
  uint8 msb;
  uint8 lsb;

  ENTRY;
  
  otb_ads_build_msb_lsb_conf(ads, &msb, &lsb);
  rc = otb_i2c_ads_set_cont_mode(ads->addr, msb, lsb);

  EXIT;
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_init_samples(otb_conf_ads *ads, otb_i2c_ads_samples *samples)
{

  ENTRY;
  
  OTB_ASSERT(sizeof(otb_i2c_ads_rms_samples) == sizeof(otb_i2c_ads_nonrms_samples));

  // Structure of nonrms samples is the same
  os_memset(samples, 0, sizeof(otb_i2c_ads_samples));
  samples->ads = ads;
  
  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_on_timer(void *arg)
{
  otb_i2c_ads_samples *samples;
  
  ENTRY;
  
  OTB_ASSERT(arg != NULL);
  samples = (otb_i2c_ads_samples *)arg;
  otb_i2c_ads_start_sample(samples);
  
  EXIT;
  
  return;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_sample_timer(void *arg)
{
  otb_i2c_ads_samples *samples;
  
  ENTRY;
  
  OTB_ASSERT(arg != NULL);
  samples = (otb_i2c_ads_samples *)arg;
  otb_i2c_ads_get_sample(samples);
  
  EXIT;
  
  return;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_start_sample(otb_i2c_ads_samples *samples)
{
  bool rc;

  ENTRY;
  
  os_timer_disarm((os_timer_t*)&(samples->timer));
  os_timer_setfn((os_timer_t*)&(samples->timer), (os_timer_func_t *)otb_i2c_ads_sample_timer, samples);

  // Record start time
  samples->time = system_get_time();
  
  // Get a sample (handles errors and if finished)
  rc = otb_i2c_ads_get_sample(samples);

  if (rc)
  {  
    // Run the timer at 860Hz (in fact just less) and repeat
    // We start it here so we don't go for less than 1/860Hz between reads
    os_timer_arm_us((os_timer_t*)&(samples->timer), (1000000/860)+1, 1);  
  }

  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_read_rms_sample(otb_i2c_ads_samples *samples)
{
  bool rc;
  otb_i2c_ads_rms_samples *rms_samples;
  
  ENTRY;
  
  rms_samples = (otb_i2c_ads_rms_samples *)samples;
  rc = otb_i2c_ads_read(samples->ads->addr, &(rms_samples->samples[samples->next_sample]));
  
  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_read_nonrms_sample(otb_i2c_ads_samples *samples)
{
  bool rc;
  otb_i2c_ads_nonrms_samples *nonrms_samples;
  
  ENTRY;
  
  nonrms_samples = (otb_i2c_ads_nonrms_samples *)samples;
  rc = otb_i2c_ads_read(samples->ads->addr, &(nonrms_samples->samples[samples->next_sample]));
  
  EXIT;

  return rc;
}

uint16_t ICACHE_FLASH_ATTR otb_i2c_ads_finish_rms_samples(otb_i2c_ads_samples *samples)
{
  otb_i2c_ads_rms_samples *rms_samples;
  uint16_t val;
  unsigned long long sum;
  int ii;
  uint32_t working;

  ENTRY;

  rms_samples = (otb_i2c_ads_rms_samples *)samples;
  sum = 0;
  for (ii = 0; ii < samples->ads->samples; ii++)
  {
    sum += (rms_samples->samples[ii] * rms_samples->samples[ii]);
  }
  
  working = sum/(samples->ads->samples);
  working = isqrt(working);
  OTB_ASSERT(working < 0xffff);
  val = (uint16_t)working;

  EXIT;
  
  return val;
}

int16_t ICACHE_FLASH_ATTR otb_i2c_ads_finish_nonrms_samples(otb_i2c_ads_samples *samples)
{
  otb_i2c_ads_nonrms_samples *nonrms_samples;
  int16_t val;
  int sum;
  int ii;
  int working;

  ENTRY;

  nonrms_samples = (otb_i2c_ads_nonrms_samples *)samples;
  sum = 0;
  for (ii = 0; ii < samples->ads->samples; ii++)
  {
    sum += nonrms_samples->samples[ii];
  }
  
  working = sum/(samples->ads->samples);
  val = (int16_t)working;

  EXIT;
  
  return val;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_get_sample(otb_i2c_ads_samples *samples)
{
  bool rc = FALSE;
  int time_now;
  otb_i2c_ads_rms_samples rms_samples;
  otb_i2c_ads_nonrms_samples nonrms_samples;
  
  ENTRY;

  // Get a sample
  if (samples->ads->rms)
  {
    rc = otb_i2c_ads_read_rms_sample(samples);
  }
  else
  {
    rc = otb_i2c_ads_read_nonrms_sample(samples);
  }
  
  if (!rc)
  { 
    goto EXIT_LABEL;
  }
  
  if (samples->next_sample > 0)
  {
    // This is a bit hacky - should really do rms and nonrms separately or assert
    // somewhere that offset of samples in samples is same for rms and nonrms (it is).
    // Also note will get lots of dupes for non AC current
    if (((otb_i2c_ads_rms_samples*)samples)->samples[samples->next_sample-1] == ((otb_i2c_ads_rms_samples*)samples)->samples[samples->next_sample])
    {
      samples->dupes++;
    }
  }

  samples->next_sample++;

EXIT_LABEL:

  if (rc)
  {
    // See if finished
    if ((samples->next_sample == 0) || (samples->next_sample >= samples->ads->samples))
    {
      os_timer_disarm((os_timer_t*)&(samples->timer));
      samples->time = system_get_time() - samples->time;
      otb_i2c_ads_finish_sample(samples);
      // Reinitialise samples!    
      otb_i2c_ads_init_samples(samples->ads, samples);
    }
  }
  else
  {
    // Failed, give up this time around - will sent MQTT message if connected
    MERROR("Failed to collect samples from ADS %d 0x%02x", samples->ads->index, samples->ads->addr);
    os_timer_disarm((os_timer_t*)&(samples->timer));
    // Reinitialise samples!    
    otb_i2c_ads_init_samples(samples->ads, samples);
  }

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_i2c_ads_finish_sample(otb_i2c_ads_samples *samples)  
{
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
  int val;
  otb_conf_ads *ads;
#define OTB_I2C_ADS_ON_TIMER_RESISTOR_VALUE 22.14 // measured with isotech dmm
#define OTB_I2C_ADS_ON_TIMER_TRANSFORMER_TURNS 2000
#define OTB_I2C_ADS_ON_TIMER_MAINS_VOLTAGE 245 // measured with isotech dmm, but will vary

  ENTRY;
  
  ads = samples->ads;
  // Different messages if RMS or not (in latter case may be negative)
  if (ads->rms)
  {
    val = otb_i2c_ads_finish_rms_samples(samples);
    chars = os_snprintf(message, OTB_I2C_ADS_ON_TIMER_MSG_LEN, "0x%04x", val);
  }
  else
  {
    val = otb_i2c_ads_finish_nonrms_samples(samples);
    chars = os_snprintf(message,
                        OTB_I2C_ADS_ON_TIMER_MSG_LEN,
                        "%s0x%04x",
                        (val<0)?"-":"",
                        (val<0)?-val:val);
  }

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
  chars += os_snprintf(message+chars, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars, ":%dus", samples->time); 
  chars += os_snprintf(message+chars, OTB_I2C_ADS_ON_TIMER_MSG_LEN-chars, ":%d", samples->dupes); 
  
  // Calculate voltage of mains supply
  // No op at the moment XXX
  
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
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_ADC,
                   ads->loc,
                   message,
                   "",
                   0,
                   0,
                   NULL,
                   0);
  otb_mqtt_publish(&otb_mqtt_client,
                   OTB_MQTT_POWER,
                   ads->loc,
                   message_mains,
                   "",
                   0,
                   0,
                   NULL,
                   0);

  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_init()
{
  bool rc = FALSE;

  ENTRY;

  //i2c_master_gpio_init();
  otb_brzo_i2c_setup(0);
  otb_i2c_initialized = TRUE;
  rc = TRUE;

  EXIT;

  return(rc);
}

char ICACHE_FLASH_ATTR *otb_i2c_mqtt_error_write(char *error)
{
  int len = 0;
  //char buffer[128];
  
  ENTRY;
  
  //otb_util_copy_flash_to_ram(buffer, error, 128);
  
  //len = os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%s", buffer);
  otb_i2c_mqtt_error[0] = 0;
  len = os_snprintf(otb_i2c_mqtt_error, OTB_I2C_MQTT_ERROR_LEN, "%s", error);
  
  EXIT;
  
  return(otb_i2c_mqtt_error);
}

bool ICACHE_FLASH_ATTR otb_i2c_mqtt_get_addr(char *byte, uint8 *addr)
{
  uint8 digits[2];
  int ii;
  bool rc = FALSE;

  ENTRY;
  
  MDEBUG("Decode bytes: %c %c", byte[0], byte[1]);
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

  MDEBUG("address 0x%02x", *addr);

  EXIT;

  return(rc);      
}

bool ICACHE_FLASH_ATTR otb_i2c_mqtt_get_num(char *byte, int *num)
{
  uint8 digit;
  int ii;
  bool rc = TRUE;

  ENTRY;
  
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

  MDEBUG("num %d", *num);

  EXIT;

  return(rc);      
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_get_binary_val(char *byte, uint8_t *num)
{
  uint8 digit;
  int ii;
  bool rc = TRUE;

  ENTRY;
  
  *num=0;
  if (byte != NULL)
  {
    MDETAIL("byte value: %s", byte);
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
  else
  {
    MDETAIL("Null string passed in");
    rc = FALSE;
  }
  
EXIT_LABEL:  

  MDEBUG("num %d", *num);

  EXIT;

  return(rc);      
}

void ICACHE_FLASH_ATTR otb_i2c_mqtt(char *cmd, char **sub_cmd)
{
  bool rc;
  char *error;
  rc = FALSE;
  uint8 addr;
  
  ENTRY;
  
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

  EXIT;

}

void ICACHE_FLASH_ATTR otb_i2c_ads_mqtt(char *cmd, char **sub_cmd)
{
  bool rc;
  char *error;
  rc = FALSE;
  uint8 addr;
  
  ENTRY;
  
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
            else
            {
              error = otb_i2c_mqtt_error_write("failed to read value");
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

  EXIT;

}

bool ICACHE_RAM_ATTR otb_i2c_test(uint8 addr)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  uint8_t buffer[1];
  
  ENTRY;
  
  brzo_i2c_start_transaction(addr, 100);
  buffer[0] = 0b0;
  brzo_i2c_write(buffer, 1, FALSE);
  rc = brzo_i2c_end_transaction();
  
  if (!brzo_rc)
  {
    rc = TRUE;
  }
    
  EXIT;
  
  return(rc);
}

bool ICACHE_RAM_ATTR otb_i2c_ads_set_cont_mode(uint8 addr, uint8 msb, uint8 lsb)
{
  bool rc = FALSE;
  uint8_t brzo_rc;
  uint8_t buffer[3];

  ENTRY;
  
  brzo_i2c_start_transaction(addr, 100);
  buffer[0] = 0b00000001;
  // MSB
  // Bit 15        1 = begin conversion
  // Bits 14:12  000 = AINp = AIN0 and AINn = AIN1
  // Bits 11:9   010 = +- 2.048V (001=+-4.096V)
  // Bit 8         0 = Continuous conversion mode
  buffer[1] = msb;
  // LSB
  // Bits 7:5    100 = 128SPS (111=860SPS)
  // Bit 4         0 = Traditiona; comparator
  // Bit 3         0 = Comparator active low
  // Bit 2         0 = Non latching comparator
  // Bits 1:0     11 = Disable comparator
  buffer[2] = lsb;
  brzo_i2c_write(buffer, 3, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  else
  {
    MERROR("brzo_rc = 0x%02x", brzo_rc);
  }
  
  EXIT;

  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_ads_set_read_mode(uint8 addr, uint8 mode)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;

  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(&mode, 1, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return(rc);
}

bool ICACHE_RAM_ATTR otb_i2c_ads_read(uint8 addr, int16_t *val)
{
  uint8_t brzo_rc; 
  bool rc = FALSE;
  uint8_t buffer[2];
  
  ENTRY;

  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_read(buffer, 2, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    *val = (buffer[0] << 8) | buffer[1];
    rc = TRUE;
  }
  
EXIT_LABEL:

  EXIT;

  return rc;
}

// Came from http://www.codecodex.com/wiki/Calculate_an_integer_square_root
unsigned long ICACHE_FLASH_ATTR isqrt(unsigned long x)  
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
  
  ENTRY;
  
  MDEBUG("Get range %d", num);
  
  if (num > 1024)
  {
    MWARN("Too many samples requested - max is 1024");
    goto EXIT_LABEL;
  }
  
  start_time = system_get_time();
  
  for (ii = 0; ii < num; ii++)
  {
    rc = otb_i2c_ads_read(addr, &(samples[ii]));
    if (!rc)
    {
      MWARN("Failed to read sample num %d", ii+1);
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
  MDETAIL("Result %d working %d", *result, working);
  // Check we haven't lost any data!
  OTB_ASSERT(*result == working);
  rc = TRUE;

EXIT_LABEL:
  
  EXIT;
  
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
  
  ENTRY;
  
  MDEBUG("Get range %d", num);
  
  if (num > 1024)
  {
    MWARN("Too many samples requested - max is 1024");
    goto EXIT_LABEL;
  }
  
  start_time = system_get_time();
  
  for (ii = 0; ii < num; ii++)
  {
    rc = otb_i2c_ads_read(addr, &sample);
    if (!rc)
    {
      MWARN("Failed to read sample num %d", ii+1);
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
  
  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_conf_get_addr(uint8_t addr, otb_conf_ads **ads)
{
  bool rc = FALSE;
  int ii;

  ENTRY;
  
  for (ii = 0; ii < OTB_CONF_ADS_MAX_ADSS; ii++)
  {
    if (otb_conf->ads[ii].addr == addr)
    {
      *ads = &(otb_conf->ads[ii]);
      rc = TRUE;
      break;
    }
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_conf_get_loc(char *loc, otb_conf_ads **ads)
{
  bool rc = FALSE;
  int ii;

  ENTRY;

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
  
  EXIT;
  
  return rc;
}

int8_t ICACHE_FLASH_ATTR otb_i2c_ads_conf_field_match(char *field)
{
  int8_t match = -1; 
  int ii;
  
  EXIT;

  for (ii = 0; ii < OTB_MQTT_I2C_ADS_FIELD_LAST_+1; ii++)
  {
    if (otb_mqtt_match(field, otb_mqtt_i2c_ads_fields[ii]))
    {
      match = ii;
      break;
    }
  }

  EXIT;

  return match;
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_valid_addr(unsigned char *to_match)
{
  bool rc = FALSE;
  uint8_t addr_b;
  
  ENTRY;

  if (!otb_i2c_mqtt_get_addr(to_match, &addr_b) ||
      (addr_b < 0x48) ||
      (addr_b > 0x4b))
  {
    rc = FALSE;
    otb_cmd_rsp_append("invalid ads address");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  otb_i2c_ads_last_addr = addr_b;
  
EXIT_LABEL:

  EXIT;

 return rc;

}

bool ICACHE_FLASH_ATTR otb_i2c_ads_configured_addr(unsigned char *to_match)
{
  bool rc = FALSE;
  uint8_t addr_b;
  otb_conf_ads *ads;

  ENTRY;
  
  if (!otb_i2c_mqtt_get_addr(to_match, &addr_b) ||
      (addr_b < 0x48) ||
      (addr_b > 0x4b))
  {
    rc = FALSE;
    otb_cmd_rsp_append("invalid ads address");
    goto EXIT_LABEL;
  }
  
  rc = otb_i2c_ads_conf_get_addr(addr_b, &ads);
  if (!rc)
  {
    otb_cmd_rsp_append("unconfigured ads address");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  
EXIT_LABEL:
  
  EXIT;
  
  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_conf_set(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  unsigned char *value;
  unsigned char *addr;
  otb_conf_ads *ads = NULL;
  int slot = -1;
  uint8_t addr_b;
  uint8_t val_b;
  int val_i;
  int min;
  int max;
  int val;
  bool byte;
  char *ads_val_b;
  uint16_t *ads_val_i;
  uint8_t type;
  int offset;
    
  ENTRY;

  cmd = (int)arg;
  OTB_ASSERT((cmd >= 0) && (cmd < OTB_CMD_ADS_NUM));
  
  // We've tested the address is valid already
  addr_b = otb_i2c_ads_last_addr;
  if ((addr_b < 0x48) ||
      (addr_b > 0x4b))
  {
    // Shouldn't happen as already tested!
    OTB_ASSERT(FALSE);
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
  // See if there's already an ADS of this address
  rc = otb_i2c_ads_conf_get_addr(addr_b, &ads);
  if (!rc)
  {
    // No match, so we need to find a free slot instead
    ads = NULL;
    for (slot = 0; slot < OTB_CONF_ADS_MAX_ADSS; slot++)
    {
      if (otb_conf->ads[slot].addr == 0)
      {
        MDEBUG("Found empty slot %d", slot);
        ads = otb_conf->ads + slot;
        break;
      }
    }
  }

  // If we haven't found an ADS reject with error
  rc = FALSE;
  if (ads == NULL)
  {
    if (cmd == OTB_CMD_ADS_ADD)
    {
      otb_cmd_rsp_append("no free slots");
    }
    else
    {
      otb_cmd_rsp_append("unconfigured ads");
    }
    goto EXIT_LABEL;
  }
  
  value = next_cmd;

  // Now we're here we can assume we have a slot pointer stored off in ads
  switch(cmd)
  {
    case OTB_CMD_ADS_ADD:
      ads->addr = addr_b;
      otb_conf->adss++;
      rc = TRUE;
      break;

    case OTB_CMD_ADS_LOC:
      os_strncpy(ads->loc, value, OTB_CONF_ADS_LOCATION_MAX_LEN);
      ads->loc[OTB_CONF_ADS_LOCATION_MAX_LEN-1] = 0;
      rc = TRUE;
      break;

    case OTB_CMD_ADS_MUX:
    case OTB_CMD_ADS_RATE:
    case OTB_CMD_ADS_GAIN:
    case OTB_CMD_ADS_CONT:
    case OTB_CMD_ADS_RMS:
    case OTB_CMD_ADS_PERIOD:
    case OTB_CMD_ADS_SAMPLES:
      type = otb_i2c_ads_conf[cmd].type;
      offset = otb_i2c_ads_conf[cmd].offset;
      min = otb_i2c_ads_conf[cmd].min;
      max = otb_i2c_ads_conf[cmd].max;
      if (type == OTB_I2C_ADS_CONF_VAL_TYPE_BYTE)
      {
        rc = otb_i2c_ads_get_binary_val(value, &val_b);
        val = val_b;
      }
      else if (type == OTB_I2C_ADS_CONF_VAL_TYPE_UINT16)
      {
        rc = otb_i2c_mqtt_get_num(value, &val_i);
        val = val_i;
      }
      else
      {
        // Should handle any other type of field separately
        OTB_ASSERT(FALSE);
      }
      
      if (!rc || (val < min) || (val > max))
      {
        MDETAIL("rc: %d min: %d max: %d val: %d", rc, min, max, val);
        otb_cmd_rsp_append("invalid value");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      
      if (type == OTB_I2C_ADS_CONF_VAL_TYPE_BYTE)
      {
        ads_val_b = (char *)(((unsigned char *)ads) + offset);
        *ads_val_b = val;
      }
      else if (type == OTB_I2C_ADS_CONF_VAL_TYPE_UINT16)
      {
        ads_val_i = (uint16_t *)(((unsigned char *)ads) + offset);
        *ads_val_i = val;
      }
      break;

    default:
      OTB_ASSERT(FALSE);
      break;

  }
  
EXIT_LABEL:

  // If successful store off new config
  if (rc)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }

  EXIT;
  
  return rc;
  
}

bool ICACHE_FLASH_ATTR otb_i2c_ads_conf_delete(unsigned char *next_cmd,
                                               void *arg,
                                               unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  otb_conf_ads *ads = NULL;
  char *addr;
  uint8_t addr_b;
  
  ENTRY;
  
  cmd = (int)arg;

  if (cmd == OTB_CMD_ADS_ALL)
  {
    otb_conf_ads_init(otb_conf);
    rc = TRUE;
    goto EXIT_LABEL;
  }
  else if (cmd == OTB_CMD_ADS_ADDR)
  {
    // We've tested the address is configured already, so a bit of asserting
    addr = prev_cmd;
    rc = otb_i2c_mqtt_get_addr(addr, &addr_b);
    OTB_ASSERT(rc);
    rc = otb_i2c_ads_conf_get_addr(addr_b, &ads);
    OTB_ASSERT(rc);
    otb_conf_ads_init_one(ads, ads->index);
    otb_conf->adss--;
    rc = TRUE;
  }
  else
  {
    OTB_ASSERT(FALSE);
  }  
  
EXIT_LABEL:

  // If successful store off new config
  if (rc)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }

  EXIT;
  
  return rc;
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

  ENTRY;
  
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
    MDEBUG("index %d", slot_num);
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
  
  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_i2c_write_one_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
  bool rc;

  ENTRY;
  
  rc = otb_i2c_write_reg_seq(addr, reg, 1, &val);
  
  EXIT;
  
  return rc;
}

// Device must be in write sequential mode!
bool ICACHE_RAM_ATTR otb_i2c_write_reg_seq(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *val)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  int ii;

  ENTRY;

  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(&reg, 1, FALSE);
  brzo_i2c_write(val, count, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_read_one_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
  bool rc;

  ENTRY;
  
  rc = otb_i2c_read_reg_seq(addr, reg, 1, val);
  
  EXIT;
  
  return rc;
}

// Note the device must be in sequential read mode for this to work!
bool ICACHE_RAM_ATTR otb_i2c_read_reg_seq(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *val)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  int ii;
  
  ENTRY;
  
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(&reg, 1, FALSE);
  brzo_i2c_read(val, count, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_write_seq_vals(uint8_t addr, uint8_t count, uint8_t *val)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;
  
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(val, count, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_write_one_val(uint8_t addr, uint8_t val)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;
  
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(&val, 1, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_read_one_val(uint8_t addr, uint8_t *val)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;

  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_read(val, 1, FALSE);
  brzo_rc = brzo_i2c_end_transaction();
  if (!brzo_rc)
  {
    rc = TRUE;
  }

  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_write_one_reg_info(uint8_t addr, uint8_t reg, uint8_t val, brzo_i2c_info *info)
{
  bool rc;

  ENTRY;
  
  rc = otb_i2c_write_reg_seq_info(addr, reg, 1, &val, info);
  
  EXIT;
  
  return rc;
}

// Device must be in write sequential mode!
bool ICACHE_RAM_ATTR otb_i2c_write_reg_seq_info(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *val, brzo_i2c_info *info)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  int ii;

  ENTRY;

  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_write_info(&reg, 1, TRUE, info);
  brzo_i2c_write_info(val, count, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;

  return rc;
}

bool ICACHE_FLASH_ATTR otb_i2c_read_one_reg_info(uint8_t addr, uint8_t reg, uint8_t *val, brzo_i2c_info *info)
{
  bool rc;

  ENTRY;
  
  rc = otb_i2c_read_reg_seq_info(addr, reg, 1, val, info);
  
  EXIT;
  
  return rc;
}

// Note the device must be in sequential read mode for this to work!
bool ICACHE_RAM_ATTR otb_i2c_read_reg_seq_info(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *val, brzo_i2c_info *info)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  int ii;
  
  ENTRY;
  
  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_write_info(&reg, 1, FALSE, info);
  brzo_i2c_read_info(val, count, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_write_seq_vals_info(uint8_t addr, uint8_t count, uint8_t *val, brzo_i2c_info *info)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;
  
  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_write_info(val, count, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_write_one_val_info(uint8_t addr, uint8_t val, brzo_i2c_info *info)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;
  
  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_write_info(&val, 1, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (!brzo_rc)
  {
    rc = TRUE;
  }
  
  EXIT;
  
  return rc;
}

bool ICACHE_RAM_ATTR otb_i2c_read_one_val_info(uint8_t addr, uint8_t *val, brzo_i2c_info *info)
{
  uint8_t brzo_rc;
  bool rc = FALSE;
  
  ENTRY;

  brzo_i2c_start_transaction_info(addr, 100, info);
  brzo_i2c_read_info(val, 1, FALSE, info);
  brzo_rc = brzo_i2c_end_transaction_info(info);
  if (!brzo_rc)
  {
    rc = TRUE;
  }

  EXIT;
  
  return rc;
}

#endif // OTB_RBOOT_BOOTLOADER
