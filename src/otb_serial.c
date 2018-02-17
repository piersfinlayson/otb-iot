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

#define OTB_SERIAL_C
#include "otb.h"

void ICACHE_FLASH_ATTR otb_serial_init(void)
{
  DEBUG("SERIAL: otb_serial_init entry");
  
  // While this is a global so should be initialized to zero, we might be
  // called later on to reinit.
  os_memset(&otb_serial_conf, 0, sizeof(otb_serial_conf));
  
  otb_serial_conf.baudrate = OTB_SERIAL_B_DEFAULT;
  otb_serial_conf.rx = OTB_SERIAL_PIN_INVALID;
  otb_serial_conf.tx = OTB_SERIAL_PIN_INVALID;
  otb_serial_conf.mezz_info = NULL;
  
  DEBUG("SERIAL: otb_serial_init exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_serial_init_mbus_mezz(void *arg)
{
  uint8 temp;
  uint8 bytes[2];
  uint32 mezz_type = (uint32)arg;
  int ii;

  DEBUG("SERIAL: otb_serial_init_mbus_mezz entry");

  // Assert a supported mezzanine type (otherwise this function shouldn't be called)
  OTB_ASSERT(mezz_type == OTB_EEPROM_MODULE_TYPE_MBUS_V0_1);
  
  // Allocate memory for the serial mezz info
  otb_serial_conf.mezz_info = (otb_serial_mezz_info *)os_malloc(sizeof(otb_serial_mezz_info));
  if (!otb_serial_conf.mezz_info)
  {
    ERROR("SERIAL: Failed to allocate memory for serial mezz");
    goto EXIT_LABEL;
  }
  os_memset(otb_serial_conf.mezz_info, 0, sizeof(otb_serial_mezz_info));

  otb_serial_conf.mezz_info->mezz_inited = FALSE;
  otb_serial_conf.mezz_info->mezz_type = mezz_type;
  otb_serial_conf.mezz_info->i2c_addr = OTB_SERIAL_MBUS_V0_1_I2C_ADDR;
  otb_serial_conf.mezz_info->uart_num = 0;
  otb_serial_conf.mezz_info->buf.buf_size = OTB_SERIAL_BUF_SIZE;

  // Disable outputs for IRQ pin
  GPIO_DIS_OUTPUT(OTB_SERIAL_MBUS_V0_1_IRQ_PIN);

  // Initialise I2C bus
  otb_i2c_initialize_bus(&(otb_serial_conf.mezz_info->i2c_info),
                         OTB_SERIAL_MBUS_V0_1_SDA_PIN,
                         OTB_SERIAL_MBUS_V0_1_SCL_PIN);
  otb_serial_conf.mezz_info->i2c_info.clock_stretch_time_out_usec = 100;
  brzo_i2c_setup_info(&(otb_serial_conf.mezz_info->i2c_info));

  // Hard reset SC16IS752
  if (!otb_serial_mezz_reset(TRUE))
  {
    ERROR("SERIAL: Failed to reset serial mezz");
    goto EXIT_LABEL;
  }

  // Configure before turning on GPIOs
  if (!otb_serial_mezz_configure())
  {
    goto EXIT_LABEL;
  }

  // Now turn on connected LED
  if (!otb_serial_mezz_gpio(0b10))  // GPIO1 state 1
  {
    ERROR("SERIAL: Failed to turn on serial mezz GPIO0");
    goto EXIT_LABEL;
  }

  // Have succesfully connected so ...
  otb_serial_conf.mezz_info->mezz_inited = TRUE;
  otb_serial_conf.mezz_info->use_mezz = TRUE;  // default to using this mezzanine

  INFO("SERIAL: Initialized mezz serial board 0x%02x", otb_serial_conf.mezz_info->i2c_addr);

EXIT_LABEL:

  if (!otb_serial_conf.mezz_info->mezz_inited && (otb_serial_conf.mezz_info != NULL))
  {
    // Cleanup
    os_free(otb_serial_conf.mezz_info);
    otb_serial_conf.mezz_info = NULL;
  }

  DEBUG("SERIAL: otb_serial_init_mbus_mezz exit");

  return;
}

uint8_t ICACHE_FLASH_ATTR otb_serial_mezz_write_reg(uint8_t reg, uint8_t val)
{
  uint8_t brc;
  uint8 bytes[2];

  DEBUG("SERIAL: otb_serial_mezz_write_reg entry");

  bytes[0] = OTB_SERIAL_SC16IS_REG(reg);
  bytes[1] = val;
  brzo_i2c_start_transaction_info(otb_serial_conf.mezz_info->i2c_addr,
                                  100,
                                  &(otb_serial_conf.mezz_info->i2c_info));
  brzo_i2c_write_info(bytes, 2, FALSE, &(otb_serial_conf.mezz_info->i2c_info));
  brc = brzo_i2c_end_transaction_info(&(otb_serial_conf.mezz_info->i2c_info));
  if (brc)
  {
    DEBUG("SERIAL: Failed to write reg addr: 0x%02x reg: 0x%02x real_reg: 0x%02x val: 0x%02x rc: %d", otb_serial_conf.mezz_info->i2c_addr, reg, bytes[0], val, brc)
  }
  
  DEBUG("SERIAL: otb_serial_mezz_write_reg exit");

  return brc;
}

uint8_t ICACHE_FLASH_ATTR otb_serial_mezz_read_reg(uint8_t reg, uint8_t *val)
{
  uint8_t brc;
  uint8 bytes[1];

  DEBUG("SERIAL: otb_serial_mezz_read_reg entry");

  bytes[0] = OTB_SERIAL_SC16IS_REG(reg);
  brzo_i2c_start_transaction_info(otb_serial_conf.mezz_info->i2c_addr,
                                  100,
                                  &(otb_serial_conf.mezz_info->i2c_info));
  brzo_i2c_write_info(bytes, 1, FALSE, &(otb_serial_conf.mezz_info->i2c_info));
  brzo_i2c_read_info(val, 1, FALSE, &(otb_serial_conf.mezz_info->i2c_info));
  brc = brzo_i2c_end_transaction_info(&(otb_serial_conf.mezz_info->i2c_info));
  if (brc)
  {
    DEBUG("SERIAL: Failed to read reg addr: 0x%02x reg: 0x%02x real_reg: 0x%02x rc: %d", otb_serial_conf.mezz_info->i2c_addr, reg, bytes[0], brc)
  }
  
  DEBUG("SERIAL: otb_serial_mezz_read_reg exit");

  return brc;
}

// gpios should be bitmask with state
bool ICACHE_FLASH_ATTR otb_serial_mezz_gpio(uint8 gpios)
{
  uint8 bytes[2];
  bool rc = FALSE;
  uint8_t brc;
  int ii;
  
  DEBUG("SERIAL: otb_serial_mezz_gpio entry");

  // Reg 0x0b is "IOState"
  brc = otb_serial_mezz_write_reg(0x0B, gpios);
  if (!brc)
  {
    rc = TRUE;
  }

  DEBUG("SERIAL: otb_serial_mezz_gpio exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_serial_mezz_reset(bool hard)
{
  uint8 byte;
  bool rc = FALSE;
  uint8_t brc;
  int ii;

  DEBUG("SERIAL: otb_serial_mezz_reset entry");

  if (hard)
  {
    otb_gpio_set(OTB_SERIAL_MBUS_V0_1_RST_PIN, 0, FALSE);
    os_delay_us(1000);
    otb_gpio_set(OTB_SERIAL_MBUS_V0_1_RST_PIN, 1, FALSE);
  }
  else
  {
    // Issue a software reset - reg is 0x0e (IOControl)
    brc = otb_serial_mezz_write_reg(0x0E, 0b1000);
    // sc16IS752 doesn't ack on soft reset
    if (brc && (brc != 2))
    {
      goto EXIT_LABEL;
    }
  }

  os_delay_us(1000);

  // Now try, twice (first one after soft reset fails) to read LSR register
  for (ii = 0; ii < 2; ii++)
  {
    brc = otb_serial_mezz_read_reg(0x05, &byte);
    if (!brc)
    {
      break;
    }
  }
  if (brc)
  {
    goto EXIT_LABEL;
  }

  rc = TRUE;

EXIT_LABEL:

  DEBUG("SERIAL: otb_serial_mezz_reset exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_serial_mezz_configure(void)
{
#define OTB_SERIAL_MEZZ_CONFIGURE_REGS  11
  uint8 reg[OTB_SERIAL_MEZZ_CONFIGURE_REGS] = {0x01, 0x02, 0x03, 0x04, 0x0A, 0x0C, 0x0F, 0x03, 0x00, 0x01, 0x03};
  uint8 val[OTB_SERIAL_MEZZ_CONFIGURE_REGS] = {0b1, 0b1, 0b11, 0b0, 0b11, 0b0, 0b0, 0b10000011, 48, 0, 0b11};
  bool rc = FALSE;
  uint8_t brc;
  int ii;
  uint32_t baudrate;
  uint8_t br_lo, br_ho;
  uint8_t lcr = 0b11;

  DEBUG("SERIAL: otb_serial_mezz_configure entry");

  baudrate = 115200 / otb_serial_conf.baudrate; // Assumes a 1.8432 MHz crystal, generating 16x clock
  OTB_ASSERT(baudrate <= 0xffff);
  br_lo = baudrate & 0xff;
  br_ho = (baudrate>>8) & 0xff;
  val[8] = br_lo;
  val[9] = br_ho;

  if (otb_serial_conf.stopbit > 0)
  {
    lcr |= 0b100;
  }
  if (otb_serial_conf.parity != OTB_SERIAL_PARITY_NONE)
  {
    OTB_ASSERT((otb_serial_conf.parity == OTB_SERIAL_PARITY_EVEN) ||
               (otb_serial_conf.parity == OTB_SERIAL_PARITY_ODD));
    lcr |= (otb_serial_conf.parity == OTB_SERIAL_PARITY_EVEN) ? 0b11000 : 0b01000;
  }
  val[2] = lcr;
  val[7] = lcr | 0b10000000; // divisor latch enable
  val[10] = lcr;

  DEBUG("SERIAL: LCR 0x%02x br_lo 0x%02x br_hi: 0x%02x", lcr, br_lo, br_ho);

  for (ii = 0; ii < OTB_SERIAL_MEZZ_CONFIGURE_REGS; ii++)
  {
    brc = otb_serial_mezz_write_reg(reg[ii], val[ii]);
    if (brc)
    {
      goto EXIT_LABEL;
    }
  }

  rc = TRUE;

EXIT_LABEL:  

  DEBUG("SERIAL: otb_serial_mezz_configure exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_serial_enable_mezz()
{
  bool rc = FALSE;

  DEBUG("SERIAL: otb_serial_enable_mezz entry");

  // Reset
  if (!otb_serial_mezz_reset(TRUE))
  {
    goto EXIT_LABEL;
  }

  // Configure before turning on GPIOs
  if (!otb_serial_mezz_configure())
  {
    goto EXIT_LABEL;
  }

  if (!otb_intr_register(otb_serial_mezz_intr_handler, NULL, OTB_SERIAL_MBUS_V0_1_IRQ_PIN))
  {
    goto EXIT_LABEL;
  }

  // Turn on enabled (and connected) GPIO
  if (!otb_serial_mezz_gpio(0b11))
  {
    goto EXIT_LABEL;
  }

  rc = TRUE;

  INFO("SERIAL: Enabled serial mezz board 0x%02x at %d baud uart %d",
       otb_serial_conf.mezz_info->i2c_addr,
       otb_serial_conf.baudrate,
       otb_serial_conf.mezz_info->uart_num);

EXIT_LABEL:

  DEBUG("SERIAL: otb_serial_enable_mezz exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_serial_disable_mezz()
{
  bool rc = TRUE;

  DEBUG("SERIAL: otb_serial_disable_mezz entry");

  otb_intr_unreg(OTB_SERIAL_MBUS_V0_1_IRQ_PIN);
  
  // Reset
  if (!otb_serial_mezz_reset(TRUE))
  {
    // Try to carry on
    rc = FALSE;
  }

  // Configure before turning on GPIOs
  if (!otb_serial_mezz_configure())
  {
    // Try to carry on
    rc = FALSE;
  }

  // Turn on connected GPIO1
  if (!otb_serial_mezz_gpio(0b10))
  {
    // Try to carry on
    rc = FALSE;
  }

EXIT_LABEL:

  INFO("SERIAL: Disabled serial mezz board 0x%02x", otb_serial_conf.mezz_info->i2c_addr);

  DEBUG("SERIAL: otb_serial_disable_mezz exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_serial_mezz_byte_to_read(void)
{
  bool rc = FALSE;
  uint8_t brc;
  uint8_t rval;

  DEBUG("SERIAL: otb_serial_mezz_byte_to_read entry");

  brc = otb_serial_mezz_read_reg(0x05, &rval);
  if (!rc)
  {
    if (rval & 1)
    {
      rc = TRUE;
    }
  }

  DEBUG("SERIAL: otb_serial_mezz_byte_to_read exit");

  return rc;
}

void ICACHE_FLASH_ATTR otb_serial_mezz_read_data(void *arg)
{
  uint8_t brc;
  uint16_t pos;

  DEBUG("SERIAL: otb_serial_mezz_read_data entry");

  while (otb_serial_mezz_byte_to_read())
  {
    pos = otb_serial_conf.mezz_info->buf.tail;
    brc = otb_serial_mezz_read_reg(0x00, otb_serial_conf.mezz_info->buf.buf+pos);
    DEBUG("SERIAL: Read 0x%02x into positon: %d", otb_serial_conf.mezz_info->buf.buf[pos], pos);
    if (!brc)
    {
      // Succeeded

      // If we just wrote over the head, mark that we have overflowed the buffer
      // and move head on (but not if head and tail the same, as this happens
      // if the buffer is empty, or if have overflowed already
      if ((pos == otb_serial_conf.mezz_info->buf.head) && (otb_serial_conf.mezz_info->buf.head != otb_serial_conf.mezz_info->buf.tail))
      {
        otb_serial_conf.mezz_info->buf.head++;
        if (otb_serial_conf.mezz_info->buf.head >= otb_serial_conf.mezz_info->buf.buf_size)
        {
          otb_serial_conf.mezz_info->buf.head = 0;
        }
        otb_serial_conf.mezz_info->buf.overflow = TRUE;
      }

      // Now update the tail
      pos++;
      if (pos >= otb_serial_conf.mezz_info->buf.buf_size)
      {
        pos = 0;
      }
      otb_serial_conf.mezz_info->buf.tail = pos;
    }
  }

  DEBUG("SERIAL: otb_serial_mezz_read_data exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_serial_mezz_buf_data_avail(void)
{ 
  bool rc = FALSE;

  DEBUG("SERIAL: otb_serial_mezz_buf_data_avail entry");

  OTB_ASSERT(otb_serial_conf.mezz_info != NULL);

  if (otb_serial_conf.mezz_info->buf.tail != otb_serial_conf.mezz_info->buf.head)
  {
    rc = TRUE;
  }
  else if (otb_serial_conf.mezz_info->buf.overflow)
  {
    rc = TRUE;
  }

  DEBUG("SERIAL: otb_serial_mezz_buf_data_avail exit");

  return rc;
}

uint8_t ICACHE_FLASH_ATTR otb_serial_mezz_buf_get_next_byte(void)
{
  uint8_t byte;

  DEBUG("SERIAL: otb_serial_mezz_buf_get_next_byte entry");

  OTB_ASSERT(otb_serial_conf.mezz_info != NULL);
  OTB_ASSERT(otb_serial_mezz_buf_data_avail());

  byte = otb_serial_conf.mezz_info->buf.buf[otb_serial_conf.mezz_info->buf.head];
  DEBUG("SERIAL: Read 0x%02x from positon: %d", byte, otb_serial_conf.mezz_info->buf.head);
  otb_serial_conf.mezz_info->buf.head++;
  if (otb_serial_conf.mezz_info->buf.head >= otb_serial_conf.mezz_info->buf.buf_size)
  {
    otb_serial_conf.mezz_info->buf.head = 0;
  }
  otb_serial_conf.mezz_info->buf.overflow = FALSE;

  DEBUG("SERIAL: otb_serial_mezz_buf_get_next_byte exit");

  return byte;
}

void ICACHE_FLASH_ATTR otb_serial_mezz_intr_handler(void *arg)
{
  DEBUG("SERIAL: otb_serial_mezz_intr_handler entry");

  // Schedule timer immediately to read data from the SC16IS
  // (Don't do it in this interrupt handler)
  OTB_ASSERT(otb_serial_conf.mezz_info != NULL);
  os_timer_disarm(&otb_serial_conf.mezz_info->timer);
  os_timer_setfn(&otb_serial_conf.mezz_info->timer, otb_serial_mezz_read_data, NULL);
  os_timer_arm(&otb_serial_conf.mezz_info->timer, 0, 0);

  DEBUG("SERIAL: otb_serial_mezz_intr_handler exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_serial_mezz_send_byte(uint8_t byte)
{
  bool rc = FALSE;
  uint8_t brc;

  DEBUG("SERIAL: otb_serial_mezz_send_byte entry");

  brc = otb_serial_mezz_write_reg(0x00, byte);
  DEBUG("SERIAL: Sent 0x%02x", byte);
  if (!brc)
  {
    rc = TRUE;
  }

  DEBUG("SERIAL: otb_serial_mezz_send_byte exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_serial_config_handler(unsigned char *next_cmd,
                                                 void *arg,
                                                 unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  unsigned char *next_next_cmd;
  int cmd_type;
  otb_serial_config *conf = &otb_serial_conf;
  int pin;
  int baudrate;
  bool match;
  int ii;
  int stopbit;
  char *rs;
  uint8_t serial_data[256];
  int bytesd;
  int num;
  char *error;
    
  DEBUG("SERIAL: otb_serial_config_handler entry");
  
  cmd = (int)arg;
  cmd_type = OTB_SERIAL_CMD_TYPE_MASK & cmd;
  cmd = OTB_SERIAL_CMD_MASK & cmd;
  
  OTB_ASSERT((cmd_type == OTB_SERIAL_CMD_GET) ||
             (cmd_type == OTB_SERIAL_CMD_SET) ||
             (cmd_type == OTB_SERIAL_CMD_TRIGGER));
  
  switch (cmd)
  {
    case OTB_SERIAL_CMD_ENABLE:
      // Valid get, set
      if ((cmd_type != OTB_SERIAL_CMD_GET) && (cmd_type != OTB_SERIAL_CMD_SET))
      {
        otb_cmd_rsp_append("enable only valid on get or set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (cmd_type == OTB_SERIAL_CMD_SET)
      {
        if ((next_cmd == NULL) || (next_cmd[0] == 0) ||
            otb_mqtt_match(next_cmd, "yes") ||
            otb_mqtt_match(next_cmd, "true"))
        {
          if (((conf->mezz_info == NULL) && ((conf->rx == OTB_SERIAL_PIN_INVALID) || (conf->tx == OTB_SERIAL_PIN_INVALID))) ||
              ((conf->mezz_info == NULL) && (!conf->mezz_info->use_mezz)))
          {
            otb_cmd_rsp_append("cannot enable until rx and tx pins set");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          if (conf->enabled)
          {
            otb_cmd_rsp_append("already enabled");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          if (conf->mezz_info && conf->mezz_info->use_mezz)
          {
            conf->enabled = otb_serial_enable_mezz();
            if (conf->enabled)
            {
              rc = TRUE;
            }
            else
            {
              otb_cmd_rsp_append("failed to enable mezz");
              rc = FALSE;
              goto EXIT_LABEL;
            }
          }
          else
          {
            Softuart_SetPinRx(&(conf->softuart), conf->rx);
            Softuart_SetPinTx(&(conf->softuart), conf->tx);
            Softuart_Init(&(conf->softuart), conf->baudrate);
            conf->enabled = TRUE;
          }
          rc = TRUE;
        }
        else if (otb_mqtt_match(next_cmd, "no") ||
                 otb_mqtt_match(next_cmd, "false"))
        {
          if (!conf->enabled)
          {
            otb_cmd_rsp_append("already disabled");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          if (conf->mezz_info && conf->mezz_info->use_mezz)
          {
            otb_serial_disable_mezz();
          }
          conf->enabled = FALSE;
          rc = TRUE;
        }
        else
        {
          otb_cmd_rsp_append("invalid command: %s", next_cmd);
          rc = FALSE;
          goto EXIT_LABEL;
        }
      }
      else
      {
        // get
        rs = conf->enabled ? "yes" : "no";
        otb_cmd_rsp_append(rs);
        rc = TRUE;
      }
      break;

    case OTB_SERIAL_CMD_DISABLE:
      // Valid set
      if (cmd_type != OTB_SERIAL_CMD_SET)
      {
        otb_cmd_rsp_append("enable only valid on set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (!conf->enabled)
      {
        otb_cmd_rsp_append("already disabled");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (conf->mezz_info && conf->mezz_info->use_mezz)
      {
        otb_serial_disable_mezz();
      }
      conf->enabled = FALSE;
      rc = TRUE;
      break;

    case OTB_SERIAL_CMD_RX:
    case OTB_SERIAL_CMD_TX:
    case OTB_SERIAL_CMD_MEZZ:
      // Valid get, set
      if ((cmd_type != OTB_SERIAL_CMD_GET) && (cmd_type != OTB_SERIAL_CMD_SET))
      {
        otb_cmd_rsp_append("enable only valid on get or set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (cmd_type == OTB_SERIAL_CMD_SET)
      {
        if ((next_cmd == NULL) || (next_cmd[0] == 0))
        {
          otb_cmd_rsp_append("no value provided");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if (conf->enabled)
        {
          otb_cmd_rsp_append("can't change pin/mezz when enabled");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if (cmd == OTB_SERIAL_CMD_MEZZ)
        {
          if (conf->mezz_info == NULL)
          {
            otb_cmd_rsp_append("no mezz connected");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          if (!os_strcmp("uart", next_cmd)) // Already test for NULLness earlier
          {
            next_next_cmd = otb_cmd_get_next_cmd(next_cmd);
            if (next_next_cmd != NULL)
            {
              num = strtol(next_next_cmd, &error, 10);
              if (((error == NULL) || ((error != (char *)next_next_cmd) && (error[0] == 0))) && (num >= 0) && (num <= 1))
              {
                conf->mezz_info->uart_num = num;
              }
              else
              {
                otb_cmd_rsp_append("invalid uart num");
                rc = FALSE;
                goto EXIT_LABEL;
              }
            }
          }
          else if (!os_strcmp("on", next_cmd) ||
                   !os_strcmp("true", next_cmd) ||
                   !os_strcmp("yes", next_cmd))
          {
            // Check we're actually a serial supporting mezzanine!
            conf->mezz_info->use_mezz = TRUE;
          }
          else if (!os_strcmp("off", next_cmd) ||
                   !os_strcmp("false", next_cmd) ||
                   !os_strcmp("no", next_cmd))
          {
            conf->mezz_info->use_mezz = FALSE;
          }
          else
          {
            otb_cmd_rsp_append("invalid mezz value");
            rc = FALSE;
            goto EXIT_LABEL;
          }
        }
        else
        {
          pin = strtol(next_cmd, NULL, 10);
          if ((pin == 0) && (next_cmd[0] != '0'))
          {
            otb_cmd_rsp_append("invalid pin value");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          if (pin != -1)
          {
            if ((pin < 0) || (pin > 16))
            {
              otb_cmd_rsp_append("invalid pin value");
              rc = FALSE;
              goto EXIT_LABEL;
            }
            if (!otb_gpio_is_valid((uint8_t)pin))
            {
              otb_cmd_rsp_append("invalid pin value (may be reserved)");
              rc = FALSE;
              goto EXIT_LABEL;
            }
            if (((cmd == OTB_SERIAL_CMD_RX) && (pin == conf->tx)) ||
                ((cmd == OTB_SERIAL_CMD_TX) && (pin == conf->rx)))
            {
              otb_cmd_rsp_append("pin value clashes with other serial pin");
              rc = FALSE;
              goto EXIT_LABEL;
            }
          }
          if (cmd == OTB_SERIAL_CMD_RX)
          {
            conf->rx = pin;
          }
          else
          {
            conf->tx = pin;
          }
        }
        rc = TRUE;
      }
      else
      {
        // Get
        if (cmd == OTB_SERIAL_CMD_RX)
        {
          pin = conf->rx;
          otb_cmd_rsp_append("%d", pin);
        }
        else if (cmd == OTB_SERIAL_CMD_TX)
        {
          pin = conf->tx;
          otb_cmd_rsp_append("%d", pin);
        }
        else if (cmd == OTB_SERIAL_CMD_MEZZ)
        {
          if (conf->mezz_info == NULL)
          {
            otb_cmd_rsp_append("not present");
          }
          else if ((next_cmd != NULL) && (!os_strcmp(next_cmd, "uart")))
          {
            otb_cmd_rsp_append("%d", conf->mezz_info->uart_num);
          }
          else
          {
            otb_cmd_rsp_append("%s", conf->mezz_info->use_mezz ? "on" : "off");
          }
        }
        rc = TRUE;
      }
      break;

    case OTB_SERIAL_CMD_BAUD:
      // Valid get, set
      if ((cmd_type != OTB_SERIAL_CMD_GET) && (cmd_type != OTB_SERIAL_CMD_SET))
      {
        otb_cmd_rsp_append("baudrate only valid on get or set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (cmd_type == OTB_SERIAL_CMD_SET)
      {
        if (conf->enabled)
        {
          otb_cmd_rsp_append("can't change baudrate when enabled");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if ((next_cmd == NULL) || (next_cmd[0] == 0))
        {
          otb_cmd_rsp_append("no value provided");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        baudrate = atoi(next_cmd);
        match = FALSE;
        for (ii = 0; ii < OTB_SERIAL_B_NUM; ii++)
        {
          if (otb_serial_supported_baudrates[ii] = baudrate)
          {
            match = TRUE;
            break;
          }
        }
        if (!match)
        {
          otb_cmd_rsp_append("invalid baudrate");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        conf->baudrate = baudrate;
        rc = TRUE;
      }
      else
      {
        otb_cmd_rsp_append("%d", conf->baudrate);
        rc = TRUE;
      }
      break;

    case OTB_SERIAL_CMD_STOPBIT:
      // Valid get, set
      if ((cmd_type != OTB_SERIAL_CMD_GET) && (cmd_type != OTB_SERIAL_CMD_SET))
      {
        otb_cmd_rsp_append("enable only valid on get or set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (cmd_type == OTB_SERIAL_CMD_SET)
      {
        if (conf->enabled)
        {
          otb_cmd_rsp_append("can't change stopbit when enabled");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if ((next_cmd == NULL) || (next_cmd[0] == 0))
        {
          otb_cmd_rsp_append("no value provided");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        stopbit = atoi(next_cmd);
        if ((stopbit == 0) && (next_cmd[0] != '0'))
        {
          otb_cmd_rsp_append("invalid stopbit value provided");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if ((stopbit < OTB_SERIAL_STOPBIT_MIN) || (stopbit > OTB_SERIAL_STOPBIT_MAX))
        {
          otb_cmd_rsp_append("invalid stopbit value (min: %d, max:%d",
                             OTB_SERIAL_STOPBIT_MIN,
                             OTB_SERIAL_STOPBIT_MAX);
          rc = FALSE;
          goto EXIT_LABEL;
      
        }
        conf->stopbit = stopbit;
        rc = TRUE;
      }
      else
      {
        otb_cmd_rsp_append("%d", conf->stopbit);
        rc = TRUE;
      }
      break;

    case OTB_SERIAL_CMD_PARITY:
      // Valid get, set
      if ((cmd_type != OTB_SERIAL_CMD_GET) && (cmd_type != OTB_SERIAL_CMD_SET))
      {
        otb_cmd_rsp_append("enable only valid on get or set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (cmd_type == OTB_SERIAL_CMD_SET)
      {
        if (conf->enabled)
        {
          otb_cmd_rsp_append("can't change parity when enabled");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if ((next_cmd == NULL) || (next_cmd[0] == 0))
        {
          otb_cmd_rsp_append("no value provided");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        if (otb_mqtt_match(next_cmd, "even"))
        {
          conf->parity = OTB_SERIAL_PARITY_EVEN;
        }
        else if (otb_mqtt_match(next_cmd, "odd"))
        {
          conf->parity = OTB_SERIAL_PARITY_ODD;
        }
        else if (otb_mqtt_match(next_cmd, "none"))
        {
          conf->parity = OTB_SERIAL_PARITY_NONE;
        }
        else
        {
          otb_cmd_rsp_append("invalid parity");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        rc = TRUE;
      }
      else
      {
        switch (conf->parity)
        {
          case OTB_SERIAL_PARITY_NONE:
            rs = "none";
            break;

          case OTB_SERIAL_PARITY_EVEN:
            rs = "even";
            break;

          case OTB_SERIAL_PARITY_ODD:
            rs = "odd";
            break;

          default:
            otb_cmd_rsp_append("internal error");
            rc = FALSE;
            goto EXIT_LABEL;
            break;
        }
        otb_cmd_rsp_append(rs);
        rc = TRUE;
      }
      break;

    case OTB_SERIAL_CMD_COMMIT:
      // Valid set
      if (cmd_type != OTB_SERIAL_CMD_SET)
      {
        otb_cmd_rsp_append("enable only valid on set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      // XXX Implement commit (config)
      otb_cmd_rsp_append("not implemented");
      rc = FALSE;
      goto EXIT_LABEL;
      break;

    case OTB_SERIAL_CMD_BUFFER:
      // Valid get (all default not specified, or num bits), trigger (clear or empty)
      if (cmd_type != OTB_SERIAL_CMD_TRIGGER)
      {
        otb_cmd_rsp_append("enable only valid on trigger");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (!(conf->enabled))
      {
        otb_cmd_rsp_append("not enabled");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (otb_mqtt_match(next_cmd, "dump"))
      {
        next_next_cmd = otb_cmd_get_next_cmd(next_cmd);

        if ((next_next_cmd == NULL) || (next_next_cmd[0] == 0) || otb_mqtt_match(next_next_cmd, "all"))
        {
          ii = 0;
          if (conf->mezz_info && conf->mezz_info->use_mezz)
          {
            // Make sure we have all the data so read first
            otb_serial_mezz_read_data(NULL);
            while (otb_serial_mezz_buf_data_avail())
            {
              if (ii >= 25)
              {
                ii = 0;
                otb_mqtt_send_status(OTB_MQTT_STATUS_OK,
                                    otb_cmd_rsp_get(),
                                    "",
                                    "");
                otb_cmd_rsp_clear();
              }
              otb_cmd_rsp_append("%02x", otb_serial_mezz_buf_get_next_byte());
              ii++;
            }
          }
          else
          {
            while (Softuart_Available(&(conf->softuart)))
            {
              if (ii >= 25)
              {
                ii = 0;
                otb_mqtt_send_status(OTB_MQTT_STATUS_OK,
                                    otb_cmd_rsp_get(),
                                    "",
                                    "");
                otb_cmd_rsp_clear();
              }
              otb_cmd_rsp_append("%02x", Softuart_Read(&(conf->softuart)));
              ii++;
            }
          }
          otb_cmd_rsp_append("xx");
          rc = TRUE;
        }
        else
        {
          bytesd = atoi(next_next_cmd);
          if (((bytesd == 0) && (next_next_cmd[0] != '0')) ||
              (bytesd > 25) || (bytesd <= 0))
          {
            otb_cmd_rsp_append("invalid number of bytes (max 25)");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          if (conf->mezz_info && conf->mezz_info->use_mezz)
          {
            // Make sure we have all the data so read first
            otb_serial_mezz_read_data(NULL);
            for (ii = 0; ii < bytesd; ii++)
            {
              if (otb_serial_mezz_buf_data_avail())
              {
                otb_cmd_rsp_append("%02x", otb_serial_mezz_buf_get_next_byte());
              }
              else
              {
                // x marks end
                otb_cmd_rsp_append("xx");
                break;
              }
            }
          }
          else
          {
            for (ii = 0; ii < bytesd; ii++)
            {
              if (Softuart_Available(&(conf->softuart)))
              {
                otb_cmd_rsp_append("%02x", Softuart_Read(&(conf->softuart)));
              }
              else
              {
                // x marks end
                otb_cmd_rsp_append("xx");
                break;
              }
            }
          }
          rc = TRUE;
        }
      }
      else if ((otb_mqtt_match(next_cmd, "empty") || otb_mqtt_match(next_cmd, "clear")))
      {
        // Not the most efficient way to do it!
        if (conf->mezz_info && conf->mezz_info->use_mezz)
        {
          while (otb_serial_mezz_buf_data_avail())
          {
            otb_serial_mezz_buf_get_next_byte();
          }
        }
        else
        {
          while (Softuart_Available(&(conf->softuart)))
          {
            Softuart_Read(&(conf->softuart));
          }
        }
        rc = TRUE;
      }
      else
      {
        otb_cmd_rsp_append("invalid buffer command");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      break;

    case OTB_SERIAL_CMD_TRANSMIT:
      // Valid trigger only
      if (cmd_type != OTB_SERIAL_CMD_TRIGGER)
      {
        otb_cmd_rsp_append("enable only valid on set");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if ((next_cmd == NULL) || (next_cmd[0] == 0))
      {
        otb_cmd_rsp_append("nothing to transmit");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      if (!(conf->enabled))
      {
        otb_cmd_rsp_append("not enabled");
        rc = FALSE;
        goto EXIT_LABEL;
      }
      // Get the data
      for (ii = 0; ii < os_strlen(next_cmd); ii++)
      {
        rc = otb_i2c_mqtt_get_addr(next_cmd+(ii*2), serial_data+ii);
        if (!rc)
        {
          otb_cmd_rsp_append("invalid hex data");
          rc = FALSE;
          goto EXIT_LABEL;
        }
      }
      // Send it
      for (ii = 0; ii < (os_strlen(next_cmd)/2); ii++)
      {
        if (conf->mezz_info && conf->mezz_info->use_mezz)
        {
          rc = otb_serial_mezz_send_byte(serial_data[ii]);
          if (!rc)
          {
            otb_cmd_rsp_append("failed to send byte");
            rc = FALSE;
            goto EXIT_LABEL;
          }
          
        }
        else
        {
          Softuart_Putchar(&(conf->softuart), serial_data[ii]);
        }
      }
      rc = TRUE;
      break;

    default:
      OTB_ASSERT(FALSE);
      otb_cmd_rsp_append("Internal error");
      rc = FALSE;
      goto EXIT_LABEL;
  }

EXIT_LABEL:

  DEBUG("SERIAL: otb_serial_config_handler exit");
  
  return rc;
}