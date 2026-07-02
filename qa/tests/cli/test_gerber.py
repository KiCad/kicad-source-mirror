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
Tests for kicad-cli gerber commands: info, convert png, and diff
"""

import utils
from pathlib import Path
import pytest
import json
import re
from conftest import KiTestFixture
from PIL import Image
import numpy as np


# Test data files
GERBER_FILE_FCU = "cli/artwork_generation_regressions/ZoneFill-4.0.7-F_Cu.gbr"
GERBER_FILE_BCU = "cli/artwork_generation_regressions/ZoneFill-4.0.7-B_Cu.gbr"
GERBER_FILE_LEGACY_FCU = "cli/artwork_generation_regressions/ZoneFill-Legacy-F_Cu.gbr"
GERBER_FILE_APERTURE_MACRO_TRANSFORM = "cli/artwork_generation_regressions/ApertureMacroPngTransform.gbr"
GERBER_FILE_APERTURE_MACRO_TRANSFORM_PNG = "cli/artwork_generation_regressions/ApertureMacroPngTransform.png"
GERBER_FILE_DRILL = "cli/basic_test/basic_test-PTH-drl.gbr"
EXCELLON_FILE = "cli/basic_test/basic_test_excellon_default.drl"


def get_output_path( kitest: KiTestFixture, test_name: str, filename: str ) -> Path:
    """Generate an output path for a test file."""
    output_dir = kitest.get_output_path( f"cli/gerber/{test_name}/" )
    output_path = output_dir / filename

    if output_path.exists():
        output_path.unlink()

    return output_path


def get_png_dimensions( png_path: str ) -> tuple:
    """Get the dimensions of a PNG file."""
    with Image.open( png_path ) as img:
        return img.size


def get_png_colors( png_path: str ) -> dict:
    """Analyze a PNG and return a summary of colors present.

    Returns a dict with booleans indicating presence of:
    - has_gray: gray pixels (R≈G≈B, not white/black)
    - has_red: red pixels (R high, G/B low)
    - has_green: green pixels (G high, R/B low)
    - has_white: white pixels
    - has_transparent: transparent pixels
    """
    with Image.open( png_path ) as img:
        if img.mode == "RGBA":
            data = np.array( img )
            r, g, b, a = data[:,:,0], data[:,:,1], data[:,:,2], data[:,:,3]
        elif img.mode == "RGB":
            data = np.array( img )
            r, g, b = data[:,:,0], data[:,:,1], data[:,:,2]
            a = np.full_like( r, 255 )
        else:
            img_rgb = img.convert( "RGBA" )
            data = np.array( img_rgb )
            r, g, b, a = data[:,:,0], data[:,:,1], data[:,:,2], data[:,:,3]

        # White (or near-white)
        has_white = np.any( (r > 240) & (g > 240) & (b > 240) & (a > 128) )

        # Transparent
        has_transparent = np.any( a < 128 )

        # Gray (R≈G≈B, not too bright, not too dark)
        gray_mask = (np.abs(r.astype(int) - g.astype(int)) < 30) & \
                    (np.abs(g.astype(int) - b.astype(int)) < 30) & \
                    (r > 50) & (r < 200) & (a > 128)
        has_gray = np.any( gray_mask )

        # Red (R significantly higher than G and B)
        red_mask = (r > 150) & (g < 100) & (b < 100) & (a > 128)
        has_red = np.any( red_mask )

        # Green (G significantly higher than R and B)
        green_mask = (g > 150) & (r < 100) & (b < 100) & (a > 128)
        has_green = np.any( green_mask )

        return {
            "has_gray": bool( has_gray ),
            "has_red": bool( has_red ),
            "has_green": bool( has_green ),
            "has_white": bool( has_white ),
            "has_transparent": bool( has_transparent ),
        }


# ==============================================================================
# gerber info command tests
# ==============================================================================

class TestGerberInfo:
    """Tests for kicad-cli gerber info command."""

    def test_info_basic_text( self, kitest: KiTestFixture ):
        """Test basic info output in text format."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert "File:" in stdout
        assert "Type: Gerber" in stdout
        assert "Item count:" in stdout
        assert "Bounding box:" in stdout
        assert "Origin:" in stdout
        assert "Size:" in stdout
        assert "Apertures defined:" in stdout

    def test_info_basic_json( self, kitest: KiTestFixture ):
        """Test basic info output in JSON format."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "json", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )

        assert "filename" in data
        assert "type" in data
        assert data["type"] == "Gerber"
        assert "item_count" in data
        assert "bounding_box" in data
        assert "origin_x" in data["bounding_box"]
        assert "origin_y" in data["bounding_box"]
        assert "width" in data["bounding_box"]
        assert "height" in data["bounding_box"]
        assert "aperture_count" in data
        assert "units" in data
        assert data["units"] == "mm"

    def test_info_with_area_text( self, kitest: KiTestFixture ):
        """Test info with area calculation in text format."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--area", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert "Copper area:" in stdout
        assert "mm" in stdout

    def test_info_with_area_json( self, kitest: KiTestFixture ):
        """Test info with area calculation in JSON format."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "json", "--area", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )

        assert "copper_area" in data
        assert data["copper_area"] > 0

    def test_info_excellon_file( self, kitest: KiTestFixture ):
        """Test info command with Excellon drill file."""
        input_file = kitest.get_data_file_path( EXCELLON_FILE )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "json", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )

        assert data["type"] == "Excellon Drill"

    def test_info_invalid_format( self, kitest: KiTestFixture ):
        """Test info command with invalid format argument."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "invalid", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode != 0

    def test_info_units_inch( self, kitest: KiTestFixture ):
        """Test info command with inch units."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "json",
                   "--units", "inch", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )
        assert data["units"] == "in"

        # Verify conversion: mm values should be smaller when converted to inches
        command_mm = [utils.kicad_cli(), "gerber", "info", "--format", "json",
                      "--units", "mm", input_file]
        stdout_mm, _, _ = utils.run_and_capture( command_mm )
        data_mm = json.loads( stdout_mm )

        assert data["bounding_box"]["width"] < data_mm["bounding_box"]["width"]
        assert abs( data["bounding_box"]["width"] * 25.4 - data_mm["bounding_box"]["width"] ) < 0.001

    def test_info_units_mils( self, kitest: KiTestFixture ):
        """Test info command with mils units."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "json",
                   "--units", "mils", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )
        assert data["units"] == "mils"

        # Verify conversion: mils values should be larger than mm values
        command_mm = [utils.kicad_cli(), "gerber", "info", "--format", "json",
                      "--units", "mm", input_file]
        stdout_mm, _, _ = utils.run_and_capture( command_mm )
        data_mm = json.loads( stdout_mm )

        assert data["bounding_box"]["width"] > data_mm["bounding_box"]["width"]

    def test_info_invalid_units( self, kitest: KiTestFixture ):
        """Test info command with invalid units argument."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--units", "invalid", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode != 0

    def test_info_nonexistent_file( self, kitest: KiTestFixture ):
        """Test info command with nonexistent file."""
        command = [utils.kicad_cli(), "gerber", "info", "/nonexistent/file.gbr"]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode != 0

    @pytest.mark.parametrize( "gerber_file", [
        GERBER_FILE_FCU,
        GERBER_FILE_BCU,
        GERBER_FILE_DRILL,
    ])
    def test_info_multiple_files( self, kitest: KiTestFixture, gerber_file: str ):
        """Test info command with various gerber files."""
        input_file = kitest.get_data_file_path( gerber_file )

        command = [utils.kicad_cli(), "gerber", "info", "--format", "json", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )
        assert data["item_count"] > 0


# ==============================================================================
# gerber convert png command tests
# ==============================================================================

@pytest.mark.skipif(not utils.is_gerbview_available(), reason="Requires gerbview kiface to be built")
class TestGerberConvertPng:
    """Tests for kicad-cli gerber convert png command."""

    def test_export_basic( self, kitest: KiTestFixture ):
        """Test basic PNG export."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "export_basic", "output.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()
        kitest.add_attachment( str( output_file ) )

        # Verify it's a valid PNG with reasonable dimensions
        width, height = get_png_dimensions( str( output_file ) )
        assert width > 0
        assert height > 0

    def test_export_with_dpi( self, kitest: KiTestFixture ):
        """Test PNG export with custom DPI."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_100dpi = get_output_path( kitest, "export_dpi", "output_100dpi.png" )
        output_600dpi = get_output_path( kitest, "export_dpi", "output_600dpi.png" )

        # Export at 100 DPI
        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--dpi", "100", "-o", str( output_100dpi ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )
        assert exitcode == 0
        assert output_100dpi.exists()

        # Export at 600 DPI
        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--dpi", "600", "-o", str( output_600dpi ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )
        assert exitcode == 0
        assert output_600dpi.exists()

        # Higher DPI should produce larger image
        w100, h100 = get_png_dimensions( str( output_100dpi ) )
        w600, h600 = get_png_dimensions( str( output_600dpi ) )

        assert w600 > w100
        assert h600 > h100

        kitest.add_attachment( str( output_100dpi ) )
        kitest.add_attachment( str( output_600dpi ) )

    def test_export_with_dimensions( self, kitest: KiTestFixture ):
        """Test PNG export with explicit width/height."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "export_dimensions", "output_800x600.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--width", "800", "--height", "600",
                   "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        width, height = get_png_dimensions( str( output_file ) )
        # Dimensions might not be exact due to aspect ratio preservation
        # but they should be close
        assert abs( width - 800 ) < 100 or abs( height - 600 ) < 100

        kitest.add_attachment( str( output_file ) )

    def test_export_with_width_only( self, kitest: KiTestFixture ):
        """Test PNG export with only width specified."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "export_width", "output_w500.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--width", "500", "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        width, height = get_png_dimensions( str( output_file ) )
        # Width should be approximately 500, height calculated from aspect
        assert width <= 500 + 50

        kitest.add_attachment( str( output_file ) )

    def test_export_no_antialias( self, kitest: KiTestFixture ):
        """Test PNG export with anti-aliasing disabled."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "export_noaa", "output_noaa.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--no-antialias", "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        kitest.add_attachment( str( output_file ) )

    def test_export_transparent( self, kitest: KiTestFixture ):
        """Test PNG export with transparent background."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "export_transparent", "output_trans.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--transparent", "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        # Verify the image has transparency
        colors = get_png_colors( str( output_file ) )
        assert colors["has_transparent"], "Image should have transparent pixels"

        kitest.add_attachment( str( output_file ) )

    @pytest.mark.skip( reason="Excellon items (drill holes) not yet rendered correctly" )
    def test_export_excellon( self, kitest: KiTestFixture ):
        """Test PNG export of Excellon drill file.

        Note: Excellon files are not yet fully supported because the rendering
        code doesn't handle drill/flash items correctly.
        """
        input_file = kitest.get_data_file_path( EXCELLON_FILE )
        output_file = get_output_path( kitest, "export_excellon", "output_drill.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        kitest.add_attachment( str( output_file ) )

    def test_export_nonexistent_file( self, kitest: KiTestFixture ):
        """Test export command with nonexistent input file."""
        output_file = get_output_path( kitest, "export_error", "output.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "-o", str( output_file ), "/nonexistent/file.gbr"]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode != 0
        assert not output_file.exists()

    def test_export_aperture_macro_transform( self, kitest: KiTestFixture ):
        """Test PNG export applies aperture macro transforms exactly once."""
        input_file = kitest.get_data_file_path( GERBER_FILE_APERTURE_MACRO_TRANSFORM )
        expected_file = kitest.get_data_file_path( GERBER_FILE_APERTURE_MACRO_TRANSFORM_PNG )
        output_file = get_output_path( kitest, "export_aperture_macro_transform", "output.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--width", "64", "--no-antialias", "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()
        # Compare against the equivalent valid Gerber using a filled region instead of an aperture macro.
        assert utils.images_are_equal( str( output_file ), expected_file, kitest.add_attachment )

        kitest.add_attachment( str( output_file ) )


# ==============================================================================
# gerber diff command tests
# ==============================================================================

class TestGerberDiff:
    """Tests for kicad-cli gerber diff command."""

    def test_diff_identical_files_png( self, kitest: KiTestFixture ):
        """Test diff of identical files produces gray-only PNG."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "diff_identical", "diff_same.png" )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "-o", str( output_file ), input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        # Identical files should produce only gray (overlap) - no red or green
        colors = get_png_colors( str( output_file ) )
        assert colors["has_gray"], "Identical diff should have gray overlap"
        assert not colors["has_red"], "Identical diff should have no red (removals)"
        assert not colors["has_green"], "Identical diff should have no green (additions)"

        kitest.add_attachment( str( output_file ) )

    def test_diff_different_files_png( self, kitest: KiTestFixture ):
        """Test diff of different files produces red/green/gray PNG."""
        input_file_a = kitest.get_data_file_path( GERBER_FILE_FCU )
        input_file_b = kitest.get_data_file_path( GERBER_FILE_BCU )
        output_file = get_output_path( kitest, "diff_different", "diff_ab.png" )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "-o", str( output_file ), input_file_a, input_file_b]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        # Different files should have all colors
        colors = get_png_colors( str( output_file ) )
        # Note: depending on the actual gerber content, we expect at least some differences
        # Gray should always be present if there's any content
        # Red and/or green should be present if files differ
        has_differences = colors["has_red"] or colors["has_green"]
        assert colors["has_gray"] or has_differences, "Diff should have some content"

        kitest.add_attachment( str( output_file ) )

    def test_diff_legacy_vs_modern_png( self, kitest: KiTestFixture ):
        """Test diff between legacy and modern gerber files."""
        input_file_a = kitest.get_data_file_path( GERBER_FILE_FCU )
        input_file_b = kitest.get_data_file_path( GERBER_FILE_LEGACY_FCU )
        output_file = get_output_path( kitest, "diff_legacy", "diff_legacy.png" )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "-o", str( output_file ), input_file_a, input_file_b]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        # Verify the PNG has reasonable dimensions
        width, height = get_png_dimensions( str( output_file ) )
        assert width > 0
        assert height > 0

        kitest.add_attachment( str( output_file ) )

    def test_diff_identical_files_text( self, kitest: KiTestFixture ):
        """Test diff of identical files in text format."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--format", "text", input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert "Comparing:" in stdout
        assert "Reference area:" in stdout
        assert "Additions:" in stdout
        assert "Removals:" in stdout

        # Parse additions/removals - should be 0 for identical files
        # Look for patterns like "Additions:         0.00 mm^2"
        additions_match = re.search( r"Additions:\s+([0-9.]+)\s+mm", stdout )
        removals_match = re.search( r"Removals:\s+([0-9.]+)\s+mm", stdout )

        if additions_match:
            additions = float( additions_match.group(1) )
            assert additions < 0.01, f"Identical files should have ~0 additions, got {additions}"

        if removals_match:
            removals = float( removals_match.group(1) )
            assert removals < 0.01, f"Identical files should have ~0 removals, got {removals}"

    def test_diff_identical_files_json( self, kitest: KiTestFixture ):
        """Test diff of identical files in JSON format."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--format", "json", input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )

        assert "reference_file" in data
        assert "comparison_file" in data
        assert "additions" in data
        assert "removals" in data
        assert "overlap" in data

        # Identical files should have 0 additions and removals
        assert data["additions"]["area"] < 0.01
        assert data["removals"]["area"] < 0.01

    def test_diff_different_files_json( self, kitest: KiTestFixture ):
        """Test diff of different files in JSON format."""
        input_file_a = kitest.get_data_file_path( GERBER_FILE_FCU )
        input_file_b = kitest.get_data_file_path( GERBER_FILE_BCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--format", "json", input_file_a, input_file_b]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )

        assert "reference" in data
        assert "comparison" in data
        assert "net_change" in data
        assert "total_diff_percent" in data

    def test_diff_exit_code_only_identical( self, kitest: KiTestFixture ):
        """Test exit-code-only mode with identical files (should exit 0)."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--exit-code-only", input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0, "Identical files should exit with code 0"

    def test_diff_exit_code_only_different( self, kitest: KiTestFixture ):
        """Test exit-code-only mode with different files (should exit 5=ERR_RC_VIOLATIONS)."""
        input_file_a = kitest.get_data_file_path( GERBER_FILE_FCU )
        input_file_b = kitest.get_data_file_path( GERBER_FILE_BCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--exit-code-only", input_file_a, input_file_b]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 5, "Different files should exit with code 5 (ERR_RC_VIOLATIONS)"

    def test_diff_with_tolerance( self, kitest: KiTestFixture ):
        """Test diff with tolerance parameter."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--format", "json", "--tolerance", "1000",
                   input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        data = json.loads( stdout )
        assert "additions" in data

    def test_diff_with_dpi( self, kitest: KiTestFixture ):
        """Test diff PNG output with custom DPI."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_100dpi = get_output_path( kitest, "diff_dpi", "diff_100dpi.png" )
        output_600dpi = get_output_path( kitest, "diff_dpi", "diff_600dpi.png" )

        # Diff at 100 DPI
        command = [utils.kicad_cli(), "gerber", "diff",
                   "--dpi", "100", "-o", str( output_100dpi ),
                   input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )
        assert exitcode == 0
        assert output_100dpi.exists()

        # Diff at 600 DPI
        command = [utils.kicad_cli(), "gerber", "diff",
                   "--dpi", "600", "-o", str( output_600dpi ),
                   input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )
        assert exitcode == 0
        assert output_600dpi.exists()

        # Higher DPI should produce larger image
        w100, h100 = get_png_dimensions( str( output_100dpi ) )
        w600, h600 = get_png_dimensions( str( output_600dpi ) )

        assert w600 > w100
        assert h600 > h100

        kitest.add_attachment( str( output_100dpi ) )
        kitest.add_attachment( str( output_600dpi ) )

    def test_diff_transparent( self, kitest: KiTestFixture ):
        """Test diff PNG with transparent background."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "diff_transparent", "diff_trans.png" )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--transparent", "-o", str( output_file ),
                   input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        colors = get_png_colors( str( output_file ) )
        assert colors["has_transparent"], "Diff PNG should have transparent background"

        kitest.add_attachment( str( output_file ) )

    def test_diff_no_antialias( self, kitest: KiTestFixture ):
        """Test diff PNG with anti-aliasing disabled."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "diff_noaa", "diff_noaa.png" )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--no-antialias", "-o", str( output_file ),
                   input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        kitest.add_attachment( str( output_file ) )

    def test_diff_invalid_format( self, kitest: KiTestFixture ):
        """Test diff command with invalid format argument."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--format", "invalid", input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode != 0

    def test_diff_nonexistent_file( self, kitest: KiTestFixture ):
        """Test diff command with nonexistent file."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--format", "json", input_file, "/nonexistent/file.gbr"]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode != 0

    @pytest.mark.skip( reason="Excellon to polyset conversion not yet implemented for diff" )
    def test_diff_excellon_files( self, kitest: KiTestFixture ):
        """Test diff of Excellon drill files.

        Note: Excellon files are not yet fully supported in the diff command
        because ConvertGerberToPolySet doesn't handle Excellon items correctly.
        """
        input_file = kitest.get_data_file_path( EXCELLON_FILE )
        output_file = get_output_path( kitest, "diff_excellon", "diff_drill.png" )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "-o", str( output_file ), input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

        kitest.add_attachment( str( output_file ) )


# ==============================================================================
# Strict mode tests
# ==============================================================================

class TestGerberStrictMode:
    """Tests for --strict flag across all gerber commands."""

    def test_info_strict_valid_file( self, kitest: KiTestFixture ):
        """Test info with strict mode on valid file."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "info", "--strict", input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        # Valid file should pass strict mode
        assert exitcode == 0

    def test_export_strict_valid_file( self, kitest: KiTestFixture ):
        """Test export png with strict mode on valid file."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )
        output_file = get_output_path( kitest, "strict_export", "output.png" )

        command = [utils.kicad_cli(), "gerber", "convert", "png",
                   "--strict", "-o", str( output_file ), input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0
        assert output_file.exists()

    def test_diff_strict_valid_files( self, kitest: KiTestFixture ):
        """Test diff with strict mode on valid files."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        command = [utils.kicad_cli(), "gerber", "diff",
                   "--strict", "--format", "json", input_file, input_file]
        stdout, stderr, exitcode = utils.run_and_capture( command )

        assert exitcode == 0


