/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2019 Piers Finlayson
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
 
#ifndef OTB_BREAK_H_INCLUDED
#define OTB_BREAK_H_INCLUDED

#define OTB_BREAK_RX_BUF_LEN 16
#define OTB_BREAK_CONFIG_STRING_LEN 64
extern char otb_break_rx_buf[OTB_BREAK_RX_BUF_LEN];
extern uint8_t otb_break_rx_buf_len;

#define OTB_BREAK_WATCHDOG_TIMER 300000

#define OTB_BREAK_STATE_MAIN        0
#define OTB_BREAK_STATE_CONFIG      1
#define OTB_BREAK_STATE_GPIO        2
#define OTB_BREAK_STATE_SOFT_RESET  3

#define OTB_BREAK_CONFIG_STATE_MAIN      0
#define OTB_BREAK_CONFIG_STATE_SSID      1
#define OTB_BREAK_CONFIG_STATE_PASSWORD  2
#define OTB_BREAK_CONFIG_STATE_MQTT_SVR  3
#define OTB_BREAK_CONFIG_STATE_MQTT_PORT 4
#define OTB_BREAK_CONFIG_STATE_CHIP_ID   6
#define OTB_BREAK_CONFIG_STATE_DEVICE_ID 7

#define OTB_BREAK_GPIO_TEST_INIT 0

#define OTB_BREAK_GPIO_LED_TYPE_GPIO  0
#define OTB_BREAK_GPIO_LED_TYPE_GPA   1
#define OTB_BREAK_GPIO_LED_TYPE_GPB   2

typedef struct otb_break_gpio_test_led
{
  uint8_t type;
  uint8_t num;
} otb_break_gpio_test_led;

#define OTB_BREAK_GPIO_TEST_LED_NUM 21

void otb_break_start(void);
void otb_break_options_output(void);
void otb_break_process_char_timerfunc(void *arg);
void otb_break_options_fan_out(char input);
bool otb_break_options_select(char option);
void otb_break_start_gpio_test(void);
void otb_break_gpio_test_init(uint8_t addr, brzo_i2c_info *info);
void otb_break_gpio_timerfunc(void *arg);
void otb_break_gpio_test_cancel(void);
bool otb_break_gpio_input(char input);
bool otb_break_config_input(char input);
void otb_break_clear_string(void);
bool otb_break_collect_string(char input);
bool otb_break_config_input_main(char input);
bool otb_break_soft_reset_input(char input);
void otb_break_process_char(void);

#ifdef OTB_BREAK_C
os_timer_t otb_break_process_char_timer;
os_timer_t otb_break_gpio_timer;
char otb_break_rx_buf[OTB_BREAK_RX_BUF_LEN];
uint8_t otb_break_rx_buf_len;
uint8_t otb_break_state;
uint8_t otb_break_config_state;
char otb_break_string[OTB_BREAK_CONFIG_STRING_LEN];
uint8_t otb_break_gpio_next_led;

otb_break_gpio_test_led otb_break_gpio_test_led_seq[OTB_BREAK_GPIO_TEST_LED_NUM] = 
{
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 7},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 4},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 6},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 2},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 5},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 1},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 3},
  {OTB_BREAK_GPIO_LED_TYPE_GPB, 0},
  {OTB_BREAK_GPIO_LED_TYPE_GPIO, 13},
  {OTB_BREAK_GPIO_LED_TYPE_GPIO, 12},
  {OTB_BREAK_GPIO_LED_TYPE_GPIO, 14},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 7},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 6},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 5},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 3},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 4},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 5},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 2},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 4},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 1},
  {OTB_BREAK_GPIO_LED_TYPE_GPA, 0},
};
#endif

#endif // OTB_BREAK_H_INCLUDED
