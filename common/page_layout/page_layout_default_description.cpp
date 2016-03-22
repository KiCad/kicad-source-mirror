/**
 * @file common/page_layout/page_layout_default_description.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/* keyword used in page layout description are listed
 * in page_layout_reader.keywords file
 */

/*
 *  Items use coordinates.
 *  A coordinate is defined relative to a page corner
 *  A coordinate is the X pos, the Y pos, and the corner which is the coordinate origin
 *  the default is the bottom right corner
 *  example: (start 10 0 ltcorner) or (start 20 10)
 *  The direction depends on the corner: a positive coordinate define a point
 *  from the corner origin, to the opposite corner.
 *
 *  Items are defined by a name, coordinates in mm and some attributes,
 *  and can be repeated.
 *  for instance (repeat 2) (incrx 2) (incry 2) repeat the item 2 times,
 *  and coordinates are incremented by 2 on X direction, and 2 on Y direction
 *  Comments are allowed. they are inside (), and start by the keyword comment
 *  example:
 *  (comment rect around the title block)
 *
 *  Lines and rect are defined by 2 coordinates start and end, and attributes.
 *  Attributes are linewidth and repeat parameters.
 *  example:
 *  (line (start 50 2 ltcorner) (end 50 0 ltcorner) (repeat 30) (incrx 50) )
 *  (rect (comment rect around the title block) (linewidth 0.15) (start 110 34) (end 2 2) )
 *
 *  Texts are defined by the text (between quotes), the position, and attributes
 *  example
 *  "(tbtext \"1\" (pos 25 1 lbcorner) (font (size 1.3 1.3)) (repeat 100) (incrx 50) )"
 *  the text can be rotated by (rotation <value>) with value = rot angle in degrees
 *  (font (size 1.3 1.3) bold italic) defines a specific size,
 *    with bold and italic options
 *  (justify <justif keyword>) controls the text justification (the default is left)
 *  justif keyword is center, left, right, top and bottom
 *  (justify center top) is a text centered on X axis and top aligned on vertical axis
 *  The text size can be constrained:
 *  (maxlen <value>) and (maxheight <value>) force the actual text x and/or y size to be
 *  reduced to limit the text height to the maxheight value,
 *  and the full text x size to the maxlen value.
 *  If the actual text size is smaller than limits, its size is not modified.
 *
 * Texts can include a format symbol, a la printf.
 * At run time these format symbols will be replaced by their actual value.
 *
 * format symbols are:
 *
 * %% = replaced by %
 * %K = Kicad version
 * %Z = paper format name (A4, USLetter ...)
 * %Y = company name
 * %D = date
 * %R = revision
 * %S = sheet number
 * %N = number of sheets
 * %Cx = comment (x = 0 to 9 to identify the comment)
 * %F = filename
 * %P = sheet path (sheet full name)
 * %T = title
 *
 * example:
 * (tbtext \"Size: %Z\" ...) displays "Size A4" or Size USLetter"
 *
 *  Poly Polygons
 *  Set of filled polygons are supported.
 *
 *  The main purpose is to allow logos, or complex shapes
 *  They support the repeat and rotation options
 *  They are defined by
 *  (polygon (position ..) <rotation>  <linewidth>
 *  the parameter linewidth defines the pen size used to draw/plot
 *  the polygon outlines (default = 0)
 *  example:
 *  (polygon (pos 134 18 rbcorner) (rotate 20)  (linewidth 0.00254)
 *
 *  and a list of corners like
 *  (pts (xy 20.574 8.382) (xy 19.9009 8.382) (xy 19.9009 6.26364) (xy 19.7485 5.98932)
 *  .... )
 *
 *  each sequence like
 *  (pts (xy 20.574 8.382) (xy 19.9009 8.382) (xy 19.9009 6.26364) (xy 19.7485 5.98932)
 *  .... )
 *  defines a polygon.
 *  Each coordinate is relative to the polygon position.
 *  Therefore a "polygon" is in fact a set of polygons, of a poly polygon
 *
 */

