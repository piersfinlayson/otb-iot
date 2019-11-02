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

void otb_break_start(void);
void otb_break_options_output(void);
void otb_break_process_char_timerfunc(void *arg);
void otb_break_options_fan_out(char input);
bool otb_break_options_select(char option);
bool otb_break_gpio_input(char input);
void otb_break_process_char(void);

#ifdef OTB_BREAK_C
os_timer_t otb_break_process_char_timer;
char otb_break_rx_buf[OTB_BREAK_RX_BUF_LEN];
uint8_t otb_break_rx_buf_len;
uint8_t otb_break_state;
#endif

#endif // OTB_BREAK_H_INCLUDED