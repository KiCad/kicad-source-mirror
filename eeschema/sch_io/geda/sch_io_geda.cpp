/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file contains embedded symbol definitions and file format knowledge
 * derived from the gEDA and Lepton EDA projects:
 *
 *   gEDA/gaf  - Copyright (C) 1998-2010 Ales Hvezda
 *               Copyright (C) 1998-2016 gEDA Contributors
 *   Lepton EDA - Copyright (C) 2017-2024 Lepton EDA Contributors
 *
 * Both projects are licensed under the GNU General Public License v2 or later.
 * See https://github.com/lepton-eda/lepton-eda and
 *     https://github.com/rlutz/geda-gaf
 */

#include <sch_io/geda/sch_io_geda.h>

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/log.h>

#include <eda_shape.h>
#include <lib_id.h>
#include <page_info.h>
#include <lib_symbol.h>
#include <project.h>
#include <project_sch.h>
#include <default_values.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_no_connect.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <sch_bitmap.h>
#include <sch_text.h>
#include <schematic.h>
#include <string_utils.h>
#include <stroke_params.h>
#include <wildcards_and_files_ext.h>
#include <base_units.h>
#include <pin_type.h>
#include <reporter.h>
#include <font/text_attributes.h>
#include <math/util.h>
#include <reference_image.h>
#include <wx/base64.h>
#include <wx/image.h>

#include <algorithm>
#include <climits>
#include <vector>

// Default text size for imported text (50 mils)
static constexpr int GEDA_DEFAULT_TEXT_SIZE_MILS = 50;

// Default body size for fallback rectangular symbols
static constexpr int DEFAULT_SYMBOL_SIZE_MILS = 200;

/**
 * Convert gEDA overbar markup to KiCad syntax.
 *
 * gEDA uses \_text\_ to indicate an overbar (text with a line above it).
 * KiCad uses ~{text}. Escaped backslashes (\\) become a literal backslash.
 */
static wxString convertOverbars( const wxString& aInput )
{
    wxString result;
    result.reserve( aInput.length() + 8 );

    size_t i = 0;

    while( i < aInput.length() )
    {
        if( i + 1 < aInput.length() && aInput[i] == '\\' && aInput[i + 1] == '_' )
        {
            size_t barEnd = aInput.find( wxT( "\\_" ), i + 2 );

            if( barEnd != wxString::npos )
            {
                result += wxT( "~{" );
                result += aInput.Mid( i + 2, barEnd - ( i + 2 ) );
                result += wxT( "}" );
                i = barEnd + 2;
            }
            else
            {
                result += aInput[i];
                i++;
            }
        }
        else if( i + 1 < aInput.length() && aInput[i] == '\\' && aInput[i + 1] == '\\' )
        {
            result += '\\';
            i += 2;
        }
        else
        {
            result += aInput[i];
            i++;
        }
    }

    return result;
}


