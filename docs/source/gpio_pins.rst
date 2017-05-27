..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson


GPIO Pins
=========

The GPIO pins are used for the various purposes by otb-iot.  The purposes can be configured via an onboard :doc:`eeprom <eeprom>`, or otb-iot will use defaults.

The defaults assume a WeMoS D1 Mini board:

===== ==============================================
 Pin    Purpose 
===== ==============================================
  0    internal I2C
  1    *Reserved - UART Transmit*
  2    internal I2C
  3    *Reserved - UART Receive*
  4    available GPIO
  5    available GPIO
  6    *Reserved - SD_CLK*
  7    *Reserved - SD_DATA0*
  8    *Reserved - SD_DATA0*
  9    *Reserved - SD_DATA0*
 10    *Reserved - SD_DATA0*
 11    *Reserved - SD_CMD*
 12    available GPIO
 13    available GPIO
 14    available GPIO
 15    available GPIO
 16    available GPIO
===== ==============================================

THe otb-iot main board v0.4 uses pins for following purposes - these are configured using an onboard 24LC128 :doc:`eeprom <eeprom>`.

===== ==============================================
 Pin    Purpose 
===== ==============================================
  0    SDA - internal I2C
  1    *Reserved - UART Transmit*
  2    SCL - internal I2C
  3    *Reserved - UART Receive*
  4    available GPIO
  5    available GPIO
  6    *Reserved - SD_CLK*
  7    *Reserved - SD_DATA0*
  8    *Reserved - SD_DATA0*
  9    *Reserved - SD_DATA0*
 10    *Reserved - SD_DATA0*
 11    *Reserved - SD_CMD*
 12    available GPIO
 13    available GPIO
 14    Soft reset (reboot) and factory restore
 15    Drives status LED (WS2812B)
 16    Connected to RST for software driven reset
===== ==============================================

