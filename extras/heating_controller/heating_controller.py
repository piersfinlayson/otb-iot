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
import sys, time, syslog, datetime

#
# Config section - tweak the values in this section as appropriate
#

# how much above trigger temp will turn pump off, or below will turn it off
hysteresis = 0.5

# target temperatures
min_floor_temp = 8.0     # Minimum temp floor can be allowed get to
backup_target_room_temp = 13.0  # Target temp for room
max_floor_temp = 25.0    # Maximum floor temp (to avoid damaging it, but may exceed if and only if room temp is below min_room_temp 
min_room_temp = 5.0      # Room mustn't go below this (e.g. frozen pipes), so risk damaging floor to heat room

# amount of time to sleep between checking we've received updated - these should come
# every 30s, so 60s is a good bet.  We'll also check pump state every sleep_time to check
# it's as we expect (in case someone's changed it, or the otb-iot device controlling the
# pump reboots and starts with different state
sleep_time = 60

# Address and port of MQTT broker otb-iot devices are attached to
mqtt_addr = "mosquitto"
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
FLOOR = "floor"
WALL = "wall"
temp_topic = "/otb-iot/%s/temp/" % temp_chip_id
temp_sensor_topics = {temp_topic + floor_sensor:FLOOR, temp_topic + wall_sensor:WALL}
temp_locations = {FLOOR:temp_topic + floor_sensor, WALL:temp_topic + wall_sensor}
DAY = "day"
HOUR = "hour"
MIN = "min"
SUN = 0
MON = 1
TUE = 2
WED = 3
THU = 4
FRI = 5
SAT = 6
NUM_DAYS = 7
DAYS = [SUN, MON, TUE, WED, THU, FRI, SAT]
DAYS_TEXT = {SUN:"Sunday", MON:"Monday", TUE:"Tuesday", WED:"Wednesday", THU:"Thursday", FRI:"Friday", SAT:"Saturday"}
ROOM_TEMP = "room temp"

# Heating schedule - must be sequential within a particular day

# WINTER
temp_schedule = [{DAY:(MON, TUE, WED, THU, FRI, SAT, SUN), HOUR:6, MIN:0, ROOM_TEMP:19},
                 {DAY:(MON, TUE, WED, THU, FRI), HOUR:10, MIN:0, ROOM_TEMP:19},
                 {DAY:(MON, TUE, WED, THU, FRI), HOUR:16, MIN:0, ROOM_TEMP:19},
                 {DAY:(MON, TUE, WED, THU, FRI, SAT, SUN), HOUR:20, MIN:0, ROOM_TEMP:19}]

# SUMMER
#temp_schedule = [{DAY:(MON, TUE, WED, THU, FRI, SAT, SUN), HOUR:6, MIN:0, ROOM_TEMP:13},
#                 {DAY:(MON, TUE, WED, THU, FRI), HOUR:10, MIN:0, ROOM_TEMP:13},
#                 {DAY:(MON, TUE, WED, THU, FRI), HOUR:16, MIN:0, ROOM_TEMP:13},
#                 {DAY:(MON, TUE, WED, THU, FRI, SAT, SUN), HOUR:20, MIN:0, ROOM_TEMP:13}]

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

# Set last temps to be safeish values
last_temps = {FLOOR:min_floor_temp, WALL:backup_target_room_temp}

def get_day_time():
  now = datetime.datetime.now()
  day = int(now.strftime("%w"))
  hour = int(now.strftime("%H"))
  min = int(now.strftime("%M"))
  return {DAY:day, HOUR:hour, MIN:min}
  
def calc_target_temp(temp, now=None):
  assert(temp == ROOM_TEMP)
  if now == None:
    now = get_day_time()
  day = now[DAY]
  hour = now[HOUR]
  min = now[MIN]
  output = "It's " + DAYS_TEXT[day] + " %2.2d:%2.2d" % (hour, min) + "."
  
  lines = []
  target_temp = None
  done_days = 0
  while target_temp == None:
    for line in temp_schedule:
      if day in line[DAY]:
        lines.append(line)
    for line in reversed(lines):
      #print "Looking at line " + str(line)
      if hour > line[HOUR]:
        target_temp = line[temp]
        break
      if hour == line[HOUR]:
        if min >= line[MIN]:
          target_temp = line[temp]
          break
    day -= 1
    day %= NUM_DAYS
    hour = 23
    min = 59
    done_days += 1
    if done_days >= NUM_DAYS:
      break
  if target_temp == None:
    target_temp = backup_target_room_temp
  print output + " Target %s = %.2fC." % (temp, target_temp)
  return target_temp

