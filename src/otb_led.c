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

  if (rgb && (rgb != OTB_LED_NEO_COLOUR_OFF))
  {
    if (store)
    {
      otb_led_wifi_colour = rgb;
    }
    otb_led_wifi_on = TRUE;
  }
  else
  {
    otb_led_wifi_on = FALSE;
  }

  if ((otb_conf != NULL) &&
      ((otb_conf->status_led == OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL) || 
       ((otb_conf->status_led == OTB_CONF_STATUS_LED_BEHAVIOUR_WARN) && (rgb != OTB_LED_NEO_COLOUR_GREEN))))
  {
    // Do nothing
    DEBUG("LED: Leave as above colour");
  }
  else
  {
    // Override rgb - but not that we pretend to users of this API that the LED
    // is turned on/off based on what comes into this function!
    rgb = OTB_LED_NEO_COLOUR_OFF;
    DEBUG("LED: Override to off");
  }

  otb_led_neo_update(&rgb, 1, otb_gpio_pins.status, otb_gpio_pins.status_type, FALSE);

  DEBUG("LED: otb_led_wifi_update exit");

  return;
}

void otb_led_neo_send_0(bool flip, uint32_t pin_mask)
{
  uint32_t high_reg;
  uint32_t low_reg;

  // DEBUG("LED: otb_led_neo_send_0 entry");

  high_reg = flip ? GPIO_OUT_W1TC_ADDRESS : GPIO_OUT_W1TS_ADDRESS;
  low_reg = flip ? GPIO_OUT_W1TS_ADDRESS : GPIO_OUT_W1TC_ADDRESS;

  // High
  GPIO_REG_WRITE(high_reg, pin_mask);
  __asm__ __volatile__
  (
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
  );

  // Low
  GPIO_REG_WRITE(low_reg, pin_mask);
  __asm__ __volatile__
  (
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
  );

  // DEBUG("LED: otb_led_neo_send_0 exit");

  return;
}

void otb_led_neo_send_1(bool flip, uint32_t pin_mask)
{
  uint32_t high_reg;
  uint32_t low_reg;

  // DEBUG("LED: otb_led_neo_send_1 entry");

  high_reg = flip ? GPIO_OUT_W1TC_ADDRESS : GPIO_OUT_W1TS_ADDRESS;
  low_reg = flip ? GPIO_OUT_W1TS_ADDRESS : GPIO_OUT_W1TC_ADDRESS;

  // High
  GPIO_REG_WRITE(high_reg, pin_mask);
  __asm__ __volatile__
  (
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
  );

  // Low
  GPIO_REG_WRITE(low_reg, pin_mask);
  __asm__ __volatile__
  (
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
    "nop;"
  );

  // DEBUG("LED: otb_led_neo_send_1 exit");

  return;
}

