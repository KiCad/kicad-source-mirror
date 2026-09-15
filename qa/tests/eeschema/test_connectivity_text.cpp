/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <connectivity/conn_facade.h>
#include <eda_search_data.h>
#include <connectivity/conn_presentation.h>
#include <widgets/msgpanel.h>
#include <advanced_config.h>
#include <connection_graph.h>
#include <netclass.h>
#include <sch_connection.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <schematic_utils/schematic_file_util.h>
#include <locale_io.h>
#include <sch_label.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <scoped_set_reset.h>
#include <settings/settings_manager.h>

#include <algorithm>
#include <map>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityText )

BOOST_AUTO_TEST_CASE( DerivedPinTokensUsePublishedConnectivity )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    size_t siblingChecks = 0;
    size_t ncChecks = 0;
    size_t knownNetChecks = 0;

    for( const wxString& fixture : { wxString( "netlists/multinetclasses/multinetclasses" ),
                                    wxString( "netlists/video/video" ),
                                    wxString( "spice_netlists/multiunit_repeat_opamp/multiunit_repeat_opamp" ) } )
    {
        SETTINGS_MANAGER settings;
        std::unique_ptr<SCHEMATIC> schematic;
        KI_TEST::LoadSchematic( settings, fixture, schematic );
        schematic->ConnectionGraph()->Reset();
        schematic->Connectivity().Recalculate( *schematic, true );
        const auto& netSettings = schematic->Project().GetProjectFile().NetSettings();
        size_t checked = 0;
        std::map<wxString, std::vector<std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>>> units;

        for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* symbol = static_cast<SCH_SYMBOL*>( item );
                units[symbol->GetRef( &path )].emplace_back( symbol, path );
            }
        }

        for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* symbol = static_cast<SCH_SYMBOL*>( item );

                for( const SCH_PIN* pin : symbol->GetPins( &path ) )
                {
                    if( SCH_CONNECTION* legacy = pin->Connection( &path ) )
                        legacy->Reset();

                    const auto connection = schematic->Connectivity().Connection( pin->m_Uuid, path.Path() );
                    BOOST_REQUIRE( connection );
                    wxString localName = connection->LocalName();

                    if( localName.Lower().StartsWith( "unconnected" ) )
                    {
                        localName = "NC";
                        ++ncChecks;
                    }

                    for( const auto& [function, expected] :
                         { std::pair{ wxString( "NET_NAME" ), connection->Name() },
                           std::pair{ wxString( "SHORT_NET_NAME" ), localName },
                           std::pair{ wxString( "NET_CLASS" ),
                                      netSettings->GetEffectiveNetClass( connection->Name() )->GetName() } } )
                    {
                        wxString token = function + "(" + pin->GetNumber() + ")";
                        BOOST_TEST_CONTEXT( fixture << " " << symbol->GetRef( &path ) << " " << token )
                        {
                            BOOST_REQUIRE( symbol->ResolveTextVar( &path, &token, 0 ) );
                            BOOST_CHECK_EQUAL( token, expected );

                            for( const auto& [sibling, siblingPath] : units.at( symbol->GetRef( &path ) ) )
                            {
                                if( sibling->GetUnitSelection( &siblingPath )
                                            == symbol->GetUnitSelection( &path ) )
                                    continue;

                                const auto siblingPins = sibling->GetPins( &siblingPath );

                                if( std::any_of( siblingPins.begin(), siblingPins.end(),
                                                [&]( const SCH_PIN* candidate )
                                                { return candidate->GetNumber() == pin->GetNumber(); } ) )
                                    continue;

                                wxString siblingToken = function + "(" + pin->GetNumber() + ")";
                                BOOST_REQUIRE( sibling->ResolveTextVar( &siblingPath, &siblingToken, 0 ) );
                                BOOST_CHECK_EQUAL( siblingToken, expected );
                                ++siblingChecks;
                            }
                        }
                    }

                    if( fixture == "netlists/multinetclasses/multinetclasses"
                        && symbol->GetRef( &path ) == "R8" && pin->GetNumber() == "1" )
                    {
                        for( const auto& [source, expected] :
                             { std::pair{ wxString( "NET_NAME(1)" ), wxString( "/NET_2" ) },
                               std::pair{ wxString( "SHORT_NET_NAME(1)" ), wxString( "NET_2" ) },
                               std::pair{ wxString( "NET_CLASS(1)" ), wxString( "CLASS_COMPLETE" ) } } )
                        {
                            wxString token = source;
                            BOOST_REQUIRE( symbol->ResolveTextVar( &path, &token, 0 ) );
                            BOOST_CHECK_EQUAL( token, expected );
                        }

                        BOOST_CHECK_EQUAL( pin->GetEffectiveNetClass( &path )->GetName(), "CLASS_COMPLETE" );
                        std::vector<MSG_PANEL_ITEM> info;
                        const auto name = AppendConnectionInfo( *pin, info, &path );
                        BOOST_REQUIRE( name );
                        BOOST_CHECK_EQUAL( *name, "/NET_2" );
                        using ROWS = std::vector<std::pair<wxString, wxString>>;
                        ROWS rows;

                        for( const auto& row : info )
                        {
                            if( row.GetUpperText() != "Subgraph Code" && row.GetUpperText() != "Connection Source" )
                                rows.emplace_back( row.GetUpperText(), row.GetLowerText() );
                        }

                        const ROWS expectedRows{ { "Connection Name", "/NET_2" },
                                                 { "Resolved Netclass", "CLASS_COMPLETE" } };
                        BOOST_CHECK( rows == expectedRows );
                        ++knownNetChecks;
                    }

                    ++checked;
                }
            }
        }

        if( fixture == "netlists/multinetclasses/multinetclasses" )
        {
            size_t labelChecks = 0;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_LABEL_T ) )
                {
                    auto* label = static_cast<SCH_LABEL*>( item );

                    if( label->GetText() != "NET_2" )
                        continue;

                    if( SCH_CONNECTION* legacy = label->Connection( &path ) )
                        legacy->Reset();

                    for( const auto& [source, expected] :
                         { std::pair{ wxString( "NET_NAME" ), wxString( "/NET_2" ) },
                           std::pair{ wxString( "SHORT_NET_NAME" ), wxString( "NET_2" ) },
                           std::pair{ wxString( "NET_CLASS" ), wxString( "CLASS_COMPLETE" ) } } )
                    {
                        wxString token = source;
                        BOOST_REQUIRE( label->ResolveTextVar( &path, &token, 0 ) );
                        BOOST_CHECK_EQUAL( token, expected );
                    }

                    ++labelChecks;
                }
            }

            BOOST_CHECK_EQUAL( labelChecks, 1u );
        }

        BOOST_CHECK_GT( checked, 0u );
        BOOST_CHECK( schematic->ConnectionGraph()->GetNetMap().empty() );
    }

    BOOST_CHECK_GT( siblingChecks, 0u );
    BOOST_CHECK_GT( ncChecks, 0u );
    BOOST_CHECK_EQUAL( knownNetChecks, 1u );
}

