#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

import pytest
import utils
import os

@pytest.mark.parametrize("image1,image2,expected",
                          [
                            ("3px_base.png", "3px_mid_blk.png",         False),
                            ("3px_base.png", "3px_mid_1px_lines.png",   True), # Allow 1 px margin of error
                            ("3px_base.png", "3px_mid_transparent.png", False),
                            ("3px_base.png", "3px_offBy1_blk.png",      False),
                            ("3px_base.png", "3px_offBy1_wht.png",      False),
                            ("square_base.png", "square_blk.png",              False),
                            ("square_base.png", "square_transparent.png",      False),
                            ("square_base.png", "square_1px_bigger.png",       True), # Allow 1 px margin of error
                            ("irregular_base.png", "irregular_1px_eroded.png", True), # Allow 1 px margin of error
                            ("irregular_base.png", "irregular_2px_eroded.png", False), # 2 px error is too much
                          ]
                        )
def test_images_are_equal(image1: str, image2: str, expected: bool):
    resources_path = os.path.dirname( __file__ )
    resources_path = os.path.join( resources_path, "resources" )
    image1 = os.path.join( resources_path, image1 )
    image2 = os.path.join( resources_path, image2 )
    assert utils.images_are_equal( image2, image1 ) == expected
    assert utils.images_are_equal( image1, image2 ) == expected