// Standard gEDA symbol definitions embedded from the geda-gaf project (GPL v2+).
// These provide correct pin positions for common symbols so the importer works
// without requiring a gEDA system installation.
const std::map<wxString, wxString>& SCH_IO_GEDA::getBuiltinSymbols()
{
    static const std::map<wxString, wxString> symbols = {
        { wxT( "resistor-1.sym" ),
          wxT( "v 20031231 1\n"
               "L 600 200 500 0 3 0 0 0 -1 -1\n"
               "L 500 0 400 200 3 0 0 0 -1 -1\n"
               "L 400 200 300 0 3 0 0 0 -1 -1\n"
               "L 300 0 200 200 3 0 0 0 -1 -1\n"
               "T 300 400 5 10 0 0 0 0 1\n"
               "device=RESISTOR\n"
               "L 600 200 700 0 3 0 0 0 -1 -1\n"
               "L 700 0 750 100 3 0 0 0 -1 -1\n"
               "P 900 100 750 100 1 0 0\n"
               "{\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 800 150 5 8 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pinlabel=2\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 0 100 152 100 1 0 0\n"
               "{\n"
               "T 100 150 5 8 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 100 150 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 100 150 5 8 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 100 150 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "L 201 200 150 100 3 0 0 0 -1 -1\n"
               "T 200 300 8 10 1 1 0 0 1\n"
               "refdes=R?\n" ) },

        { wxT( "resistor-2.sym" ),
          wxT( "v 20031231 1\n"
               "B 200 0 600 200 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "T 300 400 5 10 0 0 0 0 1\n"
               "device=RESISTOR\n"
               "P 900 100 800 100 1 0 0\n"
               "{\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 800 150 5 8 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pinlabel=2\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 0 100 200 100 1 0 0\n"
               "{\n"
               "T 100 150 5 8 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 100 150 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 100 150 5 8 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 100 150 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "T 200 300 8 10 1 1 0 0 1\n"
               "refdes=R?\n" ) },

        { wxT( "capacitor-1.sym" ),
          wxT( "v 20050820 1\n"
               "P 0 200 200 200 1 0 0\n"
               "{\n"
               "T 150 250 5 8 0 1 0 6 1\n"
               "pinnumber=1\n"
               "T 150 150 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 200 200 9 8 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 200 200 5 8 0 1 0 2 1\n"
               "pintype=pas\n"
               "}\n"
               "P 900 200 700 200 1 0 0\n"
               "{\n"
               "T 750 250 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 750 150 5 8 0 1 0 2 1\n"
               "pinseq=2\n"
               "T 700 200 9 8 0 1 0 6 1\n"
               "pinlabel=2\n"
               "T 700 200 5 8 0 1 0 8 1\n"
               "pintype=pas\n"
               "}\n"
               "L 400 400 400 0 3 0 0 0 -1 -1\n"
               "L 500 400 500 0 3 0 0 0 -1 -1\n"
               "L 700 200 500 200 3 0 0 0 -1 -1\n"
               "L 400 200 200 200 3 0 0 0 -1 -1\n"
               "T 200 700 5 10 0 0 0 0 1\n"
               "device=CAPACITOR\n"
               "T 200 500 8 10 1 1 0 0 1\n"
               "refdes=C?\n" ) },

        { wxT( "capacitor-2.sym" ),
          wxT( "v 20050820 1\n"
               "P 0 200 200 200 1 0 0\n"
               "{\n"
               "T 150 250 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 200 150 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 250 200 9 8 0 1 0 0 1\n"
               "pinlabel=+\n"
               "T 250 200 5 8 0 1 0 2 1\n"
               "pintype=pas\n"
               "}\n"
               "P 900 200 700 200 1 0 0\n"
               "{\n"
               "T 750 250 5 8 1 1 0 0 1\n"
               "pinnumber=2\n"
               "T 700 150 5 8 0 1 0 2 1\n"
               "pinseq=2\n"
               "T 650 200 9 8 0 1 0 6 1\n"
               "pinlabel=-\n"
               "T 650 200 5 8 0 1 0 8 1\n"
               "pintype=pas\n"
               "}\n"
               "L 400 400 400 0 3 0 0 0 -1 -1\n"
               "A 1200 200 700 165 30 3 0 0 0 -1 -1\n"
               "L 700 200 500 200 3 0 0 0 -1 -1\n"
               "L 400 200 200 200 3 0 0 0 -1 -1\n"
               "L 289 400 289 300 3 0 0 0 -1 -1\n"
               "L 340 349 240 349 3 0 0 0 -1 -1\n"
               "T 200 700 5 10 0 0 0 0 1\n"
               "device=POLARIZED_CAPACITOR\n"
               "T 200 500 8 10 1 1 0 0 1\n"
               "refdes=C?\n" ) },

        { wxT( "gnd-1.sym" ),
          wxT( "v 20031231 1\n"
               "P 100 100 100 300 1 0 1\n"
               "{\n"
               "T 158 161 5 4 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 158 161 5 4 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 158 161 5 4 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 158 161 5 4 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 0 100 200 100 3 0 0 0 -1 -1\n"
               "L 55 50 145 50 3 0 0 0 -1 -1\n"
               "L 80 10 120 10 3 0 0 0 -1 -1\n"
               "T 300 50 8 10 0 0 0 0 1\n"
               "net=GND:1\n" ) },

        { wxT( "generic-power.sym" ),
          wxT( "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 200 250 8 10 1 1 0 3 1\n"
               "net=Vcc:1\n" ) },

        { wxT( "input-1.sym" ),
          wxT( "v 20031231 1\n"
               "P 600 100 800 100 1 0 1\n"
               "{\n"
               "T 450 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 450 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "}\n"
               "L 0 200 0 0 3 0 0 0 -1 -1\n"
               "L 0 200 500 200 3 0 0 0 -1 -1\n"
               "L 500 200 600 100 3 0 0 0 -1 -1\n"
               "L 600 100 500 0 3 0 0 0 -1 -1\n"
               "L 500 0 0 0 3 0 0 0 -1 -1\n"
               "T 0 300 5 10 0 0 0 0 1\n"
               "device=INPUT\n" ) },

        { wxT( "output-1.sym" ),
          wxT( "v 20031231 1\n"
               "P 0 100 200 100 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "}\n"
               "L 200 200 200 0 3 0 0 0 -1 -1\n"
               "L 200 200 700 200 3 0 0 0 -1 -1\n"
               "L 700 200 800 100 3 0 0 0 -1 -1\n"
               "L 800 100 700 0 3 0 0 0 -1 -1\n"
               "L 700 0 200 0 3 0 0 0 -1 -1\n"
               "T 100 300 5 10 0 0 0 0 1\n"
               "device=OUTPUT\n" ) },

        { wxT( "nc-right-1.sym" ),
          wxT( "v 20060123 1\n"
               "P 0 100 200 100 1 0 0\n"
               "{\n"
               "T 600 100 5 10 0 0 180 8 1\n"
               "pinseq=1\n"
               "T 600 300 5 10 0 0 180 8 1\n"
               "pinnumber=1\n"
               "}\n"
               "L 200 0 200 200 3 0 0 0 -1 -1\n"
               "T 100 500 8 10 0 0 0 0 1\n"
               "value=NoConnection\n"
               "T 250 100 9 10 1 0 0 1 1\n"
               "NC\n"
               "T 0 600 8 10 0 0 0 0 1\n"
               "device=DRC_Directive\n"
               "T 100 900 8 10 0 0 0 0 1\n"
               "graphical=1\n" ) },

        { wxT( "nc-left-1.sym" ),
          wxT( "v 20060123 1\n"
               "P 400 100 200 100 1 0 1\n"
               "{\n"
               "T 0 100 5 10 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 0 300 5 10 0 0 0 0 1\n"
               "pinnumber=1\n"
               "}\n"
               "L 200 0 200 200 3 0 0 0 -1 -1\n"
               "T 100 500 8 10 0 0 0 0 1\n"
               "value=NoConnection\n"
               "T 50 100 9 10 1 0 0 5 1\n"
               "NC\n"
               "T 0 600 8 10 0 0 0 0 1\n"
               "device=DRC_Directive\n"
               "T 100 900 8 10 0 0 0 0 1\n"
               "graphical=1\n" ) },

        { wxT( "terminal-1.sym" ),
          wxT( "v 20041228 1\n"
               "T 310 750 8 10 0 0 0 0 1\n"
               "device=terminal\n"
               "P 560 100 900 100 1 0 1\n"
               "{\n"
               "T 660 150 5 10 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 510 100 5 10 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 510 300 5 10 0 1 0 0 1\n"
               "pintype=pas\n"
               "T 800 100 5 10 0 1 0 0 1\n"
               "pinlabel=terminal\n"
               "}\n"
               "V 460 100 100 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "T 250 50 8 10 1 1 0 6 1\n"
               "refdes=T?\n" ) },

        // Power symbols
        { wxT( "vcc-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "Vcc\n"
               "T 450 200 8 10 0 0 0 0 1\n"
               "net=Vcc:1\n" ) },

        { wxT( "vdd-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "Vdd\n"
               "T 450 200 8 10 0 0 0 0 1\n"
               "net=Vdd:1\n" ) },

        { wxT( "5V-plus-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "+5V\n"
               "T 300 0 8 8 0 0 0 0 1\n"
               "net=+5V:1\n" ) },

        { wxT( "3.3V-plus-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "+3.3V\n"
               "T 300 0 8 8 0 0 0 0 1\n"
               "net=+3.3V:1\n" ) },

        { wxT( "12V-plus-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "+12V\n"
               "T 300 0 9 8 0 0 0 0 1\n"
               "net=+12V:1\n" ) },

        { wxT( "vcc-2.sym" ),
          wxT(                "v 20031231 1\n"
               "V 200 350 50 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "P 200 300 200 0 1 0 1\n"
               "{\n"
               "T 300 50 5 10 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 300 50 5 10 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 300 50 5 10 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 300 50 5 10 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "T 0 450 9 10 1 0 0 0 1\n"
               "Vcc\n"
               "T 400 300 8 8 0 0 0 0 1\n"
               "net=Vcc:1\n" ) },

        { wxT( "vss-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "Vss\n"
               "T 450 200 8 10 0 0 0 0 1\n"
               "net=Vss:1\n" ) },

        { wxT( "vee-1.sym" ),
          wxT(                "v 20031231 1\n"
               "P 200 0 200 200 1 0 0\n"
               "{\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 250 50 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 250 50 5 6 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 50 200 350 200 3 0 0 0 -1 -1\n"
               "T 75 250 9 8 1 0 0 0 1\n"
               "Vee\n"
               "T 450 200 8 10 0 0 0 0 1\n"
               "net=Vee:1\n" ) },

        { wxT( "gnd-2.sym" ),
          wxT(                "v 20031231 1\n"
               "P 100 100 100 300 1 0 1\n"
               "{\n"
               "T 158 161 5 4 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 158 161 5 4 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 300 200 3 10 1 1 0 0 1\n"
               "pinlabel=PGND\n"
               "T 158 161 5 4 0 1 0 0 1\n"
               "pintype=pwr\n"
               "}\n"
               "L 0 100 200 100 3 0 0 0 -1 -1\n"
               "L 55 50 145 50 3 0 0 0 -1 -1\n"
               "L 80 10 120 10 3 0 0 0 -1 -1\n"
               "T 300 50 8 10 0 0 0 0 1\n"
               "net=PGND:1\n" ) },

        // Diodes
        { wxT( "diode-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 300 400 300 0 3 0 0 0 -1 -1\n"
               "L 300 400 600 200 3 0 0 0 -1 -1\n"
               "T 400 600 5 10 0 0 0 0 1\n"
               "device=DIODE\n"
               "L 600 200 300 0 3 0 0 0 -1 -1\n"
               "L 600 400 600 0 3 0 0 0 -1 -1\n"
               "P 0 200 200 200 1 0 0\n"
               "{\n"
               "T 100 250 5 8 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 100 250 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 100 250 5 8 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 100 250 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 900 200 700 200 1 0 0\n"
               "{\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 700 250 5 8 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pinlabel=2\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "L 700 200 600 200 3 0 0 0 -1 -1\n"
               "L 300 200 200 200 3 0 0 0 -1 -1\n"
               "T 300 500 8 10 1 1 0 0 1\n"
               "refdes=D?\n" ) },

        { wxT( "zener-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 300 400 300 0 3 0 0 0 -1 -1\n"
               "L 600 200 300 0 3 0 0 0 -1 -1\n"
               "L 600 200 300 400 3 0 0 0 -1 -1\n"
               "T 400 600 5 10 0 0 0 0 1\n"
               "device=ZENER_DIODE\n"
               "L 600 400 600 0 3 0 0 0 -1 -1\n"
               "P 900 200 700 200 1 0 0\n"
               "{\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 700 250 5 8 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pinlabel=2\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 200 200 0 200 1 0 1\n"
               "{\n"
               "T 100 250 5 8 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 100 250 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 100 250 5 8 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 100 250 5 8 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "L 700 200 600 200 3 0 0 0 -1 -1\n"
               "L 300 200 200 200 3 0 0 0 -1 -1\n"
               "L 600 400 500 400 3 0 0 0 -1 -1\n"
               "L 600 0 700 0 3 0 0 0 -1 -1\n"
               "T 300 500 8 10 1 1 0 0 1\n"
               "refdes=Z?\n" ) },

        { wxT( "schottky-1.sym" ),
          wxT(                "v 20041228 1\n"
               "L 300 400 300 0 3 0 0 0 -1 -1\n"
               "L 300 400 600 200 3 0 0 0 -1 -1\n"
               "T 322 672 8 10 0 0 0 0 1\n"
               "device=DIODE\n"
               "L 600 200 300 0 3 0 0 0 -1 -1\n"
               "L 600 400 600 0 3 0 0 0 -1 -1\n"
               "P 0 200 200 200 1 0 0\n"
               "{\n"
               "T 205 256 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 170 286 5 8 0 0 90 0 1\n"
               "pinseq=2\n"
               "T -10 81 5 10 0 1 0 0 1\n"
               "pintype=pas\n"
               "T 75 288 5 10 0 1 90 0 1\n"
               "pinlabel=anode\n"
               "}\n"
               "P 900 200 700 200 1 0 0\n"
               "{\n"
               "T 700 250 5 8 0 1 0 0 1\n"
               "pinnumber=1\n"
               "T 730 50 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 798 265 5 10 0 1 0 0 1\n"
               "pintype=pas\n"
               "T 931 138 5 10 0 1 0 0 1\n"
               "pinlabel=cathode\n"
               "}\n"
               "L 700 200 600 200 3 0 0 0 -1 -1\n"
               "L 300 200 200 200 3 0 0 0 -1 -1\n"
               "A 650 400 50 0 180 3 0 0 0 -1 -1\n"
               "A 550 0 50 180 180 3 0 0 0 -1 -1\n"
               "T 300 500 8 10 1 1 0 0 1\n"
               "refdes=D?\n"
               "T 567 515 8 10 0 0 0 0 1\n"
               "numslots=0\n" ) },

        { wxT( "led-1.sym" ),
          wxT(                "v 20210407 2\n"
               "P 0 200 200 200 1 0 0\n"
               "{\n"
               "T 150 250 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 150 150 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 250 200 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 250 200 5 8 0 1 0 2 1\n"
               "pintype=pas\n"
               "}\n"
               "P 900 200 700 200 1 0 0\n"
               "{\n"
               "T 750 250 5 8 1 1 0 0 1\n"
               "pinnumber=2\n"
               "T 750 150 5 8 0 1 0 2 1\n"
               "pinseq=2\n"
               "T 650 200 9 8 0 1 0 6 1\n"
               "pinlabel=K\n"
               "T 650 200 5 8 0 1 0 8 1\n"
               "pintype=pas\n"
               "}\n"
               "L 400 300 500 200 3 0 0 0 -1 -1\n"
               "L 500 200 400 100 3 0 0 0 -1 -1\n"
               "L 400 300 400 100 3 0 0 0 -1 -1\n"
               "L 500 300 500 100 3 0 0 0 -1 -1\n"
               "L 500 200 700 200 3 0 0 0 -1 -1\n"
               "L 400 200 200 200 3 0 0 0 -1 -1\n"
               "V 450 200 200 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "T 800 600 5 10 0 0 0 0 1\n"
               "device=LED\n"
               "T 800 400 8 10 1 1 0 0 1\n"
               "refdes=LED?\n"
               "T 800 1000 5 10 0 0 0 0 1\n"
               "numslots=0\n"
               "T 800 800 5 10 0 0 0 0 1\n"
               "symversion=0.2\n" ) },

        // Transistors
        { wxT( "npn-1.sym" ),
          wxT(                "v 20210407 2\n"
               "L 200 800 200 200 3 0 0 0 -1 -1\n"
               "T 600 500 5 10 0 0 0 0 1\n"
               "device=NPN_TRANSISTOR\n"
               "L 500 800 200 500 3 0 0 0 -1 -1\n"
               "L 200 500 500 200 3 0 0 0 -1 -1\n"
               "H 3 0 0 0 -1 -1 1 -1 -1 -1 -1 -1 5\n"
               "M 410,240\n"
               "L 501,200\n"
               "L 455,295\n"
               "L 435,265\n"
               "z\n"
               "P 0 500 200 500 1 0 0\n"
               "{\n"
               "T 100 550 5 6 1 1 0 0 1\n"
               "pinnumber=B\n"
               "T 100 550 5 6 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 100 550 5 6 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 100 550 5 6 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 500 1000 500 800 1 0 0\n"
               "{\n"
               "T 400 850 5 6 1 1 0 0 1\n"
               "pinnumber=C\n"
               "T 400 850 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 400 850 5 6 0 1 0 0 1\n"
               "pinlabel=C\n"
               "T 400 850 5 6 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 500 200 500 0 1 0 1\n"
               "{\n"
               "T 400 50 5 6 1 1 0 0 1\n"
               "pinnumber=E\n"
               "T 400 50 5 6 0 0 0 0 1\n"
               "pinseq=3\n"
               "T 400 50 5 6 0 1 0 0 1\n"
               "pinlabel=E\n"
               "T 400 50 5 6 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "T 600 500 8 10 1 1 0 0 1\n"
               "refdes=Q?\n" ) },

        { wxT( "pnp-1.sym" ),
          wxT(                "v 20210407 2\n"
               "L 200 800 200 200 3 0 0 0 -1 -1\n"
               "T 600 500 5 10 0 0 0 0 1\n"
               "device=PNP_TRANSISTOR\n"
               "L 500 800 200 500 3 0 0 0 -1 -1\n"
               "L 200 500 500 200 3 0 0 0 -1 -1\n"
               "P 0 500 200 500 1 0 0\n"
               "{\n"
               "T 100 550 5 6 1 1 0 0 1\n"
               "pinnumber=B\n"
               "T 100 550 5 6 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 100 550 5 6 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 100 550 5 6 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 500 1000 500 800 1 0 0\n"
               "{\n"
               "T 400 850 5 6 1 1 0 0 1\n"
               "pinnumber=C\n"
               "T 400 850 5 6 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 400 850 5 6 0 1 0 0 1\n"
               "pinlabel=C\n"
               "T 400 850 5 6 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 500 200 500 0 1 0 1\n"
               "{\n"
               "T 400 50 5 6 1 1 0 0 1\n"
               "pinnumber=E\n"
               "T 400 50 5 6 0 0 0 0 1\n"
               "pinseq=3\n"
               "T 400 50 5 6 0 1 0 0 1\n"
               "pinlabel=E\n"
               "T 400 50 5 6 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "T 600 500 8 10 1 1 0 0 1\n"
               "refdes=Q?\n"
               "H 3 0 0 0 -1 -1 1 -1 -1 -1 -1 -1 5\n"
               "M 340,290\n"
               "L 300,401\n"
               "L 395,355\n"
               "L 365,335\n"
               "z\n" ) },

        { wxT( "nmos-1.sym" ),
          wxT(                "v 20210407 2\n"
               "L 300 600 300 200 3 0 0 0 -1 -1\n"
               "T 700 800 5 10 0 0 0 0 1\n"
               "device=NMOS_TRANSISTOR\n"
               "L 200 600 200 200 3 0 0 0 -1 -1\n"
               "L 300 600 500 600 3 0 0 0 -1 -1\n"
               "L 300 200 500 200 3 0 0 0 -1 -1\n"
               "L 300 400 500 400 3 0 0 0 -1 -1\n"
               "L 300 400 400 500 3 0 0 0 -1 -1\n"
               "L 300 400 400 300 3 0 0 0 -1 -1\n"
               "P 0 400 200 400 1 0 0\n"
               "{\n"
               "T 150 450 5 8 0 1 0 6 1\n"
               "pinnumber=G\n"
               "T 150 350 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 200 400 9 8 0 1 0 0 1\n"
               "pinlabel=G\n"
               "T 200 400 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 500 600 500 800 1 0 1\n"
               "{\n"
               "T 550 650 5 8 0 1 0 0 1\n"
               "pinnumber=D\n"
               "T 550 650 5 8 0 1 0 2 1\n"
               "pinseq=1\n"
               "T 500 600 9 8 0 1 0 5 1\n"
               "pinlabel=D\n"
               "T 500 400 5 8 0 1 0 5 1\n"
               "pintype=pas\n"
               "}\n"
               "P 500 200 500 0 1 0 1\n"
               "{\n"
               "T 550 50 5 8 0 1 0 0 1\n"
               "pinnumber=S\n"
               "T 550 50 5 8 0 1 0 2 1\n"
               "pinseq=3\n"
               "T 500 200 9 8 0 1 0 3 1\n"
               "pinlabel=S\n"
               "T 500 500 5 8 0 1 0 3 1\n"
               "pintype=pas\n"
               "}\n"
               "T 700 600 8 10 1 1 0 0 1\n"
               "refdes=Q?\n"
               "T 700 1000 5 10 0 0 0 0 1\n"
               "symversion=0.2\n" ) },

        { wxT( "pmos-1.sym" ),
          wxT(                "v 20210407 2\n"
               "L 300 600 300 200 3 0 0 0 -1 -1\n"
               "T 600 200 5 10 0 0 0 0 1\n"
               "device=PMOS_TRANSISTOR\n"
               "L 200 600 200 200 3 0 0 0 -1 -1\n"
               "L 300 600 500 600 3 0 0 0 -1 -1\n"
               "L 300 200 500 200 3 0 0 0 -1 -1\n"
               "L 300 400 500 400 3 0 0 0 -1 -1\n"
               "L 500 400 400 300 3 0 0 0 -1 -1\n"
               "L 500 400 400 500 3 0 0 0 -1 -1\n"
               "P 500 600 500 800 1 0 1\n"
               "{\n"
               "T 300 700 5 10 0 1 0 0 1\n"
               "pinnumber=D\n"
               "T 300 700 5 10 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 300 700 5 10 0 1 0 0 1\n"
               "pinlabel=D\n"
               "T 300 700 5 10 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 500 200 500 0 1 0 1\n"
               "{\n"
               "T 300 0 5 10 0 1 0 0 1\n"
               "pinnumber=S\n"
               "T 300 0 5 10 0 0 0 0 1\n"
               "pinseq=3\n"
               "T 300 0 5 10 0 1 0 0 1\n"
               "pinlabel=S\n"
               "T 300 0 5 10 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "P 200 400 0 400 1 0 1\n"
               "{\n"
               "T 0 500 5 10 0 1 0 0 1\n"
               "pinnumber=G\n"
               "T 0 500 5 10 0 0 0 0 1\n"
               "pinseq=2\n"
               "T 0 500 5 10 0 1 0 0 1\n"
               "pinlabel=G\n"
               "T 0 500 5 10 0 1 0 0 1\n"
               "pintype=pas\n"
               "}\n"
               "T 700 600 8 10 1 1 0 0 1\n"
               "refdes=Q?\n"
               "T 1200 500 8 10 0 0 0 0 1\n"
               "symversion=0.1\n" ) },

        // Analog
        { wxT( "opamp-1.sym" ),
          wxT(                "v 20050820 1\n"
               "L 200 800 200 0 3 0 0 0 -1 -1\n"
               "L 200 800 800 400 3 0 0 0 -1 -1\n"
               "T 700 800 5 10 0 0 0 0 1\n"
               "device=OPAMP\n"
               "L 800 400 200 0 3 0 0 0 -1 -1\n"
               "L 300 650 300 550 3 0 0 0 -1 -1\n"
               "L 250 600 350 600 3 0 0 0 -1 -1\n"
               "L 250 200 350 200 3 0 0 0 -1 -1\n"
               "P 0 600 200 600 1 0 0\n"
               "{\n"
               "T 150 650 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 150 550 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 250 600 9 8 0 1 0 0 1\n"
               "pinlabel=in+\n"
               "T 250 600 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 0 200 200 200 1 0 0\n"
               "{\n"
               "T 150 250 5 8 1 1 0 6 1\n"
               "pinnumber=2\n"
               "T 150 150 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 250 200 9 8 0 1 0 0 1\n"
               "pinlabel=in-\n"
               "T 250 200 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 800 400 1000 400 1 0 1\n"
               "{\n"
               "T 800 450 5 8 1 1 0 0 1\n"
               "pinnumber=5\n"
               "T 800 350 5 8 0 1 0 2 1\n"
               "pinseq=5\n"
               "T 750 400 9 8 0 1 0 6 1\n"
               "pinlabel=out\n"
               "T 750 400 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "P 500 600 500 800 1 0 1\n"
               "{\n"
               "T 550 600 5 8 1 1 0 0 1\n"
               "pinnumber=3\n"
               "T 550 600 5 8 0 1 0 2 1\n"
               "pinseq=3\n"
               "T 500 600 9 8 0 1 0 5 1\n"
               "pinlabel=V+\n"
               "T 500 550 5 8 0 1 0 5 1\n"
               "pintype=pwr\n"
               "}\n"
               "P 500 200 500 0 1 0 1\n"
               "{\n"
               "T 550 100 5 8 1 1 0 0 1\n"
               "pinnumber=4\n"
               "T 550 100 5 8 0 1 0 2 1\n"
               "pinseq=4\n"
               "T 500 200 9 8 0 1 0 3 1\n"
               "pinlabel=V-\n"
               "T 500 300 5 8 0 1 0 3 1\n"
               "pintype=pwr\n"
               "}\n"
               "T 700 600 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 700 1000 5 10 0 0 0 0 1\n"
               "numslots=0\n"
               "T 700 1400 5 10 0 0 0 0 1\n"
               "symversion=0.1\n" ) },

        { wxT( "inductor-1.sym" ),
          wxT(                "v 20210407 2\n"
               "P 900 100 750 100 1 0 0\n"
               "{\n"
               "T 800 150 5 8 0 1 0 0 1\n"
               "pinnumber=2\n"
               "T 800 50 5 8 0 1 0 2 1\n"
               "pinseq=2\n"
               "T 700 100 9 8 0 1 0 6 1\n"
               "pinlabel=2\n"
               "T 700 100 5 8 0 1 0 8 1\n"
               "pintype=pas\n"
               "}\n"
               "P 0 100 150 100 1 0 0\n"
               "{\n"
               "T 100 150 5 8 0 1 0 6 1\n"
               "pinnumber=1\n"
               "T 100 50 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 200 100 9 8 0 1 0 0 1\n"
               "pinlabel=1\n"
               "T 200 100 5 8 0 1 0 2 1\n"
               "pintype=pas\n"
               "}\n"
               "A 237 100 75 0 180 3 0 0 0 -1 -1\n"
               "A 379 100 75 0 180 3 0 0 0 -1 -1\n"
               "A 521 100 75 0 180 3 0 0 0 -1 -1\n"
               "A 663 100 75 0 180 3 0 0 0 -1 -1\n"
               "T 200 500 5 10 0 0 0 0 1\n"
               "device=INDUCTOR\n"
               "T 200 300 8 10 1 1 0 0 1\n"
               "refdes=L?\n"
               "T 200 700 5 10 0 0 0 0 1\n"
               "symversion=0.2\n" ) },

        // 7400-series logic gates
        { wxT( "7400-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 300 200 300 800 3 0 0 0 -1 -1\n"
               "T 300 0 9 8 1 0 0 0 1\n"
               "7400\n"
               "L 300 800 700 800 3 0 0 0 -1 -1\n"
               "T 500 900 5 10 0 0 0 0 1\n"
               "device=7400\n"
               "T 500 1100 5 10 0 0 0 0 1\n"
               "slot=1\n"
               "T 500 1300 5 10 0 0 0 0 1\n"
               "numslots=4\n"
               "T 500 1500 5 10 0 0 0 0 1\n"
               "slotdef=1:1,2,3\n"
               "T 500 1700 5 10 0 0 0 0 1\n"
               "slotdef=2:4,5,6\n"
               "T 500 1900 5 10 0 0 0 0 1\n"
               "slotdef=3:9,10,8\n"
               "T 500 2100 5 10 0 0 0 0 1\n"
               "slotdef=4:12,13,11\n"
               "L 300 200 700 200 3 0 0 0 -1 -1\n"
               "A 700 500 300 270 180 3 0 0 0 -1 -1\n"
               "V 1050 500 50 6 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "P 1100 500 1300 500 1 0 1\n"
               "{\n"
               "T 1100 550 5 8 1 1 0 0 1\n"
               "pinnumber=3\n"
               "T 1100 450 5 8 0 1 0 2 1\n"
               "pinseq=3\n"
               "T 950 500 9 8 0 1 0 6 1\n"
               "pinlabel=Y\n"
               "T 950 500 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "P 300 300 0 300 1 0 1\n"
               "{\n"
               "T 200 350 5 8 1 1 0 6 1\n"
               "pinnumber=2\n"
               "T 200 250 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 350 300 9 8 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 350 300 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 300 700 0 700 1 0 1\n"
               "{\n"
               "T 200 750 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 200 650 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 350 700 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 350 700 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "T 300 900 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 500 2850 5 10 0 0 0 0 1\n"
               "net=Vcc:14\n"
               "T 500 3050 5 10 0 0 0 0 1\n"
               "net=GND:7\n" ) },

        { wxT( "7404-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 300 800 800 500 3 0 0 0 -1 -1\n"
               "T 600 900 5 10 0 0 0 0 1\n"
               "device=7404\n"
               "T 600 1100 5 10 0 0 0 0 1\n"
               "slot=1\n"
               "T 600 1300 5 10 0 0 0 0 1\n"
               "numslots=6\n"
               "T 600 1500 5 10 0 0 0 0 1\n"
               "slotdef=1:1,2\n"
               "T 600 1700 5 10 0 0 0 0 1\n"
               "slotdef=2:3,4\n"
               "T 600 1900 5 10 0 0 0 0 1\n"
               "slotdef=3:5,6\n"
               "T 600 2100 5 10 0 0 0 0 1\n"
               "slotdef=4:9,8\n"
               "T 600 2300 5 10 0 0 0 0 1\n"
               "slotdef=5:11,10\n"
               "T 600 2500 5 10 0 0 0 0 1\n"
               "slotdef=6:13,12\n"
               "L 800 500 300 200 3 0 0 0 -1 -1\n"
               "L 300 800 300 200 3 0 0 0 -1 -1\n"
               "V 850 500 50 6 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "P 300 500 0 500 1 0 1\n"
               "{\n"
               "T 200 550 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 200 450 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 350 500 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 350 500 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 1100 500 900 500 1 0 0\n"
               "{\n"
               "T 900 550 5 8 1 1 0 0 1\n"
               "pinnumber=2\n"
               "T 900 450 5 8 0 1 0 2 1\n"
               "pinseq=2\n"
               "T 750 500 9 8 0 1 0 6 1\n"
               "pinlabel=Y\n"
               "T 750 500 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "T 300 0 9 8 1 0 0 0 1\n"
               "7404\n"
               "T 300 900 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 600 3100 5 10 0 0 0 0 1\n"
               "net=Vcc:14\n"
               "T 600 3300 5 10 0 0 0 0 1\n"
               "net=GND:7\n" ) },

        { wxT( "7408-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 300 200 300 800 3 0 0 0 -1 -1\n"
               "T 300 0 9 8 1 0 0 0 1\n"
               "7408\n"
               "L 300 800 700 800 3 0 0 0 -1 -1\n"
               "T 700 900 5 10 0 0 0 0 1\n"
               "device=7408\n"
               "T 700 1100 5 10 0 0 0 0 1\n"
               "slot=1\n"
               "T 700 1300 5 10 0 0 0 0 1\n"
               "numslots=4\n"
               "T 700 1500 5 10 0 0 0 0 1\n"
               "slotdef=1:1,2,3\n"
               "T 700 1700 5 10 0 0 0 0 1\n"
               "slotdef=2:4,5,6\n"
               "T 700 1900 5 10 0 0 0 0 1\n"
               "slotdef=3:9,10,8\n"
               "T 700 2100 5 10 0 0 0 0 1\n"
               "slotdef=4:12,13,11\n"
               "L 300 200 700 200 3 0 0 0 -1 -1\n"
               "A 700 500 300 270 180 3 0 0 0 -1 -1\n"
               "P 1000 500 1300 500 1 0 1\n"
               "{\n"
               "T 1100 550 5 8 1 1 0 0 1\n"
               "pinnumber=3\n"
               "T 1100 450 5 8 0 1 0 2 1\n"
               "pinseq=3\n"
               "T 950 500 9 8 0 1 0 6 1\n"
               "pinlabel=Y\n"
               "T 950 500 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "P 300 700 0 700 1 0 1\n"
               "{\n"
               "T 200 750 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 200 650 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 350 700 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 350 700 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 300 300 0 300 1 0 1\n"
               "{\n"
               "T 200 350 5 8 1 1 0 6 1\n"
               "pinnumber=2\n"
               "T 200 250 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 350 300 9 8 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 350 300 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "T 300 900 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 700 2700 5 10 0 0 0 0 1\n"
               "net=Vcc:14\n"
               "T 700 2900 5 10 0 0 0 0 1\n"
               "net=GND:7\n" ) },

        { wxT( "7432-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 260 200 600 200 3 0 0 0 -1 -1\n"
               "L 260 800 600 800 3 0 0 0 -1 -1\n"
               "T 600 900 5 10 0 0 0 0 1\n"
               "device=7432\n"
               "T 600 1100 5 10 0 0 0 0 1\n"
               "slot=1\n"
               "T 600 1300 5 10 0 0 0 0 1\n"
               "numslots=4\n"
               "T 600 1500 5 10 0 0 0 0 1\n"
               "slotdef=1:1,2,3\n"
               "T 600 1700 5 10 0 0 0 0 1\n"
               "slotdef=2:4,5,6\n"
               "T 600 1900 5 10 0 0 0 0 1\n"
               "slotdef=3:9,10,8\n"
               "T 600 2100 5 10 0 0 0 0 1\n"
               "slotdef=4:12,13,11\n"
               "T 300 0 9 8 1 0 0 0 1\n"
               "7432\n"
               "A 0 500 400 312 97 3 0 0 0 -1 -1\n"
               "P 300 700 0 700 1 0 1\n"
               "{\n"
               "T 200 750 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 200 650 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 350 700 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 350 700 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 300 300 0 300 1 0 1\n"
               "{\n"
               "T 200 350 5 8 1 1 0 6 1\n"
               "pinnumber=2\n"
               "T 200 250 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 350 300 9 8 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 350 300 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 1300 500 988 500 1 0 0\n"
               "{\n"
               "T 1100 550 5 8 1 1 0 0 1\n"
               "pinnumber=3\n"
               "T 1100 450 5 8 0 1 0 2 1\n"
               "pinseq=3\n"
               "T 950 500 9 8 0 1 0 6 1\n"
               "pinlabel=Y\n"
               "T 950 500 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "A 600 600 400 270 76 3 0 0 0 -1 -1\n"
               "A 600 400 400 14 76 3 0 0 0 -1 -1\n"
               "T 300 900 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 600 2700 5 10 0 0 0 0 1\n"
               "net=Vcc:14\n"
               "T 600 2900 5 10 0 0 0 0 1\n"
               "net=GND:7\n" ) },

        { wxT( "7486-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 260 200 600 200 3 0 0 0 -1 -1\n"
               "L 260 800 600 800 3 0 0 0 -1 -1\n"
               "T 700 900 5 10 0 0 0 0 1\n"
               "device=7486\n"
               "T 700 1100 5 10 0 0 0 0 1\n"
               "slot=1\n"
               "T 700 1300 5 10 0 0 0 0 1\n"
               "numslots=4\n"
               "T 700 1500 5 10 0 0 0 0 1\n"
               "slotdef=1:1,2,3\n"
               "T 700 1700 5 10 0 0 0 0 1\n"
               "slotdef=2:4,5,6\n"
               "T 700 1900 5 10 0 0 0 0 1\n"
               "slotdef=3:9,10,8\n"
               "T 700 2100 5 10 0 0 0 0 1\n"
               "slotdef=4:12,13,11\n"
               "A 0 500 400 312 97 3 0 0 0 -1 -1\n"
               "A -100 500 400 312 97 3 0 0 0 -1 -1\n"
               "P 300 700 0 700 1 0 1\n"
               "{\n"
               "T 150 750 5 8 1 1 0 6 1\n"
               "pinnumber=1\n"
               "T 150 650 5 8 0 1 0 8 1\n"
               "pinseq=1\n"
               "T 350 700 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 350 700 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 300 300 0 300 1 0 1\n"
               "{\n"
               "T 150 350 5 8 1 1 0 6 1\n"
               "pinnumber=2\n"
               "T 150 250 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 350 300 9 8 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 350 300 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 988 500 1300 500 1 0 1\n"
               "{\n"
               "T 1100 550 5 8 1 1 0 0 1\n"
               "pinnumber=3\n"
               "T 1100 450 5 8 0 1 0 2 1\n"
               "pinseq=3\n"
               "T 950 500 9 8 0 1 0 6 1\n"
               "pinlabel=Y\n"
               "T 950 500 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "A 600 600 400 270 76 3 0 0 0 -1 -1\n"
               "A 600 400 400 14 76 3 0 0 0 -1 -1\n"
               "T 300 900 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 300 0 9 8 1 0 0 0 1\n"
               "7486\n"
               "T 700 2700 5 10 0 0 0 0 1\n"
               "net=Vcc:14\n"
               "T 700 2900 5 10 0 0 0 0 1\n"
               "net=GND:7\n" ) },

        { wxT( "7402-1.sym" ),
          wxT(                "v 20031231 1\n"
               "L 260 200 600 200 3 0 0 0 -1 -1\n"
               "L 260 800 600 800 3 0 0 0 -1 -1\n"
               "T 600 900 5 10 0 0 0 0 1\n"
               "device=7402\n"
               "T 600 1100 5 10 0 0 0 0 1\n"
               "slot=1\n"
               "T 600 1300 5 10 0 0 0 0 1\n"
               "numslots=4\n"
               "T 600 1500 5 10 0 0 0 0 1\n"
               "slotdef=1:1,2,3\n"
               "T 600 1700 5 10 0 0 0 0 1\n"
               "slotdef=2:4,5,6\n"
               "T 600 1900 5 10 0 0 0 0 1\n"
               "slotdef=3:10,8,9\n"
               "T 600 2100 5 10 0 0 0 0 1\n"
               "slotdef=4:13,11,12\n"
               "T 300 0 9 8 1 0 0 0 1\n"
               "7402\n"
               "A 0 500 400 312 97 3 0 0 0 -1 -1\n"
               "P 300 700 0 700 1 0 1\n"
               "{\n"
               "T 200 750 5 8 1 1 0 6 1\n"
               "pinnumber=3\n"
               "T 200 650 5 8 0 1 0 8 1\n"
               "pinseq=3\n"
               "T 350 700 9 8 0 1 0 0 1\n"
               "pinlabel=B\n"
               "T 350 700 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "P 300 300 0 300 1 0 1\n"
               "{\n"
               "T 200 350 5 8 1 1 0 6 1\n"
               "pinnumber=2\n"
               "T 200 250 5 8 0 1 0 8 1\n"
               "pinseq=2\n"
               "T 350 300 9 8 0 1 0 0 1\n"
               "pinlabel=A\n"
               "T 350 300 5 8 0 1 0 2 1\n"
               "pintype=in\n"
               "}\n"
               "V 1038 500 50 6 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "P 1300 500 1088 500 1 0 0\n"
               "{\n"
               "T 1100 550 5 8 1 1 0 0 1\n"
               "pinnumber=1\n"
               "T 1100 450 5 8 0 1 0 2 1\n"
               "pinseq=1\n"
               "T 950 500 9 8 0 1 0 6 1\n"
               "pinlabel=Y\n"
               "T 950 500 5 8 0 1 0 8 1\n"
               "pintype=out\n"
               "}\n"
               "A 600 600 400 270 76 3 0 0 0 -1 -1\n"
               "A 600 400 400 14 76 3 0 0 0 -1 -1\n"
               "T 300 900 8 10 1 1 0 0 1\n"
               "refdes=U?\n"
               "T 600 2700 5 10 0 0 0 0 1\n"
               "net=Vcc:14\n"
               "T 600 2900 5 10 0 0 0 0 1\n"
               "net=GND:7\n" ) },

        // Miscellaneous
        { wxT( "busripper-1.sym" ),
          wxT(                "v 20031231 1\n"
               "T 0 400 5 8 0 0 0 0 1\n"
               "device=none\n"
               "P 0 0 100 100 1 0 0\n"
               "{\n"
               "T 0 500 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 0 600 5 8 0 0 0 0 1\n"
               "pinnumber=1\n"
               "T 0 700 5 8 0 0 0 0 1\n"
               "pintype=pas\n"
               "T 0 800 5 8 0 0 0 0 1\n"
               "pinlabel=netside\n"
               "}\n"
               "T 0 300 5 8 0 0 0 0 1\n"
               "graphical=1\n"
               "L 200 200 100 100 10 30 0 0 -1 -1\n" ) },

        { wxT( "busripper-2.sym" ),
          wxT(                "v 20031231 1\n"
               "T 0 400 5 8 0 0 0 0 1\n"
               "device=none\n"
               "P 0 0 0 100 1 0 0\n"
               "{\n"
               "T 0 500 5 8 0 0 0 0 1\n"
               "pinseq=1\n"
               "T 0 600 5 8 0 0 0 0 1\n"
               "pinnumber=1\n"
               "T 0 700 5 8 0 0 0 0 1\n"
               "pintype=pas\n"
               "T 0 800 5 8 0 0 0 0 1\n"
               "pinlabel=netside\n"
               "}\n"
               "T 0 300 5 8 0 0 0 0 1\n"
               "graphical=1\n"
               "L 0 100 100 200 3 0 0 0 -1 -1\n"
               "L 0 100 -100 200 3 0 0 0 -1 -1\n" ) },

        { wxT( "title-B.sym" ),
          wxT(                "v 20031231 1\n"
               "B 0 0 17000 11000 15 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "T 14400 1500 5 10 0 0 0 0 1\n"
               "graphical=1\n"
               "L 12900 600 12900 0 15 0 0 0 -1 -1\n"
               "T 9500 400 15 8 1 0 0 0 1\n"
               "FILE:\n"
               "T 13000 400 15 8 1 0 0 0 1\n"
               "REVISION:\n"
               "T 13000 100 15 8 1 0 0 0 1\n"
               "DRAWN BY:\n"
               "T 9500 100 15 8 1 0 0 0 1\n"
               "PAGE\n"
               "T 11200 100 15 8 1 0 0 0 1\n"
               "OF\n"
               "T 9500 700 15 8 1 0 0 0 1\n"
               "TITLE\n"
               "B 9400 0 7600 1400 15 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n"
               "L 9400 600 17000 600 15 0 0 0 -1 -1\n" ) },
    };

    return symbols;
}


