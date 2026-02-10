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
PADS Importer QA Tests

These tests verify the PADS importer by:
1. Importing PADS .asc files to KiCad format
2. Exporting gerbers from the imported board
3. Comparing exported gerbers against reference gerbers
4. Reporting diff percentages per layer

Environment Variables:
    PADS_QA_SAVE_ALL_DIFFS: Set to "1" to save diff images for all layers,
                            not just failing ones. Useful for debugging.
"""

import utils
from pathlib import Path
import pytest
import json
import os
from conftest import KiTestFixture
from dataclasses import dataclass, field
from typing import Dict, List, Optional


def should_save_all_diffs() -> bool:
    """Check if we should save diff images for all layers, not just failing ones."""
    return os.environ.get( "PADS_QA_SAVE_ALL_DIFFS", "" ) == "1"


# Map KiCad layer names to gerber export layer arguments
KICAD_LAYER_TO_GERBER_ARG = {
    "F.Cu": "F.Cu",
    "B.Cu": "B.Cu",
    "In1.Cu": "In1.Cu",
    "In2.Cu": "In2.Cu",
    "In3.Cu": "In3.Cu",
    "In4.Cu": "In4.Cu",
    "In5.Cu": "In5.Cu",
    "In6.Cu": "In6.Cu",
    "F.Mask": "F.Mask",
    "B.Mask": "B.Mask",
    "F.Paste": "F.Paste",
    "B.Paste": "B.Paste",
    "F.Silkscreen": "F.Silkscreen",
    "B.Silkscreen": "B.Silkscreen",
}

# Map KiCad layer names to gerber export file suffixes
KICAD_LAYER_TO_SUFFIX = {
    "F.Cu": "-F_Cu.gtl",
    "B.Cu": "-B_Cu.gbl",
    "In1.Cu": "-In1_Cu.g2",
    "In2.Cu": "-In2_Cu.g3",
    "In3.Cu": "-In3_Cu.g4",
    "In4.Cu": "-In4_Cu.g5",
    "In5.Cu": "-In5_Cu.g6",
    "In6.Cu": "-In6_Cu.g7",
    "F.Mask": "-F_Mask.gts",
    "B.Mask": "-B_Mask.gbs",
    "F.Paste": "-F_Paste.gtp",
    "B.Paste": "-B_Paste.gbp",
    "F.Silkscreen": "-F_Silkscreen.gto",
    "B.Silkscreen": "-B_Silkscreen.gbo",
}


@dataclass
class LayerResult:
    """Result of comparing a single layer"""
    layer_name: str
    reference_file: str
    generated_file: str
    diff_percentage: float
    passed: bool
    diff_image: Optional[str] = None


@dataclass
class DrillResult:
    """Result of comparing drill files"""
    reference_file: str
    generated_file: str
    diff_percentage: float
    passed: bool
    hole_count_reference: int = 0
    hole_count_generated: int = 0
    diff_image: Optional[str] = None


@dataclass
class TestCaseResult:
    """Result of testing a complete design"""
    name: str
    passed: bool
    layer_results: List[LayerResult] = field( default_factory=list )
    drill_results: List[DrillResult] = field( default_factory=list )
    import_report: Optional[Dict] = None
    error: Optional[str] = None


def get_pads_test_cases() -> List[str]:
    """Get list of available PADS test cases."""
    test_data_dir = Path( __file__ ).parent.parent.parent / "data" / "pcbnew" / "plugins" / "pads"

    if not test_data_dir.exists():
        return []

    test_cases = []

    for item in test_data_dir.iterdir():
        if item.is_dir() and ( item / "config.json" ).exists():
            test_cases.append( item.name )

    return test_cases


def load_test_config( test_case: str ) -> Dict:
    """Load the configuration for a test case."""
    test_data_dir = Path( __file__ ).parent.parent.parent / "data" / "pcbnew" / "plugins" / "pads"
    config_path = test_data_dir / test_case / "config.json"

    with open( config_path, "r" ) as f:
        return json.load( f )


def get_test_data_path( test_case: str ) -> Path:
    """Get the path to test data for a test case."""
    return Path( __file__ ).parent.parent.parent / "data" / "pcbnew" / "plugins" / "pads" / test_case


class PadsImporterHelper:
    """Helper class for PADS importer tests"""

    def __init__( self, kitest: KiTestFixture, test_case: str ):
        self.kitest = kitest
        self.test_case = test_case
        self.config = load_test_config( test_case )
        self.test_data_path = get_test_data_path( test_case )
        self.output_dir = kitest.get_output_path( f"cli/pads_importer/{test_case}/" )

    def import_pads_file( self ) -> tuple:
        """Import the PADS file and return (output_path, report, return_code)"""
        source_file = self.test_data_path / self.config["source_file"]
        output_path = self.output_dir / f"{self.test_case}.kicad_pcb"
        report_path = self.output_dir / "import_report.json"

        if output_path.exists():
            output_path.unlink()

        if report_path.exists():
            report_path.unlink()

        command = [
            utils.kicad_cli(),
            "pcb", "import",
            "--format", "pads",
            "--report-format", "json",
            "--report-file", str( report_path ),
            str( source_file ),
            "-o", str( output_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        report = None

        if report_path.exists():
            with open( report_path, "r" ) as f:
                report = json.load( f )

        return output_path, report, return_code

    def export_gerbers( self, board_path: Path ) -> Dict[str, Path]:
        """Export gerbers from the imported board."""
        gerber_output_dir = self.output_dir / "gerbers"
        gerber_output_dir.mkdir( parents=True, exist_ok=True )

        # Build list of all layers to export at once
        layers_to_export = []

        for ref_gerber, kicad_layer in self.config["layer_mapping"].items():
            if kicad_layer in KICAD_LAYER_TO_GERBER_ARG:
                layers_to_export.append( KICAD_LAYER_TO_GERBER_ARG[kicad_layer] )

        if not layers_to_export:
            return {}

        # Use the plural 'gerbers' command which exports all layers at once
        command = [
            utils.kicad_cli(),
            "pcb", "export", "gerbers",
            "--no-x2",
            "--layers", ",".join( layers_to_export ),
            "-o", str( gerber_output_dir ) + "/",
            str( board_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        exported_gerbers = {}

        # Map reference gerbers to exported files
        for ref_gerber, kicad_layer in self.config["layer_mapping"].items():
            if kicad_layer not in KICAD_LAYER_TO_SUFFIX:
                continue

            suffix = KICAD_LAYER_TO_SUFFIX[kicad_layer]
            expected_file = gerber_output_dir / ( board_path.stem + suffix )

            if expected_file.exists():
                exported_gerbers[ref_gerber] = expected_file

        return exported_gerbers

    def compare_layer( self, ref_gerber: str, generated_gerber: Path ) -> LayerResult:
        """Compare a generated gerber against reference."""
        reference_path = self.test_data_path / "reference_gerbers" / ref_gerber

        if not reference_path.exists():
            return LayerResult(
                layer_name=self.config["layer_mapping"][ref_gerber],
                reference_file=str( reference_path ),
                generated_file=str( generated_gerber ),
                diff_percentage=100.0,
                passed=False
            )

        diff_output = self.output_dir / "diffs" / f"{ref_gerber}_diff.png"
        diff_output.parent.mkdir( parents=True, exist_ok=True )

        # First, get JSON output for statistics
        command_json = [
            utils.kicad_cli(),
            "gerber", "diff",
            "--format", "json",
            str( reference_path ),
            str( generated_gerber )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command_json )

        diff_percentage = 100.0

        # Return code 1 means files are different, 0 means identical
        if return_code in ( 0, 1 ):
            json_start = stdout.find( "{" )
            json_end = stdout.rfind( "}" ) + 1

            if json_start >= 0 and json_end > json_start:
                try:
                    diff_result = json.loads( stdout[json_start:json_end] )
                    diff_percentage = diff_result.get( "total_diff_percent", 100.0 )
                except json.JSONDecodeError:
                    pass

        threshold = self.config.get( "pass_threshold", 5.0 )
        passed = diff_percentage <= threshold

        # Generate PNG diff image if there are differences and we need it
        if should_save_all_diffs() or not passed:
            command_png = [
                utils.kicad_cli(),
                "gerber", "diff",
                "--format", "png",
                "-o", str( diff_output ),
                str( reference_path ),
                str( generated_gerber )
            ]

            utils.run_and_capture( command_png )

        return LayerResult(
            layer_name=self.config["layer_mapping"][ref_gerber],
            reference_file=str( reference_path ),
            generated_file=str( generated_gerber ),
            diff_percentage=diff_percentage,
            passed=passed,
            diff_image=str( diff_output ) if diff_output.exists() else None
        )

    def export_drill( self, board_path: Path ) -> Optional[Path]:
        """Export drill files from the imported board."""
        drill_output_dir = self.output_dir / "drill"
        drill_output_dir.mkdir( parents=True, exist_ok=True )

        command = [
            utils.kicad_cli(),
            "pcb", "export", "drill",
            "--format", "excellon",
            "-o", str( drill_output_dir ) + "/",
            str( board_path )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command )

        # Look for the generated drill file
        expected_file = drill_output_dir / ( board_path.stem + ".drl" )

        if expected_file.exists():
            return expected_file

        # Try PTH specific file
        pth_file = drill_output_dir / ( board_path.stem + "-PTH.drl" )

        if pth_file.exists():
            return pth_file

        return None

    def compare_drill( self, ref_drill: str, generated_drill: Path ) -> DrillResult:
        """Compare a generated drill file against reference."""
        reference_path = self.test_data_path / "reference_gerbers" / ref_drill

        if not reference_path.exists():
            return DrillResult(
                reference_file=str( reference_path ),
                generated_file=str( generated_drill ),
                diff_percentage=100.0,
                passed=False
            )

        diff_output = self.output_dir / "diffs" / f"{ref_drill}_diff.png"
        diff_output.parent.mkdir( parents=True, exist_ok=True )

        # Get JSON output for statistics
        command_json = [
            utils.kicad_cli(),
            "gerber", "diff",
            "--format", "json",
            str( reference_path ),
            str( generated_drill )
        ]

        stdout, stderr, return_code = utils.run_and_capture( command_json )

        diff_percentage = 100.0
        hole_count_ref = 0
        hole_count_gen = 0

        # Return code 1 means files are different, 0 means identical
        if return_code in ( 0, 1 ):
            json_start = stdout.find( "{" )
            json_end = stdout.rfind( "}" ) + 1

            if json_start >= 0 and json_end > json_start:
                try:
                    diff_result = json.loads( stdout[json_start:json_end] )
                    diff_percentage = diff_result.get( "total_diff_percent", 100.0 )
                except json.JSONDecodeError:
                    pass

        threshold = self.config.get( "pass_threshold", 5.0 )
        passed = diff_percentage <= threshold

        # Generate PNG diff image if there are differences and we need it
        if should_save_all_diffs() or not passed:
            command_png = [
                utils.kicad_cli(),
                "gerber", "diff",
                "--format", "png",
                "-o", str( diff_output ),
                str( reference_path ),
                str( generated_drill )
            ]

            utils.run_and_capture( command_png )

        return DrillResult(
            reference_file=str( reference_path ),
            generated_file=str( generated_drill ),
            diff_percentage=diff_percentage,
            passed=passed,
            hole_count_reference=hole_count_ref,
            hole_count_generated=hole_count_gen,
            diff_image=str( diff_output ) if diff_output.exists() else None
        )

    def run_test( self ) -> TestCaseResult:
        """Run the complete import and comparison test."""
        result = TestCaseResult( name=self.test_case, passed=False )

        board_path, import_report, return_code = self.import_pads_file()
        result.import_report = import_report

        if return_code != 0 or not board_path.exists():
            result.error = f"Import failed with return code {return_code}"
            return result

        exported_gerbers = self.export_gerbers( board_path )

        if not exported_gerbers:
            result.error = "No gerbers were exported"
            return result

        all_passed = True

        for ref_gerber, generated_gerber in exported_gerbers.items():
            layer_result = self.compare_layer( ref_gerber, generated_gerber )
            result.layer_results.append( layer_result )

            if not layer_result.passed:
                all_passed = False

        # Compare drill files if configured
        drill_files = self.config.get( "drill_files", [] )

        if drill_files:
            generated_drill = self.export_drill( board_path )

            if generated_drill:
                for ref_drill in drill_files:
                    drill_result = self.compare_drill( ref_drill, generated_drill )
                    result.drill_results.append( drill_result )

                    if not drill_result.passed:
                        all_passed = False

        result.passed = all_passed
        return result

    def save_results( self, result: TestCaseResult ) -> Path:
        """Save test results to JSON file."""
        results_path = self.output_dir / "results.json"

        results_dict = {
            "name": result.name,
            "passed": result.passed,
            "error": result.error,
            "import_report": result.import_report,
            "layer_results": [
                {
                    "layer_name": lr.layer_name,
                    "reference_file": lr.reference_file,
                    "generated_file": lr.generated_file,
                    "diff_percentage": lr.diff_percentage,
                    "passed": lr.passed,
                    "diff_image": lr.diff_image
                }
                for lr in result.layer_results
            ],
            "drill_results": [
                {
                    "reference_file": dr.reference_file,
                    "generated_file": dr.generated_file,
                    "diff_percentage": dr.diff_percentage,
                    "passed": dr.passed,
                    "hole_count_reference": dr.hole_count_reference,
                    "hole_count_generated": dr.hole_count_generated,
                    "diff_image": dr.diff_image
                }
                for dr in result.drill_results
            ]
        }

        with open( results_path, "w" ) as f:
            json.dump( results_dict, f, indent=2 )

        return results_path


# Get test cases at module load time
PADS_TEST_CASES = get_pads_test_cases()


@pytest.mark.skipif( len( PADS_TEST_CASES ) == 0,
                     reason="No PADS test data available" )
class TestPadsImporter:
    """Parametrized tests for PADS importer"""

    @pytest.mark.parametrize( "test_case", PADS_TEST_CASES )
    def test_pads_import_and_compare( self, kitest: KiTestFixture, test_case: str ):
        """Test importing a PADS file and comparing gerbers and drill files."""
        helper = PadsImporterHelper( kitest, test_case )
        result = helper.run_test()
        results_path = helper.save_results( result )

        if result.error:
            pytest.fail( f"Test failed: {result.error}" )

        # Attach diff images for failed layers
        for layer_result in result.layer_results:
            if not layer_result.passed:
                if layer_result.diff_image:
                    kitest.add_attachment( layer_result.diff_image )

        # Attach diff images for failed drill comparisons
        for drill_result in result.drill_results:
            if not drill_result.passed:
                if drill_result.diff_image:
                    kitest.add_attachment( drill_result.diff_image )

        failed_layers = [lr for lr in result.layer_results if not lr.passed]
        failed_drills = [dr for dr in result.drill_results if not dr.passed]

        if failed_layers or failed_drills:
            msg = f"Comparison failures for {test_case}:\n"

            if failed_layers:
                msg += "  Layer failures:\n"

                for lr in failed_layers:
                    msg += f"    {lr.layer_name}: {lr.diff_percentage:.2f}% diff (threshold: {helper.config.get('pass_threshold', 5.0)}%)\n"

            if failed_drills:
                msg += "  Drill failures:\n"

                for dr in failed_drills:
                    ref_name = Path( dr.reference_file ).name
                    msg += f"    {ref_name}: {dr.diff_percentage:.2f}% diff (threshold: {helper.config.get('pass_threshold', 5.0)}%)\n"

            pytest.fail( msg )

    @pytest.mark.parametrize( "test_case", PADS_TEST_CASES )
    def test_pads_import_only( self, kitest: KiTestFixture, test_case: str ):
        """Test that PADS import completes without error."""
        helper = PadsImporterHelper( kitest, test_case )
        board_path, import_report, return_code = helper.import_pads_file()

        assert return_code == 0, f"Import failed with return code {return_code}"
        assert board_path.exists(), f"Output file not created: {board_path}"
        assert import_report is not None, "Import report not generated"
        assert "statistics" in import_report, "Import report missing statistics"


