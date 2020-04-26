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

##############################################################################
#
# Craft your own user_config.py based on sample_user_config.py
#
# This script will
# - take temperature readings reported from otb-iot attached DS18B20 sensors
#   and inject them into the influxdb configured in user_config.py along with
#   the following extra info:
#   - sensorId
#   - chipId
#   - location (as configured in user_config.py)
#   - time
#   - measurement (temperature)
# - take power readings reported from an SCT-013 100A current sensor, attached
#   to AIN 0 and 1 of an ADS1115 ADC at address 0x48 via a 22R burden resistor
#   and inject them into the influxdb configured in user_config.py along with
#   the following extra info:
#   - chipId
#   - location (as configured in user_config.py)
#   - time
#   - measurement (power)
#
# Note that at present otb-iot caluclates power by multiplying detected
# current by a fixed voltage of 245V.
#
##############################################################################

##############################################################################
#
# To install mosquitto module:
#
#   pip install paho-mqtt
#
# To install InfluxDBClient:
#
#   pip install influxdb
#
##############################################################################

# This script supports both old format (otb_iot) and new format (otb-iot) MQTT
# topics

import paho.mqtt.client as mqtt
import time, datetime
from influxdb import InfluxDBClient
from time import gmtime, strftime
from otbiot_mqtt_influxdb_defines import *
from user_config import *

# Globals
connected = False
was_once_connected = False
influxdb_client = None

# amount of time to sleep between checking we've received updated - these should come
# every 30s, so 60s is a good bet.  We'll also check pump state every sleep_time to check
# it's as we expect (in case someone's changed it, or the otb-iot device controlling the
# pump reboots and starts with different state
sleep_time = 60

otb_new = "otb-iot"
otb_old = "otb_iot"
espi = "espi"
otb_types = (otb_new, otb_old, espi)

def create_temp_topic(chipId, sensorId, location, otb_type):
  print("Create temp topic for chipId: %s, sensorId: %s, location: %s" % (chipId, sensorId, location))
  topic = "/%s////%s/temp/%s" % (otb_type, chipId, sensorId)
  print("Topic: %s" % topic)
  return topic

def create_temp_topic2(chipId, sensorId, location, otb_type):
  print("Create temp topic for chipId: %s, sensorId: %s, location: %s" % (chipId, sensorId, location))
  topic = "/%s/%s/temp/%s" % (otb_type, chipId, sensorId)
  print("Topic: %s" % topic)
  return topic

def create_power_topic(chipId, location, otb_type):
  print("Create power topic for chipId: %s, location: %s" % (chipId, location))
  topic = "/%s////%s/power/" % (otb_type, chipId)
  print("Topic: %s" % topic)
  return topic

# Return chipId, sensorId and location if a match, None otherwise
def process_topic(temp_sensors, power_sensors, topic, payload):
  rc = None
  substrings = topic.split('/')
  global otb_types
  try:
    # Check the format looks correct
    if (substrings[0] != ''):
      pass
    elif (substrings[1] not in otb_types):
      pass
    elif ((substrings[3] == 'temp') or (substrings[6] == 'temp')):
      # Format OK - try and match chipId and sensorId
      if (substrings[3] == 'temp'):
        chipid = substrings[2]
        sensorid = substrings[4]
      else:
        chipid = substrings[5]
        sensorid = substrings[7]
      for tsensor in temp_sensors:
        if (chipid == tsensor[CHIPID]):
          for sensor in tsensor[SENSORS]:
            if sensor[SENSORID] == sensorid:
              # Match
              send_temp_to_influx(chipid, sensorid, sensor[LOCATION], payload)
              rc = chipid, sensorid, sensor[LOCATION]
              break
          if rc != None:
            break
    elif (substrings[6] == 'power'):
      # Format OK - try and match chipId and sensorId
      chipid = substrings[5]
      for psensor in power_sensors:
        if (chipid == psensor[CHIPID]):
          # Match
          sensor = psensor[SENSORS][0]
          send_power_to_influx(chipid, sensor[LOCATION], payload)
          rc = chipid, sensor[LOCATION]
          break
  except:
    print("Exception handling incoming message")
  print("Match: " + str(rc));
  return rc

