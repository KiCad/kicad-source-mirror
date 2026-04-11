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

#include <cli_progress_reporter.h>
#include <wx/crt.h>


CLI_PROGRESS_REPORTER& CLI_PROGRESS_REPORTER::GetInstance()
{
    static CLI_PROGRESS_REPORTER s_cliReporter;

    return s_cliReporter;
}


void CLI_PROGRESS_REPORTER::AdvancePhase( const wxString& aMessage )
{
    if( m_verbose )
        printLine( aMessage );
}


void CLI_PROGRESS_REPORTER::Report( const wxString& aMessage )
{
    if( m_verbose )
        printLine( aMessage );
}


void CLI_PROGRESS_REPORTER::printLine( const wxString& aMessage )
{
    if( aMessage.EndsWith( wxS( "\n" ) ) )
        wxFprintf( stdout, aMessage );
    else
        wxFprintf( stdout, aMessage + wxS( "\n" ) );
}