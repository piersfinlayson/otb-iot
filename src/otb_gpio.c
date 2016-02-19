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

  DEBUG("GPIO: otb_gpio_init exit");
  
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

bool ICACHE_FLASH_ATTR otb_gpio_is_reserved(uint8_t pin)
{
  bool rc = FALSE;
  
  DEBUG("GPIO: otb_gpio_is_reserved entry");
  
  if ((pin == OTB_MAIN_GPIO_RESET) ||
      (pin == OTB_DS18B20_DEFAULT_GPIO))
  {
    DEBUG("GPIO: Pin is reserved");
    rc = TRUE;
  }

  DEBUG("GPIO: otb_gpio_is_reserved exit");
  
  return rc;
}

sint8 ICACHE_FLASH_ATTR otb_gpio_get(int pin)
{
  bool special;
  sint8 input = -1;
  
  DEBUG("GPIO: otb_gpio_get entry");

  if (!otb_gpio_is_valid(pin))
  {
    ERROR("GPIO: Can't get pin %d - invalid", pin);
    goto EXIT_LABEL;
  }
  
  if (otb_gpio_is_reserved(pin))
  {
    ERROR("GPIO: Can't get pin %d - reserved", pin);
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
  
  DEBUG("GPIO: otb_gpio_set entry");

  if (!otb_gpio_is_valid(pin))
  {
    ERROR("GPIO: Can't get pin %d - invalid", pin);
    goto EXIT_LABEL;
  }
  
  if (otb_gpio_is_reserved(pin))
  {
    ERROR("GPIO: Can't get pin %d - reserved", pin);
    goto EXIT_LABEL;
  }
  
  INFO("GPIO: Set pin %d value %d", pin, value);
  otb_gpio_pin_io_status[pin-1] = OTB_GPIO_PIN_IO_STATUS_OUTPUT;
  GPIO_OUTPUT_SET(pin, value);
  rc = TRUE;
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_set exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_gpio_mqtt(char *cmd1, char *cmd2, char *cmd3)
{
  int pin;
  int value;
  uint8 cmd_id = OTB_MQTT_CMD_INVALID_;
  bool rc;
  char response[8];

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
  
  pin = atoi(cmd2);
  DEBUG("GPIO: Pin %d", pin);
  if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_SET))
  {
    cmd_id = OTB_MQTT_CMD_SET_;
  }
  else if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_GET))
  {
    cmd_id = OTB_MQTT_CMD_GET_;
  }
  
  switch (cmd_id)
  {
    case OTB_MQTT_CMD_SET_:
      DEBUG("GPIO: Set command");
      if (pin >= GPIO_PIN_NUM)
      {
        INFO("GPIO: Invalid pin num %d", pin);
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             OTB_MQTT_CMD_SET,
                             OTB_MQTT_STATUS_ERROR,
                             "Invalid pin number");
        goto EXIT_LABEL;                         
      }
      if (cmd3 != NULL)
      {
        value = atoi(cmd3);
        DEBUG("GPIO: Set - new value %d", value);
        rc = otb_gpio_set(pin, value); 
        if (rc)
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               OTB_MQTT_CMD_SET,
                               OTB_MQTT_STATUS_OK,
                               "");
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               OTB_MQTT_CMD_SET,
                               OTB_MQTT_STATUS_ERROR,
                               "");
        }
      }
      else
      {
        INFO("GPIO: no value");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             OTB_MQTT_CMD_SET,
                             OTB_MQTT_STATUS_ERROR,
                             "No value");
        goto EXIT_LABEL;                         
      }

      break;
      
    case OTB_MQTT_CMD_GET_:
      DEBUG("GPIO: Get command");
      if (pin >= GPIO_PIN_NUM)
      {
        INFO("GPIO: Invalid pin num %d", pin);
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_ERROR,
                             "Invalid pin number");
        goto EXIT_LABEL;                         
      }

      value = otb_gpio_get(pin);
      if (rc >= 0)
      {
        os_snprintf(response, 8, "%d", value);
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_OK,
                             response);
      }
      else
      {
        INFO("MQTT: Failed to get GPIO");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             OTB_MQTT_CMD_GET,
                             OTB_MQTT_STATUS_ERROR,
                             "");
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
