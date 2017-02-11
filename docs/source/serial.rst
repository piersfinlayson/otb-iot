..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Serial
======

The ESP8266's serial port is used for two main purposes in otb-iot:

* To flash the otb-iot binaries.

* To output logs from the otb-iot device.

otb-iot also supports :doc:`Serial over MQTT <serial_over_mqtt>`.

Pins
----

The standard ESP8266 serial pins (1-TX, 3-RX) are used by otb-iot.

Logs
----

A wealth of very useful logs are produced and transmitted over the serial port by otb-iot.


Connecting
----------

There are a variety of different solutions for accessing the serial port of an ESP8266 based device:

1 On board USB-TTY.  Some modules, such as the WEMOS D1 Mini come with an onboard USB-TTY chip, which allows the device to be powered and the serial port to be accessed via a single USB connector from a Windows, Mac or Linux device.

2 Using a separate USB-TTY device.  This connects to the computer using a USB port and then the TX and RX pins (+ GND) are connected to the ESP8266.

3 Connecting directly to the serial pins ising GPIOs from another device - for example an Arduino, another ESP8266 or a raspberry pi.

The first solution is the simplest, but also most expensive, particular if producing devices in quantity.

Note that there are different chips used by different vendors for USB-TTY conversion, such as:

* CP2102

* CP2104

* CH340G

* FT232RL

It has been the author's experience that some converters can be flaky.

Programming
-----------

It is much easier to program the device if a pair of transistors are used to allow the RTS and DTR lines of the serial port to drive GPIO 0 and the RST pin of he ESP8266.  This allows automatic switching of the device to be programmed into flash mode and back out - rather than requiring manual fiddling with the ESP8266 pins.

Speed
-----

otb-iot's default serial port speed is 115,200 baud.  However, due to a limitation in the ESP8266 chip, upon initial boot, or immediately after flashing, the bootloader will communicate at 74,880 baud.  However, after any soft resets the bootloader will communicate at 115,200.

Therefore the first time after flashing the bootloader will start at 74,880, and you must connect to the device (quickly) at this speed to collect first boot logs.


