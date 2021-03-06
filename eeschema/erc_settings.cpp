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



#define OK PIN_ERROR::OK
#define ERR PIN_ERROR::PP_ERROR
#define WAR PIN_ERROR::WARNING

/**
 * Default Look up table which gives the ERC error level for a pair of connected pins
 */
PIN_ERROR ERC_SETTINGS::m_defaultPinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL] =
{
/*         I,   O,    Bi,   3S,   Pas,  NIC,  UnS,  PwrI, PwrO, OC,   OE,   NC */
/* I  */ { OK,  OK,   OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/* O  */ { OK,  ERR,  OK,   WAR,  OK,   OK,   WAR,  OK,   ERR,  ERR,  ERR,  ERR },
/* Bi */ { OK,  OK,   OK,   OK,   OK,   OK,   WAR,  OK,   WAR,  OK,   WAR,  ERR },
/* 3S */ { OK,  WAR,  OK,   OK,   OK,   OK,   WAR,  WAR,  ERR,  WAR,  WAR,  ERR },
/*Pas */ { OK,  OK,   OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/*NIC */ { OK,  OK,   OK,   OK,   OK,   OK,   OK,   OK,   OK,   OK,   OK,   ERR },
/*UnS */ { WAR, WAR,  WAR,  WAR,  WAR,  OK,   WAR,  WAR,  WAR,  WAR,  WAR,  ERR },
/*PwrI*/ { OK,  OK,   OK,   WAR,  OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/*PwrO*/ { OK,  ERR,  WAR,  ERR,  OK,   OK,   WAR,  OK,   ERR,  ERR,  ERR,  ERR },
/* OC */ { OK,  ERR,  OK,   WAR,  OK,   OK,   WAR,  OK,   ERR,  OK,   OK,   ERR },
/* OE */ { OK,  ERR,  WAR,  WAR,  OK,   OK,   WAR,  OK,   ERR,  OK,   OK,   ERR },
/* NC */ { ERR, ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR }
};


/**
 * Look up table which gives the minimal drive for a pair of connected pins on
 * a net.
 * <p>
 * The initial state of a net is NOC (Net with No Connection).  It can be updated to
 * NPI (Pin Isolated), NET_NC (Net with a no connect symbol), NOD (Not Driven) or DRV
 * (DRIven).  It can be updated to NET_NC with no error only if there is only one pin
 * in net.  Nets are OK when their final state is NET_NC or DRV.   Nets with the state
 * NOD have no valid source signal.
 */
int ERC_SETTINGS::m_PinMinDrive[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL] =
{
/*         I,    O,    Bi,   3S,   Pas,  NIC,  UnS,  PwrI, PwrO, OC,   OE,   NC */
/* I  */ { NOD,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/* O  */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  DRV,  DRV,  DRV,  DRV,  NPI },
/* Bi */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/* 3S */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/*Pas */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/*NIC */ { NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  NPI },
/*UnS */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/*PwrI*/ { NOD,  DRV,  NOD,  NOD,  NOD,  NOD,  NOD,  NOD,  DRV,  NOD,  NOD,  NPI },
/*PwrO*/ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  DRV,  DRV,  DRV,  DRV,  NPI },
/* OC */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/* OE */ { DRV,  DRV,  DRV,  DRV,  DRV,  NOD,  DRV,  NOD,  DRV,  DRV,  DRV,  NPI },
/* NC */ { NPI,  NPI,  NPI,  NPI,  NPI,  NPI,  NPI,  NPI,  NPI,  NPI,  NPI,  NPI }
};


ERC_SETTINGS::ERC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "erc", ercSettingsSchemaVersion, aParent, aPath )
{
    ResetPinMap();

    for( int i = ERCE_FIRST; i <= ERCE_LAST; ++i )
        m_Severities[ i ] = RPT_SEVERITY_ERROR;

    // Error is the default setting so set non-error priorities here.
    m_Severities[ERCE_UNSPECIFIED]             = RPT_SEVERITY_UNDEFINED;
    m_Severities[ERCE_PIN_TO_PIN_WARNING]      = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_SIMILAR_LABELS]          = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_GLOBLABEL]               = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_DRIVER_CONFLICT]         = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_BUS_ENTRY_CONFLICT]      = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_LIB_SYMBOL_ISSUES]       = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_NOCONNECT_CONNECTED]     = RPT_SEVERITY_WARNING;
    m_Severities[ERCE_NOCONNECT_NOT_CONNECTED] = RPT_SEVERITY_WARNING;

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "rule_severities",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const RC_ITEM& item : ERC_ITEM::GetItemsWithSeverities() )
                {
                    wxString name = item.GetSettingsKey();
                    int      code = item.GetErrorCode();

                    if( name.IsEmpty() || m_Severities.count( code ) == 0 )
                        continue;

                    ret[std::string( name.ToUTF8() )] = SeverityToString( m_Severities[code] );
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

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "erc_exclusions",
            [&]() -> nlohmann::json
            {
                nlohmann::json js = nlohmann::json::array();

                for( const auto& entry : m_ErcExclusions )
                    js.push_back( entry );

                return js;
            },
            [&]( const nlohmann::json& aObj )
            {
                m_ErcExclusions.clear();

                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() )
                        continue;

                    m_ErcExclusions.insert( entry.get<wxString>() );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "pin_map",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( int i = 0; i < ELECTRICAL_PINTYPES_TOTAL; i++ )
                {
                    nlohmann::json inner = nlohmann::json::array();

                    for( int j = 0; j < ELECTRICAL_PINTYPES_TOTAL; j++ )
                        inner.push_back( static_cast<int>( GetPinMapValue( i, j ) ) );

                    ret.push_back( inner );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() || aJson.size() != ELECTRICAL_PINTYPES_TOTAL )
                    return;

                for( size_t i = 0; i < ELECTRICAL_PINTYPES_TOTAL; i++ )
                {
                    if( i > aJson.size() - 1 )
                        break;

                    nlohmann::json inner = aJson[i];

                    if( !inner.is_array() || inner.size() != ELECTRICAL_PINTYPES_TOTAL )
                        return;

                    for( size_t j = 0; j < ELECTRICAL_PINTYPES_TOTAL; j++ )
                    {
                        if( inner[j].is_number_integer() )
                        {
                            int val = inner[j].get<int>();

                            if( val >= 0 && val <= static_cast<int>( PIN_ERROR::UNCONNECTED ) )
                                SetPinMapValue( i, j, static_cast<PIN_ERROR>( val ) );
                        }
                    }
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


SEVERITY ERC_SETTINGS::GetSeverity( int aErrorCode ) const
{
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

    wxCHECK_MSG( m_Severities.count( aErrorCode ), RPT_SEVERITY_IGNORE,
            "Missing severity from map in ERC_SETTINGS!" );

    return m_Severities.at( aErrorCode );
}


void ERC_SETTINGS::SetSeverity( int aErrorCode, SEVERITY aSeverity )
{
    m_Severities[ aErrorCode ] = aSeverity;
}


void ERC_SETTINGS::ResetPinMap()
{
    memcpy( m_PinMap, m_defaultPinMap, sizeof( m_PinMap ) );
}


void SHEETLIST_ERC_ITEMS_PROVIDER::visitMarkers( std::function<void( SCH_MARKER* )> aVisitor ) const
{
    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

    std::set<SCH_SCREEN*> seenScreens;

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        bool firstTime = seenScreens.count( sheetList[i].LastScreen() ) == 0;

        if( firstTime )
            seenScreens.insert( sheetList[i].LastScreen() );

        for( SCH_ITEM* item : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            // Don't show non-specific markers more than once
            if( !firstTime &&
                !static_cast<const ERC_ITEM*>( marker->GetRCItem().get() )->IsSheetSpecific() )
            {
                continue;
            }

            aVisitor( marker );
        }
    }
}


void SHEETLIST_ERC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    m_filteredMarkers.clear();

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    visitMarkers(
            [&]( SCH_MARKER* aMarker )
            {
                SEVERITY markerSeverity;

                if( aMarker->IsExcluded() )
                    markerSeverity = RPT_SEVERITY_EXCLUSION;
                else
                    markerSeverity = settings.GetSeverity( aMarker->GetRCItem()->GetErrorCode() );

                if( markerSeverity & m_severities )
                    m_filteredMarkers.push_back( aMarker );
            } );
}


