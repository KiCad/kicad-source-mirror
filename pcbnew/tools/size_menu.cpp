/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 CERN
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
#include <bitmaps.h>


TRACK_VIA_SIZE_MENU::TRACK_VIA_SIZE_MENU( bool aTrackSizes, bool aViaSizes ) :
    m_designSettings( nullptr ), m_tracks( aTrackSizes ), m_vias( aViaSizes )
{
    SetIcon( width_track_via_xpm );
}


void TRACK_VIA_SIZE_MENU::AppendSizes( const BOARD* aBoard )
{
    wxCHECK( aBoard, /* void */ );

    m_designSettings = &aBoard->GetDesignSettings();

    if( m_tracks )
    {
        for( unsigned i = 0; i < m_designSettings->m_TrackWidthList.size(); i++ )
            Append( ID_POPUP_PCB_SELECT_WIDTH1 + i, getTrackDescription( i ), wxEmptyString, wxITEM_CHECK );
    }

    if( m_tracks && m_vias )
        AppendSeparator();

    if( m_vias )
    {
        for( unsigned i = 0; i < m_designSettings->m_ViasDimensionsList.size(); i++ )
            Append( ID_POPUP_PCB_SELECT_VIASIZE1 + i, getViaDescription( i ), wxEmptyString, wxITEM_CHECK );
    }
}


void TRACK_VIA_SIZE_MENU::update()
{
    if( m_tracks )
    {
        size_t pos;
        unsigned int i;
        wxMenuItem* lastEntry = FindChildItem( ID_POPUP_PCB_SELECT_WIDTH1, &pos );
        wxCHECK( lastEntry, /* void */ );

        // Start update with index 1, as 0 is reserved for the 'net class' size
        for( i = 1; i < m_designSettings->m_TrackWidthList.size(); i++ )
        {
            wxMenuItem* menuItem = FindItem( ID_POPUP_PCB_SELECT_WIDTH1 + i );

            if( menuItem )      // Update an existing entry
            {
                menuItem->SetItemLabel( getTrackDescription( i ) );
            }
            else                // Add a missing entry
            {
                Insert( pos + i, ID_POPUP_PCB_SELECT_WIDTH1 + i, getTrackDescription( i ),
                        wxEmptyString, wxITEM_CHECK );
            }
        }

        // Remove entries that have been removed from the design settings
        while( ( lastEntry = FindItem( ID_POPUP_PCB_SELECT_WIDTH1 + i ) ) )
        {
            Destroy( lastEntry );
            ++i;
        }
    }

    if( m_vias )
    {
        size_t pos;
        unsigned int i;
        wxMenuItem* lastEntry = FindChildItem( ID_POPUP_PCB_SELECT_VIASIZE1, &pos );
        wxCHECK( lastEntry, /* void */ );

        // Start update with index 1, as 0 is reserved for the 'net class' size
        for( i = 1; i < m_designSettings->m_ViasDimensionsList.size(); i++ )
        {
            wxMenuItem* menuItem = FindItem( ID_POPUP_PCB_SELECT_VIASIZE1 + i );

            if( menuItem )      // Update an existing entry
            {
                menuItem->SetItemLabel( getViaDescription( i ) );
            }
            else                // Add a missing entry
            {
                Insert( pos + i, ID_POPUP_PCB_SELECT_VIASIZE1 + i, getViaDescription( i ),
                        wxEmptyString, wxITEM_CHECK );
            }
        }

        // Remove entries that have been removed from the design settings
        while( ( lastEntry = FindItem( ID_POPUP_PCB_SELECT_VIASIZE1 + i ) ) )
        {
            Destroy( lastEntry );
            ++i;
        }
    }
}


wxString TRACK_VIA_SIZE_MENU::getTrackDescription( unsigned int aIndex ) const
{
    wxString desc;

    if( m_vias )        // == if( m_tracks && m_vias )
        desc = _( "Track ");

    if( aIndex == 0 )
        desc << _( "net class width" );
    else
        desc << StringFromValue( g_UserUnit, m_designSettings->m_TrackWidthList[aIndex], true );

    return desc;
}


wxString TRACK_VIA_SIZE_MENU::getViaDescription( unsigned int aIndex ) const
{
    wxString desc;

    if( m_tracks )      // == if( m_tracks && m_vias )
        desc = _( "Via " );

    if( aIndex == 0 )
    {
        desc << _( "net class size" );
    }
    else
    {
        desc << StringFromValue( g_UserUnit,
                m_designSettings->m_ViasDimensionsList[aIndex].m_Diameter, true );

        if( m_designSettings->m_ViasDimensionsList[aIndex].m_Drill <= 0 )
        {
            desc << _( ", drill: default" );
        }
        else
        {
            desc << _( ", drill: " ) << StringFromValue( g_UserUnit,
                    m_designSettings->m_ViasDimensionsList[aIndex].m_Drill, true );
        }
    }

    return desc;
}
