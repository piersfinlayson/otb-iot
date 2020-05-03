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
#include "esp8266/gpio_struct.h"

MLOG("otb-led");

#if 0
bool otb_led_test(unsigned char *name, bool repeat, unsigned char **error_text)
{
  bool rc = FALSE;
  int ii;
  otb_led_type_info *led_type = NULL;
  otb_led_sequence *seq = NULL;
  
  ENTRY;

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

  EXIT;
  
  return rc;
}

bool otb_led_test_stop(unsigned char *name, unsigned char **error_text)
{
  bool rc = FALSE;
  int ii;
  otb_led_type_info *led_type = NULL;
  otb_led_sequence *seq = NULL;
  
  ENTRY;

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

  EXIT;
  
  return rc;
}

void otb_led_get_colours(uint32_t colour,
                                           uint8_t *red,
                                           uint8_t *green,
                                           uint8_t *blue)
{
  ENTRY;
    
  // Colour is 0xrrggbb where rr = one byte of hex info for red, gg = one byte of hex
  // for green and bb = one byte of hex for blue
  OTB_ASSERT((colour & 0xff0000) == 0);
  
  *red = (colour >> 16) & 0xff;
  *green = (colour >> 8) & 0xff;
  *blue = colour & 0xff;

  ENTRY;

  return;
}

bool otb_led_control_once(uint8_t type,
                                            uint8_t on,
                                            uint8_t red,
                                            uint8_t green,
                                            uint8_t blue)
{
  otb_led_sequence sseq;
  otb_led_sequence_step sseq_step;
  bool rc;

  ENTRY;
  
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
  
  EXIT;
  
  return rc;
}

bool otb_led_control_step(otb_led_sequence *seq)
{
  bool rc = FALSE;
  otb_led_sequence_step *step;
  otb_led_type_info *type_info;

  ENTRY;
  
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
      // XXX Should really check xTimerChangePeriod and xTimerStart return codes
      xTimerChangePeriod(seq->timer, TICKS_MS(step->timer_ms), 0);
      xTimerStart(seq->tkimer, 0);
      os_timer_arm((os_timer_t*)&(seq->timer), step->timer_ms, 0);  
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
  
  EXIT;
  
  return rc;
}

void otb_led_control_on_timer(void *arg)
{
  otb_led_sequence *seq;
  bool rc;

  ENTRY;

  OTB_ASSERT(arg != NULL);
  seq = (otb_led_sequence *)arg;
  
  rc = otb_led_control_step(seq);
  
  if (!rc)
  {
    // No need to report done - that's done in otb_led_control_step
    MDETAIL("Hit error when doing something to an LED within callback timer");
  }
  
  ENTRY;
  
  return;
}

void otb_led_control_init(otb_led_sequence *seq)
{

  ENTRY;
  
  // No need to do anything fancier
  os_memset(seq, 0, sizeof(otb_led_sequence));
  
  // Application must now set up:
  // - type
  // - steps
  // - steps!
  // - afterwards (if default of stop not OK)
  // - on_done (if want callback)
  // - handle (if required for callback)
  
  EXIT;
  
  return;
}

bool otb_led_control_seq(otb_led_sequence *seq)
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  bool success = FALSE;
  
  ENTRY;
  
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
  seq->timer = xTimerCreate("otb_led_control_on_timer",
                            TICKS_MS(1000), // Arbitarily - will change later
                            FALSE, // Don't repeat
                            seq,
                            otb_led_control_on_timer);
  if (seq->timer == NULL)
  {
    MWARN("Failed to create timer for LED control sequence");
    success = FALSE;
    goto EXIT_LABEL;
  }
  rc = otb_led_control_step(seq);

EXIT_LABEL:  

  EXIT;
  
  return rc;
}



void otb_led_wifi_update(uint32_t rgb, bool store)
{

  ENTRY;

  MDEBUG("Update wifi LED 0x%06x", rgb);

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
    MDEBUG("Leave as above colour");
  }
  else
  {
    // Override rgb - but not that we pretend to users of this API that the LED
    // is turned on/off based on what comes into this function!
    rgb = OTB_LED_NEO_COLOUR_OFF;
    MDEBUG("Override to off");
  }

  otb_led_neo_update(&rgb, 1, otb_gpio_pins.status, otb_gpio_pins.status_type, FALSE);

  EXIT;

  return;
}

