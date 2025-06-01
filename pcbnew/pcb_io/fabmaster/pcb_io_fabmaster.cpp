/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 BeagleBoard Foundation
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @brief Pcbnew PLUGIN for FABMASTER ASCII *.txt / *.fab format.
 */

#include "pcb_io_fabmaster.h"
#include <board.h>
#include <progress_reporter.h>
#include <common.h>
#include <macros.h>


PCB_IO_FABMASTER::PCB_IO_FABMASTER() : PCB_IO( wxS( "Fabmaster" ) )
{
}


PCB_IO_FABMASTER::~PCB_IO_FABMASTER()
{
}


BOARD* PCB_IO_FABMASTER::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                    const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "File import canceled by user." ) );
    }

    if( !m_fabmaster.Read( aFileName.ToStdString() ) )
    {
        std::string readerr;

        readerr = _( "Could not read file " ) + aFileName.ToStdString();
        THROW_IO_ERROR( readerr );
    }

    m_fabmaster.Process();
    m_fabmaster.LoadBoard( m_board, m_progressReporter );

    return m_board;
}
