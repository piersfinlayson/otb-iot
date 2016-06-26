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

import sys
import time
import rrdtool

k = 0
try:
  buff = ''
  while True:
    buff += sys.stdin.read(1)
    if buff.endswith('\n'):
      print buff
      vars = buff.split(':')
      power = vars[0][:-1]
      current = vars[1][:-1]
      voltage = vars[2][:-2]
      print "power %s current %s voltage %s" % (power, current, voltage)
      power = int(power)
      current = float(current)
      voltage = float(voltage)
      print "power %d current %f voltage %f" % (power, current, voltage)
      timestamp = time.time()
      updateString = "N:%s:%s:%s" % (power, current, voltage)
      print "Send to rrdtool: %s" % updateString
      rrdtool.update('/data/house/power/main.rrd', updateString)
      buff = ''
      k = k + 1
  
except KeyboardInterrupt:
   sys.stdout.flush()
   pass
print k
