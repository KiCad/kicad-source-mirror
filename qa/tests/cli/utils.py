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

import difflib
import os
import pathlib
import json

import cairosvg
import logging
import subprocess
from typing import Tuple, Optional, Callable
from pathlib import Path
from PIL import Image, ImageChops, ImageFilter, ImageEnhance
import numpy as np

logger = logging.getLogger("cli_util")
Image.MAX_IMAGE_PIXELS = 800 * 1024 * 1024 // 4 # Increase limit to ~800MB uncompressed RGBA, 4bpp (~600MB RGB, 3bpp)

def kicad_cli() -> str:
    if 'KICAD_CLI' in os.environ:
        return os.environ.get('KICAD_CLI')

    return "kicad-cli"


def _get_kicad_major_version() -> int:
    """Get the major version from kicad-cli so library env vars match the running binary."""
    try:
        proc = subprocess.run( [kicad_cli(), 'version', '--format', 'plain'],
                               capture_output=True, text=True, timeout=10 )

        if proc.returncode == 0 and proc.stdout.strip():
            major = proc.stdout.strip().split( '.' )[0]
            return int( major )
    except Exception:
        pass

    logger.warning( "Could not determine kicad-cli version, defaulting to 10" )
    return 10


_kicad_major_version = None

def _get_cached_kicad_major_version() -> int:
    global _kicad_major_version

    if _kicad_major_version is None:
        _kicad_major_version = _get_kicad_major_version()

    return _kicad_major_version


def run_and_capture( command: list[str], cwd: Optional[Path] = None ) -> Tuple[ str, str, int ]:
    command = [str( c ) for c in command]
    logger.info("Executing command \"%s\"", " ".join( command ))

    env = {}
    env.update(os.environ)

    if 'KICAD_CONFIG_HOME' not in env:
        if 'QA_DATA_ROOT' in env:
            base_path = Path( env.get('QA_DATA_ROOT') )
        else:
            # Derive the QA data path from the directory the tests run from. This is a
            # separate concept from the subprocess working directory in `cwd`; do not
            # reuse that name here or it would clobber the caller's requested cwd.
            test_cwd = Path.cwd()
            base_path = None

            try:
                if 'qa' in test_cwd.parts:
                    idx = test_cwd.parts.index('qa')
                    base_path = test_cwd.parents[len(test_cwd.parents) - idx - 1] / 'data'
            except ValueError:
                pass

        if base_path is not None:
            logger.info("Using QA data base path '%s'", str(base_path))
            ver = _get_cached_kicad_major_version()
            env['KICAD_CONFIG_HOME'] = str(base_path / 'config')
            env[f'KICAD{ver}_SYMBOL_DIR'] = str(base_path / 'libraries')
            env[f'KICAD{ver}_FOOTPRINT_DIR'] = str(base_path / 'libraries')
        else:
            logger.warning("Unexpected cwd '%s', tests will likely fail", Path.cwd())

    proc = subprocess.Popen( command,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        encoding = 'utf-8',
        env = env,
        cwd = str( cwd ) if cwd is not None else None
    )

    out,err = proc.communicate()

    if proc.returncode != 0 or len(err) != 0:
        logger.info(f"command returned {proc.returncode}")
        logger.info(f"stdout: {out}")
        logger.info(f"stderr: {err}")

    return out, err, proc.returncode


def textdiff_files( golden_filepath: Path, new_filepath: Path, skip: int = 0 ) -> bool:
    with open( golden_filepath, 'r' ) as f:
        golden_lines = f.readlines()[skip:]

    with open( new_filepath, 'r' ) as f:
        new_lines = f.readlines()[skip:]

    diff = difflib.unified_diff(
        golden_lines, new_lines, fromfile=str(golden_filepath), tofile=str(new_filepath)
    )
    diff_text = ''.join(list(diff))

    if diff_text != "":
        logger.info( "Text diff found:" )
        logger.info( diff_text )

    return diff_text == ""


def image_is_blank( image_path: str ) -> bool:
    img = Image.open( image_path )
    sum = np.sum( np.asarray( img ) )
    img.close()

    return sum == 0


