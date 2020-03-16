/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ERC_ITEM_H
#define ERC_ITEM_H

#include <macros.h>
#include <base_struct.h>
#include <rc_item.h>
#include <marker_base.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include "erc_settings.h"


class ERC_ITEM : public RC_ITEM
{
public:
    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    wxString GetErrorText() const override;
};



/**
 * SHEETLIST_ERC_ITEMS_PROVIDER
 * is an implementation of the RC_ITEM_LISTinterface which uses the global SHEETLIST
 * to fulfill the contract.
 */
class SHEETLIST_ERC_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
private:
    int                      m_severities;
    std::vector<SCH_MARKER*> m_filteredMarkers;

public:
    SHEETLIST_ERC_ITEMS_PROVIDER() :
            m_severities( 0 )
    { }

    void SetSeverities( int aSeverities ) override
    {
        m_severities = aSeverities;

        m_filteredMarkers.clear();

        SCH_SHEET_LIST sheetList( g_RootSheet);

        for( unsigned i = 0; i < sheetList.size(); i++ )
        {
            for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
            {
                SCH_MARKER* marker = static_cast<SCH_MARKER*>( aItem );
                int markerSeverity;

                if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                    continue;

                if( marker->IsExcluded() )
                    markerSeverity = RPT_SEVERITY_EXCLUSION;
                else
                    markerSeverity = GetSeverity( marker->GetRCItem()->GetErrorCode() );

                if( markerSeverity & m_severities )
                    m_filteredMarkers.push_back( marker );
            }
        }
    }

    int GetCount( int aSeverity = -1 ) override
    {
        if( aSeverity < 0 )
            return m_filteredMarkers.size();

        int count = 0;

        SCH_SHEET_LIST sheetList( g_RootSheet);

        for( unsigned i = 0; i < sheetList.size(); i++ )
        {
            for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
            {
                SCH_MARKER* marker = static_cast<SCH_MARKER*>( aItem );
                int markerSeverity;

                if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                    continue;

                if( marker->IsExcluded() )
                    markerSeverity = RPT_SEVERITY_EXCLUSION;
                else
                    markerSeverity = GetSeverity( marker->GetRCItem()->GetErrorCode() );

                if( markerSeverity == aSeverity )
                    count++;
            }
        }

        return count;
    }

    ERC_ITEM* GetItem( int aIndex ) override
    {
        SCH_MARKER* marker = m_filteredMarkers[ aIndex ];

        return marker ? static_cast<ERC_ITEM*>( marker->GetRCItem() ) : nullptr;
    }

    void DeleteItem( int aIndex, bool aDeep ) override
    {
        SCH_MARKER* marker = m_filteredMarkers[ aIndex ];
        m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

        if( aDeep )
        {
            SCH_SCREENS ScreenList;
            ScreenList.DeleteMarker( marker );
        }
    }

    void DeleteAllItems() override
    {
        SCH_SCREENS ScreenList;
        ScreenList.DeleteAllMarkers( MARKER_BASE::MARKER_ERC );
        m_filteredMarkers.clear();
    }
};


#endif      // ERC_ITEM_H
