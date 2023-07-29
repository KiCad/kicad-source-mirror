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


def images_are_equal( image1: str, image2: str ) -> bool:
    # Increase limit to ~500MB uncompressed
    Image.MAX_IMAGE_PIXELS=2 * 1024 * 1024 * 1024 // 4 // 3

    image1 = Image.open( image1 )
    image2 = Image.open( image2 )

    if image1.size != image2.size:
        return False

    if image1.mode != image2.mode:
        return False

    sum = np.sum( ( np.asarray ( image1 ).astype( np.float32 ) - np.asarray( image2 ).astype( np.float32 ) ) ** 2.0 )
    retval = True

    if sum != 0.0:
        # images are not identical - lets allow 1 pixel error difference (for curved edges)

        diff = ImageChops.difference( image1, image2 )
        diffThresholded = diff.point( lambda x: 255 if x > 1 else 0 )

        # erode binary image by 1 pixel
        diffEroded = diffThresholded.filter(ImageFilter.MinFilter(3))
        
        erodedSum = np.sum( np.asarray( diffEroded ).astype( np.float32 ) )

        retval = erodedSum == 0

        # Save images
        diff_name = image1.filename + ".diff1.png"
        diff.save( diff_name )
        diff_name = image1.filename + ".diffthresholded.png"
        diffThresholded.save( diff_name )
        diffEroded_name = image1.filename + ".diffEroded_erodedsum" + str(erodedSum)+ ".png"
        diffEroded.save( diffEroded_name )


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

