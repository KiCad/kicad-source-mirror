/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef TREE_FILE_TYPE_H
#define TREE_FILE_TYPE_H

// Identify the type of files handled by KiCad manager
//
// When changing this enum  please verify (and perhaps update)
// PROJECT_TREE_PANE::GetFileExt(),
// s_AllowedExtensionsToList[]
enum class TREE_FILE_TYPE
{
    ROOT = 0,
    LEGACY_PROJECT,   // Legacy project file (.pro)
    JSON_PROJECT,     // JSON formatted project file (.kicad_pro)
    LEGACY_SCHEMATIC, // Schematic file (.sch)
    SEXPR_SCHEMATIC,  // Schematic file (.kicad_sch)
    LEGACY_PCB,       // board file (.brd) legacy format
    SEXPR_PCB,        // board file (.kicad_brd) new s expression format
    GERBER,           // Gerber  file (.pho, .g*)
    GERBER_JOB_FILE,  // Gerber  file (.gbrjob)
    HTML,             // HTML file (.htm, *.html)
    PDF,              // PDF file (.pdf)
    TXT,              // ascii text file (.txt)
    NET,              // netlist file (.net)
    UNKNOWN,
    DIRECTORY,
    CMP_LINK,              // cmp/footprint link file (.cmp)
    REPORT,                // report file (.rpt)
    FP_PLACE,              // footprints position (place) file (.pos)
    DRILL,                 // Excellon drill file (.drl)
    DRILL_NC,              // Similar Excellon drill file (.nc)
    DRILL_XNC,             // Similar Excellon drill file (.xnc)
    SVG,                   // SVG file (.svg)
    PAGE_LAYOUT_DESCR,     // Page layout and title block descr file (.kicad_wks)
    FOOTPRINT_FILE,        // footprint file (.kicad_mod)
    SCHEMATIC_LIBFILE,     // schematic library file (.lib)
    SEXPR_SYMBOL_LIB_FILE, // s-expression symbol library file (.kicad_sym)
    DESIGN_RULES,          // design rules (.kicad_dru)
    MAX
};

#endif // TREE_FILE_TYPE_H
