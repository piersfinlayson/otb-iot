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
  otb_led_sequence *seq = NULL;
  
  DEBUG("LED: otb_led_test entry");

  *error_text = "";
  for (ii = 0; ii < OTB_LED_TYPE_NUM; ii++)
  {
    if (otb_mqtt_match(name, OTB_LED_TYPE_INFO[ii].name))
    {
      led_type = OTB_LED_TYPE_INFO + ii;
      seq = led_type->seq;
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
      if ((seq->handle == 0) || (seq->done))
      {
        otb_led_control_init(seq);
        seq->type = led_type->type;
        seq->step_array = otb_led_test_steps;
        seq->steps = 18;
        seq->afterwards = repeat ? OTB_LED_SEQUENCE_AFTER_RESTART_SEQ : OTB_LED_SEQUENCE_AFTER_OFF;
        seq->handle = (void *)-1;
        rc = otb_led_control_seq(seq);
      }
      else
      {
        rc = FALSE;
        *error_text = "test already in progress";
      }
    }
    else
    {
      *error_text = "reserved pin";
      rc = FALSE;
    }
  }

EXIT_LABEL:

  DEBUG("LED: otb_led_test exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_led_test_stop(unsigned char *name, unsigned char **error_text)
{
  bool rc = FALSE;
  int ii;
  otb_led_type_info *led_type = NULL;
  otb_led_sequence *seq = NULL;
  
  DEBUG("LED: otb_led_test_stop entry");

  *error_text = "";
  for (ii = 0; ii < OTB_LED_TYPE_NUM; ii++)
  {
    if (otb_mqtt_match(name, OTB_LED_TYPE_INFO[ii].name))
    {
      led_type = OTB_LED_TYPE_INFO + ii;
      seq = led_type->seq;
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
      seq->stop = TRUE;
      rc = TRUE;
    }
    else
    {
      *error_text = "reserved pin";
      rc = FALSE;
    }
  }

EXIT_LABEL:

  DEBUG("LED: otb_led_test_stop exit");
  
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

  type_info = &OTB_LED_TYPE_INFO[seq->type];
  
  // Get the step to execute
  step = (seq->step_array + seq->next_step);

  if (seq->stop)
  {
    // Don't set done to true as we aren't!
    // Need to do this after have found type_info (and step for belt and braces!)
    rc = TRUE;
    goto EXIT_LABEL;
  }  
  
  if (type_info->is_gpio)
  {
    // Do GPIO - only valid for non RGB LEDs
    OTB_ASSERT(!type_info->is_rgb);
    rc = otb_gpio_set(type_info->pin, step->on ? 1 : 0, FALSE);
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
            rc = otb_gpio_set(type_info->pin, 0, FALSE);
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
      if (seq->stop)
      {
        seq->stop = FALSE;
        if (type_info->is_gpio)
        {
          // Really not a lot to do if this fails!
          rc = otb_gpio_set(type_info->pin, 0, FALSE);
        }
        else
        {
          // I2C XXX implement turning it off!
          OTB_ASSERT(FALSE);
        }
        seq->done = TRUE;
      }
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
  seq->stop = FALSE;

  // Kick it off
  os_timer_setfn((os_timer_t*)&(seq->timer), (os_timer_func_t *)otb_led_control_on_timer, seq);
  rc = otb_led_control_step(seq);

  DEBUG("LED: otb_led_control_seq exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_led_wifi_update(uint32_t rgb, bool store)
{

  DEBUG("LED: otb_led_wifi_update entry");

  DEBUG("LED: Update wifi LED 0x%06x", rgb);
  if (store)
  {
    otb_led_wifi_colour = rgb;
  }
  otb_led_neo_update(&rgb, 1, otb_gpio_pins.status);
  if (rgb)
  {
    otb_led_wifi_on = TRUE;
  }
  else
  {
    otb_led_wifi_on = FALSE;
  }

  DEBUG("LED: otb_led_wifi_update exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_update(uint32_t *rgb, int num, uint32_t pin)
{
  int ii, jj;
  uint32_t colour_mask;
  uint32_t ccount;
  int wait_high_cycles;
  int wait_low_cycles;
  uint32_t start_ccount;
  uint32_t pin_mask;

  DEBUG("LED: otb_led_neo_update entry");

  if (pin == OTB_GPIO_INVALID_PIN)
  {
    DEBUG("LED: Status LED is not configured - not updating");
    goto EXIT_LABEL;
  }

  pin_mask = 1 << pin;

  ETS_UART_INTR_DISABLE();

  for (ii = 0; ii < num; ii++)
  {
    colour_mask = 1 << 23;
    while (colour_mask)
    {
      if (colour_mask & rgb[ii])
      {
        wait_high_cycles = OTB_LED_NEO_T1H_CYCLES;
        wait_low_cycles = OTB_LED_NEO_T1L_CYCLES;
      }
      else
      {
        wait_high_cycles = OTB_LED_NEO_T0H_CYCLES;
        wait_low_cycles = OTB_LED_NEO_T0L_CYCLES;
      }
      GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pin_mask);
      start_ccount = otb_util_get_cycle_count();
      for (ccount = otb_util_get_cycle_count();
           ccount - start_ccount < wait_high_cycles;
           ccount = otb_util_get_cycle_count());
      GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pin_mask);
      start_ccount = otb_util_get_cycle_count();
      for (ccount = otb_util_get_cycle_count();
           ccount - start_ccount < wait_low_cycles;
           ccount = otb_util_get_cycle_count());
      colour_mask >>= 1;
    }
  }

  ETS_UART_INTR_ENABLE();

  os_delay_us(50);

EXIT_LABEL:  

  DEBUG("LED: otb_led_neo_update exit");

  return;
}

uint32_t ICACHE_FLASH_ATTR otb_led_neo_get_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
  uint32_t rgb;

  DEBUG("LED: otb_led_neo_get_rgb entry");

  rgb = 0;
  
  rgb = red << 16;
  rgb += green << 8;
  rgb += blue;

  return(rgb);

  DEBUG("LED: otb_led_neo_get_rgb exit");

}

void ICACHE_FLASH_ATTR otb_led_wifi_blink(uint8 times)
{
  DEBUG("LED: otb_led_wifi_blink entry");
  
  otb_led_wifi_blink_times += times;
  // Only store a max number of blinks!
  if (otb_led_wifi_blink_times > OTB_LED_WIFI_BLINK_MAX)
  {
    otb_led_wifi_blink_times = OTB_LED_WIFI_BLINK_MAX;
  }
  otb_led_wifi_disable_blink_timer();
  otb_led_wifi_init_blink_timer();
  
  DEBUG("LED: otb_led_wifi_blink exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_led_wifi_init_blink_timer(void)
{

  DEBUG("LED: otb_led_wifi_init_blink_timer entry");

  otb_util_timer_set((os_timer_t*)&otb_led_wifi_blink_timer, 
                     (os_timer_func_t *)otb_led_wifi_blink_timerfunc,
                     NULL,
                     100,
                     1);

  DEBUG("LED: otb_led_wifi_init_blink_timer exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_led_wifi_disable_blink_timer(void)
{

  DEBUG("LED: otb_led_wifi_disable_blink_timer entry");

  otb_util_timer_cancel((os_timer_t*)&otb_led_wifi_blink_timer);
  // Make sure LED is left lit!
  otb_led_wifi_update(otb_led_wifi_colour, FALSE);

  DEBUG("LED: otb_led_wifi_disable_blink_timer exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_led_wifi_blink_it(void)
{
  uint32_t colour;

  DEBUG("LED: otb_led_wifi_blink entry");

  colour = otb_led_wifi_on ? OTB_LED_NEO_COLOUR_OFF : otb_led_wifi_colour;

  DEBUG("LED: status colour 0x%06x", colour);

  otb_led_wifi_update(colour, FALSE);

  DEBUG("LED: otb_led_wifi_blink exit");
  
  return;
}


void ICACHE_FLASH_ATTR otb_led_wifi_blink_timerfunc(void *arg)
{

  DEBUG("LED: otb_led_wifi_blink_timerfunc entry");
  
  // Blink it!
  otb_led_wifi_blink_it();
  if (otb_led_wifi_on)
  {
    otb_led_wifi_blink_times--;
  }
  if (otb_led_wifi_blink_times <= 0)
  {
    // Done
    otb_led_wifi_disable_blink_timer();
  }
  
  DEBUG("LED: otb_led_wifi_blink_timerfunc exit");
  
  return;
}
