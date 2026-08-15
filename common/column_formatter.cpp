/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "column_formatter.h"

#include <wx/translation.h>

#include <i18n_utility.h>


#define BOOL_TRUE _HKI( "True" )
#define BOOL_FALSE _HKI( "False" )


bool MatchTranslationOrNative( const wxString& aStr, const wxString& aNativeLabel, bool aCaseSensitive )
{
    return wxGetTranslation( aNativeLabel ).IsSameAs( aStr, aCaseSensitive )
           || aStr.IsSameAs( aNativeLabel, aCaseSensitive );
}


wxString COLUMN_FORMATTER::stringFromBool( bool aValue ) const
{
    switch( m_boolFormat )
    {
    case BOOL_FORMAT::ZERO_ONE:
        return aValue ? wxT( "1" ) : wxT( "0" );
    case BOOL_FORMAT::TRUE_FALSE:
        return wxGetTranslation( aValue ? BOOL_TRUE : BOOL_FALSE );
    default:
        wxFAIL_MSG( "Invalid BOOL_FORMAT" );
        return wxEmptyString;
    }

}


bool COLUMN_FORMATTER::boolFromString( const wxString& aValue, REPORTER& aReporter ) const
{
    if( aValue == wxS( "1" ) )
        return true;
    else if( aValue == wxS( "0" ) )
        return false;
    else if( MatchTranslationOrNative( aValue, BOOL_TRUE, false ) )
        return true;
    else if( MatchTranslationOrNative( aValue, BOOL_FALSE, false ) )
        return false;

    aReporter.Report( wxString::Format( _( "The value '%s' can't be converted to boolean correctly, "
                                           "it has been interpreted as 'False'" ),
                                        aValue ),
                      RPT_SEVERITY_ERROR );
    return false;
}