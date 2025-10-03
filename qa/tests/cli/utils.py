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

def run_and_capture( command: list ) -> Tuple[ str, str, int ]:
    logger.info("Executing command \"%s\"", " ".join( command ))

    env = {}
    env.update(os.environ)

    if 'KICAD_CONFIG_HOME' not in env:
        if 'QA_DATA_ROOT' in env:
            base_path = env.get('QA_DATA_ROOT')
        else:
            cwd = Path.cwd()
            base_path = None

            try:
                if 'qa' in cwd.parts:
                    idx = cwd.parts.index('qa')
                    base_path = cwd.parents[len(cwd.parents) - idx - 1] / 'data'
            except ValueError:
                pass

        if base_path is not None:
            logger.info("Using QA data base path '%s'", str(base_path))
            env['KICAD_CONFIG_HOME'] = str(base_path / 'config')
            env['KICAD9_SYMBOL_DIR'] = str(base_path / 'libraries')
            env['KICAD9_FOOTPRINT_DIR'] = str(base_path / 'libraries')
        else:
            logger.warning("Unexpected cwd '%s', tests will likely fail", cwd)

    proc = subprocess.Popen( command,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        encoding = 'utf-8',
        env = env
    )

    out,err = proc.communicate()

    if proc.returncode != 0 or len(err) != 0:
        logger.info(f"command returned {proc.returncode}")
        logger.info(f"stdout: {out}")
        logger.info(f"stderr: {err}")

    return out, err, proc.returncode

def textdiff_files( golden_filepath: str, new_filepath: str, skip: int = 0 ) -> bool:
    status: bool = True

    with open( golden_filepath, 'r' ) as f:
        golden_lines = f.readlines()[skip:]

    with open( new_filepath, 'r' ) as f:
        new_lines = f.readlines()[skip:]

    diff = difflib.unified_diff( golden_lines, new_lines, fromfile = golden_filepath, tofile = new_filepath )
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


def images_are_equal( image1_path: str, image2_path: str, diff_handler: Optional[Callable[[str], None]] = None ) -> bool:
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
        # Images are not identical - lets allow 1 pixel error difference (for curved edges)
        diff_multi_bands = diff.split()
        binary_multi_bands = []

        for band_diff in diff_multi_bands:
            thresholded_band = band_diff.point( lambda x: x > 0 and 255 )
            binary_multi_bands.append( thresholded_band.convert( "1" ) )

        binary_result = binary_multi_bands[0]

        for i in range( 1, len( binary_multi_bands ) ):
            binary_result = ImageChops.logical_or( binary_result, binary_multi_bands[i] )

        eroded_result = binary_result.copy().filter( ImageFilter.MinFilter( 3 ) ) # erode once (trim 1 pixel)

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


def gerbers_are_equivalent( gerber_generated_path : str, gerber_source_path : str, comparison_dpi : int,
                            originInches :  Tuple[float, float],
                            windowsizeInches :  Tuple[float, float],
                            diff_handler: Optional[Callable[[str], None]] = None ) -> bool:

    # Calculate tiles required
    noTilesRowsCols = np.array( [1,1] )
    increaseRow = True
    tileSizeInches=np.array( windowsizeInches ) / noTilesRowsCols

    while( np.prod( tileSizeInches * comparison_dpi ) > Image.MAX_IMAGE_PIXELS // 2 ):
        if increaseRow:
            noTilesRowsCols[0]+=1
        else:
            noTilesRowsCols[1]+=1

        increaseRow=not increaseRow
        tileSizeInches=np.array( windowsizeInches ) / noTilesRowsCols


    gerberGeneratedIsBlank=True
    gerberSourceIsBlank=True
    gerbersAreEqual=True

    for row in range( noTilesRowsCols[0] ):
        for col in range( noTilesRowsCols[1] ):
            tileOrigin=np.array( originInches ) + ( np.array( [row,col] ) * tileSizeInches )
            tile_name=f"R{row}C{col}"
            png_generated, png_source = get_png_paths( gerber_generated_path, gerber_source_path, tile_name )

            convert_gerber_to_png( gerber_generated_path, png_generated, comparison_dpi, tileOrigin, tileSizeInches )
            convert_gerber_to_png( gerber_source_path,    png_source,    comparison_dpi, tileOrigin, tileSizeInches )

            gerberGeneratedIsBlank = gerberGeneratedIsBlank and image_is_blank( png_generated )
            gerberSourceIsBlank = gerberSourceIsBlank and image_is_blank( png_source )

            if( not images_are_equal( png_generated, png_source, diff_handler ) ):
                gerbersAreEqual = False

    assert( not gerberGeneratedIsBlank )
    assert( not gerberSourceIsBlank )    # make sure test case is generated correctly

    return gerbersAreEqual


def convert_gerber_to_png( gerber_path : str, png_path : str, dpi : int,
                           originInches :  Tuple[float, float],
                           windowsizeInches :  Tuple[float, float] ):

    originStr="{:.2f}".format(originInches[0]) + "x" + "{:.2f}".format(originInches[1])
    windowsizeInchesStr="{:.2f}".format(windowsizeInches[0]) + "x" + "{:.2f}".format(windowsizeInches[1])

    stdout, stderr, exitcode = run_and_capture(["gerbv", "--export=png", f"--dpi={dpi}",
                                                f"--origin={originStr}",
                                                f"--window_inch={windowsizeInchesStr}",
                                                f"--output={png_path}",
                                                "--foreground=#FFFFFF", "--background=#000000",
                                                gerber_path
                                                ])


def is_gerbv_installed() -> bool:
    try:
        stdout, stderr, exitcode = run_and_capture(["gerbv", "--version"])
    except:
        return False

    return exitcode == 0 and stdout is not None and stdout.startswith("gerbv version")
