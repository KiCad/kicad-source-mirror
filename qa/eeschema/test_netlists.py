#!/usr/bin/env python3

# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
# Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import argparse
import os
import subprocess
import sys


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Tests eeschema netlist generation')
    parser.add_argument('--initialize', action='store_true',
                        help='Generates the "good" reference netlists for each test case')
    parser.add_argument('binary_dir', default='.')

    args = parser.parse_args()

    eeschema = os.path.abspath(os.path.join(args.binary_dir, 'eeschema/eeschema'))

    print('eeschema binary: {}'.format(eeschema))

    # The way we discover testcases is simple:
    #
    # The root directory is hardcoded as ./data/netlists
    # Inside that are testcases in folders.
    #
    # Each testcase must have the root sheet named the same as the folder, for example:
    #   ./data/netlists/video/video.sch
    #
    # The good netlist will be stored as ./data/netlists/video/video.net
    # The test netlist will be generated as ./data/netlists/video/video_test.net

    data_dir = os.path.join(os.getcwd(), 'data/netlists')

    with os.scandir(data_dir) as it:
        for entry in it:
            if entry.is_dir():

                project = entry.name
                project_dir = os.path.join(data_dir, project)

                sch_file = os.path.join(project_dir, project + '.kicad_sch')
                good_net_file = os.path.join(project_dir, project + '.net')

                net_file = good_net_file if args.initialize else os.path.join(
                        project_dir, project + '_test.net')

                if not os.path.exists(good_net_file) and not args.initialize:
                    print("FAILED: {} missing good netlist file for comparison".format(project))
                    sys.exit(-1)

                subprocess.run([eeschema, '--netlist', net_file, sch_file], cwd=project_dir)

                result = subprocess.run(['./netdiff.py', good_net_file, net_file],
                                        stdout=subprocess.PIPE)

                if not args.initialize:
                    os.remove(net_file)

                if result.returncode != 0:
                    print("FAILED: {} netlist does not match:".format(project))
                    print(result.stdout)

                    sys.exit(result.returncode)

    sys.exit(0)
