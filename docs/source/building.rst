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

* Plug your ESP8266 device into a USB port on your linux machine (or map through from your host to your linxu guest in Virtualbox if applicable)

* Install docker:

::

  curl https://get.docker.com/|sh

* Run the container containing pre-build otb-iot images:

::

  docker run --rm -ti --device /dev/ttyUSB0:/dev/ttyUSB0 piersfinlayson/otbiot

* Flash the device and connect to it over serial:

::

  make flash_initial && make con

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

