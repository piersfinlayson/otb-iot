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

#define OTB_GPIO_C
#include "otb.h"

MLOG("otb-gpio");

void otb_gpio_init(otb_run_state_t state)
{
  const otb_gpio_state_t *target;
  otb_gpio_state_t *current;
  bool apply_state;
  bool success;

  ENTRY;

  MDETAIL("Apply gpio state for run state: %d", state);

  current = otb_gpio_state_current;

  // Find state to apply to GPIOs based on run state (for later states
  // retrieve from config)
  switch (state)
  {
    case OTB_RUN_STATE_BOOT:
      target = otb_gpio_state_target_boot;
      memset(current,
             0,
             sizeof(otb_gpio_state_t) * OTB_GPIO_ESP8266_NUM_GPIOS);
      break;
    
    default:
      MERROR("Invalid target state %d", state);
      OTB_ASSERT(FALSE);
      break;
  }

  // Actually apply each GPIO's state
  while (target->gpio_num < OTB_GPIO_ESP8266_NUM_GPIOS)
  {
    apply_state = FALSE;
    switch(target->state)
    {
      case OTB_GPIO_STATE_DISABLE:
      case OTB_GPIO_STATE_INPUT:
      case OTB_GPIO_STATE_OUTPUT:
      case OTB_GPIO_STATE_OUTPUT_OD:
        MDEBUG("Apply state for gpio %d", target->gpio_num);
        apply_state = TRUE;
        break;

      case OTB_GPIO_STATE_IGNORE:
        MDEBUG("Ignore gpio %d", target->gpio_num);
        apply_state = FALSE;
        break;
      
      default:
        MERROR("Invalid target state %d for gpio %d", target->state, target->gpio_num);
        apply_state = FALSE;
        OTB_ASSERT(FALSE);
        break;
    }

    memcpy(current, target, sizeof(*current));

    if (apply_state)
    {
      success = otb_gpio_apply_pin_state(current);
      if (success)
      {
        MDETAIL("Applied state %d for gpio %d", current->state, current->gpio_num);
      }
      else
      {
        MWARN("Failed to apply pin state for gpio %d", current->gpio_num);
      }
    }

    current++;
    target++;
  }

  EXIT;

  return;
}

bool otb_gpio_apply_pin_state(otb_gpio_state_t *state)
{
  bool success;
  gpio_config_t conf;

  ENTRY;

  conf.pin_bit_mask = 1 << state->gpio_num;
  switch (state->state)
  {
    case OTB_GPIO_STATE_DISABLE:
      conf.mode = GPIO_MODE_DISABLE;
      break;

    case OTB_GPIO_STATE_INPUT:
      conf.mode = GPIO_MODE_INPUT;
      break;

    case OTB_GPIO_STATE_OUTPUT:
      conf.mode = GPIO_MODE_OUTPUT;
      break;

    case OTB_GPIO_STATE_OUTPUT_OD:
      conf.mode = GPIO_MODE_OUTPUT_OD;
      break;

    default:
      MERROR("Invalid gpio state %d to apply to gpio %d", state->state, state->gpio_num);
      OTB_ASSERT(FALSE);
      conf.mode = GPIO_MODE_DISABLE;
      break;
  }
  conf.mode = state->state;
  conf.pull_up_en = state->pull_up;
  conf.pull_down_en = state->pull_down;
  conf.intr_type = GPIO_INTR_DISABLE; // XXX allow to be configured
  ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success, gpio_config(&conf));

  if ((conf.mode ==  GPIO_MODE_OUTPUT) ||
      (conf.mode ==  GPIO_MODE_OUTPUT_OD))
  {
    MDEBUG("Apply level %d for gpio %d", state->level, state->gpio_num);
    ESP_ERR_WARN_AND_GOTO_EXIT_LABEL(success,
                                      gpio_set_level(state->gpio_num,
                                                     state->level));
  }

  success = TRUE;

EXIT_LABEL:

  EXIT;

  return success;
}
