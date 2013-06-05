/**
 * @file page_layout_default_description.cpp
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

#include <fctsys.h>

// height of the band reference grid  2.0 mm
// worksheet frame reference text size 1.3 mm
// default text size 1.5 mm
// default line width 0.15 mm
// frame ref pitch 50 mm

extern const wxString defaultPageLayout;

// Default page layout (sizes are in mm)
const wxString defaultPageLayout( wxT( "( page_layout\n"
"(setup (textsize 1.5 1.5) (linewidth 0.15) (textlinewidth 0.15) )"
"(rect (comment rect around the title block) (linewidth 0.15) (start 110 34) (end 2 2) )\n"
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
"(line (start 110 5.5) end 2 5.5) )\n"
"(tbtext \"%K\" (pos 108 4.1) (comment Kicad version ) )\n"
"(line (start 110 8.5) end 2 8.5) )\n"
"(tbtext \"Rev: %R\" (pos 24 6.9)(font bold italic)(justify left) )\n"
"(tbtext \"Size: %Z\" (comment Paper format name)(pos 105 6.9) )\n"
"(tbtext \"Id: %S/%N\" (comment Sheet id)(pos 24 4.1) )\n"
"(line (start 110 12.5) end 2 12.5) )\n"
"(tbtext \"Title: %T\" (pos 108 10.7)(font bold (size 2 2)) )\n"
"(tbtext \"File: %F\" (pos 108 14.3) )\n"
"(line (start 110 18.5) end 2 18.5) )\n"
"(tbtext \"Sheet: %P\" (pos 108 17) )\n"
"(tbtext \"%Y\" (comment Company name) (pos 108 20)(font bold) )\n"
"(tbtext \"%C0\" (comment Comment 0) (pos 108 23) )\n"
"(tbtext \"%C1\" (comment Comment 0) (pos 108 26) )\n"
"(tbtext \"%C2\" (comment Comment 0) (pos 108 29) )\n"
"(tbtext \"%C3\" (comment Comment 0) (pos 108 32) )\n"
"(line (start 90 8.5) end 90 5.5) )\n"
"(line (start 26 8.5) end 26 2) )\n"
")\n") );
