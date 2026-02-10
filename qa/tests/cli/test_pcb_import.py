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
Tests for kicad-cli pcb import command
"""

import utils
from pathlib import Path
import pytest
import json
import os
from conftest import KiTestFixture


def get_output_path( kitest: KiTestFixture, test_name: str, filename: str ) -> Path:
    """Generate an output path for a test file."""
    output_dir = kitest.get_output_path( f"cli/pcb_import/{test_name}/" )
    output_path = output_dir / filename

    if output_path.exists():
        output_path.unlink()

    return output_path


def get_pads_test_file() -> str:
    """Get path to a PADS test file from the QA test data directory."""
    test_data_dir = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "pcbnew", "plugins", "pads" )
    pads_file = os.path.join( test_data_dir, "ClaySight_MK1", "ClaySight_MK1.asc" )

    if os.path.exists( pads_file ):
        return pads_file

    return None


class TestPcbImportHelp:
    """Test help output for pcb import command"""

    def test_pcb_import_help( self, kitest: KiTestFixture ):
        """Test that pcb import --help shows usage information"""
        command = [ utils.kicad_cli(), "pcb", "import", "--help" ]

        stdout, _, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert "Import a non-KiCad PCB file" in stdout
        assert "--format" in stdout
        assert "--report-format" in stdout


class TestPcbImportErrors:
    """Test error handling for pcb import command"""

    def test_import_missing_file( self, kitest: KiTestFixture ):
        """Test error when input file doesn't exist"""
        output_path = get_output_path( kitest, "errors", "output.kicad_pcb" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "/nonexistent/path/file.asc",
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0
        assert not output_path.exists()

    def test_import_invalid_format( self, kitest: KiTestFixture ):
        """Test error when invalid format specified"""
        output_path = get_output_path( kitest, "errors", "invalid_format.kicad_pcb" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "invalid_format",
            "/tmp/test.asc",
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0


@pytest.mark.skipif( get_pads_test_file() is None,
                     reason="PADS test files not available at /tmp/JILA_pads_importer/" )
class TestPcbImportPads:
    """Test PADS file import"""

    def test_import_pads_file( self, kitest: KiTestFixture ):
        """Test importing a PADS file"""
        pads_file = get_pads_test_file()
        output_path = get_output_path( kitest, "pads", "imported.kicad_pcb" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            pads_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()
        assert output_path.stat().st_size > 0

        # Verify it's a valid KiCad file
        with open( output_path, "r" ) as f:
            content = f.read( 100 )
            assert "(kicad_pcb" in content

    def test_import_auto_detect( self, kitest: KiTestFixture ):
        """Test auto-detection of PADS format"""
        pads_file = get_pads_test_file()
        output_path = get_output_path( kitest, "pads", "auto_detected.kicad_pcb" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            pads_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()

    def test_import_json_report( self, kitest: KiTestFixture ):
        """Test JSON report generation"""
        pads_file = get_pads_test_file()
        output_path = get_output_path( kitest, "pads", "with_report.kicad_pcb" )
        report_path = get_output_path( kitest, "pads", "import_report.json" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--report-format", "json",
            "--report-file", str( report_path ),
            pads_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()
        assert report_path.exists()

        # Validate JSON report
        with open( report_path, "r" ) as f:
            report = json.load( f )

        assert "source_file" in report
        assert "source_format" in report
        assert "output_file" in report
        assert "statistics" in report
        assert "footprints" in report["statistics"]
        assert "tracks" in report["statistics"]
        assert "vias" in report["statistics"]
        assert "zones" in report["statistics"]

    def test_import_text_report( self, kitest: KiTestFixture ):
        """Test text report generation"""
        pads_file = get_pads_test_file()
        output_path = get_output_path( kitest, "pads", "with_text_report.kicad_pcb" )
        report_path = get_output_path( kitest, "pads", "import_report.txt" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--report-format", "text",
            "--report-file", str( report_path ),
            pads_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()
        assert report_path.exists()

        # Validate text report content
        with open( report_path, "r" ) as f:
            content = f.read()

        assert "Import Report" in content
        assert "Source file:" in content
        assert "Statistics:" in content
        assert "Footprints:" in content

    def test_import_json_report_stdout( self, kitest: KiTestFixture ):
        """Test JSON report output to stdout"""
        pads_file = get_pads_test_file()
        output_path = get_output_path( kitest, "pads", "report_stdout.kicad_pcb" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--report-format", "json",
            pads_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0

        # Find JSON in stdout
        json_start = stdout.find( "{" )
        json_end = stdout.rfind( "}" ) + 1

        assert json_start >= 0
        assert json_end > json_start

        report = json.loads( stdout[json_start:json_end] )
        assert "statistics" in report
