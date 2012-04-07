/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <kicad_string.h>
#include <common.h>
#include <build_version.h>      // LEGACY_BOARD_FILE_VERSION

#include <class_board.h>
#include <kicad_plugin.h>

#include <wx/wfstream.h>


void KICAD_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    LOCALE_IO toggle;     // toggles on, then off, the C locale.

    m_board = aBoard;

    wxFileOutputStream fs( aFileName );

    if( !fs.IsOk() )
    {
        m_error.Printf( _( "cannot open file '%s'" ), aFileName.GetData() );
        THROW_IO_ERROR( m_error );
    }

    STREAM_OUTPUTFORMATTER formatter( fs );

    formatter.Print( 0, "(kicad-board (version %d) (host pcbnew %s)\n", SEXPR_BOARD_FILE_VERSION,
                     formatter.Quotew( GetBuildVersion() ).c_str() );
    aBoard->Format( (OUTPUTFORMATTER*) &formatter, 1, 0 );
    formatter.Print( 0, ")\n" );
}
