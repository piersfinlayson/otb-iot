..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson


GPIO Pins
=========

The GPIO pins are used for the following purposes by otb-iot:

===== ==============================================
 Pin    Purpose 
===== ==============================================
  0    SDA - internal I2C
  1    *Reserved - UART Transmit*
  2    SCL - internal I2C
  3    *Reserved - UART Receive*
  4    SDA - external I2C
  5    SCL - external I2C
  6    *Reserved - SD_CLK*
  7    *Reserved - SD_DATA0*
  8    *Reserved - SD_DATA0*
  9    *Reserved - SD_DATA0*
 10    *Reserved - SD_DATA0*
 11    *Reserved - SD_CMD*
 12    Used for one-wire protocol to external devices
 13    Drives status LED (WS2812B)
 14    Soft reset (reboot) and factory restore
 15    *Reserved for future use*
 16    Connected to RST for software driven reset
===== ==============================================

