/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Jon Evans <jon@craftyjon.com>
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

#include <erc_item.h>
#include <erc_settings.h>
#include <schematic.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <settings/parameters.h>


const int ercSettingsSchemaVersion = 0;


ERC_SETTINGS::ERC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "erc", ercSettingsSchemaVersion, aParent, aPath )
{
    for( int i = ERCE_FIRST; i <= ERCE_LAST; ++i )
        m_Severities[ i ] = RPT_SEVERITY_ERROR;

    m_Severities[ ERCE_UNSPECIFIED ] = RPT_SEVERITY_UNDEFINED;

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "rule_severities",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const RC_ITEM& item : ERC_ITEM::GetItemsWithSeverities() )
                {
                    int code = item.GetErrorCode();

                    if( !m_Severities.count( code ) )
                        continue;

                    wxString name = item.GetSettingsKey();

                    ret[std::string( name.ToUTF8() )] =
                            SeverityToString( static_cast<SEVERITY>( m_Severities[code] ) );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                for( const RC_ITEM& item : ERC_ITEM::GetItemsWithSeverities() )
                {
                    int      code = item.GetErrorCode();
                    wxString name = item.GetSettingsKey();

                    std::string key( name.ToUTF8() );

                    if( aJson.contains( key ) )
                        m_Severities[code] = SeverityFromString( aJson[key] );
                }
            },
            {} ) );
}


ERC_SETTINGS::~ERC_SETTINGS()
{
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


int ERC_SETTINGS::GetSeverity( int aErrorCode ) const
{
    wxCHECK_MSG( m_Severities.count( aErrorCode ), RPT_SEVERITY_IGNORE,
            "Missing severity from map in ERC_SETTINGS!" );

    // Special-case pin-to-pin errors:
    // Ignore-or-not is controlled by ERCE_PIN_TO_PIN_WARNING (for both)
    // Warning-or-error is controlled by which errorCode it is
    if( aErrorCode == ERCE_PIN_TO_PIN_ERROR )
    {
        wxASSERT( m_Severities.count( ERCE_PIN_TO_PIN_WARNING ) );

        if( m_Severities.at( ERCE_PIN_TO_PIN_WARNING ) == RPT_SEVERITY_IGNORE )
            return RPT_SEVERITY_IGNORE;
        else
            return RPT_SEVERITY_ERROR;
    }
    else if( aErrorCode == ERCE_PIN_TO_PIN_WARNING )
    {
        wxASSERT( m_Severities.count( ERCE_PIN_TO_PIN_WARNING ) );

        if( m_Severities.at( ERCE_PIN_TO_PIN_WARNING ) == RPT_SEVERITY_IGNORE )
            return RPT_SEVERITY_IGNORE;
        else
            return RPT_SEVERITY_WARNING;
    }

    return m_Severities.at( aErrorCode );
}


void ERC_SETTINGS::SetSeverity( int aErrorCode, int aSeverity )
{
    m_Severities[ aErrorCode ] = aSeverity;
}


void SHEETLIST_ERC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    m_filteredMarkers.clear();

    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();
    ERC_SETTINGS&  settings  = m_schematic->ErcSettings();

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
                markerSeverity = settings.GetSeverity( marker->GetRCItem()->GetErrorCode() );

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
    ERC_SETTINGS&  settings  = m_schematic->ErcSettings();

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
                markerSeverity = settings.GetSeverity( marker->GetRCItem()->GetErrorCode() );

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
