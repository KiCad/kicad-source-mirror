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
import os

def pytest_report_header(config):
    """Print key environment vars."""
    path_val = os.environ.get("PATH", "")
    pythonpath_val = os.environ.get("PYTHONPATH", "")
    kibuildpaths_val = os.environ.get("KICAD_BUILD_PATHS", "")

    message = "Environment:\n"
    message += f"PATH={path_val}\n"
    message += f"PYTHONPATH={pythonpath_val}\n"
    message += f"KICAD_BUILD_PATHS={kibuildpaths_val}\n"

    return message
    
# We need this on Windows when executing from ctest
if os.name == 'nt':
    for p in os.environ[ 'KICAD_BUILD_PATHS' ].split( ':' ):
        if os.path.isdir( p ):
            os.add_dll_directory( p )