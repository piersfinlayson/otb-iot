/*
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
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef UART_APP_H
#define UART_APP_H

#include "uart_register.h"
#include "eagle_soc.h"
#include "esp_types.h"

#define RX_BUFF_SIZE    256
#define TX_BUFF_SIZE    100
#define UART0   0
#define UART1   1

typedef enum {
    FIVE_BITS = 0x0,
    SIX_BITS = 0x1,
    SEVEN_BITS = 0x2,
    EIGHT_BITS = 0x3
} UartBitsNum4Char;

typedef enum {
    ONE_STOP_BIT             = 0,
    ONE_HALF_STOP_BIT        = BIT2,
    TWO_STOP_BIT             = BIT2
} UartStopBitsNum;

typedef enum {
    NONE_BITS = 0,
    ODD_BITS   = 0,
    EVEN_BITS = BIT4
} UartParityMode;

typedef enum {
    STICK_PARITY_DIS   = 0,
    STICK_PARITY_EN    = BIT3 | BIT5
} UartExistParity;

typedef enum {
    BIT_RATE_9600     = 9600,
    BIT_RATE_19200   = 19200,
    BIT_RATE_38400   = 38400,
    BIT_RATE_57600   = 57600,
    BIT_RATE_74880   = 74880,
    BIT_RATE_115200 = 115200,
    BIT_RATE_230400 = 230400,
    BIT_RATE_256000 = 256000,
    BIT_RATE_460800 = 460800,
    BIT_RATE_921600 = 921600
} UartBautRate;

typedef enum {
    NONE_CTRL,
    HARDWARE_CTRL,
    XON_XOFF_CTRL
} UartFlowCtrl;

typedef enum {
    EMPTY,
    UNDER_WRITE,
    WRITE_OVER
} RcvMsgBuffState;

typedef struct {
    uint32_t     RcvBuffSize;
    uint8_t     *pRcvMsgBuff;
    uint8_t     *pWritePos;
    uint8_t     *pReadPos;
    uint8_t      TrigLvl; //JLU: may need to pad
    RcvMsgBuffState  BuffState;
} RcvMsgBuff;

typedef struct {
    uint32_t   TrxBuffSize;
    uint8_t   *pTrxBuff;
} TrxMsgBuff;

typedef enum {
    BAUD_RATE_DET,
    WAIT_SYNC_FRM,
    SRCH_MSG_HEAD,
    RCV_MSG_BODY,
    RCV_ESC_CHAR,
} RcvMsgState;

typedef struct {
    UartBautRate 	     baut_rate;
    UartBitsNum4Char  data_bits;
    UartExistParity      exist_parity;
    UartParityMode 	    parity;    // chip size in byte
    UartStopBitsNum   stop_bits;
    UartFlowCtrl         flow_ctrl;
    RcvMsgBuff          rcv_buff;
    TrxMsgBuff           trx_buff;
    RcvMsgState        rcv_state;
    int                      received;
    int                      buff_uart_no;  //indicate which uart use tx/rx buffer
} UartDevice;

void uart_init(UartBautRate uart0_br, UartBautRate uart1_br);
void uart0_sendStr(const char *str);
#endif

