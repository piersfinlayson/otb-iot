#!/usr/bin/python
#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2017 Piers Finlayson
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#

#
# To install mosquitto module:
#
# pip install paho-mqtt
#

# Note this assumes old otb-iot MQTT API for pump control (relay) device, and new for
# temperature device

import paho.mqtt.client as mqtt
import time, syslog, datetime
from random import randint

#
# Config section - tweak the values in this section as appropriate
#

sleep_time = 0.5

# Address and port of MQTT broker otb-iot devices are attached to
mqtt_addr = "iotmqtt"
mqtt_port = 1883

#
# End config section
#

topic = "/otb-iot/18fe34d76b95"

def on_connect(client, userdata, flags, rc):
  print("Connected to broker with result code "+str(rc))
  global connected    
  client.publish(topic, "trigger/nixie/init")
  connected = True

def on_disconnect(client, userdata, rc):
  print("Disconnected from broker!!!")
  connected = False
  client.connect_async(mqtt_addr, mqtt_port, 60)

def main():  
  global client, connected

  client = mqtt.Client(protocol=mqtt.MQTTv31)
  client.on_connect = on_connect
  client.on_disconnect = on_disconnect

  connected = False
  client.connect_async(mqtt_addr, mqtt_port, 60)
  client.loop_start()

  run = True
  while run:
    if connected:
      dot0 = False
      dot1 = False
      if (randint(0,19) == 0):
        client.publish(topic, "trigger/nixie/cycle")
      elif (randint(0,19) == 0):
        client.publish(topic, "trigger/nixie/clear")
      elif (randint(0,19) == 0):
        client.publish(topic, "trigger/nixie/init")
      elif (randint(0,49) == 0):
        client.publish(topic, "trigger/nixie/show/..")
        for ii in range(0, 100):
          time.sleep(sleep_time/2)
          output = "{0:0>2}".format(ii)
          client.publish(topic, "trigger/nixie/show/%s" % output)
      else:  
        if (randint(0, 9) == 0):
          dot0 = True
        if (randint(0, 9) == 0):
          dot1 = True
        to_show = "%d%d" % (randint(0, 9), randint(0, 9))
        if (dot1):
          to_show = to_show[0] + '.' + to_show[1]
        if (dot0):
          to_show = '.' + to_show
        client.publish(topic, "trigger/nixie/show/%s" % to_show)
    time.sleep(sleep_time)
    
  print ("Exiting!!!")
  
if __name__ == "__main__":
  main()

