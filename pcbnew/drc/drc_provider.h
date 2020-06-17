/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see change_log.txt for contributors.
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

#include <class_board.h>
#include <class_marker_pcb.h>
#include <pcb_base_frame.h>
#include <drc/drc.h>
#include <widgets/ui_common.h>
#include <functional>


/**
 * DRC_TEST_PROVIDER
 * is a base class that represents a DRC "provider" which runs some DRC functions over a
 * #BOARD and spits out #PCB_MARKERs as needed.
 */
class DRC_TEST_PROVIDER
{
public:
    /**
     * A callable that can handle a single generated PCB_MARKER
     */
    using MARKER_HANDLER = std::function<void( MARKER_PCB* )>;

    /**
     * Runs this provider against the given PCB with configured options (if any).
     *
     * Note: Board is non-const, as some DRC functions modify the board (e.g. zone fill
     * or polygon coalescing)
     */
    virtual bool RunDRC( EDA_UNITS aUnits, BOARD& aBoard ) = 0;

    virtual ~DRC_TEST_PROVIDER() {}

protected:
    DRC_TEST_PROVIDER( MARKER_HANDLER aMarkerHandler ) :
            m_marker_handler( std::move( aMarkerHandler ) )
    {
    }

    /**
     * Pass a given marker to the marker handler
     */
    void HandleMarker( MARKER_PCB* aMarker ) const
    {
        m_marker_handler( aMarker );
    }

private:
    /// The handler for any generated markers
    MARKER_HANDLER m_marker_handler;
};


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
    std::vector<MARKER_PCB*> m_filteredMarkers;

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

        for( MARKER_PCB* marker : m_board->Markers() )
        {
            int markerSeverity;

            if( marker->IsExcluded() )
                markerSeverity = RPT_SEVERITY_EXCLUSION;
            else
                markerSeverity = bds.GetSeverity( marker->GetRCItem()->GetErrorCode() );

            if( markerSeverity & m_severities )
                m_filteredMarkers.push_back( marker );
        }
    }

    int GetCount( int aSeverity = -1 ) override
    {
        if( aSeverity < 0 )
            return m_filteredMarkers.size();

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        int count = 0;

        for( MARKER_PCB* marker : m_board->Markers() )
        {
            int markerSeverity;

            if( marker->IsExcluded() )
                markerSeverity = RPT_SEVERITY_EXCLUSION;
            else
                markerSeverity = bds.GetSeverity( marker->GetRCItem()->GetErrorCode() );

            if( markerSeverity == aSeverity )
                count++;
        }

        return count;
    }

    DRC_ITEM* GetItem( int aIndex ) override
    {
        MARKER_PCB* marker = m_filteredMarkers[ aIndex ];

        return marker ? static_cast<DRC_ITEM*>( marker->GetRCItem() ) : nullptr;
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        MARKER_PCB* marker = m_filteredMarkers[ aIndex ];
        m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

        if( aDeep )
            m_board->Delete( marker );
    }

    void DeleteAllItems() override
    {
        m_board->DeleteMARKERs();
        m_filteredMarkers.clear();
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
    std::vector<DRC_ITEM*>* m_sourceVector;     // owns its DRC_ITEMs

    int                     m_severities;
    std::vector<DRC_ITEM*>  m_filteredVector;   // does not own its DRC_ITEMs

public:

    VECTOR_DRC_ITEMS_PROVIDER( PCB_BASE_FRAME* aFrame, std::vector<DRC_ITEM*>* aList ) :
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
            for( DRC_ITEM* item : *m_sourceVector )
            {
                if( bds.GetSeverity( item->GetErrorCode() ) & aSeverities )
                    m_filteredVector.push_back( item );
            }
        }
    }

    int  GetCount( int aSeverity = -1 ) override
    {
        if( aSeverity < 0 )
            return m_filteredVector.size();

        int count = 0;
        BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

        if( m_sourceVector )
        {
            for( DRC_ITEM* item : *m_sourceVector )
            {
                if( bds.GetSeverity( item->GetErrorCode() ) == aSeverity )
                    count++;
            }
        }

        return count;
    }

    DRC_ITEM* GetItem( int aIndex ) override
    {
        return (m_filteredVector)[aIndex];
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        DRC_ITEM* item = m_filteredVector[aIndex];
        m_filteredVector.erase( m_filteredVector.begin() + aIndex );

        if( aDeep )
        {
            for( size_t i = 0; i < m_sourceVector->size(); ++i )
            {
                if( m_sourceVector->at( i ) == item )
                {
                    delete item;
                    m_sourceVector->erase( m_sourceVector->begin() + i );
                    break;
                }
            }
        }
    }

    void DeleteAllItems() override
    {
        if( m_sourceVector )
        {
            for( DRC_ITEM* item : *m_sourceVector )
                delete item;

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
    RATSNEST_DRC_ITEMS_PROVIDER( PCB_BASE_FRAME* aFrame, std::vector<DRC_ITEM*>* aList ) :
            VECTOR_DRC_ITEMS_PROVIDER( aFrame, aList )
    { }
};


#endif // DRC_PROVIDER__H
