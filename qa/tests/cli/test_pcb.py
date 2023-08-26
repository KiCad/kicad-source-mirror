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
from typing import List, Tuple
from conftest import KiTestFixture


def get_generated_path(kitest: KiTestFixture,
                       input_file: Path,
                       test_name: str,
                       layer_name: str,
                       ) -> Tuple[Path, str]:
    layer_name_fixed = "-" + layer_name.replace( ".", "_" )
    generated_dir = str( kitest.get_output_path( "cli/{}/{}/".format( test_name, input_file.stem ) ) )
    generated_name = input_file.stem + layer_name_fixed + "-generated" + input_file.suffix
    generated_path = Path( generated_dir + "/" + generated_name )

    if generated_path.exists():
        generated_path.unlink()  # Delete file

    return [generated_path, layer_name_fixed]


def run_and_check_export_command(kitest: KiTestFixture,
                                 command: List[str],
                                 expected_output_file: Path):
    assert not expected_output_file.exists()

    stdout, stderr, exitcode = utils.run_and_capture( command )
    assert exitcode == 0
    # Don't assert stderr (legacy fills will have errors)
    assert stdout is not None
    assert expected_output_file.exists()

    kitest.add_attachment( expected_output_file )


@pytest.mark.parametrize("test_file,layers_to_test",
                         [
                            (
                                "cli/artwork_generation_regressions/ZoneFill-4.0.7.kicad_pcb",
                                ["F.Cu","B.Cu"]
                            ),
                            (   "cli/artwork_generation_regressions/ZoneFill-Legacy.brd",
                                ["F.Cu","B.Cu"]
                            )
                         ])
def test_pcb_export_svg( kitest: KiTestFixture,
                         test_file: str,
                         layers_to_test: List[str] ):

    input_file = kitest.get_data_file_path( test_file )

    for layer_name in layers_to_test:
        generated_svg_path, layer_name_fixed = get_generated_path( kitest,
                                                                   Path( input_file ).with_suffix( ".svg" ),
                                                                   "export_svg",
                                                                   layer_name )

        command = ["kicad-cli", "pcb", "export", "svg", "--page-size-mode", "1",  # 1=Current page size
                   "--exclude-drawing-sheet", "--black-and-white", "--layers", layer_name,
                   "-o", str(generated_svg_path), input_file]

        run_and_check_export_command( kitest, command, generated_svg_path )

        svg_source_path = str( Path( input_file ).with_suffix( "" ) )
        svg_source_path += layer_name_fixed + ".svg"

        # Comparison DPI = 1270 => 1px == 20um. I.e. allowable error of 60 um after eroding
        assert utils.svgs_are_equivalent( str( generated_svg_path ), svg_source_path, 1270 )


@pytest.mark.skipif(not utils.is_gerbv_installed(), reason="Requires gerbv to be installed")
@pytest.mark.parametrize("test_file,layers_to_test,originInches,windowsizeInches",
                         [
                            (
                                "cli/artwork_generation_regressions/ZoneFill-4.0.7.kicad_pcb",
                                ["F.Cu","B.Cu"],
                                ( 3.5, -4.6 ),
                                ( 4.3, 2.2 )
                            ),
                            (   "cli/artwork_generation_regressions/ZoneFill-Legacy.brd",
                                ["F.Cu","B.Cu"],
                                ( -0.6, -0.4 ),
                                ( 1.7, 0.8 )
                            )
                         ])
def test_pcb_export_gerber( kitest: KiTestFixture,
                            test_file: str,
                            layers_to_test: List[str],
                            originInches :  Tuple[float, float],
                            windowsizeInches :  Tuple[float, float] ):

    input_file = kitest.get_data_file_path( test_file )

    for layer_name in layers_to_test:
        generated_gerber_path, layer_name_fixed = get_generated_path( kitest,
                                                                      Path( input_file ).with_suffix( ".gbr" ),
                                                                      "export_gerber",
                                                                      layer_name )

        command = ["kicad-cli", "pcb", "export", "gerber", "--no-x2", "--use-drill-file-origin",
                   "--layers", layer_name,
                   "-o", str(generated_gerber_path), input_file]

        run_and_check_export_command( kitest, command, generated_gerber_path )

        gbr_source_path = str( Path( input_file ).with_suffix( "" ) )
        gbr_source_path += layer_name_fixed + ".gbr"

        # Comparison DPI = 5080 => 1px == 5um. I.e. allowable error of 15 um after eroding
        assert utils.gerbers_are_equivalent( str( generated_gerber_path ), gbr_source_path, 5080,
                                             originInches, windowsizeInches )
