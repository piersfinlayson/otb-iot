/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2020 Piers Finlayson
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
 * 
 */

#ifndef OTB_MBUS_H_INCLUDED
#define OTB_MBUS_H_INCLUDED

#define OTB_MBUS_MCP23017_ADDR 0x20

#define OTB_MBUS_RCV_BUF_LEN 256

extern UartDevice    UartDev;
extern bool otb_mbus_hat_installed;
typedef void (otb_mbus_uart_intr_handler_fn)(void *arg);

typedef struct otb_mbus_rx_buf {
  uint8_t *buf;
  uint16_t len;
  uint16_t bytes;
} otb_mbus_rx_buf;

#ifdef OTB_MBUS_C
bool otb_mbus_hat_installed = FALSE;
brzo_i2c_info *otb_mbus_i2c_bus;
uint8_t otb_mbus_mcp23017_addr;
bool otb_mbus_enabled = FALSE;
UartDevice uart_dev_backup;
otb_mbus_rx_buf otb_mbus_rx_buf_g;
os_timer_t otb_mbus_recv_buf_timer;
#endif // OTB_MBUS_C

void otb_mbus_hat_init(void);
void otb_mbus_uart_conf(otb_mbus_uart_intr_handler_fn *intr_handler, void *arg);
bool otb_mbus_hat_enable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_mbus_hat_disable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_mbus_scan(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
bool otb_mbus_get_data(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);
void otb_mbus_recv_data(void *arg);
void otb_mbus_recv_intr_handler(void *arg);


#endif // OTB_MBUS_H_INCLUDED