// ==========================================================================
// Construction
// ==========================================================================

SCH_IO_GEDA::SCH_IO_GEDA() :
        SCH_IO( wxS( "gEDA/gschem Schematic" ) ),
        m_screen( nullptr ),
        m_rootSheet( nullptr ),
        m_schematic( nullptr ),
        m_maxY( 0 ),
        m_releaseVersion( 0 ),
        m_fileFormatVersion( 0 ),
        m_symLibraryInitialized( false ),
        m_powerCounter( 0 ),
        m_properties( nullptr )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_GEDA::~SCH_IO_GEDA() = default;


// ==========================================================================
// Format detection
// ==========================================================================

bool SCH_IO_GEDA::CanReadSchematicFile( const wxString& aFileName ) const
{
    wxFileName fn( aFileName );

    if( fn.GetExt().CmpNoCase( wxT( "sch" ) ) != 0 )
        return false;

    wxTextFile file;

    if( !file.Open( aFileName ) )
        return false;

    if( file.GetLineCount() == 0 )
        return false;

    wxString firstLine = file.GetFirstLine().Trim( false );

    if( !firstLine.StartsWith( wxT( "v " ) ) )
        return false;

    wxStringTokenizer tok( firstLine );
    tok.GetNextToken(); // skip 'v'

    if( !tok.HasMoreTokens() )
        return false;

    wxString dateStr = tok.GetNextToken();

    if( dateStr.length() != 8 )
        return false;

    long dateVal = 0;
    return dateStr.ToLong( &dateVal ) && dateVal >= 19700101;
}


// ==========================================================================
// Coordinate transformation
// ==========================================================================

int SCH_IO_GEDA::toKiCadDist( int aMils ) const
{
    return static_cast<int>( static_cast<int64_t>( aMils ) * MILS_TO_IU );
}


VECTOR2I SCH_IO_GEDA::toKiCad( int aGedaX, int aGedaY ) const
{
    return VECTOR2I( aGedaX * MILS_TO_IU, ( m_maxY - aGedaY ) * MILS_TO_IU );
}


// ==========================================================================
// Version parsing
// ==========================================================================

bool SCH_IO_GEDA::parseVersionLine( const wxString& aLine )
{
    wxStringTokenizer tokenizer( aLine );

    if( tokenizer.CountTokens() < 2 )
        return false;

    wxString v = tokenizer.GetNextToken();

    if( v != wxT( "v" ) )
        return false;

    wxString dateStr = tokenizer.GetNextToken();
    dateStr.ToLong( &m_releaseVersion );

    if( m_releaseVersion < 19700101 )
        return false;

    // File format version is optional in very old files
    if( tokenizer.HasMoreTokens() )
        tokenizer.GetNextToken().ToLong( &m_fileFormatVersion );
    else
        m_fileFormatVersion = 0;

    return true;
}


// ==========================================================================
// Attribute parsing
// ==========================================================================

std::vector<SCH_IO_GEDA::GEDA_ATTR> SCH_IO_GEDA::parseAttributes( wxTextFile& aFile,
                                                                     size_t&     aLineIdx )
{
    std::vector<GEDA_ATTR> attrs;

    while( aLineIdx < aFile.GetLineCount() )
    {
        wxString line = aFile.GetLine( aLineIdx );

        if( line.Trim() == wxT( "}" ) )
        {
            aLineIdx++;
            break;
        }

        if( line.StartsWith( wxT( "T " ) ) )
        {
            // T x y color size visibility show_name_value angle alignment num_lines
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip 'T'

            long x = 0, y = 0, color = 0, size = 0, vis = 0, showNV = 0, angle = 0, align = 0;
            long numLines = 1;

            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &size );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &vis );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &showNV );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &angle );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &align );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &numLines );

            aLineIdx++;

            wxString textContent;

            for( long i = 0; i < numLines && aLineIdx < aFile.GetLineCount(); i++ )
            {
                if( i > 0 )
                    textContent += wxT( "\n" );

                textContent += aFile.GetLine( aLineIdx );
                aLineIdx++;
            }

            int eqPos = textContent.Find( '=' );

            if( eqPos != wxNOT_FOUND )
            {
                GEDA_ATTR attr;
                attr.name    = textContent.Left( eqPos );
                attr.value   = textContent.Mid( eqPos + 1 );

                // Strip surrounding double quotes from attribute values
                if( attr.value.length() >= 2
                    && attr.value.StartsWith( wxT( "\"" ) )
                    && attr.value.EndsWith( wxT( "\"" ) ) )
                {
                    attr.value = attr.value.Mid( 1, attr.value.length() - 2 );
                }

                attr.x       = static_cast<int>( x );
                attr.y       = static_cast<int>( y );
                attr.size    = static_cast<int>( size );
                attr.angle   = static_cast<int>( angle );
                attr.align   = static_cast<int>( align );
                attr.visible = ( vis == 1 );
                attr.showNV  = static_cast<int>( showNV );
                attrs.push_back( attr );
            }

            continue;
        }

        aLineIdx++;
    }

    return attrs;
}


wxString SCH_IO_GEDA::findAttr( const std::vector<GEDA_ATTR>& aAttrs,
                                 const wxString&                aName ) const
{
    for( const GEDA_ATTR& attr : aAttrs )
    {
        if( attr.name == aName )
            return attr.value;
    }

    return wxEmptyString;
}


const SCH_IO_GEDA::GEDA_ATTR* SCH_IO_GEDA::findAttrStruct( const std::vector<GEDA_ATTR>& aAttrs,
                                                              const wxString& aName ) const
{
    for( const GEDA_ATTR& attr : aAttrs )
    {
        if( attr.name == aName )
            return &attr;
    }

    return nullptr;
}


std::vector<SCH_IO_GEDA::GEDA_ATTR> SCH_IO_GEDA::maybeParseAttributes( wxTextFile& aFile,
                                                                          size_t&     aLineIdx )
{
    if( aLineIdx < aFile.GetLineCount() && aFile.GetLine( aLineIdx ).Trim() == wxT( "{" ) )
    {
        aLineIdx++;
        return parseAttributes( aFile, aLineIdx );
    }

    return {};
}


// ==========================================================================
// Style mapping
// ==========================================================================

LINE_STYLE SCH_IO_GEDA::toLineStyle( int aDashStyle )
{
    switch( aDashStyle )
    {
    case 1:  return LINE_STYLE::DOT;
    case 2:  return LINE_STYLE::DASH;
    case 3:  return LINE_STYLE::DASHDOT;
    case 4:  return LINE_STYLE::DASHDOTDOT;
    default: return LINE_STYLE::DEFAULT;
    }
}


FILL_T SCH_IO_GEDA::toFillType( int aFillType )
{
    switch( aFillType )
    {
    case 1:  return FILL_T::FILLED_SHAPE;
    case 2:  return FILL_T::CROSS_HATCH;
    case 3:  return FILL_T::HATCH;
    default: return FILL_T::NO_FILL;
    }
}


// ==========================================================================
// Orientation mapping
// ==========================================================================

int SCH_IO_GEDA::toKiCadOrientation( int aAngle, int aMirror ) const
{
    int orientation = SYM_ORIENT_0;

    switch( aAngle )
    {
    case 0:   orientation = SYM_ORIENT_0;   break;
    case 90:  orientation = SYM_ORIENT_90;  break;
    case 180: orientation = SYM_ORIENT_180; break;
    case 270: orientation = SYM_ORIENT_270; break;
    default:  orientation = SYM_ORIENT_0;   break;
    }

    if( aMirror )
        orientation |= SYM_MIRROR_Y;

    return orientation;
}


wxString SCH_IO_GEDA::getLibName() const
{
    wxString libName;

    if( m_schematic )
        libName = m_schematic->Project().GetProjectName();

    if( libName.IsEmpty() )
        libName = m_filename.GetName();

    if( libName.IsEmpty() )
        libName = wxT( "noname" );

    libName += wxT( "-geda-import" );
    libName = LIB_ID::FixIllegalChars( libName, true ).wx_str();
    return libName;
}


// ==========================================================================
// Object parsers - connectivity
// ==========================================================================

