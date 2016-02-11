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

bool ICACHE_FLASH_ATTR otb_gpio_get(int pin)
{
  bool rc = FALSE;
  bool special;
  uint8 input;
  
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
  
  ERROR("GPIO: Pin %d state %d", pin, input);
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_get exit");
  
  return rc;
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
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_set exit");
  
  return rc;
}