#!/usr/bin/env python3

from argparse import ArgumentParser
from time import sleep, time

import pymavlink.dialects.v20.common as mavlink
from pymavlink import mavutil

parser = ArgumentParser()
parser.add_argument('-m', '--manager',
                    help='MARSH Manager IP addr', default='127.0.0.1')
args = parser.parse_args()

connection_string = f'udpout:{args.manager}:24400'
mav = mavlink.MAVLink(mavutil.mavlink_connection(connection_string))
mav.srcComponent = 26
print(f'Sending to {connection_string}')

next_send = 0.0
send_interval = 0.5

while True:
    if time() >= next_send:
        mav.heartbeat_send(
            mavlink.MAV_TYPE_GENERIC,
            mavlink.MAV_AUTOPILOT_INVALID,
            mavlink.MAV_MODE_FLAG_TEST_ENABLED,
            0,
            mavlink.MAV_STATE_ACTIVE
        )
        next_send = time() + send_interval