def turn_pump(requested_state):
  global pump_state, requested_pump_state, client, last_temps
  assert(requested_state in pump_states)
  if requested_pump_state != INVALID:
    print ("Previous pump state update not yet completed");
  do_it = False
  print "Pump state currently " + pump_states[pump_state]
  if requested_state == ON:
    if pump_state == OFF or pump_state == INVALID:
      print ("Turn pump on")
      if requested_pump_state != ON:
        requested_pump_state = ON
        do_it = True
        syslog.syslog(syslog.LOG_INFO, "heating: set pump state to on, wall=%.2fC, floor=%.2fC" % (last_temps[WALL], last_temps[FLOOR]))
        print("heating: set pump state to on, wall=%.2fC, floor=%.2fC" % (last_temps[WALL], last_temps[FLOOR]))
        sys.stdout.flush()
  else:
    if pump_state == ON or pump_state == INVALID:
      if requested_pump_state != OFF:
        print ("Turn pump off")
        # Turn it off
        requested_pump_state = OFF
        do_it = True
        syslog.syslog(syslog.LOG_INFO, "heating: set pump state to off, wall=%.2fC, floor=%.2fC" % (last_temps[WALL], last_temps[FLOOR]))
        print("heating: set pump state to off, wall=%.2fC, floor=%.2fC" % (last_temps[WALL], last_temps[FLOOR]))
        sys.stdout.flush()
  if do_it:
    client.publish(pump_update_topic, pump_update_msg % requested_pump_state)
  else:
    print "No action required"
  
def check_pump_state():
  client.publish(pump_update_topic, pump_query)
  print "Sent pump state query"

def make_pump_decision():
  global last_temps, temp_updates_received, pump_state, requested_pump_state
  target_pump_state = pump_state
  print "Making pump decision based on floor: %.2fC wall: %.2fC" % (last_temps[FLOOR], last_temps[WALL])
  target_room_temp = calc_target_temp(ROOM_TEMP)
  print "min_floor_temp: %.2f target_room_temp: %.2f max_floor_temp: %.2f min_room_temp: %.2f" % (min_floor_temp, target_room_temp, max_floor_temp, min_room_temp)
  # If pump is already on, then only turn off if either:
  # - floor is too hot (hotter than max), and temp is not below min_room_temp 
  # - both wall and floor above target temps
  if pump_state == ON:
    if (last_temps[FLOOR] > (max_floor_temp + hysteresis)):
      if (last_temps[WALL] > min_room_temp): # No hysteresis performed
        target_pump_state = OFF
        print "Turning pump off as wall temp above min room temp, and floor temp above max floor temp"
    if (last_temps[FLOOR] > (min_floor_temp + hysteresis)):
      if (last_temps[WALL] > (target_room_temp + hysteresis)):
        target_pump_state = OFF
        print "Turning pump off floor temp above min floor temp, and wall temp above target room temp"
        
  # If pump is off, then turn on if any of the following are true:
  # - floor temp is below minimum temp
  # - wall temp is below target room temp and floor isn't too hot
  # - wall temp is below minimum (even if floor is too hot)
  elif pump_state == OFF:
    if (last_temps[FLOOR] < (min_floor_temp - hysteresis)):
      target_pump_state = ON
      print "Turning pump on as floor temp is below min floor temp"
    if (last_temps[WALL] < (target_room_temp - hysteresis)):
      if (last_temps[FLOOR] < (max_floor_temp - hysteresis)):
        target_pump_state = ON
        print "Turning pump on as wall temp below target room temp and floor doesn't exceed max"
    if (last_temps[WALL] < (min_room_temp)): # No hysteresis
      target_pump_state = ON
      print "Turning pump on as wall temp min room temp (even though max floor temp might have been exceed"
      if (last_temps[FLOOR] > max_floor_temp):
        syslog.syslog(syslog.LOG_WARNING, "heating: turning pump on even though max floor temp exceeded, as room temp below minimum")
        print("heating: turning pump on even though max floor temp exceeded, as room temp below minimum")
        sys.stdout.flush()
      
  # Pump state is invalid, meaning we haven't set it yet - this will be set after sleep_time (60s default)
  else:
    print "Not yet read pump state, no action"
    syslog.syslog(syslog.LOG_INFO, "heating: not yet read pump state, no action")
    print("heating: not yet read pump state, no action")
    sys.stdout.flush()
    
  # Actually change pump state  
  if (target_pump_state != pump_state):
    turn_pump(target_pump_state)
  else:
    print "No pump state change"
  
