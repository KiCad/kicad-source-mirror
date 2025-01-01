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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cli_progress_reporter.h>
#include <wx/crt.h>


PROGRESS_REPORTER& CLI_PROGRESS_REPORTER::GetInstance()
{
    static CLI_PROGRESS_REPORTER s_cliReporter;

    return s_cliReporter;
}


void CLI_PROGRESS_REPORTER::AdvancePhase( const wxString& aMessage )
{
    printLine( aMessage );
}


void CLI_PROGRESS_REPORTER::Report( const wxString& aMessage )
{
    printLine( aMessage );
}


void CLI_PROGRESS_REPORTER::printLine( const wxString& aMessage )
{
    if( aMessage.EndsWith( wxS( "\n" ) ) )
        wxFprintf( stdout, aMessage );
    else
        wxFprintf( stdout, aMessage + wxS( "\n" ) );
}