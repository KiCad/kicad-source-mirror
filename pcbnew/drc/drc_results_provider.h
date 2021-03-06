/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 KiCad Developers, see change_log.txt for contributors.
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


#ifndef DRC_PROVIDER__H
#define DRC_PROVIDER__H

#include <board.h>
#include <pcb_marker.h>
#include <pcb_base_frame.h>
#include <drc/drc_item.h>
#include <widgets/ui_common.h>
#include <functional>


/**
 * BOARD_DRC_ITEMS_PROVIDER
 * is an implementation of the RC_ITEMS_PROVIDER interface which uses a BOARD instance
 * to fulfill the interface.
 */
class BOARD_DRC_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
private:
    BOARD*                   m_board;

    int                      m_severities;
    std::vector<PCB_MARKER*> m_filteredMarkers;

public:
    BOARD_DRC_ITEMS_PROVIDER( BOARD* aBoard ) :
            m_board( aBoard ),
            m_severities( 0 )
    {
    }

    void SetSeverities( int aSeverities ) override
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

    int GetCount( int aSeverity = -1 ) const override
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

    std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const override
    {
        PCB_MARKER* marker = m_filteredMarkers[ aIndex ];

        return marker ? marker->GetRCItem() : nullptr;
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        PCB_MARKER* marker = m_filteredMarkers[ aIndex ];
        m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

        if( aDeep )
            m_board->Delete( marker );
    }

    void DeleteAllItems( bool aIncludeExclusions, bool aDeep ) override
    {
        // Filtered list was already handled through DeleteItem() by the tree control

        if( aDeep )
            m_board->DeleteMARKERs( true, aIncludeExclusions );
    }
};


/**
 * VECTOR_DRC_ITEMS_PROVIDER
 * is an implementation of the interface named DRC_ITEMS_PROVIDER which uses a vector
 * of pointers to DRC_ITEMs to fulfill the interface.  No ownership is taken of the
 * vector.
 */
class VECTOR_DRC_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
    PCB_BASE_FRAME*         m_frame;
    std::vector<std::shared_ptr<DRC_ITEM> >* m_sourceVector;     // owns its DRC_ITEMs

    int                     m_severities;
    std::vector<std::shared_ptr<DRC_ITEM> >  m_filteredVector;   // does not own its DRC_ITEMs

public:

    VECTOR_DRC_ITEMS_PROVIDER( PCB_BASE_FRAME* aFrame, std::vector<std::shared_ptr<DRC_ITEM> >* aList ) :
            m_frame( aFrame ),
            m_sourceVector( aList ),
            m_severities( 0 )
    {
    }

    void SetSeverities( int aSeverities ) override
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

    int GetCount( int aSeverity = -1 ) const override
    {
        if( aSeverity < 0 )
            return m_filteredVector.size();

        int count = 0;
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

    std::shared_ptr<RC_ITEM> GetItem( int aIndex ) const override
    {
        return (m_filteredVector)[aIndex];
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        auto item = m_filteredVector[aIndex];
        m_filteredVector.erase( m_filteredVector.begin() + aIndex );

        if( aDeep )
        {
            for( size_t i = 0; i < m_sourceVector->size(); ++i )
            {
                if( m_sourceVector->at( i ) == item )
                {
                    m_sourceVector->erase( m_sourceVector->begin() + i );
                    break;
                }
            }
        }
    }

    void DeleteAllItems( bool aIncludeExclusions, bool aDeep ) override
    {
        if( aDeep )
        {
            m_sourceVector->clear();
        }

        m_filteredVector.clear();   // no ownership of DRC_ITEM pointers
    }
};


/**
 * RATSNEST_DRC_ITEMS_PROVIDER
 */
class RATSNEST_DRC_ITEMS_PROVIDER : public VECTOR_DRC_ITEMS_PROVIDER
{
    // TODO: for now this is just a vector, but we need to map it to some board-level
    // data-structure so that deleting/excluding things can do a deep delete/exclusion
    // which will be reflected in the ratsnest....
public:
    RATSNEST_DRC_ITEMS_PROVIDER( PCB_BASE_FRAME* aFrame, std::vector<std::shared_ptr<DRC_ITEM> >* aList ) :
            VECTOR_DRC_ITEMS_PROVIDER( aFrame, aList )
    { }
};


#endif // DRC_PROVIDER__H