def images_are_equal( image1_path: str, image2_path: str, diff_handler: Optional[Callable[[str], None]] = None,
                      erosion_pixels: int = 1 ) -> bool:
    # Note: if modifying this function - please add new tests for it in test_utils.py

    image1 = Image.open( image1_path )
    image2 = Image.open( image2_path )

    if image1.size != image2.size:
        logger.error("Images sizes are different.")
        return False

    if image1.mode != image2.mode:
        logger.error("Images modes are different.")
        return False

    diff = ImageChops.difference( image1, image2 )
    sum = np.sum( np.asarray( diff ) )
    retval = True

    if sum != 0.0:
        diff_multi_bands = diff.split()
        binary_multi_bands = []

        for band_diff in diff_multi_bands:
            thresholded_band = band_diff.point( lambda x: x > 0 and 255 )
            binary_multi_bands.append( thresholded_band.convert( "1" ) )

        binary_result = binary_multi_bands[0]

        for i in range( 1, len( binary_multi_bands ) ):
            binary_result = ImageChops.logical_or( binary_result, binary_multi_bands[i] )

        filter_size = 2 * erosion_pixels + 1
        eroded_result = binary_result.copy().filter( ImageFilter.MinFilter( filter_size ) )

        eroded_result_sum = np.sum( np.asarray( eroded_result ) )
        retval = eroded_result_sum == 0
        diff.close()
        binary_result.close()

        # Save diff
        if not retval:
            diff_name = image1.filename + ".DIFF_eroded_" + str( eroded_result_sum )+ ".png"
            red = Image.new( "RGB", image1.size, (255,0,0))
            imageEnhanced = ImageEnhance.Contrast(image1).enhance(0.3)
            imageEnhanced.paste( red,mask=eroded_result)
            imageEnhanced.save(diff_name)
            logger.error( "Images not equal. Diff stored at '%s'", diff_name )
            if diff_handler is not None:
                diff_handler( diff_name )
                diff_handler( image1.filename )
                diff_handler( image2.filename )
            imageEnhanced.close()

        # Cleanup
        eroded_result.close()

    # Cleanup
    image1.close()
    image2.close()

    return retval


def get_png_paths( generated_path: str, source_path : str, suffix : str = "" ) -> Tuple[str, str]:
    generated_stem = Path( generated_path ).stem
    generated_png_path = Path( generated_path ).with_stem( generated_stem + suffix).with_suffix( ".png" )

    if generated_png_path.exists():
        generated_png_path.unlink()  # Delete file

    # store source png in same folder as generated, easier to compare
    source_stem = Path( source_path ).stem
    source_png_path = Path( generated_path ).with_stem( source_stem + "-source" + suffix).with_suffix( ".png" )

    if source_png_path.exists():
        source_png_path.unlink()  # Delete file

    return str( generated_png_path ), str( source_png_path )


def svgs_are_equivalent( svg_generated_path: str, svg_source_path: str, comparison_dpi: int,
                         diff_handler: Optional[Callable[[str], None]] = None ) -> bool:
    png_generated, png_source = get_png_paths( svg_generated_path, svg_source_path )

    cairosvg.svg2png( url=svg_generated_path,
                      write_to=png_generated,
                      dpi=comparison_dpi )

    cairosvg.svg2png( url=svg_source_path,
                      write_to=png_source,
                      dpi=comparison_dpi )

    return images_are_equal( png_generated , png_source, diff_handler )


def gerbers_are_equivalent( gerber_generated_path: str, gerber_source_path: str,
                            diff_handler: Optional[Callable[[str], None]] = None,
                            max_diff_percent: float = 0.0 ) -> bool:

    stdout, stderr, exitcode = run_and_capture( [kicad_cli(), "gerber", "diff",
                                                 "--format", "json",
                                                 "--no-align",
                                                 gerber_generated_path, gerber_source_path] )

    if exitcode != 0:
        logger.error( "Gerber diff command failed (exit code %d): %s", exitcode, stderr )
        return False

    try:
        result = json.loads( stdout )
    except json.JSONDecodeError:
        logger.error( "Failed to parse gerber diff JSON output: %s", stdout[:500] )
        return False

    total_diff = result.get( 'total_diff_percent', 0.0 )
    additions_pct = result.get( 'additions', {} ).get( 'percent', 0 )
    removals_pct = result.get( 'removals', {} ).get( 'percent', 0 )

    if total_diff <= max_diff_percent:
        if total_diff > 0.0:
            logger.info( "Gerber diff within tolerance: total=%.4f%% (max=%.4f%%)",
                         total_diff, max_diff_percent )

        return True

    logger.error( "Gerber files differ: total=%.4f%% (max=%.4f%%), additions=%.4f%%, removals=%.4f%%",
                  total_diff, max_diff_percent, additions_pct, removals_pct )

    if diff_handler is not None:
        diff_png = gerber_generated_path + ".DIFF.png"
        run_and_capture( [kicad_cli(), "gerber", "diff",
                          "--format", "png",
                          "--no-align",
                          "-o", diff_png,
                          gerber_generated_path, gerber_source_path] )

        if Path( diff_png ).exists():
            diff_handler( diff_png )

    return False


def is_gerbview_available() -> bool:
    """Check if the gerbview kiface is built and loadable by kicad-cli."""
    try:
        stdout, stderr, exitcode = run_and_capture([kicad_cli(), "gerber", "info", "--help"])
    except Exception:
        return False

    return exitcode == 0


