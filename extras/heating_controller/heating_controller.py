#!/usr/bin/python
#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2016 Piers Finlayson
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
import time, syslog

#
# Config section - tweak the values in this section as appropriate
#

# how much above trigger temp will turn pump off, or below will turn it off
hysteresis = 0.5

# target temperature
trigger_temp = 13.0

# amount of time to sleep between checking we've received updated - these should come
# every 30s, so 60s is a good bet.  We'll also check pump state every sleep_time to check
# it's as we expect (in case someone's changed it, or the otb-iot device controlling the
# pump reboots and starts with different state
sleep_time = 60

# Address and port of MQTT broker otb-iot devices are attached to
mqtt_addr = "localhost"
mqtt_port = 1883

# Addresses of DS18B20 temperature sensors for zone being controlled
floor_sensor = "28-021562ab19ff"
wall_sensor = "28-0415a18a8fff"

# Chip IDs of otb-iot devices with temperature sensors, and controlling pump
temp_chip_id = "289cde"
pump_chip_id = "d76a7d"

#
# End config section
#

ON = 1
OFF = 0
INVALID = -1
pump_state = INVALID
requested_pump_state = INVALID
pump_states = {ON:"on", OFF:"off", INVALID:"unknown"}

temp_topic = "/otb-iot/%s/temp/" % temp_chip_id
temp_sensor_topics = {temp_topic + floor_sensor:"floor", temp_topic + wall_sensor:"wall"}

#
# If using updated otb-iot MQTT API for pump controller, tweak this section
#
pump_topic = "/otb_iot/%s/status" % pump_chip_id
pump_update_topic = "/otb_iot/%s/system" % pump_chip_id
pump_update_msg = "gpio:set:5:%d"
pump_response = "gpio:set:ok"
pump_query = "gpio:get:5"
pump_query_response = "gpio:get:ok"

temp_sub = temp_topic + '#'
pump_sub = pump_topic

def turn_pump(requested_state):
  global pump_state, requested_pump_state, client
  assert(requested_state in pump_states)
  if requested_pump_state != INVALID:
    print ("Previous pump state update not yet completed");
  do_it = False
  print "Pump state currently " + pump_states[pump_state]
  print "Target temp " + str(trigger_temp) + "C"
  if requested_state == ON:
    if pump_state == OFF or pump_state == INVALID:
      print ("Turn pump on")
      if requested_pump_state != ON:
        requested_pump_state = ON
        do_it = True
        syslog.syslog(syslog.LOG_INFO, "heating: set pump state to on")
  else:
    if pump_state == ON or pump_state == INVALID:
      if requested_pump_state != OFF:
        print ("Turn pump off")
        # Turn it off
        requested_pump_state = OFF
        do_it = True
        syslog.syslog(syslog.LOG_INFO, "heating: set pump state to off")
  if do_it:
    client.publish(pump_update_topic, pump_update_msg % requested_pump_state)
  else:
    print "No action required"
  
def check_pump_state():
  client.publish(pump_update_topic, pump_query)
  print "Sent pump state query"

def on_message(client, userdata, msg):
  global pump_state, requested_pump_state, temp_updates_received
  if msg.topic in temp_sensor_topics:
    # Don't care which sensor
    temp_updates_received += 1
    temp = float(msg.payload)
    print "Got temp " + str(temp) + "C from " + temp_sensor_topics[msg.topic]
    if (temp < (trigger_temp - hysteresis)):
      # turn pump on
      turn_pump(ON)
    elif (temp > (trigger_temp + hysteresis)):
      # turn pump off
      turn_pump(OFF)
  elif msg.topic == pump_topic:
    if (msg.payload == pump_response):
      if requested_pump_state == INVALID:
        print("Pump successfully set to %s" % pump_states[pump_state])
        syslog.syslog(syslog.LOG_INFO, "heating: pump successfully set to %s" % pump_states[pump_state])
      else:
        pump_state = requested_pump_state
        print("Pump successfully set to %s" % pump_states[pump_state])
        syslog.syslog(syslog.LOG_INFO, "heating: pump successfully set to %s" % pump_states[pump_state])
        requested_pump_state = INVALID
    elif msg.payload.startswith(pump_query_response):
      state = int(msg.payload[-1])
      if state in pump_states:
        print ("Pump state is " + pump_states[state])
      else:
        print ("Unknown pump state " + state)
      if state != pump_state:
        pump_state = state
        print "Pump state updated to " + pump_states[pump_state]
        syslog.syslog(syslog.LOG_INFO, "Pump state updated to " + pump_states[pump_state])
      elif state not in pump_states:
        pump_state = INVALID
        print "Pump state updated to " + pump_states[pump_state]
        syslog.syslog(syslog.LOG_INFO, "Pump state updated to " + pump_states[pump_state])
    else:
      print("Error pump response!!!")
      requested_pump_state = INVALID
  else:
      print("Got unknown topic/message: " + msg.topic+" "+str(msg.payload))
  print ""


    
def on_connect(client, userdata, flags, rc):
  print("Connected to broker with result code "+str(rc))

  # Subscribing in on_connect() means that if we lose the connection and
  # reconnect then subscriptions will be renewed.
  client.subscribe(temp_sub)
  client.subscribe(pump_sub)

  global connected    
  connected = True
  
  print ""

def on_disconnect(client, userdata, rc):
  print("Disconnected from broker!!!")
  syslog.syslog(syslog.LOG_INFO, "heating: disconnected from broker")
  run = False

syslog.syslog(syslog.LOG_INFO, "heating: starting")

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
    print ("Still connected")
    check_pump_state()
  else:
    print("No temp updates received in %ds!!!" % sleep_time)
    syslog.syslog(syslog.LOG_INFO, "heating: no temp updates received in %ds" % sleep_time)
    run = False
  print ""
    
print ("Exiting!!!")
syslog.syslog("heating: exiting")
