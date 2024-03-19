#!/usr/bin/env python3

from argparse import ArgumentParser
from os import path
import sys
from time import time
from typing import Optional

from pymavlink import mavutil

# add repository root path to import local module regardless of location
root_path = path.abspath(path.join(path.dirname(__file__), '..'))
sys.path.insert(1, root_path)
try:
    # fmt: off - don't move import to top of file
    import dist.all_marsh as mavlink
    # fmt: on
except ImportError:
    print('could not import generated dialect, run update_mavlink.py first')

parser = ArgumentParser()
parser.add_argument('-m', '--manager',
                    help='MARSH Manager IP addr', default='127.0.0.1')
args = parser.parse_args()

connection_string = f'udpout:{args.manager}:24400'
mav = mavlink.MAVLink(mavutil.mavlink_connection(connection_string))
mav.srcSystem = 1  # default system
mav.srcComponent = mavlink.MARSH_COMP_ID_FLIGHT_MODEL
print(f'Sending to {connection_string}')

# controlling when messages should be sent
heartbeat_next = 0.0
heartbeat_interval = 1.0
state_next = 0.0
state_interval = 0.25

# monitoring connection to manager with heartbeat
timeout_interval = 5.0
manager_timeout = 0.0
manager_connected = False

# the loop goes as fast as it can, relying on the variables above for timing
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

    # handle incoming messages
    message: Optional[mavlink.MAVLink_message] = mav.file.recv_msg()
    if message is not None and message.get_type() == 'HEARTBEAT':
        # the following line only helps with type hints
        heartbeat: mavlink.MAVLink_heartbeat_message = message

        if heartbeat.get_srcComponent() == mavlink.MARSH_COMP_ID_MANAGER:
            if not manager_connected:
                # example of showing text for enum
                state = mavlink.enums['MAV_STATE'][heartbeat.system_status]
                print('Connected to simulation manager in state', state.name)
            manager_connected = True
            manager_timeout = time() + timeout_interval

    if manager_connected and time() > manager_timeout:
        manager_connected = False
        print('Lost connection to simulation manager')