void SCH_IO_GEDA::parseComponent( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // Flush any previous pending component before starting a new one
    flushPendingComponent();

    // C x y selectable angle mirror basename.sym
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'C'

    long x = 0, y = 0, selectable = 0, angle = 0, mirror = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &selectable );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &angle );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &mirror );

    wxString basename;

    if( tok.HasMoreTokens() )
        basename = tok.GetNextToken();

    // gEDA prefixes embedded component basenames with "EMBEDDED"
    if( basename.StartsWith( wxT( "EMBEDDED" ) ) )
        basename = basename.Mid( 8 );

    m_pendingComp = std::make_unique<PENDING_COMPONENT>();
    m_pendingComp->basename   = basename;
    m_pendingComp->x          = static_cast<int>( x );
    m_pendingComp->y          = static_cast<int>( y );
    m_pendingComp->selectable = static_cast<int>( selectable );
    m_pendingComp->angle      = static_cast<int>( angle );
    m_pendingComp->mirror     = static_cast<int>( mirror );

    m_pendingComp->attrs = maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parseNet( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // N x1 y1 x2 y2 color
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'N'

    long x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y2 );

    VECTOR2I start = toKiCad( static_cast<int>( x1 ), static_cast<int>( y1 ) );
    VECTOR2I end = toKiCad( static_cast<int>( x2 ), static_cast<int>( y2 ) );

    auto wire = std::make_unique<SCH_LINE>( start, LAYER_WIRE );
    wire->SetEndPoint( end );
    m_screen->Append( wire.release() );

    trackEndpoint( static_cast<int>( x1 ), static_cast<int>( y1 ) );
    trackEndpoint( static_cast<int>( x2 ), static_cast<int>( y2 ) );

    std::vector<GEDA_ATTR> attrs = maybeParseAttributes( aFile, aLineIdx );
    wxString netname = findAttr( attrs, wxT( "netname" ) );

    if( !netname.IsEmpty() )
    {
        auto label = std::make_unique<SCH_LABEL>( start, netname );
        int textSize = toKiCadDist( GEDA_DEFAULT_TEXT_SIZE_MILS / 2 );
        label->SetTextSize( VECTOR2I( textSize, textSize ) );
        m_screen->Append( label.release() );
    }
}


void SCH_IO_GEDA::parseBus( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // U x1 y1 x2 y2 color [ripperdir]
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'U'

    long x1 = 0, y1 = 0, x2 = 0, y2 = 0, color = 0, ripperDir = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );

    if( m_releaseVersion > 20020825 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &ripperDir );
    }

    VECTOR2I start = toKiCad( static_cast<int>( x1 ), static_cast<int>( y1 ) );
    VECTOR2I end = toKiCad( static_cast<int>( x2 ), static_cast<int>( y2 ) );

    auto line = std::make_unique<SCH_LINE>( start, LAYER_BUS );
    line->SetEndPoint( end );
    m_screen->Append( line.release() );

    m_busSegments.push_back( { start, end, static_cast<int>( ripperDir ) } );

    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parsePin( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // P x1 y1 x2 y2 color [pintype whichend]
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'P'

    long x1 = 0, y1 = 0, x2 = 0, y2 = 0, color = 0, pintype = 0, whichend = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );

    if( m_releaseVersion > 20020825 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pintype );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &whichend );
    }

    VECTOR2I start = toKiCad( static_cast<int>( x1 ), static_cast<int>( y1 ) );
    VECTOR2I end = toKiCad( static_cast<int>( x2 ), static_cast<int>( y2 ) );

    // In schematic context, pin lines become wire stubs for connectivity
    auto wire = std::make_unique<SCH_LINE>( start, LAYER_WIRE );
    wire->SetEndPoint( end );
    m_screen->Append( wire.release() );

    VECTOR2I connPt = ( whichend == 0 ) ? start : end;
    int connX = ( whichend == 0 ) ? static_cast<int>( x1 ) : static_cast<int>( x2 );
    int connY = ( whichend == 0 ) ? static_cast<int>( y1 ) : static_cast<int>( y2 );
    trackEndpoint( connX, connY );

    std::vector<GEDA_ATTR> attrs = maybeParseAttributes( aFile, aLineIdx );

    wxString pinlabel = findAttr( attrs, wxT( "pinlabel" ) );

    if( !pinlabel.IsEmpty() )
    {
        auto label = std::make_unique<SCH_LABEL>( connPt, pinlabel );
        int textSize = toKiCadDist( GEDA_DEFAULT_TEXT_SIZE_MILS / 2 );
        label->SetTextSize( VECTOR2I( textSize, textSize ) );
        m_screen->Append( label.release() );
    }
}


void SCH_IO_GEDA::parseEmbeddedComponent( wxTextFile& aFile, size_t& aLineIdx )
{
    if( !m_pendingComp )
        return;

    auto libSym = std::make_unique<LIB_SYMBOL>( m_pendingComp->basename );

    // Parse until ] closing bracket
    while( aLineIdx < aFile.GetLineCount() )
    {
        wxString line = aFile.GetLine( aLineIdx );

        if( line.Trim() == wxT( "]" ) )
        {
            aLineIdx++;
            break;
        }

        if( line.IsEmpty() )
        {
            aLineIdx++;
            continue;
        }

        wxChar type = line[0];
        aLineIdx++;

        switch( type )
        {
        case 'P':
        {
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip 'P'
            long px1 = 0, py1 = 0, px2 = 0, py2 = 0, pc = 0, pt = 0, pw = 0;

            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &px1 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &py1 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &px2 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &py2 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pc );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pt );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pw );

            std::vector<GEDA_ATTR> pinAttrs = maybeParseAttributes( aFile, aLineIdx );
            addSymbolPin( *libSym, static_cast<int>( px1 ), static_cast<int>( py1 ),
                          static_cast<int>( px2 ), static_cast<int>( py2 ),
                          static_cast<int>( pw ), pinAttrs );
            break;
        }

        case 'L':
        case 'B':
        case 'V':
        case 'A':
        case 'H':
            addSymbolGraphic( *libSym, line, aFile, aLineIdx, type );
            break;

        case 'T':
        {
            // Skip text lines in embedded symbols (they're attribute text)
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip 'T'
            long numLines = 1;

            // Parse enough to get numLines (9th field)
            for( int i = 0; i < 8 && tok.HasMoreTokens(); i++ )
            {
                if( i == 7 )
                    tok.GetNextToken().ToLong( &numLines );
                else
                    tok.GetNextToken();
            }

            for( long i = 0; i < numLines && aLineIdx < aFile.GetLineCount(); i++ )
                aLineIdx++;

            maybeParseAttributes( aFile, aLineIdx );
            break;
        }

        case '{':
        {
            // Skip nested attribute block
            while( aLineIdx < aFile.GetLineCount() )
            {
                if( aFile.GetLine( aLineIdx ).Trim() == wxT( "}" ) )
                {
                    aLineIdx++;
                    break;
                }

                aLineIdx++;
            }

            break;
        }

        case 'v':
        default:
            break;
        }
    }

    m_pendingComp->embedded = true;
    m_pendingComp->embeddedSym = std::move( libSym );
}


// ==========================================================================
// Object parsers - graphics
// ==========================================================================

void SCH_IO_GEDA::parseText( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // T x y color size visibility show_name_value angle alignment num_lines
    // Old formats may omit alignment and/or num_lines fields
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'T'

    long x = 0, y = 0, color = 0, size = 0, vis = 0, showNV = 0, angle = 0, align = 0;
    long numLines = 1;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &size );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &vis );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &showNV );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &angle );

    if( m_fileFormatVersion >= 1 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &align );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &numLines );
    }
    else if( m_releaseVersion >= 20000220 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &align );

        numLines = 1;
    }
    else
    {
        align = 0;
        numLines = 1;
    }

    wxString textContent;

    for( long i = 0; i < numLines && aLineIdx < aFile.GetLineCount(); i++ )
    {
        if( i > 0 )
            textContent += wxT( "\n" );

        textContent += aFile.GetLine( aLineIdx );
        aLineIdx++;
    }

    textContent = convertOverbars( textContent );

    if( vis == 0 )
    {
        maybeParseAttributes( aFile, aLineIdx );
        return;
    }

    // Handle attribute text (contains '=') with show_name_value field
    int eqPos = textContent.Find( '=' );

    if( eqPos != wxNOT_FOUND )
    {
        wxString name = textContent.Left( eqPos );
        wxString value = textContent.Mid( eqPos + 1 );

        switch( static_cast<int>( showNV ) )
        {
        case 1:  textContent = value;                          break;
        case 2:  textContent = name;                           break;
        default: textContent = name + wxT( "=" ) + value;     break;
        }
    }

    VECTOR2I pos = toKiCad( static_cast<int>( x ), static_cast<int>( y ) );

    auto text = std::make_unique<SCH_TEXT>( pos, textContent );

    int textSize = toKiCadDist( static_cast<int>( size ) * 10 );

    if( textSize < toKiCadDist( 10 ) )
        textSize = toKiCadDist( 10 );

    text->SetTextSize( VECTOR2I( textSize, textSize ) );

    // gEDA allows arbitrary angles but KiCad text only supports orthogonal.
    // Snap to the nearest 90-degree increment.
    int normAngle = ( ( static_cast<int>( angle ) % 360 ) + 360 ) % 360;
    int snapped = ( ( normAngle + 45 ) / 90 ) * 90;

    if( snapped == 360 )
        snapped = 0;

    switch( snapped )
    {
    case 90:  text->SetTextAngle( ANGLE_90 );  break;
    case 180: text->SetTextAngle( ANGLE_180 ); break;
    case 270: text->SetTextAngle( ANGLE_270 ); break;
    default:  break;
    }

    // Horizontal alignment: gEDA align / 3 gives column (0=left, 1=center, 2=right)
    switch( static_cast<int>( align ) / 3 )
    {
    case 1: text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER ); break;
    case 2: text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );  break;
    default: text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );  break;
    }

    // Vertical alignment: gEDA align % 3 gives row (0=bottom, 1=middle, 2=top)
    switch( static_cast<int>( align ) % 3 )
    {
    case 1: text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER ); break;
    case 2: text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );    break;
    default: text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM ); break;
    }

    m_screen->Append( text.release() );
    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parseLine( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // L x1 y1 x2 y2 color [width capstyle dashstyle dashlength dashspace]
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'L'

    long x1 = 0, y1 = 0, x2 = 0, y2 = 0, color = 0, width = 0;
    long capstyle = 0, dashstyle = 0, dashlength = 0, dashspace = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );

    if( m_releaseVersion > 20000704 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &width );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
    }

    VECTOR2I start = toKiCad( static_cast<int>( x1 ), static_cast<int>( y1 ) );
    VECTOR2I end = toKiCad( static_cast<int>( x2 ), static_cast<int>( y2 ) );

    auto line = std::make_unique<SCH_LINE>( start, LAYER_NOTES );
    line->SetEndPoint( end );
    line->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( width ) ),
                                     toLineStyle( static_cast<int>( dashstyle ) ) ) );
    m_screen->Append( line.release() );
    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parseBox( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // B x y width height color [linewidth capstyle dashstyle dashlength dashspace
    //   filltype fillwidth a1 p1 a2 p2]
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'B'

    long x = 0, y = 0, w = 0, h = 0, color = 0, linewidth = 0;
    long capstyle = 0, dashstyle = 0, dashlength = 0, dashspace = 0;
    long filltype = 0, fillwidth = 0, a1 = 0, p1 = 0, a2 = 0, p2 = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &w );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &h );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );

    if( m_releaseVersion > 20000704 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &linewidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &filltype );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &fillwidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a2 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p2 );
    }

    VECTOR2I topLeft = toKiCad( static_cast<int>( x ), static_cast<int>( y + h ) );
    VECTOR2I botRight = toKiCad( static_cast<int>( x + w ), static_cast<int>( y ) );

    auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_NOTES );
    rect->SetStart( topLeft );
    rect->SetEnd( botRight );
    rect->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( linewidth ) ),
                                     toLineStyle( static_cast<int>( dashstyle ) ) ) );

    FILL_T fill = toFillType( static_cast<int>( filltype ) );

    if( fill != FILL_T::NO_FILL )
    {
        rect->SetFilled( true );
        rect->SetFillMode( fill );
    }

    m_screen->Append( rect.release() );
    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parseCircle( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // V cx cy radius color [linewidth capstyle dashstyle dashlength dashspace
    //   filltype fillwidth a1 p1 a2 p2]
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'V'

    long cx = 0, cy = 0, radius = 0, color = 0, linewidth = 0;
    long capstyle = 0, dashstyle = 0, dashlength = 0, dashspace = 0;
    long filltype = 0, fillwidth = 0, a1 = 0, p1 = 0, a2 = 0, p2 = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cx );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cy );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &radius );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );

    if( m_releaseVersion > 20000704 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &linewidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &filltype );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &fillwidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a2 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p2 );
    }

    VECTOR2I center = toKiCad( static_cast<int>( cx ), static_cast<int>( cy ) );

    auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_NOTES );
    circle->SetCenter( center );
    circle->SetEnd( VECTOR2I( center.x + toKiCadDist( static_cast<int>( radius ) ), center.y ) );
    circle->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( linewidth ) ),
                                       toLineStyle( static_cast<int>( dashstyle ) ) ) );

    FILL_T fill = toFillType( static_cast<int>( filltype ) );

    if( fill != FILL_T::NO_FILL )
    {
        circle->SetFilled( true );
        circle->SetFillMode( fill );
    }

    m_screen->Append( circle.release() );
    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parseArc( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // A cx cy radius startangle sweepangle color [linewidth capstyle dashstyle dashlength dashspace]
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'A'

    long cx = 0, cy = 0, radius = 0, startAngle = 0, sweepAngle = 0;
    long color = 0, linewidth = 0, capstyle = 0, dashstyle = 0;
    long dashlength = 0, dashspace = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cx );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cy );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &radius );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &startAngle );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &sweepAngle );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );

    if( m_releaseVersion > 20000704 )
    {
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &linewidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
    }

    VECTOR2I center = toKiCad( static_cast<int>( cx ), static_cast<int>( cy ) );
    int r = toKiCadDist( static_cast<int>( radius ) );

    double startRad = DEG2RAD( static_cast<double>( startAngle ) );
    double endRad = DEG2RAD( static_cast<double>( startAngle + sweepAngle ) );

    // Y-flip negates the Y component of angle calculations
    VECTOR2I arcStart( center.x + KiROUND( r * cos( startRad ) ),
                       center.y - KiROUND( r * sin( startRad ) ) );
    VECTOR2I arcEnd( center.x + KiROUND( r * cos( endRad ) ),
                     center.y - KiROUND( r * sin( endRad ) ) );

    auto arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_NOTES );
    arc->SetCenter( center );

    // Y-flip reverses angular sweep direction, so swap start and end
    // to preserve the original arc visual appearance.
    arc->SetStart( arcEnd );
    arc->SetEnd( arcStart );

    arc->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( linewidth ) ),
                                    toLineStyle( static_cast<int>( dashstyle ) ) ) );
    m_screen->Append( arc.release() );
    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parsePath( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // H color width capstyle dashstyle dashlength dashspace
    //   filltype fillwidth a1 p1 a2 p2 numlines
    // followed by numlines of SVG-style path data
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'H'

    long color = 0, width = 0, capstyle = 0, dashstyle = 0;
    long dashlength = 0, dashspace = 0, filltype = 0, fillwidth = 0;
    long a1 = 0, p1 = 0, a2 = 0, p2 = 0, numlines = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &width );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &filltype );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &fillwidth );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p1 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p2 );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &numlines );

    // Collect all path data lines
    wxString pathData;

    for( long i = 0; i < numlines && aLineIdx < aFile.GetLineCount(); i++ )
    {
        if( i > 0 )
            pathData += wxT( " " );

        pathData += aFile.GetLine( aLineIdx ).Trim();
        aLineIdx++;
    }

    VECTOR2I startPt;
    VECTOR2I currentPt;
    STROKE_PARAMS stroke( toKiCadDist( static_cast<int>( width ) ),
                          toLineStyle( static_cast<int>( dashstyle ) ) );

    auto emitSeg = [&]( const VECTOR2I& aFrom, const VECTOR2I& aTo )
    {
        auto seg = std::make_unique<SCH_LINE>( aFrom, LAYER_NOTES );
        seg->SetEndPoint( aTo );
        seg->SetStroke( stroke );
        m_screen->Append( seg.release() );
    };

    wxStringTokenizer pathTok( pathData, wxT( " ,\t" ) );

    // For relative (lowercase) SVG commands, deltas are in gEDA mils (Y-up).
    // Convert to KiCad IU (Y-down) by scaling X and negating+scaling Y.
    auto relToAbs = [&]( long dx, long dy ) -> VECTOR2I
    {
        return currentPt + VECTOR2I( static_cast<int>( dx ) * MILS_TO_IU,
                                     -static_cast<int>( dy ) * MILS_TO_IU );
    };

    while( pathTok.HasMoreTokens() )
    {
        wxString token = pathTok.GetNextToken();

        if( token == wxT( "M" ) )
        {
            long px = 0, py = 0;

            if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &px )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &py ) )
            {
                currentPt = toKiCad( static_cast<int>( px ), static_cast<int>( py ) );
                startPt = currentPt;
            }
        }
        else if( token == wxT( "m" ) )
        {
            long dx = 0, dy = 0;

            if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &dx )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &dy ) )
            {
                currentPt = relToAbs( dx, dy );
                startPt = currentPt;
            }
        }
        else if( token == wxT( "L" ) )
        {
            long px = 0, py = 0;

            if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &px )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &py ) )
            {
                VECTOR2I pt = toKiCad( static_cast<int>( px ), static_cast<int>( py ) );
                emitSeg( currentPt, pt );
                currentPt = pt;
            }
        }
        else if( token == wxT( "l" ) )
        {
            long dx = 0, dy = 0;

            if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &dx )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &dy ) )
            {
                VECTOR2I pt = relToAbs( dx, dy );
                emitSeg( currentPt, pt );
                currentPt = pt;
            }
        }
        else if( token == wxT( "C" ) )
        {
            long cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0, px = 0, py = 0;

            if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cx1 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cy1 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cx2 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cy2 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &px )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &py ) )
            {
                VECTOR2I cp1 = toKiCad( static_cast<int>( cx1 ), static_cast<int>( cy1 ) );
                VECTOR2I cp2 = toKiCad( static_cast<int>( cx2 ), static_cast<int>( cy2 ) );
                VECTOR2I endPt = toKiCad( static_cast<int>( px ), static_cast<int>( py ) );

                auto bezier = std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER, LAYER_NOTES );
                bezier->SetStart( currentPt );
                bezier->SetBezierC1( cp1 );
                bezier->SetBezierC2( cp2 );
                bezier->SetEnd( endPt );
                bezier->SetStroke( stroke );
                bezier->RebuildBezierToSegmentsPointsList(
                        schIUScale.mmToIU( ARC_LOW_DEF_MM ) );
                m_screen->Append( bezier.release() );
                currentPt = endPt;
            }
        }
        else if( token == wxT( "c" ) )
        {
            long cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0, dx = 0, dy = 0;

            if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cx1 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cy1 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cx2 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cy2 )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &dx )
                && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &dy ) )
            {
                VECTOR2I cp1 = relToAbs( cx1, cy1 );
                VECTOR2I cp2 = relToAbs( cx2, cy2 );
                VECTOR2I endPt = relToAbs( dx, dy );

                auto bezier = std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER, LAYER_NOTES );
                bezier->SetStart( currentPt );
                bezier->SetBezierC1( cp1 );
                bezier->SetBezierC2( cp2 );
                bezier->SetEnd( endPt );
                bezier->SetStroke( stroke );
                bezier->RebuildBezierToSegmentsPointsList(
                        schIUScale.mmToIU( ARC_LOW_DEF_MM ) );
                m_screen->Append( bezier.release() );
                currentPt = endPt;
            }
        }
        else if( token == wxT( "Z" ) || token == wxT( "z" ) )
        {
            if( currentPt != startPt )
            {
                emitSeg( currentPt, startPt );
                currentPt = startPt;
            }
        }
    }

    maybeParseAttributes( aFile, aLineIdx );
}


