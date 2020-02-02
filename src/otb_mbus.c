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

void ICACHE_FLASH_ATTR otb_mbus_hat_init(void)
{
  DEBUG("MBUS: otb_mbus_hat_init entry");

  DETAIL("MBUS: M-Bus Master Hat is installed");
  otb_mbus_rx_buf_g.len = OTB_MBUS_RCV_BUF_LEN;
  otb_mbus_rx_buf_g.buf = os_malloc(otb_mbus_rx_buf_g.len);
  if (otb_mbus_rx_buf_g.buf == NULL)
  {
    ERROR("MBUS: Failed to initialize M-bus Master Hat");
    DETAIL("MBUS: Failed to allocate receive buffer %d",
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

  otb_mbus_hat_installed = TRUE;

EXIT_LABEL:

  DEBUG("MBUS: otb_mbus_hat_init exit");

  return;
}

void ICACHE_FLASH_ATTR otb_mbus_uart_conf(otb_mbus_uart_intr_handler_fn *intr_handler, void *arg)
{
  uint8_t conf0;
  DEBUG("MBUS: otb_mbus_uart_conf entry");
  
  // Attach interrupt handler
  if (intr_handler != NULL)
  {
    ETS_UART_INTR_ATTACH(intr_handler, arg);
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

  if (intr_handler != NULL)
  {
    ETS_UART_INTR_ENABLE();
  }
  else
  {
    ETS_UART_INTR_DISABLE();
  }

  DEBUG("MBUS: otb_mbus_uart_conf exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_enable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint8_t gpa, gpb;

  // DEBUG("MBUS: otb_mbus_hat_enable entry");

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
  UartDev.baut_rate = 2400;
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

  // DEBUG("MBUS: otb_mbus_hat_enable exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_hat_disable(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint8_t gpa, gpb;

  DEBUG("MBUS: otb_mbus_hat_disable entry");

  // Need to enable GPIO26 (GPA0 on MCP23017)  

  if (!otb_mbus_enabled)
  {
    otb_cmd_rsp_append("not enabled");
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

  DEBUG("MBUS: otb_mbus_hat_disable exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_scan(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char scan_str[6];
  int ii;

  DEBUG("MBUS: otb_mbus_scan entry");

  if (!otb_mbus_enabled)
  {
    otb_cmd_rsp_append("not enabled");
    goto EXIT_LABEL;
  }

  scan_str[0] = 0x10;
  scan_str[1] = 0x40;
  scan_str[2] = 0x30;
  scan_str[3] = 0x70;
  scan_str[4] = 0x16;
  scan_str[5] = 0;

  ets_printf(scan_str);

  otb_cmd_rsp_append("sent message");

  rc = TRUE;

EXIT_LABEL:

  DEBUG("MBUS: otb_mbus_scan exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_mbus_get_data(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  unsigned char scan_str[6];
  int ii;

  DEBUG("MBUS: otb_mbus_get_data entry");

  if (!otb_mbus_enabled)
  {
    otb_cmd_rsp_append("not enabled");
    goto EXIT_LABEL;
  }

  // Init the bus
  scan_str[0] = 0x10;
  scan_str[1] = 0x40;
  scan_str[2] = 0xFD;
  scan_str[3] = 0x3D;
  scan_str[4] = 0x16;
  scan_str[5] = 0;

  ets_printf(scan_str);
  ets_printf(scan_str);

  // Query data
  scan_str[0] = 0x10;
  scan_str[1] = 0x5B;
  scan_str[2] = 0x30;
  scan_str[3] = 0x8B;
  scan_str[4] = 0x16;
  scan_str[5] = 0;
  ets_printf(scan_str);

  otb_cmd_rsp_append("sent message");

  rc = TRUE;

EXIT_LABEL:

  DEBUG("MBUS: otb_mbus_get_data exit");

  return rc;
}

void ICACHE_FLASH_ATTR otb_mbus_recv_data(void *arg)
{
  int ii;
  uint8_t byte;
  unsigned char scratch[(OTB_MBUS_RCV_BUF_LEN*2)+1];
  otb_mbus_rx_buf *buf = arg;

  DEBUG("MBUS: otb_mbus_recv_data entry");

  // Don't disarm timer, as it's possible interrupt has fired again and
  // re-armed timer
  OTB_ASSERT(buf != NULL); // Passed in from interrupt routine
  for (ii = 0; ii < buf->bytes; ii++)
  {
    os_sprintf(scratch + (ii*2), "%02x", buf->buf[ii]);
  }
  buf->bytes = 0;
  if (ii > 0)
  {
    otb_mqtt_send_status("mbus", "data", scratch, NULL);
  }

  DEBUG("MBUS: otb_mbus_recv_data exit");

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
