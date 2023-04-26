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

class KiTestFixture:
    def __init__( self, config ) -> None:
        self._junit = False
        junitxml = config.getoption("xmlpath")
        
        if junitxml is not None:
            p = Path( junitxml )
            p = Path( p.parent ) # get the directory as junitxml points to a file
            p = p.resolve() # get absolute path
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

        if self._junit:
            print( "[[ATTACHMENT|{}]]".format( path ) )
    
    
@pytest.fixture
def kitest( pytestconfig ):
    kitesthelper = KiTestFixture( pytestconfig )
    yield kitesthelper