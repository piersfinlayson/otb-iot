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
 * This program is distributed in the hope that it will be useful, but WITfHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#define OTB_LED_C
#include "otb.h"

bool ICACHE_FLASH_ATTR otb_led_test(unsigned char *name, bool repeat, unsigned char **error_text)
{
  bool rc = FALSE;
  int ii;
  otb_led_type_info *led_type = NULL;
  
  DEBUG("LED: otb_led_test entry");

  *error_text = "";
  for (ii = 0; ii < OTB_LED_TYPE_NUM; ii++)
  {
    if (!os_strcmp(OTB_LED_TYPE_INFO[ii].name, name))
    {
      led_type = OTB_LED_TYPE_INFO + ii;
      break;
    }
  }
  
  if (led_type == NULL)
  {
    *error_text = "invalid led type";
    rc = FALSE;
    goto EXIT_LABEL;
  }
  else
  { 
    if (!led_type->is_gpio || (!otb_gpio_is_reserved(led_type->pin, (char **)error_text)))
    {
      if ((otb_led_test_seq.handle == 0) || (otb_led_test_seq.done))
      {
        otb_led_control_init(&otb_led_test_seq);
        otb_led_test_seq.type = led_type->type;
        otb_led_test_seq.step_array = otb_led_test_steps;
        otb_led_test_seq.steps = 18;
        otb_led_test_seq.afterwards = repeat ? OTB_LED_SEQUENCE_AFTER_RESTART_SEQ : OTB_LED_SEQUENCE_AFTER_OFF;
        otb_led_test_seq.handle = (void *)-1;
        rc = otb_led_control_seq(&otb_led_test_seq);
      }
      else
      {
        rc = FALSE;
        *error_text = "test already in progress";
      }
    }
    else
    {
      rc = FALSE;
    }
  }

EXIT_LABEL:

  DEBUG("LED: otb_led_test exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_led_get_colours(uint32_t colour,
                                           uint8_t *red,
                                           uint8_t *green,
                                           uint8_t *blue)
{
  DEBUG("LED: otb_led_get_colours entry");
    
  // Colour is 0xrrggbb where rr = one byte of hex info for red, gg = one byte of hex
  // for green and bb = one byte of hex for blue
  OTB_ASSERT((colour & 0xff0000) == 0);
  
  *red = (colour >> 16) & 0xff;
  *green = (colour >> 8) & 0xff;
  *blue = colour & 0xff;

  DEBUG("LED: otb_led_get_colours entry");

  return;
}

bool ICACHE_FLASH_ATTR otb_led_control_once(uint8_t type,
                                            uint8_t on,
                                            uint8_t red,
                                            uint8_t green,
                                            uint8_t blue)
{
  otb_led_sequence sseq;
  otb_led_sequence_step sseq_step;
  bool rc;

  DEBUG("LED: otb_led_control_once entry");
  
  // It's only OK to use a seq struct on the stack as we aren't doing a loop
  otb_led_control_init(&sseq);
  sseq.step_array = &sseq_step;
  sseq.steps = 1;
  sseq.afterwards = OTB_LED_SEQUENCE_AFTER_STOP;
  sseq_step.on = on;
  sseq_step.red = red;
  sseq_step.green = green;
  sseq_step.blue = blue;
  rc = otb_led_control_seq(&sseq);
  
  DEBUG("otb_led_control_once exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_led_control_step(otb_led_sequence *seq)
{
  bool rc = FALSE;
  otb_led_sequence_step *step;
  otb_led_type_info *type_info;

  DEBUG("LED: otb_led_control_step entry");
  
  OTB_ASSERT(seq != NULL);
  OTB_ASSERT(seq->steps > 0);
  OTB_ASSERT(seq->type < OTB_LED_TYPE_NUM);

  if (seq->stop)
  {
    // Don't set done to true as we aren't!
    rc = TRUE;
    goto EXIT_LABEL;
  }  
  
  type_info = &OTB_LED_TYPE_INFO[seq->type];
  
  // Get the step to execute
  step = (seq->step_array + seq->next_step);

  if (type_info->is_gpio)
  {
    // Do GPIO - only valid for non RGB LEDs
    OTB_ASSERT(!type_info->is_rgb);
    rc = otb_gpio_set(type_info->pin, step->on ? 1 : 0);
  }
  else
  {
    // Do I2C
    // XXX To implement
  }

  seq->next_step++;

  if (rc)
  {
    if ((seq->next_step == 0) || (seq->next_step >= seq->steps))
    {
      // We have either reached the last step (0xff and overflowed to 0, or max steps)
      seq->done = TRUE;
      
      // Do we need to restart or turn the LED off?
      switch (seq->afterwards)
      {
        case OTB_LED_SEQUENCE_AFTER_OFF:
          if (type_info->is_rgb)
          {
            // Do I2C
            // XXX To implement
          }
          else
          {
            rc = otb_gpio_set(type_info->pin, 0);
          }
          break;
          
        case OTB_LED_SEQUENCE_AFTER_RESTART_SEQ:
          // Reset everything
          seq->done = FALSE;
          seq->iterations++;
          seq->next_step = 0;
          break; 
          
        case OTB_LED_SEQUENCE_AFTER_STOP:
          // Do nothing
          break; 
          
        default:
          OTB_ASSERT(FALSE);
          break;
      }
    }
  }
  
EXIT_LABEL:
  
  // Belt and braces - doesn't hurt
  os_timer_disarm((os_timer_t*)&(seq->timer));

  // rc may have changed above
  if (rc)
  {
    if ((seq->stop) || (seq->done))
    {
      if ((seq->next_step > 1) && (seq->on_done != NULL))
      {
        // Only notify function if there was more than 1 step, and have a function to call
        seq->on_done(seq->handle, TRUE, 0);
      }
    }
    else
    {
      // Set up timer (including if 0)
      os_timer_arm_us((os_timer_t*)&(seq->timer), step->timer_us, 0);  
      //ERROR("Arm for %dus", step->timer_us);
    }
  }
  else
  {
    seq->error = TRUE;
    if ((seq->next_step > 0) && (seq->on_done != NULL))
    {
      // Notify failure
      seq->on_done(seq->handle, FALSE, seq->next_step-1);
    }
  }
  
  DEBUG("LED: otb_led_control_step exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_led_control_on_timer(void *arg)
{
  otb_led_sequence *seq;
  bool rc;

  DEBUG("LED: otb_led_control_on_timer entry");

  OTB_ASSERT(arg != NULL);
  seq = (otb_led_sequence *)arg;
  
  rc = otb_led_control_step(seq);
  
  if (!rc)
  {
    // No need to report done - that's done in otb_led_control_step
    INFO("LED: Hit error when doing something to an LED within callback timer");
  }
  
  DEBUG("LED: otb_led_control_on_timer entry");
  
  return;
}

void ICACHE_FLASH_ATTR otb_led_control_init(otb_led_sequence *seq)
{

  DEBUG("LED: otb_led_control_init entry");
  
  // No need to do anything fancier
  os_memset(seq, 0, sizeof(otb_led_sequence));
  
  // Application must now set up:
  // - type
  // - steps
  // - steps!
  // - afterwards (if default of stop not OK)
  // - on_done (if want callback)
  // - handle (if required for callback)
  
  DEBUG("LED: otb_led_control_init exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_led_control_seq(otb_led_sequence *seq)
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  bool rc = FALSE;
  
  DEBUG("LED: otb_led_control_seq entry");
  
  OTB_ASSERT(seq->type < OTB_LED_TYPE_NUM);
  OTB_ASSERT(seq != NULL);
  OTB_ASSERT(seq->steps > 0);
  OTB_ASSERT(seq->afterwards < OTB_LEB_SEQUENCE_AFTER_NUM);
  
  if (seq->afterwards == OTB_LED_SEQUENCE_AFTER_RESTART_SEQ)
  {
    // Won't notify if restarting after completed!
    OTB_ASSERT(seq->on_done == NULL);
  }
  
  // Initialize private stuff
  seq->next_step = 0;
  seq->done = FALSE;
  seq->iterations = 0;
  seq->error = FALSE;

  // Kick it off
  os_timer_setfn((os_timer_t*)&(seq->timer), (os_timer_func_t *)otb_led_control_on_timer, seq);
  rc = otb_led_control_step(seq);

  DEBUG("LED: otb_led_control_seq exit");
  
  return rc;
}