void SCH_IO_GEDA::parsePicture( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx )
{
    // G x y width height angle mirrored embedded
    wxStringTokenizer tok( aLine );
    tok.GetNextToken(); // skip 'G'

    long x = 0, y = 0, w = 0, h = 0, angle = 0, mirror = 0, embedded = 0;

    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &w );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &h );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &angle );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &mirror );
    if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &embedded );

    wxString filename;

    if( aLineIdx < aFile.GetLineCount() )
    {
        filename = aFile.GetLine( aLineIdx ).Trim().Trim( false );
        aLineIdx++;
    }

    wxString base64Data;

    if( embedded )
    {
        while( aLineIdx < aFile.GetLineCount() )
        {
            wxString dataLine = aFile.GetLine( aLineIdx );
            aLineIdx++;

            if( dataLine.Trim() == wxT( "." ) )
                break;

            base64Data += dataLine.Trim();
        }
    }

    auto bitmap = std::make_unique<SCH_BITMAP>();
    REFERENCE_IMAGE& refImage = bitmap->GetReferenceImage();
    bool loaded = false;

    int savedLoadFlags = wxImage::GetDefaultLoadFlags();
    wxImage::SetDefaultLoadFlags( savedLoadFlags & ~wxImage::Load_Verbose );

    if( embedded && !base64Data.IsEmpty() )
    {
        wxMemoryBuffer buf = wxBase64Decode( base64Data );

        if( buf.GetDataLen() > 0 )
            loaded = refImage.ReadImageFile( buf );
    }
    else if( !filename.IsEmpty() )
    {
        wxFileName imgFile( filename );

        if( !imgFile.IsAbsolute() && m_filename.IsOk() )
            imgFile.MakeAbsolute( m_filename.GetPath() );

        if( imgFile.FileExists() )
            loaded = refImage.ReadImageFile( imgFile.GetFullPath() );
    }

    wxImage::SetDefaultLoadFlags( savedLoadFlags );

    if( !loaded )
        return;

    // gEDA specifies x,y as the lower-left corner of the picture bounding box.
    // Compute center position from the lower-left corner + size.
    int gCenterX = static_cast<int>( x ) + static_cast<int>( w ) / 2;
    int gCenterY = static_cast<int>( y ) + static_cast<int>( h ) / 2;
    VECTOR2I center = toKiCad( gCenterX, gCenterY );

    int targetWidth = toKiCadDist( static_cast<int>( w ) );
    VECTOR2I imgSize = refImage.GetSize();

    if( imgSize.x > 0 )
    {
        double scaleFactor = static_cast<double>( targetWidth ) / imgSize.x;
        refImage.SetImageScale( scaleFactor );
    }

    bitmap->SetPosition( center );

    if( mirror )
        bitmap->MirrorHorizontally( center.x );

    for( long a = angle; a > 0; a -= 90 )
        bitmap->Rotate( center, false );

    m_maxY = std::max( m_maxY, static_cast<int>( y + h ) );
    m_screen->Append( bitmap.release() );
}


// ==========================================================================
// Symbol loading
// ==========================================================================

void SCH_IO_GEDA::initSymbolLibrary()
{
    if( m_symLibraryInitialized )
        return;

    m_symLibraryInitialized = true;

    // Check GEDADATA environment variable first (highest priority for system symbols)
    wxString gedaData;

    if( wxGetEnv( wxT( "GEDADATA" ), &gedaData ) && !gedaData.IsEmpty() )
    {
        wxString envSymDir = gedaData + wxFileName::GetPathSeparator() + wxT( "sym" );

        if( wxDir::Exists( envSymDir ) )
            scanSymbolDir( envSymDir );
    }

    // Scan standard gEDA and Lepton-EDA symbol directories
    static const wxString defaultPaths[] = {
        wxT( "/usr/share/gEDA/sym" ),
        wxT( "/usr/share/geda-symbols" ),
        wxT( "/usr/local/share/gEDA/sym" ),
        wxT( "/usr/local/share/geda-symbols" ),
        wxT( "/usr/share/lepton-eda/sym" ),
        wxT( "/usr/local/share/lepton-eda/sym" ),
    };

    for( const wxString& dir : defaultPaths )
    {
        if( wxDir::Exists( dir ) )
            scanSymbolDir( dir );
    }

    // Scan XDG data directories for Lepton-EDA symbols.
    // $XDG_DATA_DIRS defaults to /usr/local/share:/usr/share per the spec.
    wxString xdgDataDirs;

    if( wxGetEnv( wxT( "XDG_DATA_DIRS" ), &xdgDataDirs ) && !xdgDataDirs.IsEmpty() )
    {
        wxStringTokenizer xdgTok( xdgDataDirs, wxT( ":" ) );

        while( xdgTok.HasMoreTokens() )
        {
            wxString dataDir = xdgTok.GetNextToken();

            if( dataDir.IsEmpty() )
                continue;

            wxString leptonSymDir = dataDir + wxFileName::GetPathSeparator()
                                    + wxT( "lepton-eda" ) + wxFileName::GetPathSeparator()
                                    + wxT( "sym" );

            if( wxDir::Exists( leptonSymDir ) )
                scanSymbolDir( leptonSymDir );

            wxString gedaSymDir = dataDir + wxFileName::GetPathSeparator()
                                  + wxT( "gEDA" ) + wxFileName::GetPathSeparator()
                                  + wxT( "sym" );

            if( wxDir::Exists( gedaSymDir ) )
                scanSymbolDir( gedaSymDir );
        }
    }

    // Scan relative to the schematic file location.
    // Properties-based paths are handled at the end regardless of whether
    // a valid schematic path exists.
    wxString schDir;

    if( m_filename.IsOk() )
        schDir = m_filename.GetPath();

    // Scan user-level RC files for additional library paths
    wxString homeDir = wxGetHomeDir();

    if( !homeDir.IsEmpty() )
    {
        wxString userGedaDir = homeDir + wxFileName::GetPathSeparator() + wxT( ".gEDA" );

        parseRcFileForLibraries( userGedaDir + wxFileName::GetPathSeparator() + wxT( "gafrc" ),
                                 userGedaDir );
        parseRcFileForLibraries( userGedaDir + wxFileName::GetPathSeparator() + wxT( "gschemrc" ),
                                 userGedaDir );

        wxString leptonDir;
        wxString sep( wxFileName::GetPathSeparator() );

        if( wxGetEnv( wxT( "XDG_CONFIG_HOME" ), &leptonDir ) && !leptonDir.IsEmpty() )
            leptonDir += sep + wxT( "lepton-eda" );
        else
            leptonDir = homeDir + sep + wxT( ".config" ) + sep + wxT( "lepton-eda" );

        parseRcFileForLibraries( leptonDir + wxFileName::GetPathSeparator() + wxT( "gafrc" ),
                                 leptonDir );
    }

    // Parse project-level RC files and scan schematic-relative directories
    if( !schDir.IsEmpty() )
    {
        parseRcFileForLibraries( schDir + wxFileName::GetPathSeparator() + wxT( "gafrc" ),
                                 schDir );
        parseRcFileForLibraries( schDir + wxFileName::GetPathSeparator() + wxT( "gschemrc" ),
                                 schDir );

        // Parse Lepton-EDA INI-style config files (lepton.conf or geda.conf)
        // for [libs] section with component-library entries
        static const wxString configNames[] = {
            wxT( "lepton.conf" ),
            wxT( "geda.conf" ),
        };

        for( const wxString& configName : configNames )
        {
            wxString confPath = schDir + wxFileName::GetPathSeparator() + configName;

            if( !wxFileExists( confPath ) )
                continue;

            wxTextFile confFile;

            if( !confFile.Open( confPath ) )
                continue;

            bool inLibsSection = false;

            for( size_t i = 0; i < confFile.GetLineCount(); ++i )
            {
                wxString line = confFile.GetLine( i );
                line.Trim( true );
                line.Trim( false );

                if( line.IsEmpty() || line[0] == '#' || line[0] == ';' )
                    continue;

                if( line[0] == '[' )
                {
                    inLibsSection = ( line.CmpNoCase( wxT( "[libs]" ) ) == 0 );
                    continue;
                }

                if( !inLibsSection )
                    continue;

                if( line.StartsWith( wxT( "component-library" ) ) )
                {
                    int eqPos = line.Find( '=' );

                    if( eqPos == wxNOT_FOUND )
                        continue;

                    wxString   libPath = line.Mid( eqPos + 1 ).Trim( false ).Trim( true );
                    wxFileName libDir( libPath );

                    if( !libDir.IsAbsolute() )
                    {
                        libDir.MakeAbsolute( schDir );
                        libPath = libDir.GetFullPath();
                    }

                    if( wxDir::Exists( libPath ) )
                        scanSymbolDir( libPath );
                }
            }

            break;
        }

        scanSymbolDir( schDir );

        wxString symDir = schDir + wxFileName::GetPathSeparator() + wxT( "sym" );

        if( wxDir::Exists( symDir ) )
            scanSymbolDir( symDir );

        wxFileName parentDir( schDir );
        parentDir.RemoveLastDir();
        wxString parentSymDir =
                parentDir.GetPath() + wxFileName::GetPathSeparator() + wxT( "sym" );

        if( wxDir::Exists( parentSymDir ) )
            scanSymbolDir( parentSymDir );
    }

    if( m_properties )
    {
        auto it = m_properties->find( "sym_search_paths" );

        if( it != m_properties->end() )
        {
            wxString paths = it->second;
            wxStringTokenizer tok( paths, wxT( "\n;" ) );

            while( tok.HasMoreTokens() )
            {
                wxString dir = tok.GetNextToken().Trim().Trim( false );

                if( !dir.IsEmpty() && wxDir::Exists( dir ) )
                    scanSymbolDir( dir );
            }
        }
    }
}


void SCH_IO_GEDA::scanSymbolDir( const wxString& aDir )
{
    wxDir dir( aDir );

    if( !dir.IsOpened() )
        return;

    wxString filename;
    bool cont = dir.GetFirst( &filename );

    while( cont )
    {
        wxString fullPath = aDir + wxFileName::GetPathSeparator() + filename;

        if( wxDir::Exists( fullPath ) )
        {
            // Recurse into subdirectories
            scanSymbolDir( fullPath );
        }
        else if( filename.EndsWith( wxT( ".sym" ) ) )
        {
            SYM_CACHE_ENTRY entry;
            entry.path = fullPath;
            m_symLibrary[filename] = std::move( entry );
        }

        cont = dir.GetNext( &filename );
    }
}


void SCH_IO_GEDA::parseRcFileForLibraries( const wxString& aPath, const wxString& aBaseDir )
{
    if( !wxFileExists( aPath ) )
        return;

    wxTextFile rcFile;

    if( !rcFile.Open( aPath ) )
        return;

    for( size_t i = 0; i < rcFile.GetLineCount(); ++i )
    {
        wxString line = rcFile.GetLine( i );
        line.Trim( true );
        line.Trim( false );

        if( !line.StartsWith( wxT( "(component-library-search " ) )
            && !line.StartsWith( wxT( "(component-library " ) ) )
        {
            continue;
        }

        int firstQuote = line.Find( '"' );

        if( firstQuote == wxNOT_FOUND )
            continue;

        // Find the closing quote of the first argument. The two-argument
        // form (component-library "path" "name") has a second quoted
        // string that we ignore.
        int secondQuote = line.Mid( firstQuote + 1 ).Find( '"' );

        if( secondQuote != wxNOT_FOUND )
            secondQuote += firstQuote + 1;

        if( secondQuote == wxNOT_FOUND || secondQuote <= firstQuote )
            continue;

        wxString libPath = line.Mid( firstQuote + 1, secondQuote - firstQuote - 1 );
        wxFileName libDir( libPath );

        if( !libDir.IsAbsolute() )
        {
            libDir.MakeAbsolute( aBaseDir );
            libPath = libDir.GetFullPath();
        }

        if( wxDir::Exists( libPath ) )
            scanSymbolDir( libPath );
    }
}


std::unique_ptr<LIB_SYMBOL> SCH_IO_GEDA::loadSymbolFile( const wxString& aPath,
                                                          wxString* aSymversion,
                                                          wxString* aNetAttr )
{
    wxTextFile file;

    if( !file.Open( aPath ) )
        return nullptr;

    if( file.GetLineCount() == 0 )
        return nullptr;

    // Validate version line
    wxString firstLine = file.GetFirstLine().Trim( false );

    if( !firstLine.StartsWith( wxT( "v " ) ) )
        return nullptr;

    wxFileName fn( aPath );
    wxString symName = fn.GetName();
    auto libSym = std::make_unique<LIB_SYMBOL>( symName );
    libSym->SetShowPinNumbers( true );
    libSym->SetShowPinNames( true );

    size_t lineIdx = 1; // Skip version line

    while( lineIdx < file.GetLineCount() )
    {
        wxString line = file.GetLine( lineIdx );

        if( line.IsEmpty() )
        {
            lineIdx++;
            continue;
        }

        wxChar type = line[0];
        lineIdx++;

        switch( type )
        {
        case 'P':
        {
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip 'P'
            long px1 = 0, py1 = 0, px2 = 0, py2 = 0, pc = 0, pt = 0, pw = 0;

            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &px1 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &py1 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &px2 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &py2 );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pc );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pt );
            if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &pw );

            std::vector<GEDA_ATTR> pinAttrs = maybeParseAttributes( file, lineIdx );
            addSymbolPin( *libSym, static_cast<int>( px1 ), static_cast<int>( py1 ),
                          static_cast<int>( px2 ), static_cast<int>( py2 ),
                          static_cast<int>( pw ), pinAttrs );
            break;
        }

        case 'L':
        case 'B':
        case 'V':
        case 'A':
        case 'H':
            addSymbolGraphic( *libSym, line, file, lineIdx, type );
            break;

        case 'T':
        {
            // T x y color size vis show_nv angle align num_lines
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip 'T'
            long numLines = 1;

            for( int i = 0; i < 9 && tok.HasMoreTokens(); i++ )
            {
                wxString fieldVal = tok.GetNextToken();

                if( i == 8 )
                    fieldVal.ToLong( &numLines );
            }

            for( long i = 0; i < numLines && lineIdx < file.GetLineCount(); i++ )
            {
                wxString textLine = file.GetLine( lineIdx );

                if( aSymversion && textLine.StartsWith( wxT( "symversion=" ) ) )
                    *aSymversion = textLine.Mid( 11 );

                if( aNetAttr && textLine.StartsWith( wxT( "net=" ) ) )
                    *aNetAttr = textLine.Mid( 4 );

                lineIdx++;
            }

            maybeParseAttributes( file, lineIdx );
            break;
        }

        case '{':
        {
            while( lineIdx < file.GetLineCount() )
            {
                if( file.GetLine( lineIdx ).Trim() == wxT( "}" ) )
                {
                    lineIdx++;
                    break;
                }

                lineIdx++;
            }

            break;
        }

        case 'v':
        default:
            break;
        }
    }

    return libSym;
}


void SCH_IO_GEDA::addSymbolPin( LIB_SYMBOL& aSymbol, int aX1, int aY1, int aX2, int aY2,
                                  int aWhichEnd, const std::vector<GEDA_ATTR>& aAttrs )
{
    // whichend determines which endpoint is the connection point (active end)
    // 0 = (x1,y1) is connection point, 1 = (x2,y2) is connection point
    int connX = 0, connY = 0, otherX = 0, otherY = 0;

    if( aWhichEnd == 0 )
    {
        connX = aX1;   connY = aY1;
        otherX = aX2;  otherY = aY2;
    }
    else
    {
        connX = aX2;   connY = aY2;
        otherX = aX1;  otherY = aY1;
    }

    // Symbol coordinates use the same mil system but are relative to the symbol origin.
    // The connection point goes at the tip, the other end is toward the symbol body.
    VECTOR2I connPt( connX * MILS_TO_IU, -connY * MILS_TO_IU );
    VECTOR2I bodyPt( otherX * MILS_TO_IU, -otherY * MILS_TO_IU );

    // PIN_ORIENTATION describes which direction the pin body extends FROM the
    // connection point, so the delta must point from connection toward body.
    int dx = otherX - connX;
    int dy = otherY - connY;
    int length = KiROUND( std::hypot( dx, dy ) ) * MILS_TO_IU;

    PIN_ORIENTATION orient = PIN_ORIENTATION::PIN_RIGHT;

    if( dx > 0 )
        orient = PIN_ORIENTATION::PIN_RIGHT;
    else if( dx < 0 )
        orient = PIN_ORIENTATION::PIN_LEFT;
    else if( dy > 0 )
        orient = PIN_ORIENTATION::PIN_UP;
    else if( dy < 0 )
        orient = PIN_ORIENTATION::PIN_DOWN;

    wxString pinnumber = findAttr( aAttrs, wxT( "pinnumber" ) );
    wxString pinlabel  = findAttr( aAttrs, wxT( "pinlabel" ) );
    wxString pintypeStr = findAttr( aAttrs, wxT( "pintype" ) );

    if( pinnumber.IsEmpty() )
        pinnumber = wxT( "?" );

    if( pinlabel.IsEmpty() )
        pinlabel = pinnumber;

    ELECTRICAL_PINTYPE elecType = ELECTRICAL_PINTYPE::PT_PASSIVE;

    if( pintypeStr == wxT( "in" ) )
        elecType = ELECTRICAL_PINTYPE::PT_INPUT;
    else if( pintypeStr == wxT( "out" ) )
        elecType = ELECTRICAL_PINTYPE::PT_OUTPUT;
    else if( pintypeStr == wxT( "io" ) )
        elecType = ELECTRICAL_PINTYPE::PT_BIDI;
    else if( pintypeStr == wxT( "oc" ) )
        elecType = ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    else if( pintypeStr == wxT( "oe" ) )
        elecType = ELECTRICAL_PINTYPE::PT_OPENEMITTER;
    else if( pintypeStr == wxT( "pas" ) )
        elecType = ELECTRICAL_PINTYPE::PT_PASSIVE;
    else if( pintypeStr == wxT( "tp" ) )
        elecType = ELECTRICAL_PINTYPE::PT_TRISTATE;
    else if( pintypeStr == wxT( "tri" ) )
        elecType = ELECTRICAL_PINTYPE::PT_TRISTATE;
    else if( pintypeStr == wxT( "clk" ) )
        elecType = ELECTRICAL_PINTYPE::PT_INPUT;
    else if( pintypeStr == wxT( "pwr" ) )
        elecType = ELECTRICAL_PINTYPE::PT_POWER_IN;

    SCH_PIN* pin = new SCH_PIN( &aSymbol );
    pin->SetPosition( connPt );
    pin->SetLength( length );
    pin->SetOrientation( orient );
    pin->SetNumber( pinnumber );
    pin->SetName( pinlabel );
    pin->SetType( elecType );

    aSymbol.AddDrawItem( pin );
}