def on_message(client, userdata, msg):
  global pump_state, requested_pump_state, temp_updates_received
  if msg.topic in temp_sensor_topics:
    # Don't care which sensor
    temp_updates_received += 1
    temp = float(msg.payload)
    print "Got temp " + str(temp) + "C from " + temp_sensor_topics[msg.topic]
    last_temps[temp_sensor_topics[msg.topic]] = temp
    make_pump_decision()
  elif msg.topic == pump_topic:
    if (msg.payload == pump_response):
      if requested_pump_state == INVALID:
        syslog.syslog(syslog.LOG_INFO, "heating: pump successfully set to %s" % pump_states[pump_state])
        print("heating: pump successfully set to %s" % pump_states[pump_state])
        sys.stdout.flush()
      else:
        pump_state = requested_pump_state
        syslog.syslog(syslog.LOG_INFO, "heating: pump successfully set to %s" % pump_states[pump_state])
        print("heating: pump successfully set to %s" % pump_states[pump_state])
        sys.stdout.flush()
        requested_pump_state = INVALID
    elif msg.payload.startswith(pump_query_response):
      state = int(msg.payload[-1])
      if state in pump_states:
        print ("Pump state is " + pump_states[state])
        sys.stdout.flush()
      else:
        print ("Unknown pump state " + state)
        sys.stdout.flush()
      if state != pump_state:
        pump_state = state
        print "Internal pump state updated to " + pump_states[pump_state]
        syslog.syslog(syslog.LOG_INFO, "Internal pump state updated to " + pump_states[pump_state])
        sys.stdout.flush()
      elif state not in pump_states:
        pump_state = INVALID
        print "Internal pump state updated to " + pump_states[pump_state]
        syslog.syslog(syslog.LOG_INFO, "Internal pump state updated to " + pump_states[pump_state])
        sys.stdout.flush()
    else:
      print("Error pump response!!!")
      sys.stdout.flush()
      requested_pump_state = INVALID
  else:
    print("Got unknown topic/message: " + msg.topic+" "+str(msg.payload))
    sys.stdout.flush()
  print ""
    
def on_connect(client, userdata, flags, rc):
  print("Connected to broker with result code "+str(rc))
  sys.stdout.flush()

  # Subscribing in on_connect() means that if we lose the connection and
  # reconnect then subscriptions will be renewed.
  client.subscribe(temp_sub)
  client.subscribe(pump_sub)
  check_pump_state()

  global connected    
  connected = True
  
  print ""

def on_disconnect(client, userdata, rc):
  syslog.syslog(syslog.LOG_INFO, "heating: disconnected from broker")
  print("heating: disconnected from broker")
  sys.stdout.flush()
  run = False

def main():  
  syslog.syslog(syslog.LOG_INFO, "heating: starting")
  print("heating: starting")
  sys.stdout.flush()
  global client, connected, temp_updates_received

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
      syslog.syslog(syslog.LOG_INFO, "heating: no temp updates received in %ds" % sleep_time)
      print("heating: no temp updates received in %ds" % sleep_time)
      sys.stdout.flush()
      run = False
    print ""
    
  syslog.syslog("heating: exiting")
  print("heating: exiting")
  sys.stdout.flush()
  
if __name__ == "__main__":
  main()