#endif

// rgb[]: [0xRRGGBB, ...]
// num: How many entries in rgb
// pin: Which pin - must alraeady be pulled low (or high if flip selected)
// flip: Reverse polarity - high->low, low->high
void otb_led_neo_update(uint32_t *rgb, int num, uint32_t pin, bool flip)
{
  int ii;
  uint32_t colour_mask;
  uint32_t scratch;
  uint32_t pixel;
  bool __attribute__((unused)) rc;
  uint32_t start_clk, high_clk, total_clk;
  uint32_t pin_mask;
  volatile uint32_t *w1ts, *w1tc;

  ENTRY;

  if (otb_led_neo_lock)
  {
    MERROR("Neo lock held - function not re-entrant");
    goto EXIT_LABEL;
  }

  otb_led_neo_lock = TRUE;

#if 0
  // XXX
  if (pin == OTB_GPIO_INVALID_PIN)
  {
    MDEBUG("Status LED is not configured - not updating");
    goto EXIT_LABEL;
  }
#endif

  // XXX Check pin type
  // Flip r and g around - WS2812Bs expect green first
  for (ii = 0; ii < num; ii++)
  {
    MDEBUG("LED %d RGB: 0x%06x", ii, rgb[ii]);
    scratch = rgb[ii] & 0xff; //blue
    scratch += (rgb[ii] & 0xff00) << 8; //green
    scratch += (rgb[ii] & 0xff0000) >> 8; //red
    rgb[ii] = scratch;
    MDEBUG("LED %d Pixel 0x%06x", ii, rgb[ii]);
  }

  MDEBUG("LED flip: %d", flip);
  w1ts = !flip ? &(GPIO.out_w1ts) : &(GPIO.out_w1tc);
  w1tc = !flip ? &(GPIO.out_w1tc) : &(GPIO.out_w1ts);
  total_clk = OTB_LED_NEO_PERIOD_CLK;
  pin_mask = 0x1 << pin;

  MDEBUG("Pin: %d", pin);
  MDEBUG("Pin Mask: 0x%x", pin_mask);
  MDEBUG("T0H: %d ns", OTB_LED_NEO_T0H_NS);
  MDEBUG("T0H: %d ticks", OTB_LED_NEO_T0H_CLK);
  MDEBUG("T1H: %d ns", OTB_LED_NEO_T1H_NS);
  MDEBUG("T1H: %d ticks", OTB_LED_NEO_T1H_CLK);
  MDEBUG("Period: %d ns", OTB_LED_NEO_PERIOD_NS);
  MDEBUG("Period: %d ticks", OTB_LED_NEO_PERIOD_CLK);

  portENTER_CRITICAL();

  // Various notes on this:
  // - Need to manipulate the GPIO registers directl as gpio_set_level isn't
  //   fast enough
  // - Am assuming clock speed of 80MHz - could query this
  // - Need to access CCOUNT register to do good enough timing
  // - Do as much work during waiting period as possible
  // - This takes about 1400ns per bit, instead of 1250ns specced, but within
  //   tolerance
  // - Can do any logging in critical section, nor to disrupt timing of bit-
  //   banging

  // Pull low briefly before start
  *w1tc |= pin_mask;
  OTB_LED_NEO_WAIT_CLK(soc_get_ccount(), OTB_LED_NEO_START_CLK)

  // Send the bytes
  for (ii = 0; ii < num; ii++)
  {
    colour_mask = 1 << 23;
    pixel = rgb[ii];
    while (colour_mask)
    {
      start_clk = 0;
      soc_set_ccount(start_clk);
      *w1ts |= pin_mask;
      high_clk = colour_mask & pixel ? OTB_LED_NEO_T1H_CLK : OTB_LED_NEO_T0H_CLK;
      OTB_LED_NEO_WAIT_CLK(start_clk, high_clk);
      *w1tc |= pin_mask;
      colour_mask >>= 1;
      OTB_LED_NEO_WAIT_CLK(start_clk, total_clk);
    }
  }

  // Wait for device to latch (already pulled low from end of last bit)
  OTB_LED_NEO_WAIT_CLK(soc_get_ccount(), OTB_LED_NEO_RESET_CLK)

  portEXIT_CRITICAL();

EXIT_LABEL:  

  otb_led_neo_lock = FALSE;

  EXIT;

  return;
}

