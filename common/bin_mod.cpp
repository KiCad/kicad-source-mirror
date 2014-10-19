/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <bin_mod.h>
#include <online_help.h>
#include <common.h>


BIN_MOD::BIN_MOD( const char* aName ) :
    m_name( aName ),
    m_config( 0 )
{
}


void BIN_MOD::Init()
{
    // do an OS specific wxConfig instantiation, using the bin_mod (EXE/DLL/DSO) name.
    m_config = GetNewConfig( wxString::FromUTF8( m_name ) );

    m_history.Load( *m_config );

    // Prepare On Line Help. Use only lower case for help file names, in order to
    // avoid problems with upper/lower case file names under windows and unix.
    // Help files are now using html format.
    // Old help files used pdf format.
    // so when searching a help file, the .html file will be searched,
    // and if not found, the .pdf file  will be searched.
    m_help_file = wxString::FromUTF8( m_name );     // no ext given. can be .html or .pdf
}


void BIN_MOD::End()
{
    if( m_config )
    {
        m_history.Save( *m_config );

        // Deleting a wxConfigBase writes its contents to disk if changed.
        // Might be NULL if called twice, in which case nothing happens.
        delete m_config;
        m_config = 0;
    }
}


BIN_MOD::~BIN_MOD()
{
    End();
}

