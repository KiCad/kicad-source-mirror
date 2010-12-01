/*****************/
/* am_param.cpp */
/*****************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "class_am_param.h"

extern int    ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );

/*Class AM_PARAM
 * holds a parameter for an "aperture macro" as defined within
 * standard RS274X.  The \a value field can be a constant, i.e. "immediate"
 * parameter or it may not be used if this param is going to defer to the
 * referencing aperture macro.  In that case, the \a index field is an index
 * into the aperture macro's parameters.
 */

double AM_PARAM::GetValue( const D_CODE* aDcode ) const
{
    if( IsImmediate() )
        return value;
    else
    {
        // the first one was numbered 1, not zero, as in $1, see page 19 of spec.
        unsigned ndx = GetIndex();
        wxASSERT( aDcode );

        // get the parameter from the aDcode
        if( ndx <= aDcode->GetParamCount() )
            return aDcode->GetParam( ndx );
        else
        {
            wxASSERT( GetIndex() <= aDcode->GetParamCount() );
            return 0.0;
        }
    }
}


/**
 * Function ReadParam
 * Read one aperture macro parameter
 * a parameter can be:
 *      a number
 *      a reference to an aperture definition parameter value: $1 ot $3 ...
 * a parameter definition can be complex and have operators between numbers and/or other parameter
 * like $1+3 or $2x2..
 * Parameters are separated by a comma ( of finish by *)
 * @param aText = pointer to the parameter to read. Will be modified to point to the next field
 * @return true if a param is read, or false
 */
bool AM_PARAM::ReadParam( char*& aText  )
{
    bool found = false;

    if( *aText == '$' ) // value defined later, in aperture description
    {
        ++aText;
        SetIndex( ReadInt( aText, false ) );
        found = true;
    }
    else
    {
        SetValue( ReadDouble( aText, false ) );
        found = true;
    }

    // Skip extra characters and separator
    while( *aText && (*aText != ',') && (*aText != '*') )
        aText++;

    if( *aText == ',' )
        aText++;

    return found;
}
