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

#define OTB_MBUS_C
#include "otb.h"

MLOG("MBUS");

void ICACHE_FLASH_ATTR otb_mbus_hat_init(void)
{
  ENTRY;

  MDETAIL("M-Bus Master Hat is installed");
  otb_mbus_rx_buf_g.len = OTB_MBUS_RCV_BUF_LEN;
  otb_mbus_rx_buf_g.buf = os_malloc(otb_mbus_rx_buf_g.len);
  if (otb_mbus_rx_buf_g.buf == NULL)
  {
    MERROR("Failed to initialize M-bus Master Hat");
    MDETAIL("Failed to allocate receive buffer %d",
           otb_mbus_rx_buf_g.len);
    otb_mbus_hat_installed = FALSE;
    goto EXIT_LABEL;
  }
  otb_mbus_rx_buf_g.bytes = 0;

  // Need to use the MCP23017 which is on the internal I2C bus
  otb_mbus_i2c_bus = &otb_i2c_bus_internal;
  otb_mbus_mcp23017_addr = OTB_MBUS_MCP23017_ADDR;
  otb_i2c_initialize_bus_internal();
  otb_i2c_mcp23017_init(otb_mbus_mcp23017_addr, otb_mbus_i2c_bus);
  otb_mbus_baudrate = OTB_MBUS_B_DEFAULT;

  otb_mbus_hat_installed = TRUE;

EXIT_LABEL:

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_mbus_uart_conf(otb_mbus_uart_intr_handler_fn *intr_handler, void *arg)
{
  uint8_t conf0;
  ENTRY;
  
  // Attach interrupt handler
  if (intr_handler != NULL)
  {
    // XXX RTOS change
    // XXX Check return code
    gpio_isr_register(intr_handler, arg, 0, NULL);
  }

  // Set baud rate
  uart_div_modify(UART0, UART_CLK_FREQ / (UartDev.baut_rate));

  // Set first config byte
  SET_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST);
  CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST);
  // [0] 0 = even, 1 odd
  // [1] parity (1 = enabled, 0 disabled)
  // [3:2] 11 = 8 bits, 10 = 7 bits, 01 = 6 bits, 00 = 5 bits
  // [5:4] 01 = 1 stop bit
  conf0 = UartDev.parity & 1;
  conf0 |= (UartDev.exist_parity & 1) << 1;
  conf0 |= (UartDev.data_bits & 0b11) << 2;
  conf0 |= (UartDev.stop_bits & 0b11) << 4;
  WRITE_PERI_REG(UART_CONF0(UART0), conf0);
  WRITE_PERI_REG(UART_CONF1(UART0),
  ((100 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
  (0x02 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S |
  UART_RX_TOUT_EN);
  SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_TOUT_INT_ENA |UART_FRM_ERR_INT_ENA);
  WRITE_PERI_REG(UART_INT_CLR(UART0), 0xffff);
  SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA);
  PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

  // XXX RTOS change
  if (intr_handler != NULL)
  {
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_ANYEDGE);
  }
  else
  {
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_DISABLE);
  }

  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_enable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint8_t gpa, gpb;

  ENTRY;

  if (!otb_mbus_hat_installed)
  {
    otb_cmd_rsp_append("hat not installed");
    goto EXIT_LABEL;
  }

  if (otb_mbus_enabled)
  {
    otb_cmd_rsp_append("already enabled");
    goto EXIT_LABEL;
  }

  // The M-Bus Master Hat uses the TX/RX pins so disable logging
  otb_util_disable_logging();

  // Store off old UART config and apply M-Bus config
  os_memcpy(&uart_dev_backup, &UartDev, sizeof(UartDev));
  UartDev.baut_rate = otb_mbus_baudrate;
  UartDev.data_bits = 0b11;
  UartDev.exist_parity = 1;
  UartDev.parity = 0;
  UartDev.stop_bits = 1;
  otb_mbus_uart_conf(otb_mbus_recv_intr_handler, &otb_mbus_rx_buf_g);

  // XXX I don't know why I need to reinitialize the bus here - but I do
  otb_i2c_initialize_bus_internal();
  otb_i2c_mcp23017_init(otb_mbus_mcp23017_addr, otb_mbus_i2c_bus);

  // Need to enable GPIO26 (GPA0 on MCP23017)  
  gpa = 1; // GPA0 = 1
  gpb = 0;
  otb_i2c_mcp23017_write_gpios(gpa,
                              gpb,
                              otb_mbus_mcp23017_addr,
                              otb_mbus_i2c_bus);
  rc = TRUE;
  otb_mbus_enabled = TRUE;

