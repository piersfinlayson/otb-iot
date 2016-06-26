#!/bin/sh
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

# 1s 31622400 = 1 year
# 10s 15811200 = 5 years
# 60s 15811200 = 30 years
# Takes about 1.5GB of data
rrdtool create /data/house/power/main.rrd \
  --start now --step 1 \
  DS:power:GAUGE:5:0:U \
  DS:current:GAUGE:5:0:U \
  DS:voltage:GAUGE:5:0:U \
  RRA:AVERAGE:0.5:1:31622400 \
  RRA:AVERAGE:0.5:10:15811200 \
  RRA:AVERAGE:0.5:60:15811200 