uint32_t otb_led_neo_get_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
  uint32_t rgb;

  ENTRY;

  rgb = 0;
  
  rgb = red << 16;
  rgb += green << 8;
  rgb += blue;

  EXIT;

  return(rgb);
}

#if 0
void otb_led_wifi_blink(uint8 times)
{
  ENTRY;
  
  otb_led_wifi_blink_times += times;
  // Only store a max number of blinks!
  if (otb_led_wifi_blink_times > OTB_LED_WIFI_BLINK_MAX)
  {
    otb_led_wifi_blink_times = OTB_LED_WIFI_BLINK_MAX;
  }
  otb_led_wifi_disable_blink_timer();
  otb_led_wifi_init_blink_timer();
  
  EXIT;
  
  return;
}

void otb_led_wifi_init_blink_timer(void)
{

  ENTRY;

  otb_util_timer_set((os_timer_t*)&otb_led_wifi_blink_timer, 
                     (os_timer_func_t *)otb_led_wifi_blink_timerfunc,
                     NULL,
                     100,
                     1);

  EXIT;
  
  return;
}

void otb_led_wifi_disable_blink_timer(void)
{

  ENTRY;

  otb_util_timer_cancel((os_timer_t*)&otb_led_wifi_blink_timer);
  // Make sure LED is left lit!
  otb_led_wifi_update(otb_led_wifi_colour, FALSE);

  EXIT;
  
  return;
}

void otb_led_wifi_blink_it(void)
{
  uint32_t colour;

  ENTRY;

  colour = otb_led_wifi_on ? OTB_LED_NEO_COLOUR_OFF : otb_led_wifi_colour;

  MDEBUG("status colour 0x%06x", colour);

  otb_led_wifi_update(colour, FALSE);

  EXIT;
  
  return;
}


void otb_led_wifi_blink_timerfunc(void *arg)
{

  ENTRY;
  
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
  
  EXIT;
  
  return;
}

int rep = 0;
bool otb_led_trigger_sf(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32_t rgb[252];
  int ii;
  
  ENTRY;

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
  
  EXIT;
  
  return rc;

}

uint32_t otb_led_neo_calc_rainbow(uint32_t colour_start, uint32_t colour_end, int step, uint32_t num, bool o360)
{
  uint32_t result;
  int32_t diff, inc;
  int mult;

  ENTRY;

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

  MDEBUG("Colour start: %x", colour_start);
  MDEBUG("Colour end:   %x", colour_end);
  MDEBUG("Diff:         %x", diff);
  MDEBUG("Inc:          %x", inc);
  MDEBUG("Result:       %x", result);

  EXIT;

  return result;
}