void SCH_IO_GEDA::addSymbolGraphic( LIB_SYMBOL& aSymbol, const wxString& aLine,
                                      wxTextFile& aFile, size_t& aLineIdx, wxChar aType )
{
    switch( aType )
    {
    case 'L':
    {
        wxStringTokenizer tok( aLine );
        tok.GetNextToken(); // skip 'L'
        long x1 = 0, y1 = 0, x2 = 0, y2 = 0, color = 0, width = 0;
        long capstyle = 0, dashstyle = 0, dashlength = 0, dashspace = 0;

        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x2 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y2 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &width );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );

        VECTOR2I start( static_cast<int>( x1 ) * MILS_TO_IU,
                        -static_cast<int>( y1 ) * MILS_TO_IU );
        VECTOR2I end( static_cast<int>( x2 ) * MILS_TO_IU,
                      -static_cast<int>( y2 ) * MILS_TO_IU );

        SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        shape->AddPoint( start );
        shape->AddPoint( end );
        shape->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( width ) ),
                                          toLineStyle( static_cast<int>( dashstyle ) ) ) );
        aSymbol.AddDrawItem( shape );
        break;
    }

    case 'B':
    {
        wxStringTokenizer tok( aLine );
        tok.GetNextToken(); // skip 'B'
        long x = 0, y = 0, w = 0, h = 0, color = 0, linewidth = 0;
        long capstyle = 0, dashstyle = 0, dashlength = 0, dashspace = 0;
        long filltype = 0;

        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &x );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &y );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &w );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &h );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &linewidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &filltype );

        VECTOR2I topLeft( static_cast<int>( x ) * MILS_TO_IU,
                          -static_cast<int>( y + h ) * MILS_TO_IU );
        VECTOR2I botRight( static_cast<int>( x + w ) * MILS_TO_IU,
                           -static_cast<int>( y ) * MILS_TO_IU );

        SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        rect->SetStart( topLeft );
        rect->SetEnd( botRight );
        rect->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( linewidth ) ),
                                         toLineStyle( static_cast<int>( dashstyle ) ) ) );

        FILL_T fill = toFillType( static_cast<int>( filltype ) );

        if( fill != FILL_T::NO_FILL )
        {
            rect->SetFilled( true );
            rect->SetFillMode( fill );
        }

        aSymbol.AddDrawItem( rect );
        break;
    }

    case 'V':
    {
        wxStringTokenizer tok( aLine );
        tok.GetNextToken(); // skip 'V'
        long cx = 0, cy = 0, r = 0, color = 0, linewidth = 0;
        long capstyle = 0, dashstyle = 0, dashlength = 0, dashspace = 0;
        long filltype = 0;

        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cx );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cy );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &r );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &linewidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &filltype );

        VECTOR2I center( static_cast<int>( cx ) * MILS_TO_IU,
                         -static_cast<int>( cy ) * MILS_TO_IU );

        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
        circle->SetCenter( center );
        circle->SetEnd( VECTOR2I( center.x + toKiCadDist( static_cast<int>( r ) ), center.y ) );
        circle->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( linewidth ) ),
                                           toLineStyle( static_cast<int>( dashstyle ) ) ) );

        FILL_T fill = toFillType( static_cast<int>( filltype ) );

        if( fill != FILL_T::NO_FILL )
        {
            circle->SetFilled( true );
            circle->SetFillMode( fill );
        }

        aSymbol.AddDrawItem( circle );
        break;
    }

    case 'A':
    {
        wxStringTokenizer tok( aLine );
        tok.GetNextToken(); // skip 'A'
        long cx = 0, cy = 0, r = 0, sa = 0, da = 0;
        long color = 0, linewidth = 0;

        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cx );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &cy );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &r );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &sa );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &da );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &linewidth );

        VECTOR2I center( static_cast<int>( cx ) * MILS_TO_IU,
                         -static_cast<int>( cy ) * MILS_TO_IU );
        int radius = toKiCadDist( static_cast<int>( r ) );

        double startRad = DEG2RAD( static_cast<double>( sa ) );
        double endRad = DEG2RAD( static_cast<double>( sa + da ) );

        VECTOR2I arcStart( center.x + KiROUND( radius * cos( startRad ) ),
                           center.y - KiROUND( radius * sin( startRad ) ) );
        VECTOR2I arcEnd( center.x + KiROUND( radius * cos( endRad ) ),
                         center.y - KiROUND( radius * sin( endRad ) ) );

        SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );
        arc->SetCenter( center );

        // Symbol coordinates use negative-Y, which reverses angular sweep
        // direction. Swap start/end to preserve the correct arc extent.
        arc->SetStart( arcEnd );
        arc->SetEnd( arcStart );
        arc->SetStroke( STROKE_PARAMS( toKiCadDist( static_cast<int>( linewidth ) ),
                                        LINE_STYLE::DEFAULT ) );
        aSymbol.AddDrawItem( arc );
        break;
    }

    case 'H':
    {
        wxStringTokenizer tok( aLine );
        tok.GetNextToken(); // skip 'H'

        long color = 0, width = 0, capstyle = 0, dashstyle = 0;
        long dashlength = 0, dashspace = 0, filltype = 0, fillwidth = 0;
        long a1 = 0, p1 = 0, a2 = 0, p2 = 0, numlines = 0;

        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &color );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &width );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &capstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashstyle );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashlength );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &dashspace );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &filltype );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &fillwidth );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p1 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &a2 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &p2 );
        if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &numlines );

        wxString pathData;

        for( long i = 0; i < numlines && aLineIdx < aFile.GetLineCount(); i++ )
        {
            if( i > 0 )
                pathData += wxT( " " );

            pathData += aFile.GetLine( aLineIdx ).Trim();
            aLineIdx++;
        }

        std::vector<VECTOR2I> polyPts;
        VECTOR2I startPt;
        VECTOR2I currentPt;
        STROKE_PARAMS symStroke( toKiCadDist( static_cast<int>( width ) ),
                                 toLineStyle( static_cast<int>( dashstyle ) ) );
        FILL_T symFill = toFillType( static_cast<int>( filltype ) );

        auto flushPoly = [&]()
        {
            if( polyPts.size() < 2 )
                return;

            SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

            for( const VECTOR2I& pt : polyPts )
                poly->AddPoint( pt );

            poly->SetStroke( symStroke );

            if( symFill != FILL_T::NO_FILL )
            {
                poly->SetFilled( true );
                poly->SetFillMode( symFill );
            }

            aSymbol.AddDrawItem( poly );
            polyPts.clear();
        };

        wxStringTokenizer pathTok( pathData, wxT( " ,\t" ) );

        while( pathTok.HasMoreTokens() )
        {
            wxString pathToken = pathTok.GetNextToken();

            // Symbol path coordinates are in mils (Y-up). Convert to IU (Y-down).
            auto symAbsPt = []( long aX, long aY ) -> VECTOR2I
            {
                return VECTOR2I( static_cast<int>( aX ) * MILS_TO_IU,
                                 -static_cast<int>( aY ) * MILS_TO_IU );
            };

            auto symRelPt = [&currentPt]( long aDx, long aDy ) -> VECTOR2I
            {
                return currentPt + VECTOR2I( static_cast<int>( aDx ) * MILS_TO_IU,
                                             -static_cast<int>( aDy ) * MILS_TO_IU );
            };

            bool isRelative = ( pathToken.IsAscii() && islower( pathToken[0u] ) );

            if( pathToken == wxT( "M" ) || pathToken == wxT( "m" ) )
            {
                long px = 0, py = 0;

                if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &px )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &py ) )
                {
                    currentPt = isRelative ? symRelPt( px, py ) : symAbsPt( px, py );
                    startPt = currentPt;

                    if( polyPts.empty() )
                        polyPts.push_back( currentPt );
                }
            }
            else if( pathToken == wxT( "L" ) || pathToken == wxT( "l" ) )
            {
                long px = 0, py = 0;

                if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &px )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &py ) )
                {
                    currentPt = isRelative ? symRelPt( px, py ) : symAbsPt( px, py );
                    polyPts.push_back( currentPt );
                }
            }
            else if( pathToken == wxT( "C" ) || pathToken == wxT( "c" ) )
            {
                long cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0, px = 0, py = 0;

                if( pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cx1 )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cy1 )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cx2 )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &cy2 )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &px )
                    && pathTok.HasMoreTokens() && pathTok.GetNextToken().ToLong( &py ) )
                {
                    flushPoly();

                    VECTOR2I cp1 = isRelative ? symRelPt( cx1, cy1 ) : symAbsPt( cx1, cy1 );
                    VECTOR2I cp2 = isRelative ? symRelPt( cx2, cy2 ) : symAbsPt( cx2, cy2 );
                    VECTOR2I endPt = isRelative ? symRelPt( px, py ) : symAbsPt( px, py );

                    SCH_SHAPE* bezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
                    bezier->SetStart( currentPt );
                    bezier->SetBezierC1( cp1 );
                    bezier->SetBezierC2( cp2 );
                    bezier->SetEnd( endPt );
                    bezier->SetStroke( symStroke );
                    bezier->RebuildBezierToSegmentsPointsList(
                            schIUScale.mmToIU( ARC_LOW_DEF_MM ) );

                    if( symFill != FILL_T::NO_FILL )
                    {
                        bezier->SetFilled( true );
                        bezier->SetFillMode( symFill );
                    }

                    aSymbol.AddDrawItem( bezier );
                    currentPt = endPt;
                    polyPts.push_back( currentPt );
                }
            }
            else if( pathToken == wxT( "Z" ) || pathToken == wxT( "z" ) )
            {
                if( currentPt != startPt )
                {
                    polyPts.push_back( startPt );
                    currentPt = startPt;
                }
            }
        }

        flushPoly();

        break;
    }

    default:
        break;
    }

    maybeParseAttributes( aFile, aLineIdx );
}


/**
 * Compute the Levenshtein edit distance between two strings.
 */
static int editDistance( const wxString& a, const wxString& b )
{
    size_t m = a.length();
    size_t n = b.length();

    std::vector<int> prev( n + 1 );
    std::vector<int> curr( n + 1 );

    for( size_t j = 0; j <= n; ++j )
        prev[j] = static_cast<int>( j );

    for( size_t i = 1; i <= m; ++i )
    {
        curr[0] = static_cast<int>( i );

        for( size_t j = 1; j <= n; ++j )
        {
            int cost = ( a[i - 1] == b[j - 1] ) ? 0 : 1;
            curr[j] = std::min( { prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost } );
        }

        std::swap( prev, curr );
    }

    return prev[n];
}


std::unique_ptr<LIB_SYMBOL> SCH_IO_GEDA::loadBuiltinSymbol( const wxString& aBasename )
{
    const auto& builtins = getBuiltinSymbols();
    auto it = builtins.find( aBasename );

    if( it == builtins.end() )
        return nullptr;

    wxString tempPath = wxFileName::CreateTempFileName( wxT( "geda_sym_" ) );

    if( tempPath.IsEmpty() )
        return nullptr;

    {
        wxFile temp( tempPath, wxFile::write );

        if( !temp.IsOpened() || !temp.Write( it->second ) )
        {
            wxRemoveFile( tempPath );
            return nullptr;
        }
    }

    wxString netAttr;
    auto result = loadSymbolFile( tempPath, nullptr, &netAttr );
    wxRemoveFile( tempPath );

    if( result )
    {
        wxString symName = aBasename;

        if( symName.EndsWith( wxT( ".sym" ) ) )
            symName = symName.Left( symName.length() - 4 );

        result->SetName( symName );

        if( !netAttr.IsEmpty() )
        {
            SYM_CACHE_ENTRY& entry = m_symLibrary[aBasename];
            entry.netAttr = netAttr;
        }
    }

    return result;
}


LIB_SYMBOL* SCH_IO_GEDA::getOrLoadSymbol( const wxString& aBasename )
{
    // Check if already loaded
    auto it = m_libSymbols.find( aBasename );

    if( it != m_libSymbols.end() )
        return it->second.get();

    // Try the library cache
    initSymbolLibrary();

    auto cacheIt = m_symLibrary.find( aBasename );

    if( cacheIt != m_symLibrary.end() )
    {
        if( !cacheIt->second.symbol )
        {
            cacheIt->second.symbol = loadSymbolFile( cacheIt->second.path,
                                                      &cacheIt->second.symversion,
                                                      &cacheIt->second.netAttr );

            // When a project-local symbol overrides a builtin power symbol but lacks a
            // net= attribute, inherit the builtin's net= so power detection still works.
            // This handles projects that provide custom graphics for standard power symbols.
            if( cacheIt->second.symbol && cacheIt->second.netAttr.IsEmpty() )
            {
                const auto& builtins = getBuiltinSymbols();
                auto builtinIt = builtins.find( aBasename );

                if( builtinIt != builtins.end() )
                {
                    // Scan the builtin symbol text for a net= attribute line.
                    // In gEDA .sym format, net= always starts a line.
                    const wxString& src = builtinIt->second;
                    int pos = src.Find( wxT( "\nnet=" ) );

                    if( pos != wxNOT_FOUND )
                    {
                        wxString rest = src.Mid( pos + 5 );
                        int      eol = rest.Find( '\n' );

                        cacheIt->second.netAttr = ( eol != wxNOT_FOUND )
                                                          ? rest.Left( eol ).Trim()
                                                          : rest.Trim();
                    }
                }
            }
        }

        if( cacheIt->second.symbol )
        {
            m_libSymbols[aBasename] = std::make_unique<LIB_SYMBOL>( *cacheIt->second.symbol );
            return m_libSymbols[aBasename].get();
        }
    }

    // Try built-in standard symbols before falling back to a placeholder
    auto builtin = loadBuiltinSymbol( aBasename );

    if( builtin )
    {
        m_libSymbols[aBasename] = std::move( builtin );
        return m_libSymbols[aBasename].get();
    }

    // Not found anywhere - create a fallback placeholder
    m_libSymbols[aBasename] = createFallbackSymbol( aBasename );

    if( m_reporter )
    {
        wxString suggestion;
        int      bestDist = INT_MAX;

        for( const auto& [name, entry] : m_symLibrary )
        {
            int dist = editDistance( aBasename, name );

            if( dist < bestDist && dist <= 3 )
            {
                bestDist = dist;
                suggestion = name;
            }
        }

        // Also check builtins, which may have a closer match than the library cache
        for( const auto& [name, content] : getBuiltinSymbols() )
        {
            int dist = editDistance( aBasename, name );

            if( dist < bestDist && dist <= 3 )
            {
                bestDist = dist;
                suggestion = name;
            }
        }

        wxString msg = wxString::Format( _( "Symbol '%s' not found in gEDA libraries." ),
                                          aBasename );

        if( !suggestion.IsEmpty() )
            msg += wxString::Format( _( " Did you mean '%s'?" ), suggestion );

        m_reporter->Report( msg );
    }

    return m_libSymbols[aBasename].get();
}


std::unique_ptr<LIB_SYMBOL> SCH_IO_GEDA::createFallbackSymbol( const wxString& aBasename )
{
    wxString symName = aBasename;

    if( symName.EndsWith( wxT( ".sym" ) ) )
        symName = symName.Left( symName.length() - 4 );

    auto libSymbol = std::make_unique<LIB_SYMBOL>( symName );
    libSymbol->SetShowPinNumbers( true );
    libSymbol->SetShowPinNames( true );

    int halfSize = DEFAULT_SYMBOL_SIZE_MILS * MILS_TO_IU / 2;

    SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );
    rect->SetStart( VECTOR2I( -halfSize, -halfSize ) );
    rect->SetEnd( VECTOR2I( halfSize, halfSize ) );
    rect->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT ) );
    libSymbol->AddDrawItem( rect );

    return libSymbol;
}


