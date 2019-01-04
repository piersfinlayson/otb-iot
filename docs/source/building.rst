..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Building
========

Introduction
------------

otb-iot is intended to be built on linux and has been successfully built on various distributions including:

* Ubuntu

* Raspbian

* Arch Linux

Quick-Start
-----------

Plug your ESP8266 device into a USB port on your linux machine (or, if you're using VirtualBox, [map the USB device](https://www.eltima.com/article/virtualbox-usb-passthrough/) through from your host to your guest).

Then run:

```
dmesg | grep usb
```

You should see output like this (in this example I am using a Wemos D1 mini - the precise text with vary depending on the USB TTL device you are using):

```
[90279.476382] usbcore: registered new interface driver usbserial_generic
[90279.476412] usbserial: USB Serial support registered for generic
[90279.480721] usbcore: registered new interface driver ch341
[90279.480755] usbserial: USB Serial support registered for ch341-uart
[90279.481709] usb 2-1.8: ch341-uart converter now attached to ttyUSB1
```

Note the value provided right at the end - here _ttyUSB1_ - you'll need this in a sec.


Install docker if not already installed:

```
curl https://get.docker.com/|sh
```

You may need to log out and log back in again at this point so that your user is part of the docker group.

Run the container containing pre-built otb-iot images.  Change <usb-device> to the value you noted earlier - in my example this would be _ttyUSB1_:

```
docker run --rm -ti --device /dev/<usb-device>:/dev/ttyUSB0 piersfinlayson/otbiot
```

Once the container has been pulled and run, Flash the device and connect to it over serial:

```
make flash_initial && make con
```

Use Ctrl-] to terminate the serial connection.

When you want to terminate the container run:

```
exit
```

Do It Yourself
--------------

If you'd rather download and build everything yourself read on.

Pre-requisites
^^^^^^^^^^^^^^

Install the esp-open-sdk by following the instructions here: https://github.com/pfalcon/esp-open-sdk

Getting the Code
^^^^^^^^^^^^^^^^

Get the code from: https://github.com/piersfinlayson/otb-iot

::

  git clone https://github.com/piersfinlayson/otb-iot --recursive

  cd otb-iot

Configuring the Makefile
^^^^^^^^^^^^^^^^^^^^^^^^

You may need to modify various values within the Makefile - in particular:

::

  SDK_BASE ?= /opt/esp-open-sdk  # Should point at where you installed the esp-open-sdk

  SERIAL_PORT ?= /dev/ttyUSB0    # Port your ESP8266 is connected to for programming

Alternatively export shell variables to override the values in the Makefile:

::

  export SDK_BASE=/opt/esp-open-sdk

  export SERIAL_PORT=/dev/ttyUSB0

You could add these lines to your .bash_profile file.

Building
^^^^^^^^

Run:

::

  make all

Installing
^^^^^^^^^^

Ensure your ESP8266 device is connected to the serial port you configured earlier and run:

::

  make flash_initial

This command will erase the flash and then write:

* the bootloader

* ESP8266 SDK init data

* the application

* a backup "factory" application which can be used to recover the device using a factory reset

First Steps
^^^^^^^^^^^

You should now be ready to take your first steps with otb-iot.  :doc:`Continue here <first_steps>`.