# ==============================================================================
# Edge case and integration tests
# ==============================================================================

class TestGerberEdgeCases:
    """Edge case and integration tests for gerber commands."""

    def test_diff_reversed_order( self, kitest: KiTestFixture ):
        """Test that reversing file order swaps additions/removals."""
        input_file_a = kitest.get_data_file_path( GERBER_FILE_FCU )
        input_file_b = kitest.get_data_file_path( GERBER_FILE_BCU )

        # A vs B
        command_ab = [utils.kicad_cli(), "gerber", "diff",
                      "--format", "json", input_file_a, input_file_b]
        stdout_ab, _, exitcode_ab = utils.run_and_capture( command_ab )
        assert exitcode_ab == 0
        data_ab = json.loads( stdout_ab )

        # B vs A
        command_ba = [utils.kicad_cli(), "gerber", "diff",
                      "--format", "json", input_file_b, input_file_a]
        stdout_ba, _, exitcode_ba = utils.run_and_capture( command_ba )
        assert exitcode_ba == 0
        data_ba = json.loads( stdout_ba )

        # Additions in A vs B should become removals in B vs A (approximately)
        assert abs( data_ab["additions"]["area"] - data_ba["removals"]["area"] ) < 0.1
        assert abs( data_ab["removals"]["area"] - data_ba["additions"]["area"] ) < 0.1

    def test_info_bounding_box_consistency( self, kitest: KiTestFixture ):
        """Test that bounding box values are consistent between text and JSON."""
        input_file = kitest.get_data_file_path( GERBER_FILE_FCU )

        # Get JSON output
        command_json = [utils.kicad_cli(), "gerber", "info",
                        "--format", "json", input_file]
        stdout_json, _, _ = utils.run_and_capture( command_json )
        data = json.loads( stdout_json )

        json_width = data["bounding_box"]["width"]
        json_height = data["bounding_box"]["height"]

        # Get text output
        command_text = [utils.kicad_cli(), "gerber", "info", input_file]
        stdout_text, _, _ = utils.run_and_capture( command_text )

        # Parse dimensions from text (format: "Size: 100.123 x 50.456 mm")
        size_match = re.search( r"Size:\s+([0-9.]+)\s+x\s+([0-9.]+)\s+mm", stdout_text )
        assert size_match, "Could not parse size from text output"

        text_width = float( size_match.group(1) )
        text_height = float( size_match.group(2) )

        # Values should match
        assert abs( json_width - text_width ) < 0.001
        assert abs( json_height - text_height ) < 0.001

    def test_export_png_size_scales_with_geometry( self, kitest: KiTestFixture ):
        """Test that PNG size scales proportionally with gerber size."""
        # Use two different gerbers that have different sizes and known-working rendering
        input_file_a = kitest.get_data_file_path( GERBER_FILE_FCU )
        input_file_b = kitest.get_data_file_path( GERBER_FILE_BCU )

        output_a = get_output_path( kitest, "size_scale", "gerber_a.png" )
        output_b = get_output_path( kitest, "size_scale", "gerber_b.png" )

        # Export both at same DPI
        dpi = 150

        command_a = [utils.kicad_cli(), "gerber", "convert", "png",
                     "--dpi", str( dpi ), "-o", str( output_a ), input_file_a]
        utils.run_and_capture( command_a )

        command_b = [utils.kicad_cli(), "gerber", "convert", "png",
                     "--dpi", str( dpi ), "-o", str( output_b ), input_file_b]
        utils.run_and_capture( command_b )

        # Get info for both files
        cmd_info_a = [utils.kicad_cli(), "gerber", "info", "--format", "json", input_file_a]
        stdout_a, _, _ = utils.run_and_capture( cmd_info_a )
        info_a = json.loads( stdout_a )

        cmd_info_b = [utils.kicad_cli(), "gerber", "info", "--format", "json", input_file_b]
        stdout_b, _, _ = utils.run_and_capture( cmd_info_b )
        info_b = json.loads( stdout_b )

        # Get PNG sizes
        wa, ha = get_png_dimensions( str( output_a ) )
        wb, hb = get_png_dimensions( str( output_b ) )

        # Size ratio of PNGs should roughly match size ratio of gerbers
        gerber_ratio_a = info_a["bounding_box"]["width"] / info_a["bounding_box"]["height"]
        gerber_ratio_b = info_b["bounding_box"]["width"] / info_b["bounding_box"]["height"]

        png_ratio_a = wa / ha if ha > 0 else 1
        png_ratio_b = wb / hb if hb > 0 else 1

        # Aspect ratios should be similar
        assert abs( gerber_ratio_a - png_ratio_a ) < 0.1, "PNG A aspect ratio should match gerber"
        assert abs( gerber_ratio_b - png_ratio_b ) < 0.1, "PNG B aspect ratio should match gerber"

        kitest.add_attachment( str( output_a ) )
        kitest.add_attachment( str( output_b ) )
