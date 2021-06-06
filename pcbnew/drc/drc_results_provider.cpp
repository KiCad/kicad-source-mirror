/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <board_design_settings.h>
#include "drc_results_provider.h"

void BOARD_DRC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    m_filteredMarkers.clear();

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        SEVERITY markerSeverity;

        if( marker->IsExcluded() )
            markerSeverity = RPT_SEVERITY_EXCLUSION;
        else
            markerSeverity = bds.GetSeverity( marker->GetRCItem()->GetErrorCode() );

        if( markerSeverity & m_severities )
            m_filteredMarkers.push_back( marker );
    }
}


int BOARD_DRC_ITEMS_PROVIDER::GetCount( int aSeverity ) const
{
    if( aSeverity < 0 )
        return m_filteredMarkers.size();

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    int count = 0;

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        SEVERITY markerSeverity;

        if( marker->IsExcluded() )
            markerSeverity = RPT_SEVERITY_EXCLUSION;
        else
            markerSeverity = bds.GetSeverity( marker->GetRCItem()->GetErrorCode() );

        if( markerSeverity == aSeverity )
            count++;
    }

    return count;
}


void VECTOR_DRC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

    m_filteredVector.clear();

    if( m_sourceVector )
    {
        for( const std::shared_ptr<DRC_ITEM>& item : *m_sourceVector )
        {
            if( bds.GetSeverity( item->GetErrorCode() ) & aSeverities )
                m_filteredVector.push_back( item );
        }
    }
}


int VECTOR_DRC_ITEMS_PROVIDER::GetCount( int aSeverity ) const
{
    if( aSeverity < 0 )
        return m_filteredVector.size();

    int                    count = 0;
    BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

    if( m_sourceVector )
    {
        for( const std::shared_ptr<DRC_ITEM>& item : *m_sourceVector )
        {
            if( bds.GetSeverity( item->GetErrorCode() ) == aSeverity )
                count++;
        }
    }

    return count;
}