# Subscribe to all sensor topics
def subscribe(client, temp_sensors):
  global otb_types
  for tsensor in temp_sensors:
    for sensor in tsensor[SENSORS]:
      for otb_type in otb_types:
        topic = create_temp_topic(tsensor[CHIPID], sensor[SENSORID], sensor[LOCATION], otb_type)
        client.subscribe(topic)
        print("Subscribed to topic: %s" % topic)
        topic2 = create_temp_topic2(tsensor[CHIPID], sensor[SENSORID], sensor[LOCATION], otb_type)
        client.subscribe(topic2)
        print("Subscribed to topic: %s" % topic2)
  for psensor in power_sensors:
    for sensor in psensor[SENSORS]:
      for otb_type in otb_types:
        topic = create_power_topic(psensor[CHIPID], sensor[LOCATION], otb_type)
        client.subscribe(topic)
        print("Subscribed to topic: %s" % topic)
  return

def on_message(client, userdata, msg):
  global temp_sensors, power_sensors
  print("Received MQTT message: %s %s" % (msg.topic, msg.payload))
  rc = process_topic(temp_sensors, power_sensors, msg.topic, msg.payload)
  if rc == None:
    print("No matching sensors (or invalid MQTT message) - ignoring: %s %s" % (msg.topic, msg.payload))
  return

def get_time():
  time_now = strftime("%Y-%m-%dT%H:%M:%SZ", gmtime())
  return time_now

def send_temp_to_influx(chipid, sensorid, location, temp):
  global influxdb_client
  try:
    json_body = [{'measurement':'temperature',
                  'tags':{'chipId':chipid,
                          'sensorId':sensorid,
                          'location':location},
                  'time':get_time(),
                  'fields':{'value':temp}}]
    influxdb_client.write_points(json_body)
    print("Written data to influxdb: " + str(json_body))
  except:
    print("EXCEPTION: Failed to write temp to influxdb")

def send_power_to_influx(chipid, location, power):
  global influxdb_client
  try:
    power, current, voltage = power.split(':')
    json_body = [{'measurement':'power',
                  'tags':{'chipId':chipid,
                          'location':location},
                  'time':get_time(),
                  'fields':{'power':power[:-1],
                            'current':current[:-1],
                            'voltage':voltage[:-1]}}]
    influxdb_client.write_points(json_body)
    print("Written data to influxdb: " + str(json_body))
  except:
    print("EXCEPTION: Failed to write power to influxdb")

def on_connect(client, userdata, flags, rc):
  global temp_sensors, connected
  print("Connected to broker with result code: " + str(rc))
  subscribe(client, temp_sensors)
  connected = True
  was_once_connected = True

def on_disconnect(client, userdata, rc):
  print("Disconnected from broker!!!")
  run = False

def main():
  global client, connected, mqtt_addr, mqtt_port, sleep_time, was_once_connected
  global influxdb_client, influxdb_host, influxdb_port, influxdb_user, influxdb_password, influxdb_dbname

  print("Create connection to MQTT broker: %s:%d" % (mqtt_addr, mqtt_port))

  client = mqtt.Client(protocol=mqtt.MQTTv31)
  client.on_connect = on_connect
  client.on_message = on_message
  client.on_disconnect = on_disconnect

  connected = False
  was_once_connected = False
  temp_updates_received = 0
  client.connect_async(mqtt_addr, mqtt_port, 60)
  client.loop_start()

  influxdb_client = InfluxDBClient(influxdb_host, influxdb_port, influxdb_user, influxdb_password, influxdb_dbname)

  run = True
  while run:
    time.sleep(sleep_time)
    if not connected and was_once_connected:
      run = False
      break

  print("Exiting as disconnected")

if __name__ == "__main__":
  main()

