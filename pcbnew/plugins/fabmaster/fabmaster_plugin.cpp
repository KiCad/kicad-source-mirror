/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 BeagleBoard Foundation
 * Copyright (C) 2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcad_plugin.cpp
 * @brief Pcbnew PLUGIN for FABMASTER ASCII *.txt / *.fab format.
 */

#include "fabmaster_plugin.h"
#include <board.h>

#include <common.h>
#include <macros.h>


FABMASTER_PLUGIN::FABMASTER_PLUGIN()
{
    m_board = NULL;
    m_props = NULL;
}


FABMASTER_PLUGIN::~FABMASTER_PLUGIN()
{
}


const wxString FABMASTER_PLUGIN::PluginName() const
{
    return wxT( "Fabmaster" );
}


const wxString FABMASTER_PLUGIN::GetFileExtension() const
{
    return wxT( "txt" );
}


BOARD* FABMASTER_PLUGIN::Load( const wxString &aFileName, BOARD *aAppendToMe,
                               const PROPERTIES *aProperties, PROJECT *aProject )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    if( !m_fabmaster.Read( aFileName.ToStdString() ) )
    {
        std::string readerr;

        readerr = _( "Could not read file " ) + aFileName.ToStdString();
        THROW_IO_ERROR( readerr );
    }

    m_fabmaster.Process();
    m_fabmaster.LoadBoard( m_board );
    return m_board;
}
