..
.. OTB-IOT - Out of The Box Internet Of Things
..
.. Copyright (C) 2017 Piers Finlayson
..

Building
========

Introduction
------------

otb-iot is intended to be built on linux and has been successfully built on various distributions including:

* Ubuntu

* Raspbian

* Arch Linux

Pre-requisites
--------------

Install the esp-open-sdk by following the instructions here: https://github.com/pfalcon/esp-open-sdk

Getting the Code
----------------

Get the code from: https://github.com/piersfinlayson/otb-iot

::

  git clone https://github.com/piersfinlayson/otb-iot --recursive

Configuring the Makefile
------------------------

You may need to modify various values within the Makefile - in particular:

::

  SDK_BASE ?= /opt/esp-open-sdk  # Should point at where you installed the esp-open-sdk

  SERIAL_PORT ?= /dev/ttyUSB0    # Port your ESP8266 is connected to for programming

Alternatively export shell variables to override the values in the Makefile:

::

  export SDK_BASE=/opt/esp-open/sdk

  export SERIAL_PORT=/dev/ttyUSB0

You could add these lines to your .bash_profile file.

Building
--------

Run:

::

  make all

Installing
----------

Ensure your ESP8266 device is connected to the serial port you configured earlier and run:

::

  make flash_initial

This command will erase the flash and then write:

* the bootloader

* ESP8266 SDK init data

* the application

* a backup "factory" application

First Steps
-----------

You should now be ready to take your first steps with otb-iot.  :doc:`Continue here <first_steps>`.

