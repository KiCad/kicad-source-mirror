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
from pathlib import Path

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


class KiTestFixture:
    junit: bool = False
    _output_path: Path = None
    _junit_folder: Path = None
    _ci_project_dir: Path = None

    def __init__( self, config ) -> None:
        self._junit = False

        env_project_dir = os.getenv( 'CI_PROJECT_DIR' )
        if env_project_dir is not None:
            self._ci_project_dir = Path( env_project_dir )

        junitxml = config.getoption("xmlpath")
        
        if junitxml is not None:
            p = Path( junitxml )
            p = Path( p.parent ) # get the directory as junitxml points to a file
            p = p.resolve() # get absolute path
            self._junit_folder = p
            self._junit = True
        else:
            p = Path.cwd()
            
        p = p.joinpath('output/')
        
        self._output_path = p
        
    def get_output_path( self, sub: str ) -> Path:
        """Return the calculated output path for test artifacts"""
        
        output_path =  self._output_path.joinpath( sub )
        
        os.makedirs( str( output_path ), exist_ok=True )
        
        return output_path
    
    def get_data_file_path( self, file: str ) -> str:
        current_dir = os.path.dirname(__file__)
        base_data_path = os.path.abspath(os.path.join(current_dir, '../../data'))

        return os.path.join(base_data_path, file)
        
    def add_attachment( self, path: str ) -> None:
        """Prints the attachment message line for junit reports"""

        if not self._junit:
            return
        
        # Make the attachment path relative, gitlab in particular wants it
        # relative tot he CI_PROJECT_DIR variable
        attach_src_path = Path( path )
        attach_path: Path = None
        if self._ci_project_dir is not None:
            attach_path = attach_src_path.relative_to( self._ci_project_dir )
        else:
            attach_path = attach_src_path.relative_to( self._junit_folder )
        
        print( "[[ATTACHMENT|{}]]".format( str( attach_path ) ) )
    
    
@pytest.fixture
def kitest( pytestconfig ):
    kitesthelper = KiTestFixture( pytestconfig )
    yield kitesthelper


# We need this on Windows when executing from ctest
if os.name == 'nt':
    for p in os.environ[ 'KICAD_BUILD_PATHS' ].split( ':' ):
        if os.path.isdir( p ):
            os.add_dll_directory( p )