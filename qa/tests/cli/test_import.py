#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

"""
Tests for the top-level kicad-cli import command
"""

import utils
from pathlib import Path
import pytest
import json
import os
import re
from conftest import KiTestFixture


def get_project_stem( kitest: KiTestFixture, test_name: str, stem: str ) -> Path:
    """Generate an output project stem for a test, clearing any prior artifacts."""
    output_dir = kitest.get_output_path( f"cli/import/{test_name}/" )
    stem_path = output_dir / stem

    for ext in ( ".kicad_pro", ".kicad_pcb", ".kicad_sch" ):
        artifact = stem_path.with_suffix( ext )

        if artifact.exists():
            artifact.unlink()

    return stem_path


def get_pads_test_file() -> str:
    """Get path to a PADS board test file from the QA test data directory."""
    pads_file = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "pcbnew", "plugins",
                              "pads", "ClaySight_MK1", "ClaySight_MK1.asc" )

    return pads_file if os.path.exists( pads_file ) else None


def get_pcad_test_files() -> tuple:
    """Paths to the P-CAD schematic and board test files, or (None, None)."""
    sch = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "eeschema", "io",
                        "pcad", "pcad_feature_test.sch" )
    pcb = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "pcbnew", "plugins",
                        "pcad", "pcad_4layer_glyph_test_ascii.PCB" )

    if os.path.exists( sch ) and os.path.exists( pcb ):
        return sch, pcb

    return None, None


def get_eagle_test_file() -> str:
    """Get path to an Eagle schematic test file from the QA test data directory."""
    eagle_file = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "eeschema", "io",
                               "eagle", "eagle-import-testfile.sch" )

    return eagle_file if os.path.exists( eagle_file ) else None


def get_easyedapro_v3_test_file() -> str:
    """Get an EasyEDA Pro v3 project fixture with multiple boards."""
    archive = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "pcbnew", "plugins",
                            "easyedapro", "ProProject_LS2K0300Core_2025-11-14.epro2" )

    return archive if os.path.exists( archive ) else None


def get_easyedapro_v2_test_file() -> str:
    """Get an EasyEDA Pro v2 project fixture with multiple boards."""
    archive = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "pcbnew", "plugins",
                            "easyedapro", "ProProject_Yuzuki Chameleon_2023-09-02.epro" )

    return archive if os.path.exists( archive ) else None


def get_easyedapro_v2_single_board_test_file() -> str:
    """Get an EasyEDA Pro v2 project fixture with one board."""
    archive = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "pcbnew", "plugins",
                            "easyedapro", "Scanning Tunneling Microscope OpenSTM ControlBoard.zip" )

    return archive if os.path.exists( archive ) else None


class TestImportHelp:
    """Test help output for the top-level import command"""

    def test_import_help( self, kitest: KiTestFixture ):
        """Test that import --help shows usage information and no --format flag"""
        command = [ utils.kicad_cli(), "import", "--help" ]

        stdout, _, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert "INPUT_FILES" in stdout
        assert "--format" not in stdout
        assert "--layer-map" in stdout


