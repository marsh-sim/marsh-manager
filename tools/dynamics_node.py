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
mav.srcSystem = 1  # default system
mav.srcComponent = 26  # USER2
print(f'Sending to {connection_string}')

heartbeat_next = 0.0
heartbeat_interval = 0.5
state_next = 0.0
state_interval = 0.25

while True:
    if time() >= heartbeat_next:
        mav.heartbeat_send(
            mavlink.MAV_TYPE_GENERIC,
            mavlink.MAV_AUTOPILOT_INVALID,
            mavlink.MAV_MODE_FLAG_TEST_ENABLED,
            0,
            mavlink.MAV_STATE_ACTIVE
        )
        heartbeat_next = time() + heartbeat_interval
    if time() >= state_next:
        mav.sim_state_send(
            1, 0, 0, 0,  # attitude quaternion
            0, 0, 0,  # attitude Euler angles, roll, pitch, yaw
            0, 0, 9.81,  # local acceleration X, Y, Z
            0, 0, 0,  # local angular speed X, Y, Z
            0, 0, 0,  # latitude, longitude, altitude
            0, 0,  # position standard deviation horizontal, vertical
            0, 0, 0,  # velocity N, E, D
            0, 0,  # lat, lon as degrees * 10^7
        )
        state_next = time() + state_interval
