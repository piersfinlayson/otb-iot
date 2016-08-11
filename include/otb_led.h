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

#ifndef OTB_LED_H

#define OTB_LED_H

#define OTB_LED_NEO_PIN  13

#define OTB_LED_NEO_COLOUR_OFF     0x000000
#define OTB_LED_NEO_COLOUR_RED     0x200000
#define OTB_LED_NEO_COLOUR_GREEN   0x002000
#define OTB_LED_NEO_COLOUR_BLUE    0x000020
#define OTB_LED_NEO_COLOUR_CYAN    0x002020
#define OTB_LED_NEO_COLOUR_PURPLE  0x200020
#define OTB_LED_NEO_COLOUR_WHITE   0x202020
#define OTB_LED_NEO_COLOUR_ORANGE  0x201000
#define OTB_LED_NEO_COLOUR_YELLOW  0x202000
#define OTB_LED_NEO_COLOUR_PINK    0x201818

#define OTB_LED_NEO_T1H  800
#define OTB_LED_NEO_T1L  450
#define OTB_LED_NEO_T0H  400
#define OTB_LED_NEO_T0L  850

#define OTB_LED_NEO_T1H_CYCLES  OTB_LED_NEO_T1H/OTB_UTIL_NS_PER_CYCLE
#define OTB_LED_NEO_T1L_CYCLES  OTB_LED_NEO_T1L/OTB_UTIL_NS_PER_CYCLE
#define OTB_LED_NEO_T0H_CYCLES  OTB_LED_NEO_T0H/OTB_UTIL_NS_PER_CYCLE
#define OTB_LED_NEO_T0L_CYCLES  OTB_LED_NEO_T0L/OTB_UTIL_NS_PER_CYCLE


// error_step is 0 if successful is True
typedef void (otb_led_control_callback)(void *, bool , uint8_t );

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
  otb_led_sequence_step *step_array;

#define OTB_LED_TYPE_GPIO0   0
#define OTB_LED_TYPE_GPIO4   1
#define OTB_LED_TYPE_GPIO5   2
#define OTB_LED_TYPE_GPIO12  3
#define OTB_LED_TYPE_GPIO13  4
#define OTB_LED_TYPE_GPIO14  5
#define OTB_LED_TYPE_GPIO15  6
#define OTB_LED_TYPE_STATUS  7
#define OTB_LED_TYPE_NUM     8
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
  unsigned char *name;
  
  // Type, one of OTB_LED_TYPE_...
  uint8_t type;
  
  // Whether RGB or not.  If not RGB, single colour
  bool is_rgb;
  
  // Whether GPIO attached (if not, I2C)
  bool is_gpio;
  
  // Which GPIO to turn on this LED
  uint8_t pin;
  
  otb_led_sequence *seq;
  
} otb_led_type_info;

#ifdef OTB_LED_C

// SOS in morse code
// Dots are 1, dashes 3, gaps between elements 1, gaps between letters 3, gaps between words 7
otb_led_sequence_step otb_led_test_steps[18] =
{
  {1, 0, 0, 0, 100000},
  {0, 0, 0, 0, 100000},
  {1, 0, 0, 0, 100000},
  {0, 0, 0, 0, 100000},
  {1, 0, 0, 0, 100000},
  {0, 0, 0, 0, 300000},
  {1, 0, 0, 0, 300000},
  {0, 0, 0, 0, 100000},
  {1, 0, 0, 0, 300000},
  {0, 0, 0, 0, 100000},
  {1, 0, 0, 0, 300000},
  {0, 0, 0, 0, 300000},
  {1, 0, 0, 0, 100000},
  {0, 0, 0, 0, 100000},
  {1, 0, 0, 0, 100000},
  {0, 0, 0, 0, 100000},
  {1, 0, 0, 0, 100000},
  {0, 0, 0, 0, 700000},
};

otb_led_sequence otb_led_test_seq[OTB_LED_TYPE_NUM] =
{
  { 0 },
  { 0 },
  { 0 },
  { 0 },
  { 0 },
  { 0 },
  { 0 },
  { 0 },
};

#endif //OTB_LED_C

extern uint32_t otb_led_wifi_colour;

#ifdef OTB_LED_C
uint32_t otb_led_wifi_colour;
static volatile os_timer_t otb_led_wifi_blink_timer;
bool otb_led_wifi_on;
sint32_t otb_led_wifi_blink_times = 0;
otb_led_type_info OTB_LED_TYPE_INFO[OTB_LED_TYPE_NUM] = 
{
  {"gpio0", OTB_LED_TYPE_GPIO0, FALSE, TRUE, 0, otb_led_test_seq+0},
  {"gpio4", OTB_LED_TYPE_GPIO4, FALSE, TRUE, 4, otb_led_test_seq+1},
  {"gpio5", OTB_LED_TYPE_GPIO5, FALSE, TRUE, 5, otb_led_test_seq+2},
  {"gpio12", OTB_LED_TYPE_GPIO12, FALSE, TRUE, 12, otb_led_test_seq+3},
  {"gpio13", OTB_LED_TYPE_GPIO13, FALSE, TRUE, 13, otb_led_test_seq+4},
  {"gpio14", OTB_LED_TYPE_GPIO14, FALSE, TRUE, 14, otb_led_test_seq+5},
  {"gpio15", OTB_LED_TYPE_GPIO15, FALSE, TRUE, 15, otb_led_test_seq+6},
  {"status", OTB_LED_TYPE_STATUS, TRUE, FALSE, 0, otb_led_test_seq+7},
};
#endif // OTB_LED_C

bool otb_led_test(unsigned char *name, bool repeat, unsigned char **error_text);
bool otb_led_test_stop(unsigned char *name, unsigned char **error_text);
void otb_led_get_colours(uint32_t colour,
                         uint8_t *red,
                         uint8_t *green,
                         uint8_t *blue);
bool otb_led_control_once(uint8_t type, uint8_t on, uint8_t red, uint8_t green, uint8_t blue);
bool otb_led_control_step(otb_led_sequence *seq);
void otb_led_control_on_timer(void *arg);
void otb_led_control_init(otb_led_sequence *seq);
bool otb_led_control_seq(otb_led_sequence *seq);
void otb_led_wifi_update(uint32_t rgb, bool store);
void otb_led_neo_update(uint32_t *rgb, int num, uint8_t pin);
uint32_t otb_led_neo_get_rgb(uint8_t red, uint8_t green, uint8_t blue);
void otb_led_wifi_blink(uint8 times);
void otb_led_wifi_init_blink_timer(void);
void otb_led_wifi_disable_blink_timer(void);
void otb_led_wifi_blink_it(void);
void otb_led_wifi_blink_timerfunc(void *arg);

#endif  // OTB_LED_H