int SHEETLIST_ERC_ITEMS_PROVIDER::GetCount( int aSeverity ) const
{
    if( aSeverity < 0 )
        return m_filteredMarkers.size();

    int count = 0;

    const ERC_SETTINGS& settings = m_schematic->ErcSettings();

    visitMarkers(
            [&]( SCH_MARKER* aMarker )
            {
                SEVERITY markerSeverity;

                if( aMarker->IsExcluded() )
                    markerSeverity = RPT_SEVERITY_EXCLUSION;
                else
                    markerSeverity = settings.GetSeverity( aMarker->GetRCItem()->GetErrorCode() );

                if( markerSeverity == aSeverity )
                    count++;
            } );

    return count;
}


std::shared_ptr<ERC_ITEM> SHEETLIST_ERC_ITEMS_PROVIDER::GetERCItem( int aIndex ) const
{
    SCH_MARKER* marker = m_filteredMarkers[ aIndex ];

    return marker ? std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() ) : nullptr;
}


std::shared_ptr<RC_ITEM> SHEETLIST_ERC_ITEMS_PROVIDER::GetItem( int aIndex ) const
{
    return GetERCItem( aIndex );
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


void SHEETLIST_ERC_ITEMS_PROVIDER::DeleteAllItems( bool aIncludeExclusions, bool aDeep )
{
    // Filtered list was already handled through DeleteItem() by the tree control

    if( aDeep )
    {
        SCH_SCREENS screens( m_schematic->Root() );
        screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, aIncludeExclusions );
    }
}
