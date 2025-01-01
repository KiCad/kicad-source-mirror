/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#include <bin_mod.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/settings_manager.h>


BIN_MOD::BIN_MOD( const char* aName ) :
    m_name( aName ),
    m_config( nullptr )
{
}


void BIN_MOD::Init()
{
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
        // The settings manager will outlive this module so we need to clean up the module level
        // settings here instead of leaving it up to the manager
        Pgm().GetSettingsManager().FlushAndRelease( m_config );
        m_config = nullptr;
    }
}


BIN_MOD::~BIN_MOD()
{
}

