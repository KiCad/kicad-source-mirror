#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
# Copyright (C) 2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

import utils
from pathlib import Path
import pytest
import re
from typing import List
from conftest import KiTestFixture


@pytest.mark.parametrize("test_file,output_dir,layers_to_test",
                          [
                            ("cli/artwork_generation_regressions/ZoneFill-4.0.7.kicad_pcb",  "artwork_generation_regressions/ZoneFill-4.0.7",  ["F.Cu","B.Cu"]),
                            #("cli/artwork_generation_regressions/ZoneFill-Legacy.kicad_pcb", "artwork_generation_regressions/ZoneFill-Legacy", ["F.Cu","B.Cu"])
                          ])
def test_pcb_export_svg( kitest: KiTestFixture,
                         test_file: str,
                         output_dir: str,
                         layers_to_test: List[str] ):
    
    input_file = kitest.get_data_file_path( test_file )
    
    for layername in layers_to_test:
        layerNameFixed = "-" + layername.replace( ".", "_" ) 
        generated_svg_dir = str( kitest.get_output_path( "cli/{}/".format( output_dir ) ) )
        generated_svg_name = Path( input_file ).stem + layerNameFixed + "-generated.svg"
        generated_svg_path = Path( generated_svg_dir + "/" + generated_svg_name )

        command = ["kicad-cli", "pcb", "export", "svg", "--page-size-mode", "1",#Current page size
                   "--exclude-drawing-sheet", "--black-and-white", "--layers", layername]
        command.append( "-o" )
        command.append( str( generated_svg_path ) )
        command.append( input_file )

        if generated_svg_path.exists():
            generated_svg_path.unlink()

        assert not generated_svg_path.exists()

        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        #don't assert stderr (legacy fills will have errors)
        assert stdout is not None
        assert generated_svg_path.exists()

        kitest.add_attachment( generated_svg_path )

        svg_source_path = str( Path( input_file ).with_suffix( "" ) )
        svg_source_path += layerNameFixed + ".svg"

        assert utils.svgs_are_equivalent( str( generated_svg_path ), svg_source_path, 1200 )