otb_led_neo_seq *otb_led_neo_seq_buf = NULL;
otb_led_neo_msg_seq otb_led_msg_seq_buf_a; // XXX Hack
otb_led_neo_msg_seq *otb_led_msg_seq_buf = &otb_led_msg_seq_buf_a;
bool otb_led_trigger_neo(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
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
  int msg_len;

  ENTRY;

  OTB_ASSERT((cmd >= OTB_CMD_LED_NEO_MIN) && (cmd <= OTB_CMD_LED_NEO_MAX));

  // Get num neo pixels
  if (cmd != OTB_CMD_LED_NEO_MESSAGE)
  {
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
  }

  // Get up to 2 more cmds - number varies based on command
  // Must test next_cmd here in case OTB_CMD_LED_NEO_MESSAGE
  if (next_cmd != NULL)
  {
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
    MDEBUG("Colour requested: 0x%06x", single_rgb);
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
      MDEBUG("Colour requested: 0x%06x", second_rgb);
    }
  }

  if (cmd == OTB_CMD_LED_NEO_MESSAGE)
  {
    num = 64;  // only 8x8 supported
  }

  if ((cmd == OTB_CMD_LED_NEO_BOUNCE) ||
      (cmd == OTB_CMD_LED_NEO_ROUND) ||
      (cmd == OTB_CMD_LED_NEO_ROTATE) ||
      (cmd == OTB_CMD_LED_NEO_ROTATEB) ||
      (cmd == OTB_CMD_LED_NEO_BOUNCER) ||
      (cmd == OTB_CMD_LED_NEO_ROUNDER) ||
      (cmd == OTB_CMD_LED_NEO_MESSAGE))
  {
    if (next_cmd4 != NULL)
    {
      min_speed = (((13 * 24 * num + 100) * 120) / 1000000) + 1; // 13 = us per bit, 24 = bits, num = pixels, 100 = 10us latch time, 120 = add 20%, 1000000 to get in ms, + 1 round up
      MDEBUG("Min speed: %d", min_speed);
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
      otb_cmd_rsp_append("missing commands");
      goto EXIT_LABEL;
    }
    MDEBUG("Speed requested: %d", speed);
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

  if (otb_led_msg_seq_buf != NULL)
  {
    os_timer_disarm((os_timer_t*)&otb_led_msg_seq_buf->timer);
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
        MDEBUG("r colour: %x", red);
        MDEBUG("g colour: %x", green);
        MDEBUG("b colour: %x", blue);
        MDEBUG("rainbow colour: %06x", rgb[ii]);
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

    case OTB_CMD_LED_NEO_MESSAGE:
      // check have enough commands
      if ((next_cmd5 == NULL) || (next_cmd5[0] == 0))
      {
        rc = FALSE;
        otb_cmd_rsp_append("not enough commands");
        goto EXIT_LABEL;
      }
      
      // Allocate buffer
      if (otb_led_msg_seq_buf == NULL)
      {
        otb_led_msg_seq_buf = (otb_led_neo_msg_seq *)os_malloc(sizeof(*otb_led_msg_seq_buf));
        if (otb_led_neo_seq_buf == NULL)
        {
          rc = FALSE;
          MWARN("Failed to allocated %d bytes", sizeof(*otb_led_msg_seq_buf));
          otb_cmd_rsp_append("not enough memory");
          goto EXIT_LABEL;
        }
      }
      os_memset(otb_led_msg_seq_buf, 0, sizeof(*otb_led_msg_seq_buf));

      // store colours
      single_rgb = strtol(next_cmd2, NULL, 16);
      if ((single_rgb < 0) || (single_rgb > 0xffffff))
      {
        rc = FALSE;
        otb_cmd_rsp_append("colour out of range 000000-ffffff");
        goto EXIT_LABEL;
      }
      otb_led_msg_seq_buf->background_colour = single_rgb;
      single_rgb = strtol(next_cmd3, NULL, 16);
      if ((single_rgb < 0) || (single_rgb > 0xffffff))
      {
        rc = FALSE;
        otb_cmd_rsp_append("colour out of range 000000-ffffff");
        goto EXIT_LABEL;
      }
      otb_led_msg_seq_buf->char_colour = single_rgb;

      // Get display type
      if (os_strncmp(next_cmd, "8x8", 3) != 0)
      {
        rc = FALSE;
        otb_cmd_rsp_append("only 8x8 display supported: %s", next_cmd);
        goto EXIT_LABEL;
      }

      // store message and space terminate the string
      msg_len = os_strnlen(next_cmd5, (OTB_LED_NEO_MSG_MAX_LEN-1));
      if (msg_len >= (OTB_LED_NEO_MSG_MAX_LEN-1))
      {
        rc = FALSE;
        otb_cmd_rsp_append("max msg length: %d", (OTB_LED_NEO_MSG_MAX_LEN-1));
        goto EXIT_LABEL;
      }
      os_memcpy(otb_led_msg_seq_buf->message, next_cmd5, msg_len);
      otb_led_msg_seq_buf->message[msg_len] = ' ';
      otb_led_msg_seq_buf->msg_len = msg_len;
      for (ii = 0; ii < otb_led_msg_seq_buf->msg_len; ii++)
      {
        if (otb_led_msg_seq_buf->message[ii] == '_')
        {
          otb_led_msg_seq_buf->message[ii] = ' ';
        }
        else if (((otb_led_msg_seq_buf->message[ii] < 'A') || (otb_led_msg_seq_buf->message[ii] > 'Z')) &&
                 ((otb_led_msg_seq_buf->message[ii] < 'a') || (otb_led_msg_seq_buf->message[ii] > 'z')) &&
                 ((otb_led_msg_seq_buf->message[ii] < '0') || (otb_led_msg_seq_buf->message[ii] > '9')) &&
                  (otb_led_msg_seq_buf->message[ii] != ' ') &&
                  (otb_led_msg_seq_buf->message[ii] != '@') &&
                  (otb_led_msg_seq_buf->message[ii] != '!') &&
                  (otb_led_msg_seq_buf->message[ii] != '?') &&
                  (otb_led_msg_seq_buf->message[ii] != '"') &&
                  (otb_led_msg_seq_buf->message[ii] != '#') &&
                  (otb_led_msg_seq_buf->message[ii] != ',') &&
                  (otb_led_msg_seq_buf->message[ii] != '.') &&
                  (otb_led_msg_seq_buf->message[ii] != ':') &&
                  (otb_led_msg_seq_buf->message[ii] != ';') &&
                  (otb_led_msg_seq_buf->message[ii] != '+') &&
                  (otb_led_msg_seq_buf->message[ii] != '-') &&
                  (otb_led_msg_seq_buf->message[ii] != '*') &&
                  (otb_led_msg_seq_buf->message[ii] != '=') &&
                  (otb_led_msg_seq_buf->message[ii] != '%') &&
                  (otb_led_msg_seq_buf->message[ii] != '&') &&
                  (otb_led_msg_seq_buf->message[ii] != '$') &&
                  (otb_led_msg_seq_buf->message[ii] != '(') &&
                  (otb_led_msg_seq_buf->message[ii] != ')') &&
                  (otb_led_msg_seq_buf->message[ii] != '\'') &&
                  (otb_led_msg_seq_buf->message[ii] != '^') &&
                  (otb_led_msg_seq_buf->message[ii] != '~'))
        {
          rc = FALSE;
          otb_cmd_rsp_append("invalid char %d %d in message - only A-Z and underscore (space) supported", otb_led_msg_seq_buf->message[ii], ii);
          goto EXIT_LABEL;
        }
        //DETAIL("LED: Char %d is %d", ii, otb_led_msg_seq_buf->message[ii]);
      }
      //DETAIL("LED: Message: %s", otb_led_msg_seq_buf->message);
      //DETAIL("LED: msg_len: %d", otb_led_msg_seq_buf->msg_len);

      // Set up other stuff
      otb_led_msg_seq_buf->pause = speed;
      otb_led_msg_seq_buf->cur_char = 0;
      otb_led_msg_seq_buf->cur_char_pos = 7; // starts at rhs

      // Kick off timer
      otb_led_msg_seq_buf->func = otb_led_neo_msg;
      otb_util_timer_set(&otb_led_msg_seq_buf->timer, 
                         (os_timer_func_t *)otb_led_msg_seq_buf->func,
                         NULL,
                         otb_led_msg_seq_buf->pause,
                         1);
      break;

    default:
      rc = FALSE;
      otb_cmd_rsp_append("internal error");
      goto EXIT_LABEL;
      break;
  }

EXIT_LABEL:

  EXIT;

  return rc;
};

