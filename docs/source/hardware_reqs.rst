..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Hardware Requirements
=====================

Supported Modules
-----------------

otb-iot has been run on many off the shelf esp8266 based modules with 4MB flash, including:

* ESP-01 (various variants, with flash upgraded to 4MB)

* ESP-12 (various variants with 4MB flash)

* ESP-07S (not the original ESP-07 which had 512KB/1MB of flash)

* WEMOS D1 Mini

CPU
---

Only the ESP8266 is currently supported.

The ESP8285 is not, as it doesn't provide sufficient flash.

The ESP32 may be supported in future.

Flash
-----

otb-iot has been designed as a hardened IoT imlementation for the ESP8266, and as such includes 2 bootable application images, plus a third factory image, which is only used to replace a bootable image (it is not booted itself).  This means otb-iot requires >= 4MB (32Mbit) flash.

It would probably be possible to squeeze the entire implementation onto a 2MB flash chip, but the author has never seen any ESP8266 modules with 2MB chips, so it doesn't seem worthwhile to try.

Most ESP8266 modules, with the notable exception of the ESP-01, now contain 4MB of flash.  

If you need to check the flash size of your module, connect it to your serial port and run:

::

  $SDK_BASE/xtensa-lx106-elf/bin/esptool.py flash_id # $SDK_BASE is esp-open-sdk location

You should get an output like the following:

::

  Manufacturer: ef

  Device: 4016

In this example the flash chip is made by manufacturer 0xef (Winbond) and the device size is shown by the least significant bit of the devce ID.  Here that's 0x16. This indicates that the flash is 2^0x16 bytes in size - that's 2^22 = 4MByte.

If your chip shows 4014 you have a 1MB chip, and 4013 indicates a 512KByte chip.

Note, some manufacturers may use different device ID schemes to indicate the flash size.

The other way of figuring out the size is to get magnifying glass out and read the part number.  W25Q08FVSIG indicates an 8Mbit = 1MByte chip.

Upgrading the flash on a ESP-01 module is straightforward with the aid of a cheap (~$30) hot air station:

* Identify the flash chip - it's the larger of the two chips, with 8 legs.

* Note the location of pin 1 - normally indicated by a dot nearest this pin.

* With the hot air station heat the flash chip while tugging very gently with a pair of tweezers.

* Once removed solder a new chip in place with the hot air station - making sure to locate pin 1 appropriately.

The author typically replaces smaller flash chips with W25Q32FVSIG.  At the time of writing, these are available on aliexpress for US$2.45 for 10 including shipping.

GPIO
----

There's no fundamental requirement to expose or use any GPIO pins to run otb-iot - with the exception of ensuring GPIO 0, 2 and 15 are in the appropriate states for flashing and running.

However, not using any GPIO pins would defeat the point of otb-iot - it's designed to interface with the real world, for which GPIO pins are required.  See :doc:`GPIO Pins <gpio_pins>` for a description of what pins are used for what purpose.

