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
 
#ifndef OTB_UTIL_H_INCLUDED
#define OTB_UTIL_H_INCLUDED

// Booting
#define OTB_UTIL_RGB_STATE_BOOT  otb_led_neo_get_rgb(0xff, 0, 0)
// Retrieved config
#define OTB_UTIL_RGB_STATE_CONF  otb_led_neo_get_rgb(0xff, 0x40, 0)

typedef struct
{
  size_t buf_len;
  size_t chars;
  char *buf;
} otb_util_rx_buf;

typedef enum
{
  OTB_RUN_STATE_BOOT,
  OTB_RUN_STATE_CONF,
  OTB_RUN_STATE_MAX
} otb_run_state_t;

typedef struct
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} otb_util_rgb_t;

void otb_util_init(void *arg);
void otb_util_state_change(otb_run_state_t state);
void otb_util_init_uart(void);
void otb_util_check_log_level(void);
void otb_util_process_log_level(char log_level);
void otb_util_assert(bool value, char *value_s, char *file, uint32_t line);
void otb_util_log_useful_info(void);
void otb_util_convert_char_to_char(char *text, int from, int to);
void otb_util_check_defs(void);

extern uint8_t otb_util_log_level;
extern unsigned long otb_build_num;
#define OTB_BUILD_NUM otb_build_num
extern char otb_hw_info[10];
extern otb_run_state_t otb_run_state;

#ifdef OTB_UTIL_C
bool otb_util_booted_g;
uint8_t otb_util_log_level = ESP_LOG_INFO;
bool otb_util_asserting;
char otb_hw_info[10];
unsigned long otb_build_num = 0;
otb_run_state_t otb_run_state;
otb_util_rgb_t otb_util_rgb_state[OTB_RUN_STATE_MAX] =
{
  {0xff, 0, 0},     // BOOT
  {0xff, 0x40, 0},  // CONF
};
#endif

#endif // OTB_UTIL_H_INCLUDED
