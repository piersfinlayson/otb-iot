..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

ESPi
====

The ESPi is a small computer powered by an ESP8266 WiFi processor in a Raspberry Pi Zero form factor, with a pin-out resembling that of the Raspberry Pi.

It can be used to drive Hats that have been developed for the ESP8266, such as the `M-Bus Master Hat`_.

.. M-Bus Master Hat: https://piers.rocks/mbus/libmbus/raspberry/pi/rpi/serial/meter/m-bus/hat/2019/03/03/m-bus-master-hat-raspberry-pi.html

Note - this documentation refers to ESPi hardware revision v1.1 onwards.

GPIOs
=====

The ESPi supports nearly as many GPIOs as the standard Raspberery Pi models, by using an `MCP23017`_ GPIO expander IC.  This can be controlled by the ESP8266 using I2C - the MCP23017 is on the internal I2C busat address 0x20.

.. MCP23017: https://piers.rocks/i2c/mcp23016/mcp23017/gpio/2018/09/11/differences-between-mcp23017-and-mcp23018.html

However, the ESPi is still 2 GPIOs short, and provides jumpers on-board to allow the user to select between exposing GPIO13 or GPIO5, and between GPIO19 or GPIO6.  The GPIO numbers here refer to the BCM GPIO numbers:

* GPIO5 is pin 29
* GPIO6 is pin 31
* GPIO13 is pin 33
* GPIO19 is pin 35

Where possible the boot pin pull-up and pull-down states of the Raspberry Pi are replicated, but this is not the case for all GPIOs.

As for the Raspberry Pi the ESP8266's GPIOs are tolerant to voltages up to 3.3V.  Connecting the ESPi's GPIOs to voltages higher than this will likely cause damage.

40-pin header
=============

The ESPi 40-pin header is designed to be compatible and interchangeable with the Raspberry Pi's 40-pin header.  The pin-out is as follows, with all GPIOs using the Raspberry Pi/BCM numbering scheme, and additional functions and mapping to ESP8266/MCP23017 pins listed in brackets:

1. 3.3V
2. 5V
3. GPIO2 (SDA, ESP_GPIO4)
4. 5V
5. GPIO3 (SCL, ESP_GPIO5)
6. GND
7. GPIO4 (MCP_GPB7)
8. GPIO14 (ESP_TX)
9. GND
10. GPIO15 (ESP_RX)
11. GPIO17 (MCP_GPB6)
12. GPIO18 (MCP_GPB4)
13. GPIO27 (MCP_GBP5)
14. GND
15. GPIO22 (MCP_GPB3)
16. GPIO23 (MCP_GPB2)
17. 3.3V
18. GPIO24 (MCP_GPB1)
19. GPIO10 (ESP_GPIO13)
20. GND
21. GPIO9 (ESP_GPIO12)
22. GPIO25 (MCP_GPB0)
23. GPIO11 (ESP_GPIO15, pulled-down)
24. GPIO8 (ESP_GPIO14, pulled-up)
25. GND
26. GPIO7 (MCP_GPA7)
27. GPIO0 (ID_SD, ESP_GPIO0)
28. GPIO1 (ID_SC, ESP_GPIO2)
29. GPIO5 (MCP_GPA4 selectable)
30. GND
31. GPIO6 (MCP_GPA5 selectable)
32. GPIO12 (MCP_GPA6)
33. GPIO13 (MCP_GPA5 selectable)
34. GND
35. GPIO19 (MCP_GPA4 selectable)
36. GPIO16 (MCP_GPA3)
37. GPIO26 (MCP_GPA0)
38. GPIO20 (MCP_GPA2)
39. GND
40. GPIO21 (MCP_GPA1)

Internal I2C Bus
================

The ESPi includes an internal I2C bus, exposed on the same pins as the Raspberry Pi's internal I2C bus.  This is on pins 27 ID_SD and 28 ID_SC, and on the Pi this bus is used to read EEPROM information from attached Hats.

As well as being able to read the EEPROM information from an attached Hat at the standard Pi Hat EEPROM I2C address of 0x50, the ESPi also has the following on-board devices attached to the I2C bus:

* MCP23017 (0x20) which acts as a GPIO expander, augmenting the ESP8266's built-in GPIOs
* config EEPROM (0x57) containing the ESPi's own configuration)

Other devices can be connected to this bus using the ID_SD and ID_SC pins, but care must be taken not to clash with the addresses of built-in devices, nor to short the bus to high or low.

Buttons and LEDs
================

LEDs
----

There are 2-3 LEDs on the ESPi:

* A green power LED in the same position as on the Raspberry Pi Zero.
* An RGB status LED (using a WS2812B, connected to ESP_GPIO15, GPIO11)
* If the ESPi uses an ESP12-F, a blue LED connected to GPIO0, which shows activity while booting)

Buttons
-------

There are 2 buttons the ESPi:

* A hard reset button, connected to ESP_GPIO16 (also exposed via the programming header), which if pulled down resets the ESPi.
* A button connected to ESP_GPIO14, GPIO8 which when using otb-iot software causes the device to reset, and if held down during rebooting resets the unit to factory defaults

ESPi Programmer
===============

The ESPi Programmer connects to the ESPi like a hat, using the ESPi's 40-pin header, and 4-pin programming header.  The pin-out is as follows:

1. D+ (from the micro USB connector on the ESPi)
2. D- (from the micro USB connector on the ESPi)
3. WP (write protect for the ESPi's config EEPROM, pull low to disable write protection)
4. ~ESP_RST (GPIO16 from the ESP8266, can be pulled low to reset the ESPi)

Note that when the programming board is connected to the ESPi, the ESPi cannot read devices on the internal I2C bus (ID_SD, ID_SC) as the prgrammer pulls GPIO0 to a high value.

Manual programming
==================

ESP8266
--------

If programming the ESP8266 on board the ESPi manually (without a dedicated programming board) the following pins must be used:

* Programming header pin 4 (ESP_RST)
* Pin 27 of the 40 pin header (ESP_GPIO0)
* Pin 8 of the 40 pin header (ESP_TX)
* Pin 10 of the 40 pin header (ESP_RX)
* A ground pin

If the USB connector on the ESPi is to be used for connecting into the machine which will program the device, then also connect:

* Programming header pin 1 (D+)
* Programming header pin 2 (D-)

If good connections are made the ESP8266 can be programmed at 921,600 baud.

EEPROM
------

If programming the ESPi's EEPROM, the following pins must be used:

* Programming header pin 3 (EEPROM_WP)
* Pin 27 of the 40 pin header ID_SD/ESP_GPIO0
* Pin 28 of the 40 pin header ID_SC/ESP_GPIO2
* A ground pin

Additional devices
==================

DS18B20
-------

Up to 8 DS18B20 temperature sensors will be detected by the otb-iot software automatically if the data pins are connected to GPIO9 (ESP_GPIO12), pin 21 of the 40 pin header.  The DS18B20s can be powered by any 3.3V pin and a ground pin.