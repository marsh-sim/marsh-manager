#!/usr/bin/env python3

"""
Generates required libraries based on the message definitions in mavlink submodule.
Doesn't accept any arguments by design.
"""

from os import path, makedirs
from shutil import copytree
import sys

# add repository root path to allow importing the module
root_path = path.abspath(path.join(path.dirname(__file__), '..'))
sys.path.insert(1, root_path)
# fmt: off
from modules.mavlink.pymavlink.generator import mavgen
from modules.mavlink.pymavlink.generator.mavparse import PROTOCOL_2_0
# fmt: on

definitions_path = path.join(root_path, 'modules', 'mavlink',
                             'message_definitions', 'v1.0')
out_path = path.join(root_path, 'include', 'mavlink')

# generate C library
opts = mavgen.Opts(out_path, wire_protocol=PROTOCOL_2_0,
                   language='C', validate=True, strict_units=True)
mavgen.mavgen(opts, [path.join(definitions_path, 'all.xml')])

out_definitions_path = path.join(out_path, 'message_definitions')
print("Copying message definitions to", out_definitions_path)
makedirs(out_definitions_path, exist_ok=True)
copytree(definitions_path, out_definitions_path, dirs_exist_ok=True)
