..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Eeprom
========

Introduction
------------

otb-iot supports an onboard I2C eeprom continuing hardware and basic configuration information.  This eeprom is expected to reside at address 0x57, and to be an 24LC128 (128kbit, 16kbyte) eeprom.

Data is stored on the eeprom using the structures defined in `otb_eeprom.h`_.

.. _otb_eeprom.h: https://github.com/piersfinlayson/otb-iot/blob/master/include/otb_eeprom.h

Information
-----------

The first structure is otb_eeprom_info and lives at address 0x0.  At the end of this structure there are a series of otb_eeprom_info_comp structures.  These point to the other structures available on the eeprom.

On an otb-iot main board, the following other stuctures are present:

* otb_eeprom_main_board

  * contains global information about the board such as chipid, mac addresses, number of modules

* otb_eeprom_main_board_sdk_init_data

  * stores sdk_init_data which is reflashed to the board in the event of a factory reset

* otb_eeprom_main_board_gpio_pins

  * purposes of the board's :doc:`gpio_pins <gpio_pins>` 

* otb_eeprom_main_board_module (may be multiple of)

  * information about each of the module exposed by this main board (number of headers, pins, what each pin is, etc)

Generating Eeprom Data
---------------------- 

The `hwinfo`_ application is provided to burn the eeprom with the appropriate strucures over I2C.  It needs to be run on a device which has native I2C support and GPIO pins - such as the raspberry pi.

.. _hwinfo: https://github.com/piersfinlayson/otb-iot/blob/master/tools/hwinfo

The following is an example invocation of hwinfo:

::

  hwinfo -v -e 128 -i 123456 -1 abcdef -2 bcdef1 -f 4096 -d 0 -t 2 -c 1 -s 1 -m 1 -z 987123

Breaking this down:

* -v

  * runs in verbose mode

* -e 128

  * size in kbit of eeprom (128kbit, 16kbyte)

* -i 123456

  * chipid of esp8266

* -1 abcdef

  * prefix of station mac address (first 3 bytes)

* -2 bcdef1

  * prefix of AP mac address (first 3 bytes)

* -f 4096

  * Size of esp8266 flash in kbyte (4Mbyte)

* -d 0

  * Type of external ADC installed on the main board (0 = none)

* -t 2

  * Configuration of esp8266 ADC configuration (2 = 220K/100K resistive divider)

* -c 1

  * otbiot hardware code (1 = otbiot main board)

* -s 1

  * otbiot hardware subcode (1 = main board v0.4)

* -m 1

  * esp module type (1 = ESP12)

* -z 987123

  * Board serial number

This generates a binary file, hwinfo.out, with the data to be burnt to the eeprom

Setting up Pi to Burn Eeprom
----------------------------

* Install raspbian_

* Set up I2C on the pi

  * sudo rasp-config

  * select option 5 (interacing options)

  * select option P5 (I2C)

  * select yes

  * exit raspi-config

  * sudo reboot

  * i2cdetect -y 1 (may need to use 0 on an early raspberry pi)

    * check this doesn't fail

* Check out otb-iot on your pi and build hwinfo:
  
  * git clone --recursive https://github.com/piersfinlayson/otbiot

  * cd otb-iot

  * make hwinfo

  * make i2c-tools

* Wire your pi up to the I2C bus the eeprom is install on - these instructions assume using a pi zero and that the I2C bus is an otb-iot v0.4 module 1

  * Wire pi pin 3 (SDA) to module 1, header 1, pin 9

  * Wire pi pin 5 (SCL) to module 1, header 1, pin 10

  * Wire pi pin 6 (GND) to module 1, header 1, pin 7

  * Wire header 1 pin 11 (GND) to header 2 pin 1 (to disable write protect on the eeprom)

* Power on the eeprom/otb-ot device

* Get the chipid and mac address information from the otbiot/esp device to provide hwinfo

  * make flash_stage (this flashes an application to the esp8266 which retrieves this information)

  * connect to the esp8266 device over serial and note down the chipid and mac address prefixes

* Run hwinfo to get hwinfo.out

.. _raspbian: https://www.raspbian.org/

Burning Eeprom
--------------

* Burn the eeprom with this command:

  * bin/eeprog /dev/i2c-1 0x57 -16 -f -w 0x0 < hwinfo.out

* Disconnect the write protect pin (to renable write protect)

* Check the data flashed corectly by reading it back:

  * bin/eeprog /dev/i2c-1 0x57 -16 -xf -r 0x0:0x3fff > eeprom.out

  * hexedit eeprom.out

    * Check the data is as expected (and not all zeros or all fs)
