/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gestfich.h>
#include <launch_ext.h>


void LaunchExternal( const wxString& aPath )
{
#ifdef __WXMAC__
    wxExecute( wxString::Format( "open \"%s\"", aPath ) );
#else
    wxString path( aPath );

#if !wxCHECK_VERSION( 3, 1, 0 )
    // Quote in case there are spaces in the path.
    // Not needed on 3.1.4, but needed in 3.0 versions
    // Moreover, on Linux, on 3.1.4 wx version, adding quotes breaks
    // wxLaunchDefaultApplication
    QuoteString( path );
#endif

    wxLaunchDefaultApplication( path );
#endif
}


void LaunchURL( const wxString& aUrl )
{
#ifdef __WXMAC__
    wxExecute( wxString::Format( "open %s", aUrl ) );
#else
    wxLaunchDefaultApplication( aUrl );
#endif
}
