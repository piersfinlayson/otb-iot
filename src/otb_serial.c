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
  
  DEBUG("SERIAL: otb_serial_init exit");
  
  return;
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
          if ((conf->rx == OTB_SERIAL_PIN_INVALID) || (conf->tx == OTB_SERIAL_PIN_INVALID))
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
          Softuart_SetPinRx(&(conf->softuart), conf->rx);
          Softuart_SetPinTx(&(conf->softuart), conf->tx);
          Softuart_Init(&(conf->softuart), conf->baudrate);
          conf->enabled = TRUE;
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
      conf->enabled = FALSE;
      rc = TRUE;
      break;

    case OTB_SERIAL_CMD_RX:
    case OTB_SERIAL_CMD_TX:
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
          otb_cmd_rsp_append("can't change pin when enabled");
          rc = FALSE;
          goto EXIT_LABEL;
        }
        pin = atoi(next_cmd);
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
        rc = TRUE;
      }
      else
      {
        // Get
        if (cmd == OTB_SERIAL_CMD_RX)
        {
          pin = conf->rx;
        }
        else
        {
          pin = conf->tx;
        }
        otb_cmd_rsp_append("%d", pin);
        rc = TRUE;
      }
      break;

    case OTB_SERIAL_CMD_BAUD:
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
        if ((stopbit < OTB_SERIAL_STOPBIT_MIN) || (stopbit > OTB_SERIAL_STOPBIT_MIN))
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
          rc = TRUE;
        }
      }
      else if ((otb_mqtt_match(next_cmd, "empty") || otb_mqtt_match(next_cmd, "clear")))
      {
        // XXX Implement trigger/buffer/empty|clear
        otb_cmd_rsp_append("Not implemented");
        rc = FALSE;
        goto EXIT_LABEL;
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
        Softuart_Putchar(&(conf->softuart), serial_data[ii]);
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