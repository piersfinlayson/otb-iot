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

uint8_t otb_gpio_pin_io_status[OTB_GPIO_ESP_GPIO_PINS];

void ICACHE_FLASH_ATTR otb_gpio_init(void)
{
  DEBUG("GPIO: otb_gpio_init entry");

  memset(otb_gpio_pin_io_status, 0, OTB_GPIO_ESP_GPIO_PINS);
  gpio_init();
  
  // Initialize all the pins we might use as GPIO.  Pins not included:
  // 1/3 (serial)
  // 6-11 (SPI flash)
  // 16 (reset)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
  otb_gpio_set(0, 0);
  otb_gpio_set(2, 0);
  otb_gpio_set(4, 0);
  otb_gpio_set(5, 0);
  otb_gpio_set(12, 0);
  otb_gpio_set(13, 0);
  // otb_gpio_set is ignored where PIN is reserved so do it manually
  GPIO_OUTPUT_SET(13, 0);
  otb_gpio_set(14, 0);
  otb_gpio_set(15, 0);

  DEBUG("GPIO: otb_gpio_init exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_gpio_apply_boot_state(void)
{
  char *dummy;
  int ii;
  bool rc;

  DEBUG("GPIO: gpio_apply_boot_state entry");

  for (ii = 0; ii < 17; ii++)
  {
    if (!otb_gpio_is_reserved(ii, &dummy))
    {
      rc = otb_gpio_set(ii, otb_conf->gpio_boot_state[ii]);
      if (!rc)
      {
        WARN("GPIO: failed to set boot state for pin %d", ii);
      }
    }
    else
    {
      WARN("GPIO: not applying boot state for reserved pin %d", ii);
    }
  }
  
  DEBUG("GPIO: gpio_apply_boot_state exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_gpio_is_valid(uint8_t pin)
{
  bool rc = TRUE;
  
  DEBUG("GPIO: otb_gpio_is_valid entry");
  
  if (pin >= OTB_GPIO_ESP_GPIO_PINS)
  {
    DEBUG("GPIO: Pin is invalid");
    rc = FALSE;
  }

  DEBUG("GPIO: otb_gpio_is_special exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_gpio_is_reserved(uint8_t pin, char **reserved_text)
{
  bool rc = FALSE;
  
  DEBUG("GPIO: otb_gpio_is_reserved entry");
  
  *reserved_text = "";

  switch (pin)
  {
    case OTB_MAIN_GPIO_RESET:
      DEBUG("GPIO: Pin is reserved");
      *reserved_text = "GPIO pin is reserved for reset";
      rc = TRUE;
      break;

    case OTB_DS18B20_DEFAULT_GPIO:
      DEBUG("GPIO: Pin is reserved");
      *reserved_text = "GPIO pin is reserved for One Wire protocol";
      rc = TRUE;
      break;
      
    case OTB_LED_NEO_PIN:
      DEBUG("GPIO: Pin is reserved");
      *reserved_text = "GPIO pin is reserved for neo pixels";
      rc = TRUE;
      break;
      
    case 1:
    case 3:
      DEBUG("GPIO: Pin is reserved");
      *reserved_text = "GPIO pin is reserved for serial";
      rc = TRUE;
      break;

    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
      DEBUG("GPIO: Pin is reserved");
      *reserved_text = "GPIO pin is reserved for flash";
      rc = TRUE;
      break;

    case 4:
    case 5:
      if (otb_i2c_initialized)
      {
        DEBUG("GPIO: Pin is reserved");
        *reserved_text = "GPIO pin is reserved for I2C bus";
        rc = TRUE;
      }
      break;
      
    default:
      break;
  }

  DEBUG("GPIO: otb_gpio_is_reserved exit");
  
  return rc;
}

sint8 ICACHE_FLASH_ATTR otb_gpio_get(int pin)
{
  bool special;
  sint8 input = -1;
  char *error_text;
  
  DEBUG("GPIO: otb_gpio_get entry");

  if (!otb_gpio_is_valid(pin))
  {
    ERROR("GPIO: Can't get pin %d - invalid", pin);
    goto EXIT_LABEL;
  }
  
  if (otb_gpio_is_reserved(pin, &error_text))
  {
    ERROR("GPIO: Can't get pin %d - %s", error_text);
    goto EXIT_LABEL;
  }
  
  input = GPIO_INPUT_GET(pin);
  
  INFO("GPIO: Pin %d state %d", pin, input);
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_get exit");
  
  return input;
}

bool ICACHE_FLASH_ATTR otb_gpio_set(int pin, int value)
{
  bool rc = FALSE;
  bool special;
  uint8 input;
  char *error_text
  
  DEBUG("GPIO: otb_gpio_set entry");

  if (!otb_gpio_is_valid(pin))
  {
    ERROR("GPIO: Can't get pin %d - invalid", pin);
    goto EXIT_LABEL;
  }
  
  if (otb_gpio_is_reserved(pin, &error_text))
  {
    ERROR("GPIO: Can't get pin %d - %s", pin, error_text);
    goto EXIT_LABEL;
  }
  
  INFO("GPIO: Set pin %d value %d", pin, value);
  // Code used to say pin - 1.  Why???
  otb_gpio_pin_io_status[pin] = OTB_GPIO_PIN_IO_STATUS_OUTPUT;
  GPIO_OUTPUT_SET(pin, value);
  rc = TRUE;
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_set exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_gpio_mqtt(char *cmd1, char *cmd2, char *cmd3)
{
  int pin;
  int value = -1;
  uint8 cmd_id = OTB_MQTT_CMD_INVALID_;
  bool rc;
  char response[8];
  bool save = FALSE;
  char *error_text;
  char *cmd = "";

  DEBUG("GPIO: otb_gpio_cmd entry");

  // Note that cmd1 and cmd2 are colon terminated
  // Supported commands:
  // cmd1 = get/set
  // cmd2 = field
  // cmd3 = value (set only)
  
  if ((cmd1 == NULL) || (cmd2 == NULL))
  {
    INFO("GPIO: Invalid GPIO command");
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                         OTB_MQTT_STATUS_ERROR,
                         "Invalid GPIO command",
                         "");
    goto EXIT_LABEL;
  }
  
  if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_SET))
  {
    cmd = OTB_MQTT_CMD_SET;
    cmd_id = OTB_MQTT_CMD_SET_;
  }
  else if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_GET))
  {
    cmd = OTB_MQTT_CMD_GET;
    cmd_id = OTB_MQTT_CMD_GET_;
  }
  else if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_SAVE))
  {
    cmd = OTB_MQTT_CMD_SAVE;
    cmd_id = OTB_MQTT_CMD_SAVE_;
  }
  
  pin = atoi(cmd2);
  DEBUG("GPIO: Pin %d", pin);
  if ((pin < 0) || (pin >= GPIO_PIN_NUM))
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                         cmd,
                         OTB_MQTT_STATUS_ERROR,
                         "invalid Pin");
  }

  switch (cmd_id)
  {
    case OTB_MQTT_CMD_SAVE_:
      // Deliberately fall through to set.  Will store once we've decided if this GPIO
      // works
      DEBUG("GPIO: Save command");
      save = TRUE;
      if (cmd3 == NULL)
      {
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             cmd,
                             OTB_MQTT_STATUS_ERROR,
                             "no state");
        goto EXIT_LABEL;
        break;
      }
      else
      {
        value = atoi(cmd3);
        if ((value >= 0) && (value <= 1))
        {
          if (!otb_gpio_is_reserved(pin, &error_text))
          {
            if (otb_conf->gpio_boot_state[pin] != value)
            {
              otb_conf->gpio_boot_state[pin] = value;
              rc = otb_conf_update(otb_conf);
              if (!rc)
              {
                ERROR("CONF: Failed to save new boot state");
                otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                                     cmd,
                                     OTB_MQTT_STATUS_ERROR,
                                     "failed to store off new boot state");
                goto EXIT_LABEL;
                break;
              }
            }
          }
          else
          {
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                                 cmd,
                                 OTB_MQTT_STATUS_ERROR,
                                 error_text);
            goto EXIT_LABEL;
            break;
          }
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "invalid state");
          goto EXIT_LABEL;
          break;
        }
      }
      // Fall through to actually set GPIO ...
    case OTB_MQTT_CMD_SET_:
      DEBUG("GPIO: Set command");
      if ((value != -1) || (cmd3 != NULL))
      {
        if (value == -1)
        {
          value = atoi(cmd3);
        }
        DEBUG("GPIO: Set - new value %d", value);
        rc = otb_gpio_set(pin, value); 
        if (rc)
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_OK,
                               "");
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "");
        }
      }
      else
      {
        INFO("GPIO: no value");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             cmd,
                             OTB_MQTT_STATUS_ERROR,
                             "No value");
        goto EXIT_LABEL;                         
      }

      break;
      
    case OTB_MQTT_CMD_GET_:
      DEBUG("GPIO: Get command");
      if (cmd3 != NULL)
      {
        if (otb_mqtt_match(cmd3, OTB_MQTT_BOOT_STATE))
        {
          value = otb_conf->gpio_boot_state[pin];
          os_snprintf(response, 8, "%d", value);
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_OK,
                               response);
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "Unknown get command");
          goto EXIT_LABEL;                         
          break;
        }
      }
      else
      {
        value = otb_gpio_get(pin);
        if (rc >= 0)
        {
          os_snprintf(response, 8, "%d", value);
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_OK,
                               response);
        }
        else
        {
          INFO("MQTT: Failed to get GPIO");
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "");
        }
      }
      break;
    
    default:
      INFO("GPIO: Unsupported command");
      otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                           OTB_MQTT_STATUS_ERROR,
                           "Unsupported command",
                           "");
      goto EXIT_LABEL;    
      break;
  }  

EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_cmd exit");
  
  return;
}
