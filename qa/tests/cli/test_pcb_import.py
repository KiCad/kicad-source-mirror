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
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
        assert "--layer-map" in stdout


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

    def test_import_layer_map_invalid_json( self, kitest: KiTestFixture ):
        """A malformed layer-map file is rejected before any output is produced"""
        output_path = get_output_path( kitest, "errors", "bad_map.kicad_pcb" )
        map_path = get_output_path( kitest, "errors", "bad_map.json" )
        map_path.parent.mkdir( parents=True, exist_ok=True )
        map_path.write_text( "{ this is not valid json" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--layer-map", str( map_path ),
            "/tmp/test.asc",
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0
        assert not output_path.exists()

    def test_import_layer_map_missing_file( self, kitest: KiTestFixture ):
        """A nonexistent layer-map file is reported as an argument error"""
        output_path = get_output_path( kitest, "errors", "no_map.kicad_pcb" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--layer-map", "/nonexistent/layers.json",
            "/tmp/test.asc",
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code != 0
        assert not output_path.exists()


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

    def test_import_report_layer_mapping_by_source( self, kitest: KiTestFixture ):
        """The report's layer_mapping is keyed by source layer names with a resolution method"""
        pads_file = get_pads_test_file()
        output_path = get_output_path( kitest, "layer_map", "report.kicad_pcb" )
        report_path = get_output_path( kitest, "layer_map", "report.json" )

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

        with open( report_path, "r" ) as f:
            report = json.load( f )

        mapping = report.get( "layer_mapping" )
        assert mapping, "report should contain a non-empty layer_mapping"

        for source, entry in mapping.items():
            assert "kicad_layer" in entry
            assert entry.get( "method" ) in ( "auto", "explicit" )

    def test_import_layer_map_override( self, kitest: KiTestFixture ):
        """An explicit --layer-map entry overrides the auto guess and is marked 'explicit'"""
        pads_file = get_pads_test_file()

        # First pass discovers a real source layer and the KiCad layer the auto-mapper chose for
        # it.  Reusing that target guarantees the override is a permitted destination.
        probe_pcb = get_output_path( kitest, "layer_map", "probe.kicad_pcb" )
        probe_report = get_output_path( kitest, "layer_map", "probe.json" )

        probe_cmd = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--report-format", "json",
            "--report-file", str( probe_report ),
            pads_file,
            "-o", str( probe_pcb )
        ]

        _, _, probe_rc = utils.run_and_capture( probe_cmd )
        assert probe_rc == 0

        with open( probe_report, "r" ) as f:
            probe_mapping = json.load( f )["layer_mapping"]

        target_source = None
        target_layer = None

        for source, entry in probe_mapping.items():
            if source and entry["method"] == "auto" and entry["kicad_layer"]:
                target_source = source
                target_layer = entry["kicad_layer"]
                break

        if target_source is None:
            pytest.skip( "No auto-mapped source layer available to override" )

        # Second pass pins that source layer explicitly; the method must flip to 'explicit'.
        map_path = get_output_path( kitest, "layer_map", "override.json" )
        map_path.parent.mkdir( parents=True, exist_ok=True )
        map_path.write_text( json.dumps( { target_source: target_layer } ) )

        out_pcb = get_output_path( kitest, "layer_map", "override.kicad_pcb" )
        out_report = get_output_path( kitest, "layer_map", "override_report.json" )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--layer-map", str( map_path ),
            "--report-format", "json",
            "--report-file", str( out_report ),
            pads_file,
            "-o", str( out_pcb )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0

        with open( out_report, "r" ) as f:
            mapping = json.load( f )["layer_mapping"]

        assert mapping[target_source]["method"] == "explicit"
        assert mapping[target_source]["kicad_layer"] == target_layer

    def test_import_layer_map_unmatched_entry_warns( self, kitest: KiTestFixture ):
        """A layer-map key that matches no source layer is reported as a warning, not silently"""
        pads_file = get_pads_test_file()
        out_pcb = get_output_path( kitest, "layer_map", "unmatched.kicad_pcb" )
        out_report = get_output_path( kitest, "layer_map", "unmatched_report.json" )

        map_path = get_output_path( kitest, "layer_map", "unmatched.json" )
        map_path.parent.mkdir( parents=True, exist_ok=True )
        map_path.write_text( json.dumps( { "No Such Source Layer": "F.Cu" } ) )

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--layer-map", str( map_path ),
            "--report-format", "json",
            "--report-file", str( out_report ),
            pads_file,
            "-o", str( out_pcb )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        assert return_code == 0

        with open( out_report, "r" ) as f:
            report = json.load( f )

        assert any( "No Such Source Layer" in w for w in report.get( "warnings", [] ) )
