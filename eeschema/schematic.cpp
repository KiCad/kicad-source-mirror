/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bus_alias.h>
#include <connection_graph.h>
#include <erc_settings.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_marker.h>


SCHEMATIC::SCHEMATIC( PROJECT* aPrj ) :
          EDA_ITEM( nullptr, SCHEMATIC_T ),
          m_project( aPrj ),
          m_rootSheet( nullptr )
{
    m_currentSheet    = new SCH_SHEET_PATH();
    m_connectionGraph = new CONNECTION_GRAPH( this );
    m_ercSettings     = new ERC_SETTINGS();
}


SCHEMATIC::~SCHEMATIC()
{
    delete m_currentSheet;
    delete m_connectionGraph;
    delete m_ercSettings;
}


void SCHEMATIC::Reset()
{
    delete m_rootSheet;

    m_rootSheet = nullptr;

    m_connectionGraph->Reset();
    m_currentSheet->clear();
}


void SCHEMATIC::SetRoot( SCH_SHEET* aRootSheet )
{
    wxCHECK_RET( aRootSheet, "Call to SetRoot with null SCH_SHEET!" );

    m_rootSheet = aRootSheet;

    m_currentSheet->clear();
    m_currentSheet->push_back( m_rootSheet );

    m_connectionGraph->Reset();
}


SCH_SCREEN* SCHEMATIC::RootScreen() const
{
    return IsValid() ? m_rootSheet->GetScreen() : nullptr;
}


wxString SCHEMATIC::GetFileName() const
{
    return IsValid() ? m_rootSheet->GetScreen()->GetFileName() : wxString( wxEmptyString );
}


std::shared_ptr<BUS_ALIAS> SCHEMATIC::GetBusAlias( const wxString& aLabel ) const
{
    for( const auto& sheet : GetSheets() )
    {
        for( const auto& alias : sheet.LastScreen()->GetBusAliases() )
        {
            if( alias->GetName() == aLabel )
                return alias;
        }
    }

    return nullptr;
}


int SCHEMATIC::GetErcSeverity( int aErrorCode ) const
{
    // Special-case pin-to-pin errors:
    // Ignore-or-not is controlled by ERCE_PIN_TO_PIN_WARNING (for both)
    // Warning-or-error is controlled by which errorCode it is
    if( aErrorCode == ERCE_PIN_TO_PIN_ERROR )
    {
        if( m_ercSettings->m_Severities[ ERCE_PIN_TO_PIN_WARNING ] == RPT_SEVERITY_IGNORE )
            return RPT_SEVERITY_IGNORE;
        else
            return RPT_SEVERITY_ERROR;
    }
    else if( aErrorCode == ERCE_PIN_TO_PIN_WARNING )
    {
        if( m_ercSettings->m_Severities[ ERCE_PIN_TO_PIN_WARNING ] == RPT_SEVERITY_IGNORE )
            return RPT_SEVERITY_IGNORE;
        else
            return RPT_SEVERITY_WARNING;
    }

    return m_ercSettings->m_Severities[ aErrorCode ];
}


void SCHEMATIC::SetErcSeverity( int aErrorCode, int aSeverity )
{
    m_ercSettings->m_Severities[ aErrorCode ] = aSeverity;
}


void SHEETLIST_ERC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    m_filteredMarkers.clear();

    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

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
                markerSeverity = m_schematic->GetErcSeverity( marker->GetRCItem()->GetErrorCode() );

            if( markerSeverity & m_severities )
                m_filteredMarkers.push_back( marker );
        }
    }
}


int SHEETLIST_ERC_ITEMS_PROVIDER::GetCount( int aSeverity )
{
    if( aSeverity < 0 )
        return m_filteredMarkers.size();

    int count = 0;

    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

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
                markerSeverity = m_schematic->GetErcSeverity( marker->GetRCItem()->GetErrorCode() );

            if( markerSeverity == aSeverity )
                count++;
        }
    }

    return count;
}


ERC_ITEM* SHEETLIST_ERC_ITEMS_PROVIDER::GetItem( int aIndex )
{
    SCH_MARKER* marker = m_filteredMarkers[ aIndex ];

    return marker ? static_cast<ERC_ITEM*>( marker->GetRCItem() ) : nullptr;
}


void SHEETLIST_ERC_ITEMS_PROVIDER::DeleteItem( int aIndex, bool aDeep )
{
    SCH_MARKER* marker = m_filteredMarkers[ aIndex ];
    m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

    if( aDeep )
    {
        SCH_SCREENS screens( m_schematic->Root() );
        screens.DeleteMarker( marker );
    }
}


void SHEETLIST_ERC_ITEMS_PROVIDER::DeleteAllItems()
{
    SCH_SCREENS screens( m_schematic->Root() );
    screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC );
    m_filteredMarkers.clear();
}
