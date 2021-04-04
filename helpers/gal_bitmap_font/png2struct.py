#!/usr/bin/env python

# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2016 CERN
# @author Maciej Suminski <maciej.suminski@cern.ch>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# Converts a bitmap font atlas image in PNG format to raw pixel data stored
# in a C structure.

import os
import png
import sys

def convert(png_file):
    r = png.Reader(file=open(png_file, 'rb'))
    img = r.read()

    print(img);

    if img[3].get("alpha") == True:
        print('Detected alpha channel!');

    output = open(os.path.splitext(png_file)[0] + '_img.c', 'w')

    width = img[0]
    height = img[1]

    # Header
    output.write(
"""
/* generated with png2struct.py, do not modify by hand */

static const struct {
    unsigned int width, height;
    unsigned char pixels[%d * %d];
} bitmap_font = {
""" % (width, height));

    output.write('%d, %d,\n{' % (width, height))
    
    if img[3].get("alpha") == True:
        for row in img[2]:
            for p in row[3::4]:
                output.write('%d,' % p)
            output.write('\n');
    else:
        for row in img[2]:
            for p in row:
                output.write('%d,' % p)
            output.write('\n');

    output.write('}\n};\n')

    output.close()

#----------------------------------------------------------------------
if __name__ == "__main__":
    argc = len(sys.argv)

    if(argc == 2):
        convert(sys.argv[1])
    else:
        print("usage: %s <xml-file>" % sys.argv[0])