BOOST_AUTO_TEST_CASE( ConnectionPanelPreservesBusAliasDetails )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    using ROWS = std::vector<std::pair<wxString, wxString>>;
    struct EXPECTED
    {
        SCH_ITEM*      item;
        SCH_SHEET_PATH path;
        ROWS           rows;
    };
    auto panel = []( const SCH_ITEM& item, const SCH_SHEET_PATH* path )
    {
        std::vector<MSG_PANEL_ITEM> info;
        BOOST_CHECK( !AppendConnectionInfo( item, info, path ) );
        ROWS rows;

        for( const auto& row : info )
        {
            const wxString& heading = row.GetUpperText();

            if( heading != "Bus Code" && heading != "Subgraph Code" && heading != "Connection Source" )
                rows.emplace_back( heading, row.GetLowerText() );
        }

        return rows;
    };
    std::vector<EXPECTED> expected;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_LABEL_T ) )
        {
            if( const SCH_CONNECTION* connection = item->Connection( &path ); connection && connection->IsBus() )
            {
                auto rows = panel( *item, &path );
                BOOST_REQUIRE_GT( rows.size(), 1u );
                expected.push_back( { item, path, std::move( rows ) } );
            }
        }
    }

    BOOST_REQUIRE( !expected.empty() );
    schematic->ConnectionGraph()->Reset();
    enabled = true;
    schematic->Connectivity().Recalculate( *schematic, true );

    for( const auto& entry : expected )
    {
        entry.item->Connection( &entry.path )->Reset();
        BOOST_CHECK( panel( *entry.item, &entry.path ) == entry.rows );
        schematic->SetCurrentSheet( entry.path );
        BOOST_CHECK( panel( *entry.item, nullptr ) == entry.rows );
    }
}

BOOST_AUTO_TEST_CASE( LabelNetSearchUsesPublishedSignalsAndBusMembers )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
    schematic->ConnectionGraph()->Reset();
    enabled = true;
    schematic->Connectivity().Recalculate( *schematic, true );
    SCH_SHEET_PATH path = schematic->Hierarchy().front();
    size_t checked = 0;

    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        auto* label = static_cast<SCH_LABEL*>( item );
        const wxString text = label->GetText();

        if( text != "NET_2" && text != "BUS{SIGNAL A[0..2]}" )
            continue;

        SCH_SEARCH_DATA search;
        search.findString = text == "NET_2" ? wxString( "/NET_2" ) : wxString( "/BUS.SIGNAL" );
        search.matchMode = EDA_SEARCH_MATCH_MODE::PLAIN;
        BOOST_CHECK( !label->Matches( search, &path ) );
        search.searchNetNames = true;
        BOOST_CHECK( label->Matches( search, &path ) );
        ++checked;
    }

    BOOST_CHECK_EQUAL( checked, 2u );
}

BOOST_AUTO_TEST_SUITE_END()
