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

import difflib
import logging
import subprocess
import os

logger = logging.getLogger("cli_util")

def run_and_capture( command: list ) -> tuple[ str, str, int ]:
    logger.info("Executing command \"%s\"", " ".join( command ))

    proc = subprocess.Popen( command,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        encoding = 'utf-8'
    )

    out,err = proc.communicate()
    
    return out, err, proc.returncode

def textdiff_files( golden_filepath: str, new_filepath: str, skip: int = 0 ) -> bool:
    status: bool = True

    with open( golden_filepath, 'r' ) as f:
        golden_lines = f.readlines()[skip:]

    with open( new_filepath, 'r' ) as f:
        new_lines = f.readlines()[skip:]

    diff = difflib.unified_diff( golden_lines, new_lines, fromfile = golden_filepath, tofile = new_filepath )
    diff_text = ''.join(list(diff))

    if diff_text != "":
        logger.info( "Text diff found:" )
        logger.info( diff_text )

    return diff_text == ""