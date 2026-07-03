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
Tests for kicad-cli sch import command
"""

import utils
from pathlib import Path
import pytest
import json
import os
from conftest import KiTestFixture


def get_output_path( kitest: KiTestFixture, test_name: str, filename: str ) -> Path:
    """Generate an output path for a test file."""
    output_dir = kitest.get_output_path( f"cli/sch_import/{test_name}/" )
    output_path = output_dir / filename

    if output_path.exists():
        output_path.unlink()

    return output_path


def get_pcad_test_file() -> str:
    """Get path to a P-CAD schematic test file from the QA test data directory."""
    test_data_dir = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "eeschema",
                                  "io", "pcad" )
    pcad_file = os.path.join( test_data_dir, "pcad_feature_test.sch" )

    if os.path.exists( pcad_file ):
        return pcad_file

    return None


def get_eagle_test_file() -> str:
    """Get path to an Eagle schematic test file from the QA test data directory."""
    test_data_dir = os.path.join( os.path.dirname( __file__ ), "..", "..", "data", "eeschema",
                                  "io", "eagle" )
    eagle_file = os.path.join( test_data_dir, "eagle-import-testfile.sch" )

    if os.path.exists( eagle_file ):
        return eagle_file

    return None


class TestSchImportHelp:
    """Test help output for sch import command"""

    def test_sch_import_help( self, kitest: KiTestFixture ):
        """Test that sch import --help shows usage information"""
        command = [ utils.kicad_cli(), "sch", "import", "--help" ]

        stdout, _, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert "Import a non-KiCad schematic file" in stdout
        assert "--format" in stdout
        assert "--report-format" in stdout


class TestSchImportErrors:
    """Test error handling for sch import command"""

    def test_import_missing_file( self, kitest: KiTestFixture ):
        """Test error when input file doesn't exist"""
        output_path = get_output_path( kitest, "errors", "output.kicad_sch" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            "/nonexistent/path/file.sch",
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0
        assert not output_path.exists()

    def test_import_invalid_format( self, kitest: KiTestFixture ):
        """Test error when invalid format specified"""
        output_path = get_output_path( kitest, "errors", "invalid_format.kicad_sch" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            "--format", "invalid_format",
            "/tmp/test.sch",
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0


@pytest.mark.skipif( get_eagle_test_file() is None,
                     reason="Eagle schematic test file not available" )
class TestSchImportEagle:
    """Test Eagle schematic import"""

    def test_import_eagle_file( self, kitest: KiTestFixture ):
        """Test importing an Eagle schematic with an explicit format"""
        eagle_file = get_eagle_test_file()
        output_path = get_output_path( kitest, "eagle", "imported.kicad_sch" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            "--format", "eagle",
            eagle_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()
        assert output_path.stat().st_size > 0

        with open( output_path, "r" ) as f:
            content = f.read( 100 )
            assert "(kicad_sch" in content

    def test_import_auto_detect( self, kitest: KiTestFixture ):
        """Test auto-detection of the Eagle format"""
        eagle_file = get_eagle_test_file()
        output_path = get_output_path( kitest, "eagle", "auto_detected.kicad_sch" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            eagle_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()

    def test_import_json_report( self, kitest: KiTestFixture ):
        """Test JSON report generation"""
        eagle_file = get_eagle_test_file()
        output_path = get_output_path( kitest, "eagle", "with_report.kicad_sch" )
        report_path = get_output_path( kitest, "eagle", "import_report.json" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            "--format", "eagle",
            "--report-format", "json",
            "--report-file", str( report_path ),
            eagle_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()
        assert report_path.exists()

        with open( report_path, "r" ) as f:
            report = json.load( f )

        assert "source_file" in report
        assert "source_format" in report
        assert "output_file" in report
        assert "statistics" in report
        assert "symbols" in report["statistics"]
        assert "sheets" in report["statistics"]


@pytest.mark.skipif( get_pcad_test_file() is None,
                     reason="P-CAD schematic test file not available" )
class TestSchImportPcad:
    """Test P-CAD schematic import"""

    def test_import_pcad_file( self, kitest: KiTestFixture ):
        """Test importing a P-CAD schematic with an explicit format"""
        pcad_file = get_pcad_test_file()
        output_path = get_output_path( kitest, "pcad", "imported.kicad_sch" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            "--format", "pcad",
            pcad_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert output_path.exists()

        with open( output_path, "r" ) as f:
            assert "(kicad_sch" in f.read( 100 )

    def test_import_pcad_auto_detect( self, kitest: KiTestFixture ):
        """P-CAD detection is content-based, not extension-based"""
        pcad_file = get_pcad_test_file()
        output_path = get_output_path( kitest, "pcad", "auto_detected.kicad_sch" )

        command = [
            utils.kicad_cli(),
            "sch", "import",
            pcad_file,
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0
        assert "P-CAD" in stdout
        assert output_path.exists()