class TestImportErrors:
    """Test error handling for the top-level import command"""

    def test_import_unsupported_file( self, kitest: KiTestFixture ):
        """Test that a file recognized by neither face fails cleanly"""
        stem = get_project_stem( kitest, "errors", "unsupported" )

        unsupported = stem.with_suffix( ".xyz" )
        unsupported.parent.mkdir( parents=True, exist_ok=True )
        unsupported.write_text( "not a real design file\n" )

        command = [ utils.kicad_cli(), "import", str( unsupported ), "-o", str( stem ) ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0
        assert not stem.with_suffix( ".kicad_pro" ).exists()


@pytest.mark.skipif( get_pads_test_file() is None,
                     reason="PADS test files not available" )
class TestImportBoardOnly:
    """Test importing a single board into a project"""

    def test_import_board_creates_project( self, kitest: KiTestFixture ):
        """A single board input yields a board-only project"""
        pads_file = get_pads_test_file()
        stem = get_project_stem( kitest, "board_only", "widget" )

        command = [ utils.kicad_cli(), "import", pads_file, "-o", str( stem ) ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert stem.with_suffix( ".kicad_pro" ).exists()
        assert stem.with_suffix( ".kicad_pcb" ).exists()
        assert not stem.with_suffix( ".kicad_sch" ).exists()

        with open( stem.with_suffix( ".kicad_pcb" ), "r" ) as f:
            assert "(kicad_pcb" in f.read( 100 )

        # Project file must be valid JSON
        with open( stem.with_suffix( ".kicad_pro" ), "r" ) as f:
            json.load( f )

    def test_import_board_with_layer_map( self, kitest: KiTestFixture ):
        """The top-level import accepts --layer-map and wires it through to the board job"""
        pads_file = get_pads_test_file()
        stem = get_project_stem( kitest, "layer_map", "widget" )

        map_path = stem.with_suffix( ".layermap.json" )
        map_path.parent.mkdir( parents=True, exist_ok=True )
        map_path.write_text( json.dumps( { "Bottom Layer": "B.Cu" } ) )

        command = [ utils.kicad_cli(), "import", "--layer-map", str( map_path ),
                    pads_file, "-o", str( stem ) ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert stem.with_suffix( ".kicad_pro" ).exists()
        assert stem.with_suffix( ".kicad_pcb" ).exists()



@pytest.mark.skipif( get_easyedapro_v3_test_file() is None,
                     reason="EasyEDA Pro v3 test file not available" )
class TestImportEasyEdaProProject:
    """Test importing every board in an EasyEDA Pro v3 project."""

    def test_import_all_project_boards( self, kitest: KiTestFixture ):
        """Each PCB document becomes a linked KiCad board."""
        archive = get_easyedapro_v3_test_file()
        stem = get_project_stem( kitest, "easyedapro", "project" )
        first_board = stem.parent / "PCB1.kicad_pcb"
        second_board = stem.parent / "PCB1_1.kicad_pcb"

        for board in ( first_board, second_board ):
            if board.exists():
                board.unlink()

        command = [ utils.kicad_cli(), "import", archive, "-o", str( stem ) ]
        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert first_board.exists()
        assert ( stem.parent / "PCB1-import-fps.pretty" ).is_dir()
        assert ( stem.parent / "PCB1_1-import-fps.pretty" ).is_dir()
        assert second_board.exists()

        with open( stem.with_suffix( ".kicad_pro" ), "r" ) as f:
            project = json.load( f )

        assert sorted( filename for _, filename in project.get( "boards", [] ) ) == [
            "PCB1.kicad_pcb",
            "PCB1_1.kicad_pcb"
        ]


@pytest.mark.skipif( get_easyedapro_v2_test_file() is None,
                     reason="EasyEDA Pro v2 test file not available" )
class TestImportEasyEdaProV2Project:
    """Test importing every board in an EasyEDA Pro v2 project."""

    def test_import_all_project_boards( self, kitest: KiTestFixture ):
        """Each PCB document becomes a source-named, linked KiCad board."""
        archive = get_easyedapro_v2_test_file()
        stem = get_project_stem( kitest, "easyedapro_v2", "project" )

        expected_boards = [
            "PCB.kicad_pcb",
            "树莓派A版形状.kicad_pcb",
            "点位图.kicad_pcb",
            "100ASK.kicad_pcb",
            "散热器外形.kicad_pcb"
        ]

        for board in ( stem.parent / name for name in expected_boards ):
            if board.exists():
                board.unlink()

        command = [ utils.kicad_cli(), "import", archive, "-o", str( stem ) ]
        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0

        for name in expected_boards:
            assert ( stem.parent / name ).exists()

        with open( stem.with_suffix( ".kicad_pro" ), "r" ) as f:
            project = json.load( f )

        assert sorted( filename for _, filename in project.get( "boards", [] ) ) == sorted( expected_boards )

        board_text = ( stem.parent / "PCB.kicad_pcb" ).read_text()
        zone_settings = re.findall(
            r"\(connect_pads\s+\(clearance ([0-9.]+)\)\s+\)\s+\(min_thickness ([0-9.]+)\)",
            board_text,
        )

        copper_zone_settings = [ ( clearance, thickness ) for clearance, thickness in zone_settings
                                  if clearance != "0" ]

        assert copper_zone_settings
        assert { clearance for clearance, _ in copper_zone_settings } == { "0.089" }
        assert { thickness for _, thickness in copper_zone_settings } == { "0.25" }
        assert project["net_settings"]["classes"][0]["clearance"] == 0.2

        rules = ( stem.parent / "PCB.kicad_dru" ).read_text()
        assert "# Board Outline ↔ Track: 11.8 mil" in rules
        assert "# Copper / Plane Zone ↔ Track: 11.8 mil" not in rules

    @pytest.mark.skipif( get_easyedapro_v2_single_board_test_file() is None,
                         reason="Single-board EasyEDA Pro v2 test file not available" )
    def test_import_single_project_board_uses_project_name( self, kitest: KiTestFixture ):
        """A one-board project uses the requested project board filename."""
        archive = get_easyedapro_v2_single_board_test_file()
        stem = get_project_stem( kitest, "easyedapro_v2_single", "project" )
        source_named_board = stem.parent / "ControlBoard.kicad_pcb"

        if source_named_board.exists():
            source_named_board.unlink()

        command = [ utils.kicad_cli(), "import", archive, "-o", str( stem ) ]
        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert stem.with_suffix( ".kicad_pcb" ).exists()
        assert not source_named_board.exists()

        with open( stem.with_suffix( ".kicad_pro" ), "r" ) as f:
            project = json.load( f )

        assert [ filename for _, filename in project.get( "boards", [] ) ] == [
            stem.with_suffix( ".kicad_pcb" ).name
        ]


@pytest.mark.skipif( get_eagle_test_file() is None,
                     reason="Eagle schematic test file not available" )
class TestImportSchematicOnly:
    """Test importing a single schematic into a project"""

    def test_import_schematic_classification( self, kitest: KiTestFixture ):
        """A schematic input is autodetected and yields a schematic-only project"""
        eagle_file = get_eagle_test_file()
        stem = get_project_stem( kitest, "sch_only", "design" )

        command = [ utils.kicad_cli(), "import", eagle_file, "-o", str( stem ) ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert stem.with_suffix( ".kicad_pro" ).exists()
        assert stem.with_suffix( ".kicad_sch" ).exists()
        assert not stem.with_suffix( ".kicad_pcb" ).exists()

        with open( stem.with_suffix( ".kicad_sch" ), "r" ) as f:
            assert "(kicad_sch" in f.read( 100 )


@pytest.mark.skipif( get_pads_test_file() is None or get_eagle_test_file() is None,
                     reason="Both PADS board and Eagle schematic test files are required" )
class TestImportLinkedProject:
    """Test importing a board and a schematic into one linked project"""

    def test_import_dual_input_linked( self, kitest: KiTestFixture ):
        """Board + schematic inputs yield a single linked project sharing a stem"""
        pads_file = get_pads_test_file()
        eagle_file = get_eagle_test_file()
        stem = get_project_stem( kitest, "linked", "board" )

        command = [ utils.kicad_cli(), "import", pads_file, eagle_file, "-o", str( stem ) ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert stem.with_suffix( ".kicad_pro" ).exists()
        assert stem.with_suffix( ".kicad_pcb" ).exists()
        assert stem.with_suffix( ".kicad_sch" ).exists()

        with open( stem.with_suffix( ".kicad_pro" ), "r" ) as f:
            project = json.load( f )

        # The project should reference the produced board and sheet.
        assert project.get( "boards" ), "project boards[] should list the imported board"
        assert project.get( "sheets" ), "project sheets[] should list the imported schematic"


@pytest.mark.skipif( get_pcad_test_files() == ( None, None ),
                     reason="P-CAD test files not available" )
class TestImportPcadProject:
    """Test importing a P-CAD schematic and board pair"""

    def test_import_pcad_pair_linked( self, kitest: KiTestFixture ):
        """P-CAD schematic + board inputs are content-detected and linked into one project"""
        sch_file, pcb_file = get_pcad_test_files()
        stem = get_project_stem( kitest, "pcad_pair", "design" )

        command = [ utils.kicad_cli(), "import", sch_file, pcb_file, "-o", str( stem ) ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert stem.with_suffix( ".kicad_pro" ).exists()
        assert stem.with_suffix( ".kicad_sch" ).exists()
        assert stem.with_suffix( ".kicad_pcb" ).exists()
