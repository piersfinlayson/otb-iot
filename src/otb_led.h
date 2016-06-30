 *
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

#ifndef OTB_LED_H

define OTB_LED_H

typedef struct otb_led_sequence_step
{
  // All fields take values 0x0 - 0xff.
  // - 0x0 turns any type of LED off
  // - For single intensity LEDs 0x1-0xff all turn the lED on
  // - For variable intensity LEDs 0x1-0xff will proportionally affect intesity, with 0xff
  //   being the maximum
  
  // On/off - only used for single colour RGBs
  uint8_t on;
  
  // Red component (for RGB LEDs)
  uint8_t red;
  
  // Green component (for RGB LEDs)
  uint8_t green;
  
  // Blue component (for RGB LEDs)
  uint8_t blue;
  
  // Time to take to fade to this colour from previous, 0 = no fade time, in us
  uint32_t timer_us;
  
} otb_led_sequence_step;

#define OTB_LED_SEQUENCE_MAX_STEPS 16
typedef struct otb_led_sequence
{
  // Pointer to array (not linked list) of steps
  otb_led_sequent_step *step_array;

#define OTB_LED_TYPE_STATUS  0
#define OTB_LED_TYPE_NUM     1
  // LED type - which LED
  uint8_t type;

  // Number of steps (max 255, 0 is invalid).
  uint8_t steps;

// Just stop on last colour
#define OTB_LED_SEQUENCE_AFTER_STOP         0
// Restart sequence after sequence
#define OTB_LED_SEQUENCE_AFTER_RESTART_SEQ  1
// Turn LED off after sequence
#define OTB_LED_SEQUENCE_AFTER_OFF          2
// Maximum number of options (for asserting)
#define OTB_LEB_SEQUENCE_AFTER_NUM          3
  // One of OTB_LED_SEQUENCE_AFTER_ options
  uint8_t afterwards;
  
  // Used by app to tell a sequence to stop - mid-way through
  bool stop;

  // Function called if done (for steps > 1);
  otb_led_control_callback *on_done;
  
  // Application handle
  void *handle;
  
  // Number of times this sequence has been run
  uint32_t iterations;
  
  // Used by LED code to signal if error was hit
  bool error;
    
  // Whether done - to signal to user
  bool done;
  
  // private - used by otb_led_ functions
  uint8_t next_step;

  // private - used by otb_led_ function
  volatile os_timer_t timer;

} otb_led_sequence;

typedef struct otb_led_type_info
{
  // Type, one of OTB_LED_TYPE_...
  uint8_t type;
  
  // Whether RGB or not.  If not RGB, single colour
  bool is_rgb;
  
  // Whether GPIO attached (if not, I2C)
  bool is_gpio;
  
  // Which GPIO to turn on this LED
  uint8_t pin;
  
} otb_led_type_info;


#ifdef OTB_LED_C
otb_led_type_info OTB_LED_TYPE_INFO[OTB_LED_TYPE_NUM] = 
[
  {OTB_LED_TYPE_STATUS, True, False, 0},
]
#endif // OTB_LED_C


void otb_led_get_colours(uint32_tcolour,
                         uint8_t *red,
                         uint8_t *green,
                         uint8_t *blue);
bool otb_led_control_once(uint8_t type, uint32_t colour);
bool otb_led_control_step(otb_led_sequence *seq);
void otb_led_control_on_timer(void *arg)
void otb_led_control_init(otb_led_sequence *seq);
bool otb_led_control_seq(uint8_t type, ot;b_led_sequence *seq);

// error_step is 0 if successful is True
void otb_led_control_callback(void *handle, bool successful, uint8_t error_step);

#endif  // OTB_LED_H