EXIT_LABEL:

  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_disable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint8_t gpa, gpb;

  ENTRY;

  // Need to enable GPIO26 (GPA0 on MCP23017)  

  if (!otb_mbus_enabled)
  {
    otb_cmd_rsp_append("not enabled");
    goto EXIT_LABEL;
  }

  if (otb_mbus_scan_g.in_progress)
  {
    otb_cmd_rsp_append("scan in progress");
    goto EXIT_LABEL;
  }

  // Turn off the bus
  gpa = 0; // GPA0 = 0
  gpb = 0;
  otb_i2c_mcp23017_write_gpios(gpa,
                              gpb,
                              otb_mbus_mcp23017_addr,
                              otb_mbus_i2c_bus);

  // Copy back old UART config
  os_memcpy(&UartDev, &uart_dev_backup, sizeof(UartDev));
  otb_mbus_uart_conf(NULL, NULL);

  // Re-enable logging
  otb_util_enable_logging();
  otb_mbus_enabled = FALSE;
  rc = TRUE;

EXIT_LABEL:  

  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_scan(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char scan_str[6];
  int ii;
  uint8_t min, max;
  unsigned char *next_next_cmd;
  unsigned char scratch[32];

  ENTRY;

  if (!otb_mbus_enabled)
  {
    otb_cmd_rsp_append("not enabled");
    goto EXIT_LABEL;
  }

  if (otb_mbus_scan_g.in_progress)
  {
    otb_cmd_rsp_append("scan in progress");
    goto EXIT_LABEL;
  }

  // Figure out what type of scan we need to
  if ((next_cmd == NULL) || (next_cmd[0] == 0))
  {
    // Need to do a full range scan
    min = OTB_MBUS_ADDR_MIN;
    max = OTB_MBUS_ADDR_MAX;
    rc = TRUE;
  }
  else
  {
    min = atoi(next_cmd);
    if ((min < OTB_MBUS_ADDR_MIN) || (min > OTB_MBUS_ADDR_MAX))
    {
      otb_cmd_rsp_append("invalid address %s", next_cmd);
      goto EXIT_LABEL;
    }
    next_next_cmd = otb_cmd_get_next_cmd(next_cmd);
    if ((next_next_cmd == NULL) || (next_next_cmd[0] == 0))
    {
      // single address scan
      max = min;
    }
    else
    {
      max = atoi(next_next_cmd);
      if ((max < OTB_MBUS_ADDR_MIN) || (max > OTB_MBUS_ADDR_MAX))
      {
        otb_cmd_rsp_append("invalid address %s", next_cmd);
        goto EXIT_LABEL;
      }
    }
  }

  otb_mqtt_send_status("mbus", "scan", "started", NULL);

  os_memset(scan_str, 0, 6);
  scan_str[0] = 0x10;
  scan_str[1] = 0x40;
  scan_str[2] = min;
  scan_str[3] = otb_mbus_crc(scan_str);
  scan_str[4] = 0x16;
  ets_printf(scan_str);
#ifdef OTB_DEBUG  
  os_sprintf(scratch,
             "0x%02x%02x%02x%02x%02x",
             scan_str[0],
             scan_str[1],
             scan_str[2],
             scan_str[3],
             scan_str[4]);
  otb_mqtt_send_status("mbus", "scan", "sent", scratch);
#endif // OTB_DEBUG

  otb_mbus_scan_g.in_progress = TRUE;
  otb_mbus_scan_g.last = min;
  otb_mbus_scan_g.max = max;
  os_timer_disarm(&(otb_mbus_scan_g.timer));
  os_timer_setfn(&(otb_mbus_scan_g.timer), otb_mbus_scan_timerfn, arg);
  os_timer_arm(&(otb_mbus_scan_g.timer), OTB_MBUS_SCAN_TIMER, 0);

  otb_cmd_rsp_clear();

  rc = TRUE;

EXIT_LABEL:

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_mbus_scan_timerfn(void *arg)
{
  unsigned char scan_str[6];
  uint8_t next;
  unsigned char scratch[32];

  ENTRY;

  if (otb_mbus_scan_g.last < otb_mbus_scan_g.max)
  {
    otb_mbus_scan_g.last++;
    next = otb_mbus_scan_g.last;
  }
  else
  {
    otb_mqtt_send_status("mbus", "scan", "done", NULL);
    otb_mbus_scan_g.in_progress = FALSE;
    os_timer_disarm(&(otb_mbus_scan_g.timer));
    goto EXIT_LABEL;
  }

  os_memset(scan_str, 0, 6);
  scan_str[0] = 0x10;
  scan_str[1] = 0x40;
  scan_str[2] = next;
  scan_str[3] = otb_mbus_crc(scan_str);
  scan_str[4] = 0x16;
  ets_printf(scan_str);
#ifdef OTB_DEBUG  
  os_sprintf(scratch,
             "0x%02x%02x%02x%02x%02x",
             scan_str[0],
             scan_str[1],
             scan_str[2],
             scan_str[3],
             scan_str[4]);
  otb_mqtt_send_status("mbus", "scan", "sent", scratch);
#endif // OTB_DEBUG

  otb_mbus_scan_g.in_progress = TRUE;
  os_timer_disarm(&(otb_mbus_scan_g.timer));
  os_timer_setfn(&(otb_mbus_scan_g.timer), otb_mbus_scan_timerfn, arg);
  os_timer_arm(&(otb_mbus_scan_g.timer), OTB_MBUS_SCAN_TIMER, 0);

EXIT_LABEL:

  EXIT;

  return;
}

// data must be NULL terminated
uint8_t ICACHE_FLASH_ATTR otb_mbus_crc(uint8_t *data)
{
  uint8_t crc = 0;
  int ii;

  ENTRY;

  // Ignore first byte
  if (data[0] == 0)
  {
    goto EXIT_LABEL;
  }
  for (ii = 1; data[ii] != 0; ii++)
  {
    crc = (crc + data[ii]) & 0xff;
  }

EXIT_LABEL:

  EXIT;

  return crc;
}

bool ICACHE_FLASH_ATTR otb_mbus_get_data(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char scan_str[6];
  int addr;

  ENTRY;

  if (!otb_mbus_enabled)
  {
    otb_cmd_rsp_append("not enabled");
    goto EXIT_LABEL;
  }

  if (otb_mbus_scan_g.in_progress)
  {
    otb_cmd_rsp_append("scan in progress");
    goto EXIT_LABEL;
  }

  if ((next_cmd == NULL) || (next_cmd[0] == 0) || (next_cmd[0] == '/'))
  {
    otb_cmd_rsp_append("no address");
    goto EXIT_LABEL;
  }

  addr = atoi(next_cmd);
  if ((addr < OTB_MBUS_ADDR_MIN) || (addr > OTB_MBUS_ADDR_MAX))
  {
    otb_cmd_rsp_append("invalid address");
    goto EXIT_LABEL;
  }

  MDETAIL("Querying slave at address %d", addr);

  // Init the bus
  os_memset(scan_str, 0, 6);
  scan_str[0] = 0x10;
  scan_str[1] = 0x40;
  scan_str[2] = 0xFD;
  scan_str[3] = otb_mbus_crc(scan_str);
  scan_str[4] = 0x16;
  ets_printf(scan_str);
  ets_printf(scan_str);

  // Query data
  os_memset(scan_str, 0, 6);
  scan_str[0] = 0x10;
  scan_str[1] = 0x5B;
  scan_str[2] = addr;
  scan_str[3] = otb_mbus_crc(scan_str);
  scan_str[4] = 0x16;
  ets_printf(scan_str);

  otb_cmd_rsp_append("sent message");

  rc = TRUE;

EXIT_LABEL:

  EXIT;

  return rc;
}

void ICACHE_FLASH_ATTR otb_mbus_recv_data(void *arg)
{
  int ii, jj;
  uint16_t left_bytes;
  uint8_t byte;
  unsigned char scratch[(OTB_MBUS_RCV_BUF_LEN*2)+1];
  otb_mbus_rx_buf *buf = arg;

  ENTRY;

  // Don't disarm timer, as it's possible interrupt has fired again and
  // re-armed timer
  OTB_ASSERT(buf != NULL); // Passed in from interrupt routine

  if (otb_mbus_scan_g.in_progress)
  {
    // We are doing a scan - so process this message
    if ((buf->bytes == 1) && (buf->buf[0] == 0xE5))
    {
      os_sprintf(scratch, "%d", otb_mbus_scan_g.last);
      otb_mqtt_send_status("mbus", "scan", "ok", scratch);
    }
    else
    {
      for (ii = 0; (ii < buf->bytes) && (ii < OTB_MBUS_MAX_SEND_LEN); ii++)
      {
        os_sprintf(scratch + (ii*2), "%02x", buf->buf[ii]);
      }
      buf->bytes -= ii;
      // Ignore duff bytes while scanning
      //if (ii > 0)
      //{
      //  otb_mqtt_send_status("mbus", "scan", "error", scratch);
      //}
    }
  }
  else
  {
    for (ii = 0; (ii < buf->bytes) && (ii < OTB_MBUS_MAX_SEND_LEN); ii++)
    {
      os_sprintf(scratch + (ii*2), "%02x", buf->buf[ii]);
    }
    buf->bytes -= ii;
    if (ii > 0)
    {
      otb_mqtt_send_status("mbus", "data", scratch, NULL);
    }
  }
  
  // Only send a certain number of bytes at once
  jj = 0;
  left_bytes = buf->bytes;
  while (jj < left_bytes)
  {
    os_memcpy(buf->buf+jj, buf->buf+ii, OTB_MBUS_MAX_SEND_LEN);
    jj += OTB_MBUS_MAX_SEND_LEN;
    ii += OTB_MBUS_MAX_SEND_LEN;
  }

  // Reschedule to send again
  if (buf->bytes > 0)
  {
    os_timer_disarm(&otb_mbus_recv_buf_timer);
    os_timer_setfn(&otb_mbus_recv_buf_timer, otb_mbus_recv_data, arg);
    os_timer_arm(&otb_mbus_recv_buf_timer, 0, 0);
  }

  EXIT;

  return;
}

// No ICACHE_FLASH_ATTR
void otb_mbus_recv_intr_handler(void *arg)
{
  uint8_t rx_len;
  char rx_char;
  uint8_t ii;
  otb_mbus_rx_buf *buf = (otb_mbus_rx_buf *)arg;

  if (UART_FRM_ERR_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_FRM_ERR_INT_ST))
  {
    // Error
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
  }
  else if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST))
  {
    // FIFO Full
    CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
  }
  else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST))
  {
    // Timeout (character received)
    CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
  }
  else if (UART_RXFIFO_OVF_INT_ST  == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_OVF_INT_ST))
  {
    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
  }

  // Read in any data
  rx_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;
  if (rx_len > 0)
  {
    os_timer_disarm(&otb_mbus_recv_buf_timer);
    os_timer_setfn(&otb_mbus_recv_buf_timer, otb_mbus_recv_data, arg);
    os_timer_arm(&otb_mbus_recv_buf_timer, 0, 0);
 }
  for (ii = 0; ii < rx_len; ii++)
  {
    rx_char = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
    if (buf != NULL)
    {
      if (buf->bytes < OTB_MBUS_RCV_BUF_LEN)
      {
        buf->buf[buf->bytes] = rx_char;
        buf->bytes++;
      }
    }
  }

  WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
  SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
}