void SCH_IO_GEDA::flushPendingComponent()
{
    if( !m_pendingComp )
        return;

    // Skip title block symbols
    if( m_pendingComp->basename.StartsWith( wxT( "title-" ) ) )
    {
        m_pendingComp.reset();
        return;
    }

    // Detect no-connect symbols (nc-right-1.sym, nc-left-1.sym, etc.) and emit
    // SCH_NO_CONNECT at the pin connection point rather than the component origin.
    wxString ncValue = findAttr( m_pendingComp->attrs, wxT( "value" ) );
    bool isNoConnect = m_pendingComp->basename.StartsWith( wxT( "nc-" ) )
                       && ncValue == wxT( "NoConnection" );

    if( isNoConnect )
    {
        LIB_SYMBOL* ncSym = getOrLoadSymbol( m_pendingComp->basename );
        int pinLocalX = 0;
        int pinLocalY = 0;

        if( ncSym )
        {
            const std::vector<SCH_PIN*>& pins = ncSym->GetPins();

            if( !pins.empty() )
            {
                VECTOR2I pinPos = pins[0]->GetPosition();
                pinLocalX = pinPos.x / MILS_TO_IU;
                pinLocalY = -pinPos.y / MILS_TO_IU;
            }
        }

        // Apply gEDA component rotation/mirror to the pin offset
        int rx = pinLocalX;
        int ry = pinLocalY;

        switch( m_pendingComp->angle )
        {
        case 90:   rx = -pinLocalY;  ry =  pinLocalX;      break;
        case 180:  rx = -pinLocalX;  ry = -pinLocalY;      break;
        case 270:  rx =  pinLocalY;  ry = -pinLocalX;      break;
        default:                                            break;
        }

        if( m_pendingComp->mirror )
            rx = -rx;

        int gedaX = m_pendingComp->x + rx;
        int gedaY = m_pendingComp->y + ry;

        m_screen->Append( new SCH_NO_CONNECT( toKiCad( gedaX, gedaY ) ) );
        m_pendingComp.reset();
        return;
    }

    // Check for hierarchical sheet reference. In gEDA, a source= attribute
    // pointing to a .sch file indicates a hierarchical sub-schematic.
    wxString sourceAttr = findAttr( m_pendingComp->attrs, wxT( "source" ) );

    if( !sourceAttr.IsEmpty() && sourceAttr.EndsWith( wxT( ".sch" ) ) )
    {
        importHierarchicalSheet( sourceAttr );
        m_pendingComp.reset();
        return;
    }

    LIB_SYMBOL* libSym = nullptr;

    if( m_pendingComp->embedded && m_pendingComp->embeddedSym )
    {
        wxString basename = m_pendingComp->basename;
        m_libSymbols[basename] = std::move( m_pendingComp->embeddedSym );
        libSym = m_libSymbols[basename].get();
    }
    else
    {
        libSym = getOrLoadSymbol( m_pendingComp->basename );
    }

    if( !libSym )
    {
        m_pendingComp.reset();
        return;
    }

    // Compare component symversion against the loaded symbol's symversion.
    // A mismatch in the major version number indicates the symbol has changed
    // in an incompatible way since the schematic was saved.
    wxString compSymver = findAttr( m_pendingComp->attrs, wxT( "symversion" ) );

    if( !compSymver.IsEmpty() && m_reporter )
    {
        wxString symSymver;
        auto cacheIt = m_symLibrary.find( m_pendingComp->basename );

        if( cacheIt != m_symLibrary.end() )
            symSymver = cacheIt->second.symversion;

        if( !symSymver.IsEmpty() )
        {
            long compMajor = 0;
            long symMajor = 0;
            compSymver.BeforeFirst( '.' ).ToLong( &compMajor );
            symSymver.BeforeFirst( '.' ).ToLong( &symMajor );

            if( compMajor != symMajor )
            {
                wxString refdes = findAttr( m_pendingComp->attrs, wxT( "refdes" ) );
                wxString label = refdes.IsEmpty() ? m_pendingComp->basename : refdes;

                m_reporter->Report(
                        wxString::Format( _( "Symbol version mismatch for '%s' (%s): "
                                             "schematic has symversion %s, "
                                             "library symbol has symversion %s." ),
                                          label, m_pendingComp->basename,
                                          compSymver, symSymver ),
                        RPT_SEVERITY_WARNING );
            }
        }
    }

    // Detect power symbols via the net= attribute in the .sym definition.
    // gEDA power symbols (gnd-1.sym, vcc-1.sym, etc.) carry a net=NETNAME:PIN
    // attribute that identifies them as implicit power connections.
    wxString symNetAttr;
    auto cacheEntry = m_symLibrary.find( m_pendingComp->basename );

    if( cacheEntry != m_symLibrary.end() )
        symNetAttr = cacheEntry->second.netAttr;

    bool isPowerSym = !symNetAttr.IsEmpty();
    wxString powerNetName;
    std::unique_ptr<LIB_SYMBOL> powerCopy;

    if( isPowerSym )
    {
        // Schematic-level net= overrides the .sym-level default (e.g. generic-power.sym
        // has net=Vcc:1 but instances use net=5V:1 or net=9V:1).
        wxString schematicNet = findAttr( m_pendingComp->attrs, wxT( "net" ) );
        wxString effectiveNet = schematicNet.IsEmpty() ? symNetAttr : schematicNet;

        int colonPos = effectiveNet.Find( ':' );

        if( colonPos != wxNOT_FOUND )
            powerNetName = effectiveNet.Left( colonPos );
        else
            powerNetName = effectiveNet;

        // Work on a copy so the shared cached symbol is not modified.
        // Each power instance may have a different net name.
        powerCopy = std::make_unique<LIB_SYMBOL>( *libSym );
        libSym = powerCopy.get();

        libSym->SetGlobalPower();
        libSym->SetShowPinNames( false );
        libSym->SetShowPinNumbers( false );
        libSym->GetReferenceField().SetText( wxT( "#PWR" ) );
        libSym->GetReferenceField().SetVisible( false );
        libSym->GetValueField().SetText( powerNetName );
        libSym->GetValueField().SetVisible( true );

        for( SCH_PIN* pin : libSym->GetPins() )
        {
            pin->SetName( powerNetName );
            pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        }
    }

    LIB_ID libId( getLibName(), libSym->GetName() );
    VECTOR2I pos = toKiCad( m_pendingComp->x, m_pendingComp->y );

    auto symbol = std::make_unique<SCH_SYMBOL>( *libSym, libId, &m_schematic->CurrentSheet(),
                                                 1, 0, pos );

    int orient = toKiCadOrientation( m_pendingComp->angle, m_pendingComp->mirror );
    symbol->SetOrientation( orient );

    if( isPowerSym )
    {
        wxString pwrRef = wxString::Format( wxT( "#PWR%04d" ), ++m_powerCounter );
        symbol->SetRef( &m_schematic->CurrentSheet(), pwrRef );
        symbol->GetField( FIELD_T::REFERENCE )->SetText( pwrRef );
        symbol->GetField( FIELD_T::REFERENCE )->SetVisible( false );
        symbol->GetField( FIELD_T::VALUE )->SetText( powerNetName );
        symbol->GetField( FIELD_T::VALUE )->SetVisible( true );
    }

    // Mark graphical-only symbols as excluded from BOM, board, and simulation
    wxString graphicalAttr = findAttr( m_pendingComp->attrs, wxT( "graphical" ) );

    if( graphicalAttr == wxT( "1" ) )
    {
        symbol->SetExcludedFromBOM( true );
        symbol->SetExcludedFromBoard( true );
        symbol->SetExcludedFromSim( true );
    }

    wxString refdes   = findAttr( m_pendingComp->attrs, wxT( "refdes" ) );
    wxString value    = findAttr( m_pendingComp->attrs, wxT( "value" ) );
    wxString footprint = findAttr( m_pendingComp->attrs, wxT( "footprint" ) );

    if( !isPowerSym && !refdes.IsEmpty() )
    {
        symbol->SetRef( &m_schematic->CurrentSheet(), refdes );

        SCH_FIELD*       refField = symbol->GetField( FIELD_T::REFERENCE );
        const GEDA_ATTR* refAttr = findAttrStruct( m_pendingComp->attrs, wxT( "refdes" ) );
        refField->SetText( refdes );

        if( refAttr )
        {
            refField->SetPosition( toKiCad( refAttr->x, refAttr->y ) );
            int textSize = toKiCadDist( refAttr->size * 10 );
            refField->SetTextSize( VECTOR2I( textSize, textSize ) );
            refField->SetVisible( refAttr->visible );
        }
    }

    if( !isPowerSym && !value.IsEmpty() )
    {
        SCH_FIELD*       valField = symbol->GetField( FIELD_T::VALUE );
        const GEDA_ATTR* valAttr = findAttrStruct( m_pendingComp->attrs, wxT( "value" ) );
        valField->SetText( value );

        if( valAttr )
        {
            valField->SetPosition( toKiCad( valAttr->x, valAttr->y ) );
            int textSize = toKiCadDist( valAttr->size * 10 );
            valField->SetTextSize( VECTOR2I( textSize, textSize ) );
            valField->SetVisible( valAttr->visible );
        }
    }

    if( !footprint.IsEmpty() )
        symbol->SetFootprintFieldText( footprint );

    // Map documentation attribute to KiCad's native DATASHEET field
    wxString documentation = findAttr( m_pendingComp->attrs, wxT( "documentation" ) );

    if( !documentation.IsEmpty() )
    {
        SCH_FIELD* dsField = symbol->GetField( FIELD_T::DATASHEET );
        dsField->SetText( documentation );
    }

    // Map description attribute to KiCad's native DESCRIPTION field
    const GEDA_ATTR* descAttr = findAttrStruct( m_pendingComp->attrs, wxT( "description" ) );

    if( descAttr && !descAttr->value.IsEmpty() )
    {
        SCH_FIELD* descField = symbol->GetField( FIELD_T::DESCRIPTION );
        descField->SetText( descAttr->value );
        descField->SetVisible( descAttr->visible );

        if( descAttr->visible )
        {
            descField->SetPosition( toKiCad( descAttr->x, descAttr->y ) );
            int textSize = toKiCadDist( descAttr->size * 10 );
            descField->SetTextSize( VECTOR2I( textSize, textSize ) );
        }
    }

    // Collect net= attributes for post-processing. Power symbols already establish
    // their net connection through the pin name, so skip global label generation.
    if( !isPowerSym )
    {
        for( const GEDA_ATTR& attr : m_pendingComp->attrs )
        {
            if( attr.name == wxT( "net" ) )
            {
                int colonPos = attr.value.Find( ':' );

                if( colonPos != wxNOT_FOUND )
                {
                    NET_ATTR_RECORD rec;
                    rec.netname = attr.value.Left( colonPos );
                    rec.pinnumber = attr.value.Mid( colonPos + 1 );
                    rec.symbol = symbol.get();
                    m_netAttrRecords.push_back( rec );
                }
            }
        }
    }

    // Store additional attributes as custom fields (skip those already handled)
    for( const GEDA_ATTR& attr : m_pendingComp->attrs )
    {
        if( attr.name == wxT( "refdes" ) || attr.name == wxT( "value" )
            || attr.name == wxT( "footprint" ) || attr.name == wxT( "net" )
            || attr.name == wxT( "device" ) || attr.name == wxT( "symversion" )
            || attr.name == wxT( "documentation" ) || attr.name == wxT( "description" )
            || attr.name == wxT( "graphical" )
            || attr.name == wxT( "slot" ) || attr.name == wxT( "numslots" )
            || attr.name == wxT( "slotdef" ) )
        {
            continue;
        }

        SCH_FIELD* field = symbol->AddField(
                SCH_FIELD( symbol.get(), FIELD_T::USER, attr.name ) );
        field->SetText( attr.value );
        field->SetVisible( attr.visible );
        field->SetPosition( pos );
    }

    // Track pin connection points for junction detection. Reverse-map the
    // KiCad IU positions back to gEDA coordinates to match the wire endpoints.
    for( SCH_PIN* pin : libSym->GetPins() )
    {
        VECTOR2I pinPos = symbol->GetPinPhysicalPosition( pin );
        int gedaX = pinPos.x / MILS_TO_IU;
        int gedaY = m_maxY - pinPos.y / MILS_TO_IU;
        trackEndpoint( gedaX, gedaY );
    }

    symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );

    // Apply multi-slot pin remapping on the symbol's PRIVATE copy of the lib symbol.
    // gEDA slotdef format is "N:pin1,pin2,pin3" where N is the slot number
    // and each pin corresponds to a pinseq-ordered pin in the symbol.
    // Operating on the private copy prevents corrupting the shared cached symbol.
    wxString slotStr = findAttr( m_pendingComp->attrs, wxT( "slot" ) );
    long slotNum = 0;

    if( !slotStr.IsEmpty() && slotStr.ToLong( &slotNum ) && slotNum > 0 )
    {
        wxString targetSlotDef;

        for( const GEDA_ATTR& attr : m_pendingComp->attrs )
        {
            if( attr.name != wxT( "slotdef" ) )
                continue;

            int colonPos = attr.value.Find( ':' );

            if( colonPos == wxNOT_FOUND )
                continue;

            long defSlot = 0;

            if( attr.value.Left( colonPos ).ToLong( &defSlot ) && defSlot == slotNum )
            {
                targetSlotDef = attr.value.Mid( colonPos + 1 );
                break;
            }
        }

        if( !targetSlotDef.IsEmpty() )
        {
            wxArrayString slotPins;
            wxStringTokenizer slotTok( targetSlotDef, wxT( "," ) );

            while( slotTok.HasMoreTokens() )
                slotPins.Add( slotTok.GetNextToken() );

            LIB_SYMBOL* privateSym = symbol->GetLibSymbolRef().get();

            // Build pinseq-ordered list from the private copy's pins.
            // Pins are added in file order which matches pinseq order.
            std::map<long, SCH_PIN*> pinsBySeq;
            int autoSeq = 1;

            for( SCH_PIN* pin : privateSym->GetPins() )
            {
                pinsBySeq[autoSeq] = pin;
                autoSeq++;
            }

            for( size_t i = 0; i < slotPins.size() && i < pinsBySeq.size(); i++ )
            {
                auto seqIt = pinsBySeq.find( static_cast<long>( i + 1 ) );

                if( seqIt != pinsBySeq.end() )
                    seqIt->second->SetNumber( slotPins[i].Trim() );
            }
        }
    }

    m_screen->Append( symbol.release() );
    m_pendingComp.reset();
}


// ==========================================================================
// Hierarchical sheet import
// ==========================================================================

void SCH_IO_GEDA::importHierarchicalSheet( const wxString& aSourceFile )
{
    wxFileName sourceFileName( aSourceFile );

    // Resolve relative paths against the directory of the current schematic
    if( !sourceFileName.IsAbsolute() )
        sourceFileName.SetPath( m_filename.GetPath() );

    wxString fullPath = sourceFileName.GetFullPath();

    if( !wxFileExists( fullPath ) )
    {
        if( m_reporter )
        {
            m_reporter->Report(
                    wxString::Format( _( "Hierarchical source '%s' not found, "
                                         "creating empty sheet." ),
                                      aSourceFile ),
                    RPT_SEVERITY_WARNING );
        }
    }

    if( m_importStack.count( fullPath ) )
    {
        if( m_reporter )
        {
            m_reporter->Report(
                    wxString::Format( _( "Circular hierarchy detected for '%s', skipping." ),
                                      aSourceFile ),
                    RPT_SEVERITY_WARNING );
        }

        return;
    }

    VECTOR2I pos = toKiCad( m_pendingComp->x, m_pendingComp->y );

    // Use a default sheet size. We'll resize after loading the sub-schematic content.
    VECTOR2I sheetSize( schIUScale.MilsToIU( 2000 ), schIUScale.MilsToIU( 1500 ) );

    auto sheet = std::make_unique<SCH_SHEET>( m_rootSheet, pos, sheetSize );

    wxString refdes = findAttr( m_pendingComp->attrs, wxT( "refdes" ) );

    if( !refdes.IsEmpty() )
    {
        sheet->GetField( FIELD_T::SHEET_NAME )->SetText( refdes );
    }
    else
    {
        sheet->GetField( FIELD_T::SHEET_NAME )->SetText(
                sourceFileName.GetName() );
    }

    sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( aSourceFile );
    sheet->SetFileName( aSourceFile );

    SCH_SHEET* sheetPtr = sheet.get();
    m_screen->Append( sheet.release() );

    if( wxFileExists( fullPath ) )
    {
        m_deferredSheets.push_back( { sheetPtr, fullPath } );
    }
}


// ==========================================================================
// Endpoint tracking
// ==========================================================================

void SCH_IO_GEDA::trackEndpoint( int aGedaX, int aGedaY )
{
    m_netEndpoints[std::make_pair( aGedaX, aGedaY )]++;
}


// ==========================================================================
// Post-processing
// ==========================================================================

void SCH_IO_GEDA::postProcess()
{
    flushPendingComponent();
    processNetAttributes();
    addBusEntries();
    addJunctions();
    loadDeferredSheets();
}


void SCH_IO_GEDA::processNetAttributes()
{
    for( const NET_ATTR_RECORD& rec : m_netAttrRecords )
    {
        if( !rec.symbol )
            continue;

        const LIB_SYMBOL* libSym = rec.symbol->GetLibSymbolRef().get();

        if( !libSym )
            continue;

        // Find the pin with matching number
        for( SCH_PIN* pin : libSym->GetPins() )
        {
            if( pin->GetNumber() == rec.pinnumber )
            {
                VECTOR2I pinPos = rec.symbol->GetPinPhysicalPosition( pin );

                auto label = std::make_unique<SCH_GLOBALLABEL>( pinPos, rec.netname );
                int textSize = toKiCadDist( GEDA_DEFAULT_TEXT_SIZE_MILS / 2 );
                label->SetTextSize( VECTOR2I( textSize, textSize ) );

                // Build a unit direction vector for the pin in library space,
                // then transform it through the symbol's rotation/mirror matrix
                // to get the world-space pin direction.
                VECTOR2I pinDir( 0, 0 );

                switch( pin->GetOrientation() )
                {
                case PIN_ORIENTATION::PIN_RIGHT: pinDir = VECTOR2I(  1,  0 ); break;
                case PIN_ORIENTATION::PIN_LEFT:  pinDir = VECTOR2I( -1,  0 ); break;
                case PIN_ORIENTATION::PIN_UP:    pinDir = VECTOR2I(  0, -1 ); break;
                case PIN_ORIENTATION::PIN_DOWN:  pinDir = VECTOR2I(  0,  1 ); break;
                default:                         pinDir = VECTOR2I(  1,  0 ); break;
                }

                VECTOR2I worldDir = rec.symbol->GetTransform().TransformCoordinate( pinDir );

                // Orient the label away from the transformed pin direction
                if( std::abs( worldDir.x ) >= std::abs( worldDir.y ) )
                {
                    label->SetSpinStyle( worldDir.x > 0 ? SPIN_STYLE::LEFT
                                                        : SPIN_STYLE::RIGHT );
                }
                else
                {
                    label->SetSpinStyle( worldDir.y > 0 ? SPIN_STYLE::UP
                                                        : SPIN_STYLE::BOTTOM );
                }

                m_screen->Append( label.release() );
                break;
            }
        }
    }
}


