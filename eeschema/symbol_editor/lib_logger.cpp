/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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


#include <wx/intl.h>
#include <lib_logger.h>

LIB_LOGGER::LIB_LOGGER() : m_previousLogger( nullptr ), m_activated( false )
{
}

LIB_LOGGER::~LIB_LOGGER()
{
    Deactivate();
}

void LIB_LOGGER::Activate()
{
    if( !m_activated )
    {
        m_previousLogger = wxLog::GetActiveTarget();
        wxLog::SetActiveTarget( this );
        m_activated = true;
    }
}

void LIB_LOGGER::Deactivate()
{
    if( m_activated )
    {
        Flush();
        m_activated = false;
        wxLog::SetActiveTarget( m_previousLogger );
    }
}


void LIB_LOGGER::Flush()
{
    if( m_bHasMessages )
    {
        wxLogMessage( _( "Not all symbol libraries could be loaded.  Use the Manage Symbol\n"
                         "Libraries dialog to adjust paths and add or remove libraries." ) );
        wxLogGui::Flush();
    }
}