otb_font_6x6 *otb_led_neo_get_font_char(unsigned char cchar)
{
  otb_font_6x6 *fchar = NULL;
  int ii;

  ENTRY;
  
  for (ii = 0; ii < OTB_FONT_LEN; ii++)
  {
    if (otb_font_6x6_1[ii].character == cchar)
    {
      fchar = otb_font_6x6_1 + ii;
      break;
    }
  }

  EXIT;

  return fchar;
}

void otb_led_neo_msg(void *arg)
{
  uint32_t neos[64];
  int ii, jj, kk;
  unsigned char cur_char[4];
  otb_font_6x6 *cur_charf[4];
  otb_font_6x6 *charf;
  int cpos, tcpos, pos, index;

  ENTRY;

  // first set the neo pixels to background colour
  for (ii = 0; ii < 64; ii++)
  {
    neos[ii] = otb_led_msg_seq_buf->background_colour;
  }

  // need current char and up to 3 others (most pathological case is 4 chars each 1 char wide with 1 char gaps)
  cur_char[0] = otb_led_msg_seq_buf->message[otb_led_msg_seq_buf->cur_char];
  ii = otb_led_msg_seq_buf->cur_char;
  for (kk = 1; kk < 4; kk++)
  {
    ii--;
    ii = ii < 0 ? (otb_led_msg_seq_buf->msg_len+ii) : ii;
    cur_char[kk] = otb_led_msg_seq_buf->message[ii];
  }
  for (kk = 0; kk < 4; kk++)
  {
    cur_charf[kk] = otb_led_neo_get_font_char(cur_char[kk]);
    OTB_ASSERT(cur_charf[0] != NULL); // If this fails, have failed to find char in font
  }
  cpos = otb_led_msg_seq_buf->cur_char_pos;

  // ii = row, jj = column
  // rows are 0 at top, 7 at bottom, columns are 0 at left, 7 at right
  // only do middle 6 rows
  for (ii = 1; ii < 7; ii++)
  {
    for (jj = 0; jj <= 7; jj++)
    {
      kk = 0;
      tcpos = cpos;
      index = jj - tcpos;
      while ((index < 0) && (kk < 4))
      {
        if (index == -1)
        {
          kk = -1; // Between chars, so skip
          break;
        }
        kk++;
        tcpos -= (cur_charf[kk]->len + 1);
        index = jj - tcpos;
      }
      OTB_ASSERT(kk != 4); // If this fails a character must be less than 1 character which isn't supported
      if ((kk != -1) && (index < cur_charf[kk]->len))
      {
        charf = cur_charf[kk];
        pos = index;
        OTB_ASSERT((pos < 6) && (pos >= 0));
        neos[(ii*8) + jj] = charf->bytes[ii-1] & (0b100000 >> pos) ? otb_led_msg_seq_buf->char_colour : otb_led_msg_seq_buf->background_colour;
      }
    }
  }
  
  // Now decrement cur_char/pos
  otb_led_msg_seq_buf->cur_char_pos--;
  if (otb_led_msg_seq_buf->cur_char_pos < (7 - cur_charf[0]->len))
  {
    otb_led_msg_seq_buf->cur_char_pos = 7;
    otb_led_msg_seq_buf->cur_char++;
    if (otb_led_msg_seq_buf->cur_char >= otb_led_msg_seq_buf->msg_len)
    {
      otb_led_msg_seq_buf->cur_char = 0;
    }
  }

  // Write the colours
  otb_led_neo_update(neos,
                     64,
                     4,
                     OTB_EEPROM_PIN_FINFO_LED_TYPE_WS2812B,
                     FALSE);

  EXIT;

  return;
}