void otb_led_neo_update(uint32_t *rgb, int num, uint32_t pin, uint32_t type, bool flip)
{
  int ii, jj;
  uint32_t colour_mask;
  uint32_t ccount;
  uint32_t start_ccount;
  uint32_t pin_mask;
  uint32_t scratch;
  uint32_t pixel;
  uint32_t low_reg;

  DEBUG("LED: otb_led_neo_update entry");

  if (pin == OTB_GPIO_INVALID_PIN)
  {
    DEBUG("LED: Status LED is not configured - not updating");
    goto EXIT_LABEL;
  }


  if (type == OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B)
  {
    // Flip r and g around - WS2812Bs expect green first
    for (ii = 0; ii < num; ii++)
    {
      scratch = rgb[ii] & 0xff; //blue
      scratch += (rgb[ii] & 0xff00) << 8; //green
      scratch += (rgb[ii] & 0xff0000) >> 8; //red
      rgb[ii] = scratch;
    }
  }

  pin_mask = 1 << pin;
  low_reg = flip ? GPIO_OUT_W1TS_ADDRESS : GPIO_OUT_W1TC_ADDRESS;

  ETS_INTR_LOCK();

  // Send the bytes
  for (ii = 0; ii < num; ii++)
  {
    colour_mask = 1 << 23;
    pixel = rgb[ii];
    while (colour_mask)
    {
      colour_mask & pixel ? otb_led_neo_send_1(flip, pin_mask) : otb_led_neo_send_0(flip, pin_mask);
      colour_mask >>= 1;
    }
  }

  // Pull low for 10us to cause devices to latch
  GPIO_REG_WRITE(low_reg, pin_mask);
  os_delay_us(10);

  ETS_INTR_UNLOCK();

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

int rep = 0;
bool ICACHE_FLASH_ATTR otb_led_trigger_sf(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32_t rgb[252];
  int ii;
  
  DEBUG("CMD: otb_led_trigger_sf entry");

  for (ii = 0; ii < 252; ii+=6)
  {
    rgb[ii+((rep+0)%6)] = 0xff0000;
    rgb[ii+((rep+1)%6)] = 0xffff00;
    rgb[ii+((rep+2)%6)] = 0x00ff00;
    rgb[ii+((rep+3)%6)] = 0x00ffff;
    rgb[ii+((rep+4)%6)] = 0x0000ff;
    rgb[ii+((rep+5)%6)] = 0xff00ff;
  }
  rep++;
  
  otb_led_neo_update(rgb, 252, 4, OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B, FALSE);
    
  rc = TRUE;
  
  DEBUG("CMD: otb_led_trigger_sf exit");
  
  return rc;

}

uint32_t ICACHE_FLASH_ATTR otb_led_neo_calc_rainbow(uint32_t colour_start, uint32_t colour_end, int step, uint32_t num, bool o360)
{
  uint32_t result;
  int32_t diff, inc;
  int mult;

  DEBUG("LED: otb_led_neo_calc_rainbow entry");

  OTB_ASSERT(num >= 2);
  OTB_ASSERT(step < num);

  // o360 means starts at starts, goes to end in middle and then back to start!
  mult = o360 ? 2 : 1;
  diff = (colour_end > colour_start) ? colour_end - colour_start : colour_start - colour_end;
  inc = (diff * step * mult) / (num - 1);
  if (!o360)
  {
    result = ((colour_end > colour_start) ? colour_start + inc : colour_start - inc) % 256;
  }
  else
  {
    if ((step * 2) >= num)
    {
      result = ((colour_end > colour_start) ? colour_end - inc : colour_end + inc) % 256;
    }
    else
    {
      result = ((colour_end > colour_start) ? colour_start + inc : colour_start - inc) % 256;
    }
  }

  DEBUG("LED: Colour start: %x", colour_start);
  DEBUG("LED: Colour end:   %x", colour_end);
  DEBUG("LED: Diff:         %x", diff);
  DEBUG("LED: Inc:          %x", inc);
  DEBUG("LED: Result:       %x", result);

  DEBUG("LED: otb_led_neo_calc_rainbow exit");

  return result;
}

otb_led_neo_seq *otb_led_neo_seq_buf = NULL;
bool ICACHE_FLASH_ATTR otb_led_trigger_neo(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  uint32_t cmd = (uint32_t)arg;
  unsigned char *next_cmd2 = NULL;
  unsigned char *next_cmd3 = NULL;
  unsigned char *next_cmd4 = NULL;
  unsigned char *next_cmd5 = NULL;
  bool rc = TRUE;
  int32_t num;
  uint32_t rgb[256];
  int32_t single_rgb = 0;
  int32_t second_rgb = 0;
  int32_t third_rgb = 0;
  int32_t speed = 15;
  int32_t min_speed;
  uint32_t red, green, blue;
  int ii;

  DEBUG("NIXIE: otb_led_trigger_neo entry");

  OTB_ASSERT((cmd >= OTB_CMD_LED_NEO_MIN) && (cmd <= OTB_CMD_LED_NEO_MAX));

  // Get num neo pixels
  if (next_cmd != NULL)
  {
    num = strtol(next_cmd, NULL, 10);
    if ((num < 1) || (num > 256))
    {
      rc = FALSE;
      otb_cmd_rsp_append("Num must be 1-256");
      goto EXIT_LABEL;
    }
  }
  else
  {
    rc = FALSE;
    otb_cmd_rsp_append("Num not specified");
    goto EXIT_LABEL;
  }

  // Get up to 2 more cmds - number varies based on command
  next_cmd2 = otb_cmd_get_next_cmd(next_cmd);
  if (next_cmd2 != NULL)
  {
    next_cmd3 = otb_cmd_get_next_cmd(next_cmd2);
    if (next_cmd3 != NULL)
    {
      next_cmd4 = otb_cmd_get_next_cmd(next_cmd3);
      if (next_cmd4 != NULL)
      {
        next_cmd5 = otb_cmd_get_next_cmd(next_cmd4);
      }
    }
  }

  if ((cmd == OTB_CMD_LED_NEO_SOLID) ||
      (cmd == OTB_CMD_LED_NEO_BOUNCE) ||
      (cmd == OTB_CMD_LED_NEO_ROUND) ||
      (cmd == OTB_CMD_LED_NEO_ROTATE) ||
      (cmd == OTB_CMD_LED_NEO_ROTATEB) ||
      (cmd == OTB_CMD_LED_NEO_RAINBOW) || 
      (cmd == OTB_CMD_LED_NEO_BOUNCER) ||
      (cmd == OTB_CMD_LED_NEO_ROUNDER))
  {
    if (next_cmd2 != NULL)
    {
      single_rgb = strtol(next_cmd2, NULL, 16);
      if ((single_rgb < 0) || (single_rgb > 0xffffff))
      {
        rc = FALSE;
        otb_cmd_rsp_append("colour out of range 000000-ffffff");
        goto EXIT_LABEL;
      }
    }
    else
    {
      rc = FALSE;
      otb_cmd_rsp_append("No first colour provided");
      goto EXIT_LABEL;
    }
    DEBUG("LED: Colour requested: 0x%06x", single_rgb);
  }

  if ((cmd == OTB_CMD_LED_NEO_BOUNCE) ||
      (cmd == OTB_CMD_LED_NEO_ROUND) ||
      (cmd == OTB_CMD_LED_NEO_ROTATE) ||
      (cmd == OTB_CMD_LED_NEO_ROTATEB) ||
      (cmd == OTB_CMD_LED_NEO_RAINBOW) ||
      (cmd == OTB_CMD_LED_NEO_BOUNCER) ||
      (cmd == OTB_CMD_LED_NEO_ROUNDER))
  {
    if (next_cmd3 != NULL)
    {
      second_rgb = strtol(next_cmd3, NULL, 16);
      if ((second_rgb < 0) || (second_rgb > 0xffffff))
      {
        rc = FALSE;
        otb_cmd_rsp_append("colour out of range 000000-ffffff");
        goto EXIT_LABEL;
      }
    }
  }

  if ((cmd == OTB_CMD_LED_NEO_BOUNCE) ||
      (cmd == OTB_CMD_LED_NEO_ROUND) ||
      (cmd == OTB_CMD_LED_NEO_ROTATE) ||
      (cmd == OTB_CMD_LED_NEO_ROTATEB) ||
      (cmd == OTB_CMD_LED_NEO_BOUNCER) ||
      (cmd == OTB_CMD_LED_NEO_ROUNDER))
  {
    if (next_cmd4 != NULL)
    {
      min_speed = (((13 * 24 * num + 100) * 120) / 1000000) + 1; // 13 = us per bit, 24 = bits, num = pixels, 100 = 10us latch time, 120 = add 20%, 1000000 to get in ms, + 1 round up
      DEBUG("LED: Min speed: %d", min_speed);
      speed = strtol(next_cmd4, NULL, 10);
      if ((speed < min_speed) || (speed > 10000))
      {
        rc = FALSE;
        otb_cmd_rsp_append("speed out of range %d-10000 ms", min_speed);
        goto EXIT_LABEL;
      }
    }
    else
    {
      rc = FALSE;
      otb_cmd_rsp_append("No speed and/or second colour provided");
      goto EXIT_LABEL;
    }
    DEBUG("LED: Colour requested: 0x%06x", second_rgb);
  }

  if ((cmd == OTB_CMD_LED_NEO_BOUNCER) || (cmd == OTB_CMD_LED_NEO_ROUNDER))
  {
    if (next_cmd5 != NULL)
    {
      third_rgb = strtol(next_cmd5, NULL, 16);
      if ((third_rgb < 0) || (third_rgb > 0xffffff))
      {
        rc = FALSE;
        otb_cmd_rsp_append("colour out of range 000000-ffffff");
        goto EXIT_LABEL;
      }
    }
    else
    {
      rc = FALSE;
      otb_cmd_rsp_append("Final colour nots provided");
      goto EXIT_LABEL;
    }
  }


  if (otb_led_neo_seq_buf != NULL)
  {
    os_timer_disarm((os_timer_t*)&otb_led_neo_seq_buf->timer);
    otb_led_neo_seq_buf->running = 0;
  }

  switch (cmd)
  {
    case OTB_CMD_LED_NEO_OFF:
      single_rgb = 0;
    case OTB_CMD_LED_NEO_SOLID:
      for (ii = 0; ii < num; ii++)
      {
        rgb[ii] = single_rgb;
      }
      otb_led_neo_update(rgb, num, 4, OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B, FALSE);
      break;

    case OTB_CMD_LED_NEO_RAINBOW:
      if (num < 2)
      {
        rc = FALSE;
        otb_cmd_rsp_append("Rainbow requires at least 2 pixels");
        goto EXIT_LABEL;
      }
      for (ii = 0; ii < num; ii++)
      {
        red = otb_led_neo_calc_rainbow((single_rgb >> 16) & 0xff, (second_rgb >> 16) & 0xff, ii, num, FALSE);
        green = otb_led_neo_calc_rainbow((single_rgb >> 8) & 0xff, (second_rgb >> 8) & 0xff, ii, num, FALSE);
        blue = otb_led_neo_calc_rainbow((single_rgb >> 0) & 0xff, (second_rgb >> 0) & 0xff, ii, num, FALSE);
        rgb[ii] = (red << 16) | (green << 8) | blue;
        DEBUG("LED: r colour: %x", red);
        DEBUG("LED: g colour: %x", green);
        DEBUG("LED: b colour: %x", blue);
        DEBUG("LED: rainbow colour: %06x", rgb[ii]);
      }
      otb_led_neo_update(rgb, num, 4, OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B, FALSE);
      break;

    case OTB_CMD_LED_NEO_BOUNCER:
    case OTB_CMD_LED_NEO_BOUNCE:
    case OTB_CMD_LED_NEO_ROUND:
    case OTB_CMD_LED_NEO_ROUNDER:
    case OTB_CMD_LED_NEO_ROTATE:
    case OTB_CMD_LED_NEO_ROTATEB:
      // Allocate buffer
      if (otb_led_neo_seq_buf == NULL)
      {
        otb_led_neo_seq_buf = (otb_led_neo_seq *)os_malloc(sizeof(*otb_led_neo_seq_buf));
        if (otb_led_neo_seq_buf == NULL)
        {
          rc = FALSE;
          otb_cmd_rsp_append("not enough memory");
          goto EXIT_LABEL;
        }
      }
      os_memset(otb_led_neo_seq_buf, 0, sizeof(*otb_led_neo_seq_buf));

      // Now set up the buf and kick the sequence off
      otb_led_neo_seq_buf->num = num;
      otb_led_neo_seq_buf->next_step = 0;
      otb_led_neo_seq_buf->fwd = 1;
      otb_led_neo_seq_buf->running = 1;
      otb_led_neo_seq_buf->first_colour = single_rgb;
      otb_led_neo_seq_buf->second_colour = second_rgb;
      otb_led_neo_seq_buf->pause = speed; //ms
      if (cmd == OTB_CMD_LED_NEO_BOUNCE)
      {
        otb_led_neo_seq_buf->func = otb_led_neo_bounce;
      }
      else if (cmd == OTB_CMD_LED_NEO_ROUND)
      {
        otb_led_neo_seq_buf->func = otb_led_neo_round;
      }
      else if (cmd == OTB_CMD_LED_NEO_BOUNCER)
      {
        otb_led_neo_seq_buf->func = otb_led_neo_bouncer;
        otb_led_neo_seq_buf->third_colour = third_rgb;
      }
      else if (cmd == OTB_CMD_LED_NEO_ROUNDER)
      {
        otb_led_neo_seq_buf->func = otb_led_neo_rounder;
        otb_led_neo_seq_buf->third_colour = third_rgb;
      }
      else if (cmd == OTB_CMD_LED_NEO_ROTATE)
      {
        otb_led_neo_seq_buf->func = otb_led_neo_rotate;
      }
      else if (cmd == OTB_CMD_LED_NEO_ROTATEB)
      {
        otb_led_neo_seq_buf->func = otb_led_neo_rotateb;
      }
      otb_util_timer_set(&otb_led_neo_seq_buf->timer, 
                         (os_timer_func_t *)otb_led_neo_seq_buf->func,
                         NULL,
                         otb_led_neo_seq_buf->pause,
                         1);
      break;

    default:
      rc = FALSE;
      otb_cmd_rsp_append("internal error");
      goto EXIT_LABEL;
      break;
  }

EXIT_LABEL:

  DEBUG("NIXIE: otb_led_trigger_neo exit");

  return rc;
};

void ICACHE_FLASH_ATTR otb_led_neo_round(void *arg)
{

  DEBUG("LED: otb_led_neo_round entry");

  otb_led_neo_bounce_or_round(arg, FALSE, FALSE);

  DEBUG("LED: otb_led_neo_round exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_bounce(void *arg)
{

  DEBUG("LED: otb_led_neo_bounce entry");

  otb_led_neo_bounce_or_round(arg, TRUE, FALSE);

  DEBUG("LED: otb_led_neo_bounce exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_bouncer(void *arg)
{

  DEBUG("LED: otb_led_neo_bouncer entry");

  otb_led_neo_bounce_or_round(arg, TRUE, TRUE);

  DEBUG("LED: otb_led_neo_bouncer exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_rounder(void *arg)
{

  DEBUG("LED: otb_led_neo_rounder entry");

  otb_led_neo_bounce_or_round(arg, FALSE, TRUE);

  DEBUG("LED: otb_led_neo_rounder exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_rotate(void *arg)
{

  DEBUG("LED: otb_led_neo_rotate entry");

  otb_led_neo_rotate_or_bounce(arg, FALSE);

  DEBUG("LED: otb_led_neo_rotate exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_rotateb(void *arg)
{

  DEBUG("LED: otb_led_neo_rotateb entry");

  otb_led_neo_rotate_or_bounce(arg, TRUE);

  DEBUG("LED: otb_led_neo_rotateb exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_rotate_or_bounce(void *arg, bool bounce)
{
  int ii;
  int num;
  uint32_t red, green, blue;

  DEBUG("LED: otb_led_neo_rotate_or_bounce entry");

  // Set up the colours
  num = otb_led_neo_seq_buf->num;
  if (num == 0)
  {
    num = 256;
  }
  for (ii = 0; ii < num; ii++)
  {
    red = otb_led_neo_calc_rainbow((otb_led_neo_seq_buf->first_colour >> 16) & 0xff, (otb_led_neo_seq_buf->second_colour >> 16) & 0xff, ii, num, TRUE);
    green = otb_led_neo_calc_rainbow((otb_led_neo_seq_buf->first_colour >> 8) & 0xff, (otb_led_neo_seq_buf->second_colour >> 8) & 0xff, ii, num, TRUE);
    blue = otb_led_neo_calc_rainbow((otb_led_neo_seq_buf->first_colour >> 0) & 0xff, (otb_led_neo_seq_buf->second_colour >> 0) & 0xff, ii, num, TRUE);
    otb_led_neo_seq_buf->rgb[(ii+otb_led_neo_seq_buf->next_step)%num] = (red << 16) | (green << 8) | blue;
  }

  // Write the colours
  otb_led_neo_update(otb_led_neo_seq_buf->rgb,
                     num,
                     4,
                     OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B,
                     FALSE);

  // Figure out the next step
  if (!bounce)
  {
    if (otb_led_neo_seq_buf->next_step != (num-1))
    {
      otb_led_neo_seq_buf->next_step++;
    }
    else
    {
      otb_led_neo_seq_buf->next_step = 0;
    }
  }
  else
  {
    if (otb_led_neo_seq_buf->fwd)
    {
      if (otb_led_neo_seq_buf->next_step != (num-1))
      {
        otb_led_neo_seq_buf->next_step++;
      }
      else
      {
          otb_led_neo_seq_buf->next_step--;
          otb_led_neo_seq_buf->fwd = 0;
      }
    }
    else
    {
      if (otb_led_neo_seq_buf->next_step != 0)
      {
        otb_led_neo_seq_buf->next_step--;
      }
      else
      {
        otb_led_neo_seq_buf->next_step++;
        otb_led_neo_seq_buf->fwd = 1;
      }
    }
    
  }

  DEBUG("LED: otb_led_neo_rotate_or_bounce exit");

  return;
}

void ICACHE_FLASH_ATTR otb_led_neo_bounce_or_round(void *arg, bool bounce, bool rainbow)
{
  int ii;
  int num;
  uint32_t red, green, blue;

  DEBUG("LED: otb_led_neo_bounce_or_round entry");

  // Set up the colours
  num = otb_led_neo_seq_buf->num;
  if (num == 0)
  {
    num = 256;
  }
  for (ii = 0; ii < num; ii++)
  {
    if (otb_led_neo_seq_buf->next_step != ii)
    {
      if (!rainbow)
      {
        otb_led_neo_seq_buf->rgb[ii] = otb_led_neo_seq_buf->first_colour;
      }
      else
      {
        red = otb_led_neo_calc_rainbow((otb_led_neo_seq_buf->first_colour >> 16) & 0xff, (otb_led_neo_seq_buf->third_colour >> 16) & 0xff, ii, num, FALSE);
        green = otb_led_neo_calc_rainbow((otb_led_neo_seq_buf->first_colour >> 8) & 0xff, (otb_led_neo_seq_buf->third_colour >> 8) & 0xff, ii, num, FALSE);
        blue = otb_led_neo_calc_rainbow((otb_led_neo_seq_buf->first_colour >> 0) & 0xff, (otb_led_neo_seq_buf->third_colour >> 0) & 0xff, ii, num, FALSE);
        otb_led_neo_seq_buf->rgb[ii] = (red << 16) | (green << 8) | blue;
      }
    }
    else
    {
      otb_led_neo_seq_buf->rgb[ii] = otb_led_neo_seq_buf->second_colour;
    }
  }

  // Write the colours
  otb_led_neo_update(otb_led_neo_seq_buf->rgb,
                     num,
                     4,
                     OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B,
                     FALSE);

  // Figure out the next step
  if (otb_led_neo_seq_buf->fwd)
  {
    if (otb_led_neo_seq_buf->next_step != (num-1))
    {
      otb_led_neo_seq_buf->next_step++;
    }
    else
    {
      if (bounce)
      {
        otb_led_neo_seq_buf->next_step--;
        otb_led_neo_seq_buf->fwd = 0;
      }
      else
      {
        otb_led_neo_seq_buf->next_step = 0;
      }
    }
  }
  else
  {
    if (otb_led_neo_seq_buf->next_step != 0)
    {
      otb_led_neo_seq_buf->next_step--;
    }
    else
    {
      otb_led_neo_seq_buf->next_step++;
      otb_led_neo_seq_buf->fwd = 1;
    }
  }

  DEBUG("LED: otb_led_neo_bounce_or_round exit");

  return;
}