void SCH_IO_GEDA::addJunctions()
{
    // Collect all wire segments and pin positions from the screen.
    std::vector<std::pair<VECTOR2I, VECTOR2I>> wireSegs;
    std::set<std::pair<int, int>>              junctionPts;

    for( SCH_ITEM* item : m_screen->Items() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* wire = static_cast<SCH_LINE*>( item );

        if( wire->GetLayer() != LAYER_WIRE )
            continue;

        wireSegs.emplace_back( wire->GetStartPoint(), wire->GetEndPoint() );
    }

    // Place junctions where 3+ endpoints coincide at the same point.
    for( const auto& [pos, count] : m_netEndpoints )
    {
        if( count >= 3 )
            junctionPts.insert( { pos.first, pos.second } );
    }

    // Detect T-junctions: a wire endpoint that lands on the interior of
    // another wire segment. Work in gEDA coordinates for exact integer match,
    // then convert when placing the junction.
    for( const auto& [pos, count] : m_netEndpoints )
    {
        if( junctionPts.count( pos ) )
            continue;

        VECTOR2I pt = toKiCad( pos.first, pos.second );

        for( const auto& [segStart, segEnd] : wireSegs )
        {
            if( pt == segStart || pt == segEnd )
                continue;

            bool isHorizontal = ( segStart.y == segEnd.y );
            bool isVertical   = ( segStart.x == segEnd.x );

            if( !isHorizontal && !isVertical )
                continue;

            bool onSeg = false;

            if( isHorizontal && pt.y == segStart.y )
            {
                int minX = std::min( segStart.x, segEnd.x );
                int maxX = std::max( segStart.x, segEnd.x );
                onSeg = ( pt.x > minX && pt.x < maxX );
            }
            else if( isVertical && pt.x == segStart.x )
            {
                int minY = std::min( segStart.y, segEnd.y );
                int maxY = std::max( segStart.y, segEnd.y );
                onSeg = ( pt.y > minY && pt.y < maxY );
            }

            if( onSeg )
            {
                junctionPts.insert( pos );
                break;
            }
        }
    }

    for( const auto& [gX, gY] : junctionPts )
    {
        VECTOR2I kicadPos = toKiCad( gX, gY );
        m_screen->Append( std::make_unique<SCH_JUNCTION>( kicadPos ).release() );
    }
}


void SCH_IO_GEDA::addBusEntries()
{
    if( m_busSegments.empty() )
        return;

    // Collect all net (wire) endpoints from the screen
    std::vector<std::pair<VECTOR2I, SCH_LINE*>> wireEndpoints;

    for( SCH_ITEM* item : m_screen->Items() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* wire = static_cast<SCH_LINE*>( item );

        if( wire->GetLayer() != LAYER_WIRE )
            continue;

        wireEndpoints.emplace_back( wire->GetStartPoint(), wire );
        wireEndpoints.emplace_back( wire->GetEndPoint(), wire );
    }

    int entrySize = schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE );

    for( const BUS_SEGMENT& bus : m_busSegments )
    {
        bool busIsHorizontal = ( bus.start.y == bus.end.y );
        bool busIsVertical = ( bus.start.x == bus.end.x );

        if( !busIsHorizontal && !busIsVertical )
            continue;

        for( const auto& [pt, wire] : wireEndpoints )
        {
            bool onBus = false;

            if( busIsHorizontal )
            {
                int minX = std::min( bus.start.x, bus.end.x );
                int maxX = std::max( bus.start.x, bus.end.x );
                onBus = ( pt.y == bus.start.y && pt.x >= minX && pt.x <= maxX );
            }
            else
            {
                int minY = std::min( bus.start.y, bus.end.y );
                int maxY = std::max( bus.start.y, bus.end.y );
                onBus = ( pt.x == bus.start.x && pt.y >= minY && pt.y <= maxY );
            }

            if( !onBus )
                continue;

            // Determine which direction the wire goes away from the bus
            VECTOR2I otherEnd = ( wire->GetStartPoint() == pt ) ? wire->GetEndPoint()
                                                                : wire->GetStartPoint();
            int dx = 0;
            int dy = 0;

            if( busIsHorizontal )
            {
                dy = ( otherEnd.y < pt.y ) ? -entrySize : entrySize;
                int sign = bus.ripperDir;

                if( sign == 0 )
                {
                    // Auto-detect from wire position relative to bus center
                    int busMidX = ( bus.start.x + bus.end.x ) / 2;
                    sign = ( pt.x <= busMidX ) ? 1 : -1;
                }

                dx = sign * entrySize;
            }
            else
            {
                dx = ( otherEnd.x < pt.x ) ? -entrySize : entrySize;
                int sign = bus.ripperDir;

                if( sign == 0 )
                {
                    int busMidY = ( bus.start.y + bus.end.y ) / 2;
                    sign = ( pt.y <= busMidY ) ? 1 : -1;
                }

                dy = sign * entrySize;
            }

            // Bus entry position is on the bus; its end (pos + size) is on the wire side
            auto entry = std::make_unique<SCH_BUS_WIRE_ENTRY>( pt );
            entry->SetSize( VECTOR2I( dx, dy ) );

            // Shorten the wire so it starts at the bus entry tip instead of the bus
            VECTOR2I entryTip = pt + VECTOR2I( dx, dy );

            if( wire->GetStartPoint() == pt )
                wire->SetStartPoint( entryTip );
            else
                wire->SetEndPoint( entryTip );

            m_screen->Append( entry.release() );
        }
    }
}


void SCH_IO_GEDA::loadDeferredSheets()
{
    for( const DEFERRED_SHEET& deferred : m_deferredSheets )
    {
        SCH_SHEET* sheet = deferred.sheet;

        SCH_SCREEN* subScreen = new SCH_SCREEN( m_schematic );
        subScreen->SetFileName( deferred.sourceFile );
        sheet->SetScreen( subScreen );

        // Use a fresh importer instance for each sub-schematic to avoid
        // clobbering parse state. Share the import stack for recursion detection
        // and the symbol library cache to avoid redundant filesystem scanning.
        SCH_IO_GEDA subImporter;
        subImporter.m_importStack = m_importStack;
        subImporter.m_importStack.insert( m_filename.GetFullPath() );
        for( const auto& [name, entry] : m_symLibrary )
        {
            SYM_CACHE_ENTRY subEntry;
            subEntry.path = entry.path;
            subImporter.m_symLibrary[name] = std::move( subEntry );
        }

        subImporter.m_symLibraryInitialized = m_symLibraryInitialized;

        try
        {
            subImporter.LoadSchematicFile( deferred.sourceFile, m_schematic,
                                           sheet, m_properties );

            // Merge any newly-discovered symbols back into the parent cache
            // so subsequent sub-schematics benefit from them.
            for( auto& [name, entry] : subImporter.m_symLibrary )
            {
                if( m_symLibrary.find( name ) == m_symLibrary.end() )
                    m_symLibrary[name] = std::move( entry );
            }
        }
        catch( const IO_ERROR& e )
        {
            if( m_reporter )
            {
                m_reporter->Report(
                        wxString::Format( _( "Failed to load sub-schematic '%s': %s" ),
                                          deferred.sourceFile, e.What() ),
                        RPT_SEVERITY_WARNING );
            }
        }
    }
}


void SCH_IO_GEDA::fitPageToContent()
{
    BOX2I bbox;

    for( SCH_ITEM* item : m_screen->Items() )
        bbox.Merge( item->GetBoundingBox() );

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
        return;

    VECTOR2I targetSize = bbox.GetSize();
    targetSize += VECTOR2I( schIUScale.MilsToIU( 1500 ), schIUScale.MilsToIU( 1500 ) );

    PAGE_INFO pageInfo = m_screen->GetPageSettings();
    VECTOR2I  pageSizeIU = pageInfo.GetSizeIU( schIUScale.IU_PER_MILS );

    if( pageSizeIU.x < targetSize.x )
        pageInfo.SetWidthMils( schIUScale.IUToMils( targetSize.x ) );

    if( pageSizeIU.y < targetSize.y )
        pageInfo.SetHeightMils( schIUScale.IUToMils( targetSize.y ) );

    m_screen->SetPageSettings( pageInfo );

    pageSizeIU = m_screen->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
    VECTOR2I sheetCentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
    VECTOR2I itemsCentre = bbox.Centre();

    VECTOR2I translation = sheetCentre - itemsCentre;
    translation.x = translation.x - translation.x % schIUScale.MilsToIU( 100 );
    translation.y = translation.y - translation.y % schIUScale.MilsToIU( 100 );

    std::vector<SCH_ITEM*> allItems;
    std::copy( m_screen->Items().begin(), m_screen->Items().end(),
               std::back_inserter( allItems ) );

    for( SCH_ITEM* item : allItems )
    {
        item->SetPosition( item->GetPosition() + translation );
        item->ClearFlags();
        m_screen->Update( item );
    }
}


// ==========================================================================
// Main entry point
// ==========================================================================

SCH_SHEET* SCH_IO_GEDA::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                             SCH_SHEET*                            aAppendToMe,
                                             const std::map<std::string, UTF8>*    aProperties )
{
    wxASSERT( !aFileName.IsEmpty() && aSchematic );

    m_properties = aProperties;
    m_schematic = aSchematic;
    m_filename = wxFileName( aFileName );
    m_libSymbols.clear();
    m_netEndpoints.clear();
    m_netAttrRecords.clear();
    m_busSegments.clear();
    m_deferredSheets.clear();
    m_pendingComp.reset();
    if( !m_symLibraryInitialized )
        m_symLibrary.clear();

    m_maxY = 0;
    m_releaseVersion = 0;
    m_fileFormatVersion = 0;
    m_powerCounter = 0;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     "Can't append to a schematic with no root!" );
        m_rootSheet = aAppendToMe;
    }
    else
    {
        m_rootSheet = new SCH_SHEET( aSchematic );
        m_rootSheet->SetFileName( aFileName );
        aSchematic->SetTopLevelSheets( { m_rootSheet } );
    }

    if( !m_rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( aFileName );
        m_rootSheet->SetScreen( screen );
        const_cast<KIID&>( m_rootSheet->m_Uuid ) = screen->GetUuid();
    }

    m_screen = m_rootSheet->GetScreen();

    wxTextFile file;

    if( !file.Open( aFileName ) )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'." ), aFileName ) );

    if( file.GetLineCount() == 0 )
        THROW_IO_ERROR( wxString::Format( _( "File '%s' is empty." ), aFileName ) );

    // First pass: scan for max Y coordinate to set up the Y-flip transform.
    // We need this before creating objects because coordinates are transformed during creation.
    for( size_t i = 0; i < file.GetLineCount(); i++ )
    {
        wxString line = file.GetLine( i );
        wxChar type = line.IsEmpty() ? '\0' : line[0];

        // Skip embedded component blocks entirely. Their coordinates are
        // symbol-local and must not affect the schematic-level Y extent.
        if( type == '[' )
        {
            while( ++i < file.GetLineCount() )
            {
                if( file.GetLine( i ).Trim() == wxT( "]" ) )
                    break;
            }

            continue;
        }

        if( type == 'C' || type == 'N' || type == 'U' || type == 'T' || type == 'L'
            || type == 'B' || type == 'V' || type == 'A' || type == 'P' || type == 'G' )
        {
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip type

            long val = 0;

            if( tok.HasMoreTokens() ) tok.GetNextToken(); // x or x1

            if( tok.HasMoreTokens() )
            {
                tok.GetNextToken().ToLong( &val );

                if( val > m_maxY )
                    m_maxY = static_cast<int>( val );
            }

            if( type == 'N' || type == 'U' || type == 'L' || type == 'P' )
            {
                if( tok.HasMoreTokens() ) tok.GetNextToken(); // x2

                if( tok.HasMoreTokens() )
                {
                    tok.GetNextToken().ToLong( &val );

                    if( val > m_maxY )
                        m_maxY = static_cast<int>( val );
                }
            }

            if( type == 'B' )
            {
                long bw = 0, bh = 0;

                if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &bw );
                if( tok.HasMoreTokens() ) tok.GetNextToken().ToLong( &bh );

                long top = val + bh;

                if( top > m_maxY )
                    m_maxY = static_cast<int>( top );
            }

            if( type == 'V' || type == 'A' )
            {
                // V: cx cy radius ...  A: cx cy radius ...
                // After reading cy into val, the next token is radius
                long radius = 0;

                if( tok.HasMoreTokens() )
                    tok.GetNextToken().ToLong( &radius );

                long extent = val + radius;

                if( extent > m_maxY )
                    m_maxY = static_cast<int>( extent );
            }

            // T lines have content lines that follow the header. Skip them
            // to avoid misinterpreting text as object lines.
            // Format: T x y color size vis show_nv angle align num_lines
            // tok has consumed: type, x (skipped), y (read). Skip remaining 6
            // fields to reach num_lines.
            if( type == 'T' )
            {
                long numTextLines = 1;

                for( int j = 0; j < 6 && tok.HasMoreTokens(); j++ )
                    tok.GetNextToken();

                if( tok.HasMoreTokens() )
                    tok.GetNextToken().ToLong( &numTextLines );

                i += static_cast<size_t>( numTextLines );
            }
        }
        else if( type == 'H' )
        {
            // Scan H path lines for Y coordinates in subsequent lines
            wxStringTokenizer tok( line );
            tok.GetNextToken(); // skip 'H'
            long numlines = 0;

            for( int j = 0; j < 12 && tok.HasMoreTokens(); j++ )
                tok.GetNextToken();

            if( tok.HasMoreTokens() )
                tok.GetNextToken().ToLong( &numlines );

            for( long j = 0; j < numlines && ( i + 1 ) < file.GetLineCount(); j++ )
            {
                i++;
                wxString pathLine = file.GetLine( i );
                wxStringTokenizer ptok( pathLine, wxT( " ,\t" ) );

                // Path data uses SVG-like commands: M x,y  L x,y  C x1,y1 x2,y2 x3,y3  z
                // Coordinate pairs alternate X then Y. Command letters reset to X.
                bool nextIsY = false;

                while( ptok.HasMoreTokens() )
                {
                    wxString token = ptok.GetNextToken();

                    if( token.length() == 1 && wxIsalpha( token[0] ) )
                    {
                        nextIsY = false;
                        continue;
                    }

                    long pval = 0;

                    if( token.ToLong( &pval ) )
                    {
                        if( nextIsY && pval > m_maxY )
                            m_maxY = static_cast<int>( pval );

                        nextIsY = !nextIsY;
                    }
                }
            }
        }
    }

    m_maxY += 1000;

    // Parse version line
    size_t lineIdx = 0;
    wxString firstLine = file.GetLine( lineIdx );
    lineIdx++;

    if( !parseVersionLine( firstLine ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "File '%s' is not a valid gEDA schematic." ),
                                           aFileName ) );
    }

    // Main parse loop
    while( lineIdx < file.GetLineCount() )
    {
        wxString line = file.GetLine( lineIdx );
        lineIdx++;

        if( line.IsEmpty() )
            continue;

        wxChar type = line[0];

        switch( type )
        {
        case 'C':
            parseComponent( line, file, lineIdx );
            break;

        case 'N':
            parseNet( line, file, lineIdx );
            break;

        case 'U':
            parseBus( line, file, lineIdx );
            break;

        case 'T':
            parseText( line, file, lineIdx );
            break;

        case 'L':
            parseLine( line, file, lineIdx );
            break;

        case 'B':
            parseBox( line, file, lineIdx );
            break;

        case 'V':
            parseCircle( line, file, lineIdx );
            break;

        case 'A':
            parseArc( line, file, lineIdx );
            break;

        case 'H':
            parsePath( line, file, lineIdx );
            break;

        case 'G':
            parsePicture( line, file, lineIdx );
            break;

        case 'P':
            parsePin( line, file, lineIdx );
            break;

        case '[':
            parseEmbeddedComponent( file, lineIdx );
            break;

        case '{':
        {
            while( lineIdx < file.GetLineCount() )
            {
                if( file.GetLine( lineIdx ).Trim() == wxT( "}" ) )
                {
                    lineIdx++;
                    break;
                }

                lineIdx++;
            }

            break;
        }

        case '#':
        case 'v':
        default:
            break;
        }
    }

    postProcess();

    // Size the page to fit the imported content and center it, following
    // the same approach as the Eagle importer.
    fitPageToContent();

    // Multi-page support: when the project handler passes additional schematic
    // files, create sub-sheets for each and load them into the hierarchy.
    if( aProperties )
    {
        auto it = aProperties->find( "additional_schematics" );

        if( it != aProperties->end() )
        {
            wxString additionalFiles = wxString::FromUTF8( it->second.c_str() );
            wxStringTokenizer tok( additionalFiles, wxT( ";" ) );
            int sheetY = schIUScale.MilsToIU( 500 );
            int sheetSpacing = schIUScale.MilsToIU( 2000 );
            int pageNum = 2;

            while( tok.HasMoreTokens() )
            {
                wxString filePath = tok.GetNextToken();
                wxFileName fn( filePath );

                VECTOR2I pos( schIUScale.MilsToIU( 500 ), sheetY );
                VECTOR2I size( schIUScale.MilsToIU( 2000 ), schIUScale.MilsToIU( 1500 ) );

                auto subSheet = std::make_unique<SCH_SHEET>( m_rootSheet, pos, size );

                subSheet->GetField( FIELD_T::SHEET_NAME )->SetText(
                        wxString::Format( wxT( "Page %d" ), pageNum ) );
                subSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fn.GetFullName() );
                subSheet->SetFileName( fn.GetFullName() );

                SCH_SHEET* subSheetPtr = subSheet.get();
                m_screen->Append( subSheet.release() );

                if( fn.FileExists() )
                {
                    SCH_SCREEN* subScreen = new SCH_SCREEN( m_schematic );
                    subScreen->SetFileName( fn.GetFullPath() );
                    subSheetPtr->SetScreen( subScreen );

                    SCH_IO_GEDA subImporter;

                    for( const auto& [name, entry] : m_symLibrary )
                    {
                        SYM_CACHE_ENTRY copy;
                        copy.path = entry.path;
                        copy.symversion = entry.symversion;
                        copy.netAttr = entry.netAttr;
                        subImporter.m_symLibrary[name] = std::move( copy );
                    }

                    subImporter.m_symLibraryInitialized = m_symLibraryInitialized;

                    try
                    {
                        subImporter.LoadSchematicFile( fn.GetFullPath(), m_schematic,
                                                       subSheetPtr, nullptr );
                    }
                    catch( const IO_ERROR& e )
                    {
                        if( m_reporter )
                        {
                            m_reporter->Report(
                                    wxString::Format( _( "Failed to load page '%s': %s" ),
                                                      fn.GetFullName(), e.What() ),
                                    RPT_SEVERITY_WARNING );
                        }
                    }
                }

                sheetY += sheetSpacing;
                pageNum++;
            }
        }
    }

    return m_rootSheet;
}