void otb_led_neo_round(void *arg)
{

  ENTRY;

  otb_led_neo_bounce_or_round(arg, FALSE, FALSE);

  EXIT;

  return;
}

void otb_led_neo_bounce(void *arg)
{

  ENTRY;

  otb_led_neo_bounce_or_round(arg, TRUE, FALSE);

  EXIT;

  return;
}

void otb_led_neo_bouncer(void *arg)
{

  ENTRY;

  otb_led_neo_bounce_or_round(arg, TRUE, TRUE);

  EXIT;

  return;
}

void otb_led_neo_rounder(void *arg)
{

  ENTRY;

  otb_led_neo_bounce_or_round(arg, FALSE, TRUE);

  EXIT;

  return;
}

void otb_led_neo_rotate(void *arg)
{

  ENTRY;

  otb_led_neo_rotate_or_bounce(arg, FALSE);

  EXIT;

  return;
}

void otb_led_neo_rotateb(void *arg)
{

  ENTRY;

  otb_led_neo_rotate_or_bounce(arg, TRUE);

  EXIT;

  return;
}

void otb_led_neo_rotate_or_bounce(void *arg, bool bounce)
{
  int ii;
  int num;
  uint32_t red, green, blue;

  ENTRY;

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

  EXIT;

  return;
}

void otb_led_neo_bounce_or_round(void *arg, bool bounce, bool rainbow)
{
  int ii;
  int num;
  uint32_t red, green, blue;

  ENTRY;

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

  EXIT;

  return;
}
#endif