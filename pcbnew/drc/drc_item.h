/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_ITEM_H
#define DRC_ITEM_H

#include <macros.h>
#include <base_struct.h>
#include <rc_item.h>
#include <marker_base.h>
#include <class_board.h>
#include <class_marker_pcb.h>
#include <pcb_base_frame.h>


class DRC_ITEM : public RC_ITEM
{
public:
    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    wxString GetErrorText() const override;

    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the wxHtmlListBox class.
     * @return wxString - the html text.
     */
    wxString ShowHtml( EDA_UNITS aUnits ) const;
};


/**
 * BOARD_DRC_ITEMS_PROVIDER
 * is an implementation of the interface named DRC_ITEM_LIST which uses a BOARD instance
 * to fulfill the interface.  No ownership is taken of the BOARD.
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


#endif      // DRC_ITEM_H