bool ICACHE_FLASH_ATTR otb_mbus_config_handler(unsigned char *next_cmd,
                                               void *arg,
                                               unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  int cmd_type;
  int baudrate;
  bool match;
  int ii;
    
  ENTRY;

  cmd = (int)arg;
  cmd_type = OTB_SERIAL_CMD_TYPE_MASK & cmd;
  cmd = OTB_SERIAL_CMD_MASK & cmd;
  
  OTB_ASSERT((cmd_type == OTB_SERIAL_CMD_SET) ||
             (cmd_type == OTB_SERIAL_CMD_GET));
  
  if (!otb_mbus_hat_installed)
  {
    otb_cmd_rsp_append("hat not installed");
    rc = FALSE;
    goto EXIT_LABEL;
  }
  switch (cmd)
  {
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
        if (otb_mbus_enabled)
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
          if (otb_mbus_supported_baudrates[ii] = baudrate)
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
        otb_mbus_baudrate = baudrate;
        rc = TRUE;
      }
      else
      {
        otb_cmd_rsp_append("%d", otb_mbus_baudrate);
        rc = TRUE;
      }
      break;

    default:
      OTB_ASSERT(FALSE);
      otb_cmd_rsp_append("Internal error");
      rc = FALSE;
      goto EXIT_LABEL;
  }

EXIT_LABEL:

  EXIT;
  
  return rc;
}