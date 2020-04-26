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

import paho.mqtt.client as mqtt
import sys, time, datetime
from user_config import *

# Name of this script - used in logs
script_name = "nixie_temp"

temp_topic = "/espi/%s/temp/" % temp_chip_id
temp_topic2 = "/otb-iot////%s/temp/" % temp_chip_id

temp_sub = temp_topic + '#'
temp_sub2 = temp_topic2 + '#'

temp_sensor_topics = (temp_topic + temp_sensor_id, temp_topic2 + temp_sensor_id)

nixie_topic = "/otb-iot/" + nixie_chip_id
nixie_status_topic = nixie_topic + "/status"
nixie_booted = "booted"

pump_topic = "/otb_iot/" + pump_chip_id
pump_status_topic = pump_topic + "/status"
pump_gpio_get_ok = "gpio:get:ok:"

INVALID_TEMP = ""

not_updated = 0
pump_on = ""

def log(to_log):
  print(script_name + ": " + to_log)
  sys.stdout.flush()

def display_temp(client, temp):
  client.publish(nixie_topic, "trigger/nixie/power/on")
  client.publish(nixie_topic, "trigger/nixie/cycle")
  client.publish(nixie_topic, "trigger/nixie/show/" + temp)
  log("display: " + temp)

def on_message(client, userdata, msg):
  global temp_updates_received, last_received_temp, last_displayed_temp, not_updated, pump_on
  if msg.topic in temp_sensor_topics:
    temp_updates_received += 1
    temp = float(msg.payload)
    log("Got temp " + str(temp) + "C")
    neg = False
    if (round(temp) < 0):
      neg = True
      temp = 0-temp
    temp = str(int(round(temp)))
    if len(temp) < 2:
      if neg:
        temp = "_." + temp
      else:
        temp = "_" + temp
    else:
      if neg:
        temp = temp[0]+"."+temp[1]
    last_received_temp = pump_on + temp
    if last_received_temp != last_displayed_temp or (not_updated >= update_anyway):
      display_temp(client, last_received_temp)
      last_displayed_temp = temp
      not_updated = 0
    else:
      not_updated += 1
  elif msg.topic == nixie_status_topic:
    #log("Got nixie status update: " + msg.payload)
    if msg.payload.startswith(nixie_booted):
      log("Nixie booted - update display next time we get a temperature")
      not_updated = update_anyway
  elif msg.topic == pump_status_topic:
    # Note we don't issue the pump query - we rely on another script doing this!
    #log("Got pump status update: " + msg.payload)
    if msg.payload.startswith(pump_gpio_get_ok):
      if msg.payload.strip(pump_gpio_get_ok) == "1":
        log("Pump is on")
        pump_on = "."
      else:
        log("Pump is off")
        pump_on = ""
  else:
    # log("ignored topic/message: " + msg.topic+" "+str(msg.payload))
    pass

def on_connect(client, userdata, flags, rc):
  global connected, last_received_temp, last_displayed_temp, not_updated
  log("Connected to broker with result code "+str(rc))

  # Subscribing in on_connect() means that if we lose the connection and
  # reconnect then subscriptions will be renewed.
  not_updated = 0
  pump_on = ""
  client.publish(nixie_topic, "trigger/nixie/init")
  client.publish(nixie_topic, "trigger/nixie/power/off")
  client.subscribe(temp_sub)
  log("Subcribed to: " + temp_sub)
  client.subscribe(temp_sub2)
  log("Subcribed to: " + temp_sub2)
  client.subscribe(nixie_status_topic)
  log("Subcribed to: " + nixie_status_topic)
  if pump_chip_id != "":
    client.subscribe(pump_status_topic)
    log("Subcribed to: " + pump_status_topic)
  last_received_temp = INVALID_TEMP
  last_displayed_temp = INVALID_TEMP
  connected = True

def on_disconnect(client, userdata, rc):
  log("disconnected from broker")
  run = False  

def main():  
  global client, connected, temp_updates_received
  log("starting")

  client = mqtt.Client(protocol=mqtt.MQTTv31)
  client.on_connect = on_connect
  client.on_message = on_message
  client.on_disconnect = on_disconnect

  connected = False
  temp_updates_received = 0
  client.connect_async(mqtt_addr, mqtt_port, 60)
  client.loop_start()

  run = True
  old_temp_updates_received = 0
  while run:
    time.sleep(sleep_time)
    if not connected:
      run = False
      break
    if old_temp_updates_received != temp_updates_received:
      old_temp_updates_received = temp_updates_received
      #log("Still connected")
    else:
      log("no temp updates received in %ds" % sleep_time)
      run = False

  if connected:
    client.publish(nixie_topic, "trigger/nixie/power/off")

  log("exiting")
  
if __name__ == "__main__":
  main()

