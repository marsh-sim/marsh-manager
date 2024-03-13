#!/usr/bin/env python3

"""
Generates required libraries based on the message definitions in mavlink submodule.
Doesn't accept any arguments by design.
"""

from os import path, makedirs
from shutil import copytree
import sys

# add repository root path to import local module regardless of location
root_path = path.abspath(path.join(path.dirname(__file__), '..'))
sys.path.insert(1, root_path)
# fmt: off - don't move import to top of file
from modules.mavlink.pymavlink.generator import mavgen
from modules.mavlink.pymavlink.generator.mavparse import PROTOCOL_2_0
# fmt: on

definitions_path = path.join(root_path, 'modules', 'mavlink',
                             'message_definitions', 'v1.0')
dialect_file = 'all.xml'
out_path = path.join(root_path, 'include', 'mavlink')

# generate C library
opts = mavgen.Opts(out_path, wire_protocol=PROTOCOL_2_0,
                   language='C', validate=True, strict_units=True)
mavgen.mavgen(opts, [path.join(definitions_path, dialect_file)])

out_definitions_path = path.join(out_path, 'message_definitions')
print("Copying message definitions to", out_definitions_path)
makedirs(out_definitions_path, exist_ok=True)
copytree(definitions_path, out_definitions_path, dirs_exist_ok=True)

# generate Python library
out_path = path.join(root_path, 'dist', 'all_marsh')
makedirs(path.dirname(out_path), exist_ok=True)
opts.output = out_path
opts.language = 'Python3'
mavgen.mavgen(opts, [path.join(definitions_path, dialect_file)])
