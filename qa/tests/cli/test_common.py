#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
# Copyright (C) 2023 KiCad Developers
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#

import pytest
import utils
import re

def test_version():
    command = [utils.kicad_cli(), "version"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0
    assert re.match("\\d+.\\d+.\\d+", stdout)
    assert stderr == ''

def test_version_plain():
    command = [utils.kicad_cli(), "version", "--format=plain"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0
    assert re.match("\\d+.\\d+.\\d+", stdout)
    assert stderr == ''

def test_version_commit():
    command = [utils.kicad_cli(), "version", "--format=commit"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 0
    assert re.match("\\b[0-9a-f]{40}\\b", stdout)
    assert stderr == ''

def test_help():
    command = [utils.kicad_cli(), "help"]
    stdout, stderr, exitcode = utils.run_and_capture(command)
    assert exitcode == 1
    assert stdout != ''
    assert stderr == ''