#include <worksheet.h>      // defaultPageLayout


// height of the band reference grid  2.0 mm
// worksheet frame reference text size 1.3 mm
// default text size 1.5 mm
// default line width 0.15 mm
// frame ref pitch 50 mm

// export defaultPageLayout:
extern const char   defaultPageLayout[];

// Default page layout (sizes are in mm)
const char          defaultPageLayout[] = "( page_layout\n"
                                          "(setup (textsize 1.5 1.5) (linewidth 0.15) (textlinewidth 0.15)\n"
                                          "(left_margin 10)(right_margin 10)(top_margin 10)(bottom_margin 10))\n"
                                          "(rect (comment \"rect around the title block\") (linewidth 0.15) (start 110 34) (end 2 2) )\n"
                                          "(rect (start 0 0 ltcorner) (end 0 0 rbcorner) (repeat 2) (incrx 2) (incry 2) )\n"
                                          "(line (start 50 2 ltcorner) (end 50 0 ltcorner) (repeat 30) (incrx 50) )\n"
                                          "(tbtext \"1\" (pos 25 1 ltcorner) (font (size 1.3 1.3))(repeat 100) (incrx 50) )\n"
                                          "(line (start 50 2 lbcorner) (end 50 0 lbcorner) (repeat 30) (incrx 50) )\n"
                                          "(tbtext \"1\" (pos 25 1 lbcorner) (font (size 1.3 1.3)) (repeat 100) (incrx 50) )\n"
                                          "(line (start 0 50 ltcorner) (end 2 50 ltcorner) (repeat 30) (incry 50) )\n"
                                          "(tbtext \"A\" (pos 1 25 ltcorner) (font (size 1.3 1.3)) (justify center)(repeat 100) (incry 50) )\n"
                                          "(line (start 0 50 rtcorner) (end 2 50 rtcorner) (repeat 30) (incry 50) )\n"
                                          "(tbtext \"A\" (pos 1 25 rtcorner) (font (size 1.3 1.3)) (justify center) (repeat 100) (incry 50) )\n"
                                          "(tbtext \"Date: %D\" (pos 87 6.9) )\n"
                                          "(line (start 110 5.5) (end 2 5.5) )\n"
                                          "(tbtext \"%K\" (pos 109 4.1) (comment \"Kicad version\" ) )\n"
                                          "(line (start 110 8.5) (end 2 8.5) )\n"
                                          "(tbtext \"Rev: %R\" (pos 24 6.9)(font bold)(justify left) )\n"
                                          "(tbtext \"Size: %Z\" (comment \"Paper format name\")(pos 109 6.9) )\n"
                                          "(tbtext \"Id: %S/%N\" (comment \"Sheet id\")(pos 24 4.1) )\n"
                                          "(line (start 110 12.5) (end 2 12.5) )\n"
                                          "(tbtext \"Title: %T\" (pos 109 10.7)(font bold italic (size 2 2)) )\n"
                                          "(tbtext \"File: %F\" (pos 109 14.3) )\n"
                                          "(line (start 110 18.5) (end 2 18.5) )\n"
                                          "(tbtext \"Sheet: %P\" (pos 109 17) )\n"
                                          "(tbtext \"%Y\" (comment \"Company name\") (pos 109 20)(font bold) )\n"
                                          "(tbtext \"%C0\" (comment \"Comment 0\") (pos 109 23) )\n"
                                          "(tbtext \"%C1\" (comment \"Comment 1\") (pos 109 26) )\n"
                                          "(tbtext \"%C2\" (comment \"Comment 2\") (pos 109 29) )\n"
                                          "(tbtext \"%C3\" (comment \"Comment 3\") (pos 109 32) )\n"
                                          "(line (start 90 8.5) (end 90 5.5) )\n"
                                          "(line (start 26 8.5) (end 26 2) )\n"
                                          ")\n"
;
