/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017 Piers Finlayson
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

#ifndef OTB_NIXIE_H
#define OTB_NIXIE_H

#define OTB_NIXIE_INDEX_0    0
#define OTB_NIXIE_INDEX_1    1
#define OTB_NIXIE_INDEX_2    2
#define OTB_NIXIE_INDEX_3    3
#define OTB_NIXIE_INDEX_4    4
#define OTB_NIXIE_INDEX_5    5
#define OTB_NIXIE_INDEX_6    6
#define OTB_NIXIE_INDEX_7    7
#define OTB_NIXIE_INDEX_8    8
#define OTB_NIXIE_INDEX_9    9
#define OTB_NIXIE_INDEX_DP   10
#define OTB_NIXIE_INDEX_NUM  11

#define OTB_NIXIE_INVALID_VALUE  0xffffffff

// Used for test purposes
#define OTB_NIXIE_PIN_DEFAULT_SRCK  4
#define OTB_NIXIE_PIN_DEFAULT_SER   5
#define OTB_NIXIE_PIN_DEFAULT_RCK   13

#define OTB_NIXIE_DIGITS  2

#define OTB_NXIE_SERIAL_TIMER_US  1

#define OTB_NIXIE_DEPOISONING_TIMER_MS  3600000 // One hour

#define OTB_NIXIE_CYCLE_LEN  13

#define OTB_NIXIE_DEPOSION_CYCLE_TIMER_US  5000000 // Five seconds
#define OTB_NIXIE_CYCLE_TIMER_US           10000

// Used to map a nixie digit to a TPIC6B595 IC and pin
typedef struct otb_nixie_index
{
  uint8_t chip;
  uint8_t pin;
} otb_nixie_index;

// Used to store information about nixie display
typedef struct otb_nixie_display
{
  char digits[OTB_NIXIE_DIGITS];
  char dots[OTB_NIXIE_DIGITS];

} otb_nixie_display;

typedef struct otb_nixie_pins
{
  uint8_t srck;
  uint8_t ser;
  uint8_t rck;
} otb_nixie_pins;

typedef struct otb_nixie_display_state
{
  // Whether nixie display has been initialized
  bool inited;

  // Whether depoisioning cycle is taking place
  bool depoisoning;

  // Current display (starts as "__")
  otb_nixie_display current;

  // Target display (starts as "__")
  otb_nixie_display target;

  // Timer which runs to depoison nixie tubes
  os_timer_t depoisoning_timer;

  // Timer which is used as part of depoisoning and other cycles
  os_timer_t display_timer;

  // Pins
  otb_nixie_pins pins;

  // Where in depoison cycle we are;
  uint32_t depoison_cycle;

  // Power
  bool power;

} otb_nixie_display_state;

void otb_nixie_module_init(void);
void otb_nixie_depoison(void *arg);
uint32_t otb_nixie_get_serial_data(char *bytes, uint8_t num_bytes, bool power);
bool otb_nixie_show_value(unsigned char *to_show, uint8_t num_bytes, bool commit);
bool otb_nixie_init(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_nixie_clear(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_nixie_show(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_nixie_cycle(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
void otb_nixie_depoison_timer_arm();
void otb_nixie_depoison_timer_disarm();
bool otb_nixie_power(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_nixie_display_update(otb_nixie_display *display);

#ifdef OTB_NIXIE_C

otb_nixie_display_state otb_nixie_info;
uint8_t srck_pin = OTB_NIXIE_PIN_DEFAULT_SRCK; 
uint8_t ser_pin = OTB_NIXIE_PIN_DEFAULT_SER;
uint8_t rck_pin = OTB_NIXIE_PIN_DEFAULT_RCK;

// First byte is TPIC6B595 instance, second byte is pin
const otb_nixie_index otb_nixie_index_left_v0_1[OTB_NIXIE_INDEX_NUM] =
{
  {1, 1}, // 0
  {0, 0}, // 1
  {0, 1}, // 2
  {0, 2}, // 3
  {0, 3}, // 4
  {0, 4}, // 5
  {0, 5}, // 6
  {0, 6}, // 7
  {0, 7}, // 8
  {1, 0}, // 9
  {1, 2}, // DP
};
const otb_nixie_index otb_nixie_index_right_v0_1[OTB_NIXIE_INDEX_NUM] =
{
  {2, 4}, // 0
  {1, 3}, // 1
  {1, 4}, // 2
  {1, 5}, // 3
  {1, 6}, // 4
  {1, 7}, // 5
  {2, 0}, // 6
  {2, 1}, // 7
  {2, 2}, // 8
  {2, 3}, // 9
  {2, 5}, // DP
};
const otb_nixie_index otb_nixie_index_power_v0_1 = {2, 6};
const otb_nixie_index *otb_nixie_indexes_v0_1[2] =
{
  otb_nixie_index_left_v0_1,
  otb_nixie_index_right_v0_1,
};

const otb_nixie_index otb_nixie_index_left_v0_2[OTB_NIXIE_INDEX_NUM] =
{
  {1, 4}, // 0
  {2, 2}, // 1
  {2, 1}, // 2
  {2, 0}, // 3
  {2, 7}, // 4
  {2, 5}, // 5
  {2, 6}, // 6
  {1, 7}, // 7
  {1, 6}, // 8
  {1, 5}, // 9
  {2, 3}, // DP
};
const otb_nixie_index otb_nixie_index_right_v0_2[OTB_NIXIE_INDEX_NUM] =
{
  {0, 7}, // 0
  {0, 5}, // 1
  {0, 4}, // 2
  {1, 3}, // 3
  {1, 2}, // 4
  {1, 0}, // 5
  {1, 1}, // 6
  {0, 2}, // 7
  {0, 1}, // 8
  {0, 0}, // 9
  {0, 6}, // DP
};
const otb_nixie_index otb_nixie_index_power_v0_2 = {2, 4};
const otb_nixie_index *otb_nixie_indexes_v0_2[2] =
{
  otb_nixie_index_left_v0_2,
  otb_nixie_index_right_v0_2,
};

const otb_nixie_index **otb_nixie_indexes;
const otb_nixie_index *otb_nixie_index_power;

char otb_nixie_cycle_display[OTB_NIXIE_CYCLE_LEN][3] =
{
  {"__"},
  {".9"},
  {"08"},
  {"17"},
  {"26"},
  {"35"},
  {"44"},
  {"53"},
  {"62"},
  {"71"},
  {"80"},
  {"9."},
  {"__"},
};

char otb_nixie_depoison_cycle_display[OTB_NIXIE_CYCLE_LEN][3] =
{
  {"__"},
  {".."},
  {"00"},
  {"11"},
  {"22"},
  {"33"},
  {"44"},
  {"55"},
  {"66"},
  {"77"},
  {"88"},
  {"99"},
  {"__"},
};

#endif // OTB_NIXIE_C

#endif // OTB_NIXIE_H
