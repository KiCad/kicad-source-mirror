/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <widgets/report_severity.h>
#include <wx/intl.h>
#include <i18n_utility.h>
#include <vector>


wxString formatSeverities( int aSeverities )
{
    wxString result;
    std::vector<wxString> items;

    if( aSeverities & RPT_SEVERITY_ERROR )
        items.push_back( _( "Errors" ) );

    if( aSeverities & RPT_SEVERITY_WARNING )
        items.push_back( _( "Warnings" ) );

    if( aSeverities & RPT_SEVERITY_EXCLUSION )
        items.push_back( _( "Exclusions" ) );

    if( items.empty() )
        return _( "None" );

    for( size_t i = 0; i < items.size(); i++ )
    {
        result += items[i];

        if( i < items.size() - 1 )
            result += wxS( ", " );
    }

    return result;
}

