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
import cairosvg
import logging
import subprocess
from typing import Tuple
from pathlib import Path
from PIL import Image, ImageChops, ImageFilter
import numpy as np

logger = logging.getLogger("cli_util")

def run_and_capture( command: list ) -> Tuple[ str, str, int ]:
    logger.info("Executing command \"%s\"", " ".join( command ))

    proc = subprocess.Popen( command,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        encoding = 'utf-8'
    )

    out,err = proc.communicate()

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


def images_are_equal( image1_path: str, image2_path: str, alpha_colour = (50, 100, 50) ) -> bool:
    # Note: if modifying this function - please add new tests for it in test_utils.py

    # Increase limit to ~500MB uncompressed
    Image.MAX_IMAGE_PIXELS=2 * 1024 * 1024 * 1024 // 4 // 3

    image1 = Image.open( image1_path )
    image2 = Image.open( image2_path )

    if image1.size != image2.size:
        return False

    if image1.mode != image2.mode:
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

        # Save images
        #if not retval:
        diff_name = image1.filename + ".diff.png"
        diff.save( diff_name ) # Note: if the image has alpha, the diff will be mostly transparent

        diff_name = image1.filename + ".binary_result.png"
        binary_result.save( diff_name )

        diff_name = image1.filename + ".eroded_result_" + str( eroded_result_sum )+ ".png"
        eroded_result.save( diff_name )

    return retval


def svgs_are_equivalent( svg_generated_path : str, svg_source_path : str, comparison_dpi : int ) -> bool:

    png_generated_path = Path( svg_generated_path ).with_suffix( ".png" )

    # store source png in same folder as generated, easier to compare
    source_stem = Path( svg_source_path ).stem
    png_source_path = Path( svg_generated_path ).with_stem( source_stem + "-source").with_suffix( ".png" )

    cairosvg.svg2png( url=svg_generated_path,
                      write_to=str( png_generated_path ),
                      dpi=comparison_dpi )

    cairosvg.svg2png( url=svg_source_path,
                     write_to=str( png_source_path ),
                      dpi=comparison_dpi )

    return images_are_equal( str( png_generated_path ), str( png_source_path ) )

