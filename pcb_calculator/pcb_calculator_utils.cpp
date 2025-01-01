/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
 */

#include <pcb_calculator_utils.h>
#include <locale.h>

bool findMatch( wxArrayString& aList, const wxString& aValue, int& aIdx )
{
    bool success = false;
    // Find the previous choice index:
    aIdx = 0;

    // Some countries use comma instead of point as separator.
    // The value can be enter with pint or comma
    // use point for string comparisons:
    wxString cvalue = aValue;
    cvalue.Replace( ',', '.' );

    // First compare strings:
    for( wxString& text : aList )
    {
        if( text.IsEmpty() ) // No match found: select the empty line choice
            break;

        wxString val_str = text.BeforeFirst( ' ' );
        val_str.Replace( ',', '.' );

        // compare string values
        if( val_str == cvalue )
        {
            success = true;
            break;
        }

        aIdx++;
    }

    // Due to multiple ways to write a double, if string values
    // do not match, compare double values
    if( !success )
    {
        struct lconv* lc = localeconv();
        char          localeDecimalSeparator = *lc->decimal_point;

        if( localeDecimalSeparator == ',' )
            cvalue.Replace( '.', ',' );

        double curr_value;
        cvalue.ToDouble( &curr_value );

        aIdx = 0;

        for( wxString& text : aList )
        {
            if( text.IsEmpty() ) // No match found: select the empty line choice
                break;

            double   val;
            wxString val_str = text.BeforeFirst( ' ' );

            if( localeDecimalSeparator == ',' )
                val_str.Replace( '.', ',' );

            val_str.ToDouble( &val );;

            if( curr_value == val )
            {
                success = true;
                break;
            }

            aIdx++;
        }
    }

    return success;
}
