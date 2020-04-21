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
  gpio_int_type_t int_type;
  gpio_isr_t isr;
  void *isr_arg;
  bool isr_added;
} otb_gpio_state_t;

#define OTB_GPIO_ESP8266_NUM_GPIOS 17

void otb_gpio_init(otb_run_state_t state);
bool otb_gpio_apply_pin_state(otb_gpio_state_t *state);
void otb_gpio_soft_reset_isr(void *arg);

#ifdef OTB_GPIO_C
static bool otb_gpio_isr_installed = FALSE;
static bool otb_gpio_applied_initial_state = FALSE;
const otb_gpio_state_t otb_gpio_state_target_boot[OTB_GPIO_ESP8266_NUM_GPIOS] =
{
  {
    0,
    OTB_GPIO_STATE_OUTPUT_OD,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    1,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    2,
    OTB_GPIO_STATE_OUTPUT_OD,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    3,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    4,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    5,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    6,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    7,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    8,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    9,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    10,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    11,
    OTB_GPIO_STATE_IGNORE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    12,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    13,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    14,
    OTB_GPIO_STATE_INPUT,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_NEGEDGE,
    otb_gpio_soft_reset_isr,
    NULL,
    FALSE,
  },
  {
    15,
    OTB_GPIO_STATE_OUTPUT,
    FALSE,
    FALSE,
    1,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  },
  {
    16,
    OTB_GPIO_STATE_DISABLE,
    FALSE,
    FALSE,
    0,
    GPIO_INTR_DISABLE,
    NULL,
    NULL,
    FALSE,
  }
};
otb_gpio_state_t otb_gpio_state_current[OTB_GPIO_ESP8266_NUM_GPIOS];
#endif

#endif // OTB_GPIO_H_INCLUDED
