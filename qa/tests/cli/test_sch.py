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

import utils
import cairosvg
import re
from pathlib import Path
import pytest
from typing import List


@pytest.mark.parametrize("test_file,output_dir,compare_fn,cli_args",
                            [("cli/basic_test/basic_test.kicad_sch", "basic_test", "cli/basic_test/basic_test.svg", []),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test_nobg_bnw_nods", "cli/basic_test/basic_test_nobg_bnw_nods.svg", ["--no-background-color", "--exclude-drawing-sheet", "--black-and-white"])
                             ])
def test_sch_export_svg( kitest,
                         test_file: str,
                         output_dir: str,
                         compare_fn: str,
                         cli_args: List[str] ):
    input_file = kitest.get_data_file_path( test_file )

    output_path =  kitest.get_output_path( "cli/{}/".format( output_dir ) )

    command = [utils.kicad_cli(), "sch", "export", "svg"]
    command.extend( cli_args )
    command.append( "-o" )
    command.append( str( output_path ) )
    command.append( input_file )

    stdout, stderr, exitcode = utils.run_and_capture( command )

    assert exitcode == 0
    assert stderr == ''
    assert stdout is not None

    stdout_regex = re.search("Plotted to '(.+)'", stdout)
    assert stdout_regex

    # now try and manipulate the extracted path
    output_svg_path = Path( stdout_regex.group(1) )
    assert output_svg_path.exists()

    kitest.add_attachment( output_svg_path )

    png_converted_from_svg_path = output_svg_path.with_suffix( '.png' )

    cairosvg.svg2png( url=str( output_svg_path ), write_to=str( png_converted_from_svg_path ), dpi=1200 )

    compare_file_path = kitest.get_data_file_path( compare_fn )
    compare_stem = f"orig_{output_dir}"
    compare_png_converted_from_svg_path = output_svg_path.with_suffix( '.png' ).with_stem(compare_stem)
    cairosvg.svg2png( url=str( compare_file_path ), write_to=str( compare_png_converted_from_svg_path ), dpi=1200 )

    assert utils.images_are_equal( png_converted_from_svg_path, compare_png_converted_from_svg_path  )


@pytest.mark.parametrize("test_file,output_fn,line_skip_count,skip_compare,cli_args",
                            [("cli/basic_test/basic_test.kicad_sch", "basic_test.netlist.kicadsexpr", 5, True, []),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.netlist.kicadsexpr", 5, True,["--format=kicadsexpr"]),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.netlist.kicadxml", 6, True,["--format=kicadxml"]),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.netlist.cadstar", 3, False, ["--format=cadstar"]),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.netlist.orcadpcb2", 1, False, ["--format=orcadpcb2"])
                             ])
def test_sch_export_netlist( kitest,
                             test_file: str,
                             output_fn: str,
                             line_skip_count: int,
                             skip_compare: bool,
                             cli_args: List[str] ):
    input_file = kitest.get_data_file_path( test_file )
    compare_filepath = kitest.get_data_file_path( "cli/basic_test/{}".format( output_fn ) )

    output_filepath =  kitest.get_output_path( "cli/" ).joinpath( output_fn )

    command = [utils.kicad_cli(), "sch", "export", "netlist"]
    command.extend( cli_args )
    command.append( "-o" )
    command.append( str( output_filepath ) )
    command.append( input_file )

    stdout, stderr, exitcode = utils.run_and_capture( command )

    assert exitcode == 0
    assert stderr == ''

    # some of our netlist formats are not cross platform so skip for now
    if not skip_compare:
        assert utils.textdiff_files( compare_filepath, str( output_filepath ), line_skip_count )

    kitest.add_attachment( str( output_filepath ) )


@pytest.mark.parametrize("test_file,output_fn,cli_args",
                            [("cli/basic_test/basic_test.kicad_sch", "basic_test.pdf", []),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.bnw.nods.nobg.pdf", ["--black-and-white","--exclude-drawing-sheet","--no-background-color"]),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.pone.pdf", ["--pages", "1"])
                             ])
def test_sch_export_pdf( kitest,
                         test_file: str,
                         output_fn: str,
                         cli_args: List[str] ):
    input_file = kitest.get_data_file_path( test_file )

    output_filepath =  kitest.get_output_path( "cli/" ).joinpath( output_fn )

    command = [utils.kicad_cli(), "sch", "export", "pdf"]
    command.extend( cli_args )
    command.append( "-o" )
    command.append( str( output_filepath ) )
    command.append( input_file )

    stdout, stderr, exitcode = utils.run_and_capture( command )

    assert exitcode == 0
    assert stderr == ''

    kitest.add_attachment( str( output_filepath ) )


