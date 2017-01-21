..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Logs
====

Categories
----------

otb-iot categorises the types of logs produced as follows:

===== ==============================
Type   Reason
===== ==============================
Boot   Messages from the bootloader

Debug  Disabled by default, provides very verbose logging

Info   An interesting event has occurred

Warn   A minor failure has occurred

Error  A major failure has occurred
===== ==============================

As indicated, all logs are contained in builds of otb-iot by default, with the exception of debugging logging, which introduces significantly larger binary sizes, so these logs should only be selectively turned on.

Accessing
---------

There are four ways of accessing logging information on otb-iot:

1 Using the :doc:`serial <serial>` port.

2 Once connected via MQTT, logs of type error will be reported automatically via MQTT.

3 The last N logs are stored in a circular RAM buffer.  These can be retrieved individually via MQTT.

4 The last N logs are stored on the flash chip before a reboot, if the device is able to do so.  These can then be read off by directly (physically) accessing the flash.

5 Not yet supported - accessing the logs when connecting to the otb-iot AP.

Using
-----

When developing devices based on otb-iot the serial port should be used wherever possible to provide the quickest and easiest access to as much diagnostic information as possible.  However, where this isn't possible, the stored logs can be accessed either by MQTT or reading the flash chip directly.

Debug Logs
----------

As noted earlier, these should be used sparingly, as the binary will be very large with debugging turned on, and this will also slow the software down significantly.  The author recommends that debugging logs are only used temporarily and when absolutely necessary.

