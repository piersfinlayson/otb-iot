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
 
#ifndef OTB_GPIO_H_INCLUDED
#define OTB_GPIO_H_INCLUDED

typedef enum
{
  OTB_GPIO_STATE_IGNORE,     // gpio_mode_t - no match
  OTB_GPIO_STATE_DISABLE,    // gpio_mode_t GPIO_MODE_DISABLE
  OTB_GPIO_STATE_INPUT,      // gpio_mode_t GPIO_MODE_INPUT
  OTB_GPIO_STATE_OUTPUT,     // gpio_mode_t GPIO_MODE_OUTPUT
  OTB_GPIO_STATE_OUTPUT_OD,  // gpio_mode_t GPIO_MODE_OUTPUT_OD
  OTB_GPIO_STATE_MAX 
} otb_gpio_pin_state_t;

typedef struct 
{
  uint16_t gpio_num;
  otb_gpio_pin_state_t state;
  bool pull_up;
  bool pull_down;
  bool level;
} otb_gpio_state_t;

#define OTB_GPIO_ESP8266_NUM_GPIOS 17

void otb_gpio_init(otb_run_state_t state);
bool otb_gpio_apply_pin_state(otb_gpio_state_t *state);

#ifdef OTB_GPIO_C
const otb_gpio_state_t otb_gpio_state_target_boot[OTB_GPIO_ESP8266_NUM_GPIOS] =
{
  {
    0,
    OTB_GPIO_STATE_OUTPUT_OD,
    FALSE,
    FALSE,
    1
  },
  {
    1,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    2,
    OTB_GPIO_STATE_OUTPUT_OD,
    FALSE,
    FALSE,
    1
  },
  {
    3,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    4,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1
  },
  {
    5,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1
  },
  {
    6,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    7,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    8,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    9,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    10,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    11,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0
  },
  {
    12,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1
  },
  {
    13,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1
  },
  {
    14,
    OTB_GPIO_STATE_INPUT,
    FALSE,
    FALSE,
    0
  },
  {
    15,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1
  },
  {
    16,
    OTB_GPIO_STATE_DISABLE,
    FALSE,
    FALSE,
    0
  }
};
otb_gpio_state_t otb_gpio_state_current[OTB_GPIO_ESP8266_NUM_GPIOS];
#endif

#endif // OTB_GPIO_H_INCLUDED
