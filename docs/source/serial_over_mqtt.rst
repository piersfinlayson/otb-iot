..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Serial over MQTT
================

Introduction
------------

otb-iot supports controlling serial devices remotely, controlled over the MQTT channel.

There are three key primitives that are supported:

* set - to set up serial port configuration (which pins to use, speed of communication)

* get - to read back serial port configuration (similar set of fields as set)

* trigger - to send data via serial, and to receive received packets

Setting it Up
-------------

An example of a series of MQTT commands to set up serial communication are as follows:

::

  topic: /otb-iot/chipid message: set/config/serial/tx/15

  topic: /otb-iot/chipid message: set/config/serial/rx/13

  topic: /otb-iot/chipid message: set/config/serial/speed/2400

  topic: /otb-iot/chipid message: set/config/serial/enable

::

Sending and Receiving Data
--------------------------

otb-iot maintains a circular buffer of received serial data.  As it's possible some noise on the line has caused data to be random received, it's worth clearing this buffer before sending data:

Sent:

::

  topic: /otb-iot/chipid message: trigger/serial/buffer/dump

::

Received:

::

  topic: /otb-iot/chipid/status message: ok:xx

::

xx indicates the buffer is empty.  A maximum of 25 bytes will be returned in each status message - multiple messages will be sent if the buffer contains more than 25 bytes.

To send serial data:

::

  topic: /otb-iot/chipid message: trigger/serial/send/105b015c16

::

Here 0x10 0x5b 0x01 0x5c 0x16 will be sent.

Data is returned as hex bytes:

::

  topic: /otb-iot/chipid message: trigger/serial/buffer/dump
  topic: /otb-iot/chipid/status   message: ok:01/02/03/04/xx

::

Here 0x01, 0x02, 0x03, 0x04 were received in that order.

The buffer is cleared as bytes are read.  If the buffer is read again immediately:

::

  topic: /otb-iot/chipid message: trigger/serial/buffer/dump
  topic: /otb-iot/chipid/status   message: ok:xx

::

More Details
------------

For more details on the commands supported see `otb_cmd.h`_.

.. _otb_cmd.h: https://github.com/piersfinlayson/otb-iot/blob/master/include/otb_cmd.h
