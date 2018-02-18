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

# Defines
CHIPID = 'chipId'
ADDRS = 'addresses'
LOCATION = 'location'
ADDR = 'address'
STATUS = 'status'
MID = 'mid'

CHIP_STATUS_INIT          = 0
CHIP_STATUS_DISABLE       = 1
CHIP_STATUS_SET_TX_PIN    = 2
CHIP_STATUS_SET_RX_PIN    = 3
CHIP_STATUS_SET_BAUD_RATE = 4
CHIP_STATUS_SET_PARITY    = 5
CHIP_STATUS_ENABLE        = 6
CHIP_STATUS_DONE          = 7
CHIP_STATUS_RESET         = 8
ADDR_STATUS_INIT          = 0
ADDR_STATUS_CLEAR_BUF     = 1
ADDR_STATUS_QUERY_MBUS    = 2
ADDR_STATUS_DUMP_DATA     = 3
ADDR_STATUS_PROCESS_DATA  = 4
ADDR_STATUS_DONE          = 5
ADDR_STATUS_RESET         = 6

MEZZ      = "mezz"
TX        = "tx"
RX        = "rx"
BAUD_RATE = "speed"
PARITY = "parity"
ENABLE    = "enable"
DISABLE   = "disable"
DUMP      = "dump"