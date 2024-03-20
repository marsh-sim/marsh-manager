#!/usr/bin/env python3

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from collections import OrderedDict
from os import path
import pygame
from pygame import joystick
from pymavlink import mavutil
import sys
from time import time
from typing import Optional

# add repository root path to import local module regardless of location
root_path = path.abspath(path.join(path.dirname(__file__), '..'))
sys.path.insert(1, root_path)
try:
    # fmt: off - don't move import to top of file
    import dist.all_marsh as mavlink
    # fmt: on
except ImportError:
    print('could not import generated dialect, run update_mavlink.py first')

parser = ArgumentParser(formatter_class=ArgumentDefaultsHelpFormatter)
parser.add_argument('-i', '--input-index', type=int,
                    help='index of input device to use', default=0)
parser.add_argument('-m', '--manager',
                    help='MARSH Manager IP addr', default='127.0.0.1')
args = parser.parse_args()
# assign to typed variables for convenience
args_manager: str = args.manager
args_input_index: int = args.input_index

pygame.init()
joystick.init()

if joystick.get_count() == 0:
    print('No joystick connected')
    exit(1)


# validate arguments
if args_input_index < 0:
    parser.error('Input index must be non-negative')

if args_input_index >= joystick.get_count():
    parser.error(
        f'Input index {args_input_index} too large, only {joystick.get_count()} joysticks connected')

# create joystick device
device = joystick.Joystick(args_input_index)
device.init()

# create MAVLink connection
connection_string = f'udpout:{args_manager}:24400'
mav = mavlink.MAVLink(mavutil.mavlink_connection(connection_string))
mav.srcSystem = 1  # default system
mav.srcComponent = mavlink.MARSH_COMP_ID_CONTROLS
print(f'Sending to {connection_string}')

# create parameters database, all parameters are float to simplify code
# default values for Thrustmaster T-Flight HOTAS X with yaw on throttle
params: OrderedDict[str, float] = OrderedDict()
params['PTCH_AXIS'] = 1.0
params['PTCH_REVERSED'] = 1.0
params['ROLL_AXIS'] = 0.0
params['ROLL_REVERSED'] = 0.0
params['THR_AXIS'] = 2.0
params['THR_REVERSED'] = 1.0
params['YAW_AXIS'] = 4.0
params['YAW_REVERSED'] = 0.0

for k in params.keys():
    assert len(k) <= 16, 'parameter names must fit into param_id field'


def send_param(index: int, name=''):
    """
    convenience function to send PARAM_VALUE
    pass index -1 to use name instead

    silently returns on invalid index or name
    """
    param_id = bytearray(16)

    if index >= 0:
        if index >= len(params):
            return

        # HACK: is there a nicer way to get items from OrderedDict by order?
        name = list(params.keys())[index]
    else:
        if name not in params:
            return

        index = list(params.keys()).index(name)
    name_bytes = name.encode('utf8')
    param_id[:len(name_bytes)] = name_bytes

    mav.param_value_send(param_id, params[name], mavlink.MAV_PARAM_TYPE_REAL32,
                         len(params), index)


# controlling when messages should be sent
heartbeat_next = 0.0
heartbeat_interval = 1.0
control_next = 0.0
control_interval = 0.02

# monitoring connection to manager with heartbeat
timeout_interval = 5.0
manager_timeout = 0.0
manager_connected = False

# the loop goes as fast as it can, relying on the variables above for timing
while True:
    # process pygame loop
    for event in pygame.event.get():
        pass

    if time() >= heartbeat_next:
        mav.heartbeat_send(
            mavlink.MAV_TYPE_GENERIC,
            mavlink.MAV_AUTOPILOT_INVALID,
            mavlink.MAV_MODE_FLAG_TEST_ENABLED,
            0,
            mavlink.MAV_STATE_ACTIVE
        )
        heartbeat_next = time() + heartbeat_interval

    if time() >= control_next:
        axes = [0x7FFF] * 4  # set each axis to invalid (INT16_MAX)

        # get joystick values based on parameters
        for i, prefix in enumerate(['PTCH', 'ROLL', 'THR', 'YAW']):
            axis_number = int(params[f'{prefix}_AXIS'])
            if axis_number >= 0 and axis_number < device.get_numaxes():
                value = device.get_axis(axis_number)
                if params[f'{prefix}_REVERSED'] != 0.0:
                    value *= -1.0
                # scale to the normalized range
                axes[i] = round(1000 * max(-1, min(1, value)))

        # set each bit in buttons corresponding to pressed state
        buttons = 0
        for button in range(min(16, device.get_numbuttons())):
            if device.get_button(button):
                buttons += 1 << button

        mav.manual_control_send(
            mav.srcSystem,
            axes[0], axes[1], axes[2], axes[3],
            buttons,
        )
        control_next = time() + control_interval

    # handle incoming messages
    message: Optional[mavlink.MAVLink_message] = mav.file.recv_msg()
    if message is None:
        pass
    elif message.get_type() == 'HEARTBEAT':
        if message.get_srcComponent() == mavlink.MARSH_COMP_ID_MANAGER:
            if not manager_connected:
                print('Connected to simulation manager')
            manager_connected = True
            manager_timeout = time() + timeout_interval
    elif message.get_type() in ['PARAM_REQUEST_READ', 'PARAM_REQUEST_LIST', 'PARAM_SET']:
        # check that this is relevant to us
        if message.target_system == mav.srcSystem and message.target_component == mav.srcComponent:
            if message.get_type() == 'PARAM_REQUEST_READ':
                m: mavlink.MAVLink_param_request_read_message = message
                send_param(m.param_index, m.param_id)
            elif message.get_type() == 'PARAM_REQUEST_LIST':
                for i in range(len(params)):
                    send_param(i)
            elif message.get_type() == 'PARAM_SET':
                m: mavlink.MAVLink_param_set_message = message
                # check that parameter is defined and sent as float
                if m.param_id in params and m.param_type == mavlink.MAV_PARAM_TYPE_REAL32:
                    params[m.param_id] = m.param_value
                send_param(-1, m.param_id)

    if manager_connected and time() > manager_timeout:
        manager_connected = False
        print('Lost connection to simulation manager')
