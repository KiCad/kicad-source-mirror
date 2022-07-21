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

# Converts a bitmap font atlas description in XML format to data stored
# in a C structure.

import xml.etree.cElementTree as ET
import sys
import os

def convert(xml_file):
    tree = ET.ElementTree(file=xml_file)
    root = tree.getroot()

    output = open(os.path.splitext(xml_file)[0] + '_desc.c', 'w')

    # Header
    output.write(
"""
/* generated with fnt2struct.py, do not modify by hand */

static const struct bitmap_glyph {
    unsigned int x, y;
    unsigned int width, height;
    int x_off, y_off;
} bitmap_chars[] = {
""");

    last_id = 0
    fallback_line = '{ 0, 0, 0, 0, 0, 0 },\t\t/* %d (not defined) */\n'

    for char in root.iter('char'):
        cur_id = int(char.attrib['id'])
        # Fill gaps for the missing characters
        while(cur_id > last_id):
            output.write(fallback_line % last_id)
            last_id = last_id + 1

        output.write('{ %d, %d, %d, %d, %d, %d },\t/* %d */\n' %
                (int(char.attrib['x']), int(char.attrib['y']),
                int(char.attrib['width']), int(char.attrib['height']),
                int(char.attrib['xoffset']), int(char.attrib['yoffset']),
                cur_id))
        last_id = cur_id + 1

    output.write('};\n')

    output.write('static const int bitmap_chars_count = %d;\n' % last_id)

    try:
        lineHeight = int(root.find('common').get('lineHeight'))
    except:
        print('Could not determine the font height')
        lineHeight = -1

    output.write('static const int bitmap_chars_height = %d;\n' % (lineHeight))

    output.close()

#----------------------------------------------------------------------
if __name__ == "__main__":
    argc = len(sys.argv)

    if(argc == 2):
        convert(sys.argv[1])
    else:
        print("usage: %s <xml-file>" % sys.argv[0])
