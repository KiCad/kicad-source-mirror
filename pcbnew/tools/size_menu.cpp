/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "size_menu.h"

#include <class_board.h>
#include <pcbnew_id.h>

CONTEXT_TRACK_VIA_SIZE_MENU::CONTEXT_TRACK_VIA_SIZE_MENU( bool aTrackSizes, bool aViaSizes ) :
    m_tracks( aTrackSizes ), m_vias( aViaSizes )
{
    SetIcon( width_track_via_xpm );
}


void CONTEXT_TRACK_VIA_SIZE_MENU::AppendSizes( const BOARD* aBoard )
{
    wxString msg;

    const BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    if( m_tracks )
    {
        for( unsigned i = 0; i < bds.m_TrackWidthList.size(); i++ )
        {
            if( m_vias )        // == if( m_tracks && m_vias )
                msg = _( "Track ");

            if( i == 0 )
                msg << _( "net class width" );
            else
                msg << StringFromValue( g_UserUnit, bds.m_TrackWidthList[i], true );

            Append( ID_POPUP_PCB_SELECT_WIDTH1 + i, msg, wxEmptyString, wxITEM_CHECK );
        }
    }

    if( m_tracks && m_vias )
        AppendSeparator();

    if( m_vias )
    {
        for( unsigned i = 0; i < bds.m_ViasDimensionsList.size(); i++ )
        {
            if( m_tracks )      // == if( m_tracks && m_vias )
                msg = _( "Via " );

            if( i == 0 )
            {
                msg << _( "net class size" );
            }
            else
            {
                msg << StringFromValue( g_UserUnit, bds.m_ViasDimensionsList[i].m_Diameter, true );
                wxString drill = StringFromValue( g_UserUnit,
                                                    bds.m_ViasDimensionsList[i].m_Drill, true );

                if( bds.m_ViasDimensionsList[i].m_Drill <= 0 )
                    msg << _( ", drill: default" );
                else
                    msg << _( ", drill: " ) << drill;
            }

            Append( ID_POPUP_PCB_SELECT_VIASIZE1 + i, msg, wxEmptyString, wxITEM_CHECK );
        }
    }
}
