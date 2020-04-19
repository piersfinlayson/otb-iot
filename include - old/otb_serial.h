/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017-2018 Piers Finlayson
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

#ifndef OTB_SERIAL_H_INCLUDED
#define OTB_SERIAL_H_INCLUDED

#define OTB_SERIAL_MBUS_V0_1_SDA_PIN   4
#define OTB_SERIAL_MBUS_V0_1_SCL_PIN   5
#define OTB_SERIAL_MBUS_V0_1_I2C_ADDR  0x48
#define OTB_SERIAL_MBUS_V0_1_IRQ_PIN   12
#define OTB_SERIAL_MBUS_V0_1_RST_PIN   13

#define OTB_SERIAL_CMD_NONE     0x0000

#define OTB_SERIAL_CMD_MASK     0xff
#define OTB_SERIAL_CMD_ENABLE   0x01  // Not valid on trigger
#define OTB_SERIAL_CMD_DISABLE  0x02  // Not valid on trigger
#define OTB_SERIAL_CMD_RX       0x03  // Not valid on trigger
#define OTB_SERIAL_CMD_TX       0x04  // Not valid on trigger
#define OTB_SERIAL_CMD_BAUD     0x05  // Not valid on trigger
#define OTB_SERIAL_CMD_STOPBIT  0x06  // Not valid on trigger
#define OTB_SERIAL_CMD_PARITY   0x07  // Not valid on trigger
#define OTB_SERIAL_CMD_MEZZ     0x08  // Not valid on trigger
#define OTB_SERIAL_CMD_COMMIT   0x09  // Only valid on set
#define OTB_SERIAL_CMD_BUFFER   0x0A  // Only valid on trigger
#define OTB_SERIAL_CMD_TRANSMIT 0x0B  // Only valid on trigger

#define OTB_SERIAL_CMD_TYPE_MASK 0xff00
#define OTB_SERIAL_CMD_GET       0x0100
#define OTB_SERIAL_CMD_SET       0x0200
#define OTB_SERIAL_CMD_TRIGGER   0x0300

#define OTB_SERIAL_B_DEFAULT    OTB_SERIAL_B9600
#define OTB_SERIAL_B300         300
#define OTB_SERIAL_B600         600
#define OTB_SERIAL_B1200        1200
#define OTB_SERIAL_B2400        2400
#define OTB_SERIAL_B4800        4800
#define OTB_SERIAL_B9600        9600
#define OTB_SERIAL_B14400       14400
#define OTB_SERIAL_B19200       19200
#define OTB_SERIAL_B28800       28800
#define OTB_SERIAL_B38400       38400
#define OTB_SERIAL_B57600       57600
#define OTB_SERIAL_B_NUM        11
extern uint32_t otb_serial_supported_baudrates[OTB_SERIAL_B_NUM];
#ifdef OTB_SERIAL_C
uint32_t otb_serial_supported_baudrates[OTB_SERIAL_B_NUM] =
{
  OTB_SERIAL_B300,
  OTB_SERIAL_B600,
  OTB_SERIAL_B1200,
  OTB_SERIAL_B2400,
  OTB_SERIAL_B4800,
  OTB_SERIAL_B9600,
  OTB_SERIAL_B14400,
  OTB_SERIAL_B19200,
  OTB_SERIAL_B28800,
  OTB_SERIAL_B38400,
  OTB_SERIAL_B57600,
};
  
#endif // OTB_SERIAL_C

typedef struct otb_serial_buf
{
#define OTB_SERIAL_BUF_SIZE 256
  uint8_t buf[OTB_SERIAL_BUF_SIZE];
  uint16_t buf_size;
  uint16_t head;
  uint16_t tail;
  bool overflow;
} otb_serial_buf;

typedef struct otb_serial_mezz_info
{
  uint32_t mezz_type;

  brzo_i2c_info i2c_info;

  os_timer_t timer;

  bool use_mezz;

  bool mezz_inited;

  uint8_t i2c_addr;

  uint8_t uart_num;  // Which UART on the SC16IS752 is being used - 0

  otb_serial_buf buf;
} otb_serial_mezz_info;

typedef struct otb_serial_config
{
  Softuart softuart;
  
  os_timer_t timer;
  
  bool enabled;

  otb_serial_mezz_info *mezz_info;
  
  // -1 = invalid/unset
#define OTB_SERIAL_PIN_INVALID  -1
  sint8_t rx;
  sint8_t tx;
  
  uint32_t baudrate;

#define OTB_SERIAL_STOPBIT_MIN   0
#define OTB_SERIAL_STOPBIT_MAX   2
  uint8_t stopbit;

#define OTB_SERIAL_PARITY_NONE   0x00
#define OTB_SERIAL_PARITY_EVEN   0x01
#define OTB_SERIAL_PARITY_ODD    0x02
  uint8_t parity;

  // If currently in the process of transmitting
  bool transmitting;
  
  // If currently in the process of dumping data via MQTT
  bool dumping;

} otb_serial_config;

extern otb_serial_config otb_serial_conf;
#ifdef OTB_SERIAL_C
otb_serial_config otb_serial_conf;
#endif // OTB_SERIAL_C

// Special case GPIO register - always apply to UART 0
#define OTB_SERIAL_SC16IS_REG(REG)  (REG << 3) | ((REG != 0x0B) ? otb_serial_conf.mezz_info->uart_num << 2 : 0)

void otb_serial_init(void);
void otb_serial_init_mbus_mezz(void *arg);
uint8_t otb_serial_mezz_write_reg(uint8_t reg, uint8_t val);
uint8_t otb_serial_mezz_read_reg(uint8_t reg, uint8_t *val);
bool otb_serial_mezz_gpio(uint8 gpios);
bool otb_serial_mezz_reset(bool hard);
bool otb_serial_mezz_configure(void);
bool otb_serial_enable_mezz();
bool otb_serial_disable_mezz();
bool otb_serial_mezz_byte_to_read(void);
void otb_serial_mezz_read_data(void *arg);
bool otb_serial_mezz_buf_data_avail(void);
uint8_t otb_serial_mezz_buf_get_next_byte(void);
void otb_serial_mezz_intr_handler(void *arg);
bool otb_serial_mezz_send_byte(uint8_t byte);
bool otb_serial_config_handler(unsigned char *next_cmd,
                               void *arg,
                               unsigned char *prev_cmd);

#endif // OTB_SERIAL_H_INCLUDED