@pytest.mark.parametrize("test_file,output_fn,line_skip_count,cli_args",
                            [("cli/basic_test/basic_test.kicad_sch", "basic_test.pythonbom", 6, [])
                             ])
def test_sch_export_pythonbom( kitest,
                         test_file: str,
                         output_fn: str,
                         line_skip_count: int,
                         cli_args: List[str] ):
    input_file = kitest.get_data_file_path( test_file )
    compare_filepath = kitest.get_data_file_path( "cli/basic_test/{}".format( output_fn ) )

    output_filepath =  kitest.get_output_path( "cli/" ).joinpath( output_fn )

    command = [utils.kicad_cli(), "sch", "export", "python-bom"]
    command.extend( cli_args )
    command.append( "-o" )
    command.append( str( output_filepath ) )
    command.append( input_file )

    stdout, stderr, exitcode = utils.run_and_capture( command )

    assert exitcode == 0
    assert stderr == ''

    # pythonbom is not currently crossplatform (platform specific paths) to enable diffs

    kitest.add_attachment( str( output_filepath ) )



@pytest.mark.parametrize("test_file,output_fn,line_skip_count,skip_compare,expected_exit_code,cli_args",
                            [("cli/basic_test/basic_test.kicad_sch", "basic_test.erc.rpt", 1, False, 0, []),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.erc.rpt", 1, False, 0, ["--format=report"]),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.erc.json", 9, False,0, ["--format=json"]),
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.erc.unitsin.rpt", 1, False, 0, ["--format=report", "--units=in"]),
                             ])
def test_sch_export_erc( kitest,
                             test_file: str,
                             output_fn: str,
                             line_skip_count: int,
                             skip_compare: bool,
                             expected_exit_code: int,
                             cli_args: List[str] ):
    input_file = kitest.get_data_file_path( test_file )
    compare_filepath = kitest.get_data_file_path( "cli/basic_test/{}".format( output_fn ) )

    output_filepath =  kitest.get_output_path( "cli/" ).joinpath( output_fn )

    command = [utils.kicad_cli(), "sch", "erc"]
    command.extend( cli_args )
    command.append( "-o" )
    command.append( str( output_filepath ) )
    command.append( input_file )

    stdout, stderr, exitcode = utils.run_and_capture( command )

    assert exitcode == expected_exit_code
    assert stderr == ''

    # some of our netlist formats are not cross platform so skip for now
    if not skip_compare:
        assert utils.textdiff_files( compare_filepath, str( output_filepath ), line_skip_count )

    kitest.add_attachment( str( output_filepath ) )


@pytest.mark.parametrize("test_file,output_fn,expected_headers,cli_args",
                            [
                             # Default fields include ${QUANTITY} and ${DNP}
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.bom_default.csv",
                              ["Refs", "Value", "Footprint", "Qty", "DNP"], []),
                             # Explicit fields with ${QUANTITY}
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.bom_quantity.csv",
                              ["Refs", "Value", "Qty"],
                              ["--fields", "Reference,Value,${QUANTITY}", "--labels", "Refs,Value,Qty"]),
                             # Explicit fields with ${ITEM_NUMBER}
                             ("cli/basic_test/basic_test.kicad_sch", "basic_test.bom_item_number.csv",
                              ["#", "Refs", "Value"],
                              ["--fields", "${ITEM_NUMBER},Reference,Value", "--labels", "#,Refs,Value"]),
                             ])
def test_sch_export_bom( kitest,
                         test_file: str,
                         output_fn: str,
                         expected_headers: List[str],
                         cli_args: List[str] ):
    """Test BOM export with various field configurations, including virtual fields like ${QUANTITY}."""
    input_file = kitest.get_data_file_path( test_file )

    output_filepath = kitest.get_output_path( "cli/" ).joinpath( output_fn )

    command = [utils.kicad_cli(), "sch", "export", "bom"]
    command.extend( cli_args )
    command.append( "-o" )
    command.append( str( output_filepath ) )
    command.append( input_file )

    stdout, stderr, exitcode = utils.run_and_capture( command )

    assert exitcode == 0, f"BOM export failed with exit code {exitcode}: {stderr}"
    assert output_filepath.exists(), f"Output file not created: {output_filepath}"

    # Read the BOM file and verify headers
    with open( output_filepath, 'r' ) as f:
        first_line = f.readline().strip()

    # Parse the CSV header (removing quotes)
    actual_headers = [h.strip().strip('"') for h in first_line.split(',')]

    for expected in expected_headers:
        assert expected in actual_headers, f"Expected header '{expected}' not found in BOM output. Got: {actual_headers}"

    kitest.add_attachment( str( output_filepath ) )
