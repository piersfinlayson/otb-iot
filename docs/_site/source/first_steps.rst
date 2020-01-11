..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

First Steps
===========

Once you've :doc:`built the software and flashed your device <building>` you're ready to connect to and configure otb-iot.

Configuring 
-----------

otb-iot devices that aren't yet configured expose a WiFi Access Point (AP) and captive portal to enable the user to provide the minimal configuration required to get up and running.

Open the WiFi settings on a phone or computer and look for an access point with the named otb-iot.xxxxxx, where xxxxxx is unique to your device.  (xxxxxx is the chip ID of your ESP8266.)

Connect to the AP and your device should automatically open up a browser window containing captive portal.  If this doesn't happen, open a browser and navigate to http://192.168.4.1/

In the window that appears enter the following information:

* SSID of an available WiFi AP to connect to

* Password for the WiFi services

* IP address of your MQTT server (look :doc:`here <mqtt>` for help on setting this up)

When you press Submit the otb-iot device will reset, and attempt to connect to the WiFi AP and MQTT server you have specified.  If this succeeds it will send a message to the MQTT server using topic:

::

  /otb-iot/xxxxxx

Where xxxxxx again is your ESP8266's chipid.

If the otb-iot device fails to connect it will continue exposing its own AP to allow you to correct any configuration errors

Logging
-------

Depending on your persuasion, the first thing you may chose to do after flashing your device is connect to the serial port and review logs.  See :doc:`serial <serial>` for how to do this.

Next Steps
----------

Now your otb-iot device is connected send it a message to check it's working.  Send the following MQTT topic and message:

::

  /otb-iot/xxxxxx trigger/ping

With mosquitto and a local MQTT server you'd run the following command:

In one terminal, monitor messages from the MQTT broker:

::

  mosquitt_sub -v -t /otb-iot/#

And in another:

::

  mosquitto_pub -t /otb-iot/xxxxxx trigger/ping

You should receive:

::

  /otb-iot/xxxxxx ok:pong

in response.

Your now ready to try some more advanced stuff - head over to :doc:`mqtt <mqtt>` for more on the supported MQTT commands.

