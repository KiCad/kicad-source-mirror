/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <advanced_config.h>
#include <connection_graph.h>
#include <erc/erc.h>
#include <locale_io.h>
#include <sch_field.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_CASE( ERCMissingNetclassesIncludeSheetsWithoutElectricalIslands )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool backend : { false, true } )
    {
        enabled = backend;
        SETTINGS_MANAGER settings;
        std::unique_ptr<SCHEMATIC> schematic;
        KI_TEST::LoadSchematic( settings, "legacy_hierarchy/legacy_hierarchy", schematic );
        SCH_SCREEN* screen = schematic->RootScreen();
        SCH_SHEET* child = nullptr;
        std::vector<SCH_ITEM*> removed;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( !child && item->Type() == SCH_SHEET_T )
                child = static_cast<SCH_SHEET*>( item );
            else
                removed.push_back( item );
        }

        BOOST_REQUIRE( child );

        for( SCH_ITEM* item : removed )
            screen->DeleteItem( item );

        const auto pins = child->GetPins();

        for( SCH_SHEET_PIN* pin : pins )
        {
            child->RemovePin( pin );
            delete pin;
        }

        child->GetFields().emplace_back( child, FIELD_T::USER, "Netclass" );
        child->GetFields().back().SetText( "MissingRootClass" );
        screen->Update( child, false );
        schematic->RebuildConnectivity();
        const SCH_SHEET_PATH root = schematic->Hierarchy().front();
        BOOST_REQUIRE( root.LastScreen() == screen );

        if( backend )
            schematic->ConnectionGraph()->Reset();

        ERC_TESTER tester( schematic.get() );
        BOOST_CHECK_EQUAL( tester.TestMissingNetclasses(), 1 );
        size_t count = 0;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            const auto* marker = static_cast<SCH_MARKER*>( item );
            const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

            if( error->GetErrorCode() != ERCE_UNDEFINED_NETCLASS )
                continue;

            BOOST_CHECK( error->GetMainItemID() == child->m_Uuid );
            BOOST_CHECK( marker->GetPosition() == child->GetPosition() );
            BOOST_REQUIRE( error->IsSheetSpecific() );
            BOOST_CHECK( error->GetSpecificSheetPath().PathRef() == root.PathRef() );
            BOOST_CHECK( error->GetErrorMessage( true ).Contains( "MissingRootClass" ) );
            ++count;
        }

        BOOST_CHECK_EQUAL( count, 1 );
    }
}


BOOST_AUTO_TEST_CASE( ERCMissingNetclassesUseActiveVariantPerInstance )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine=" << backend )
        {
            enabled = backend;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "legacy_hierarchy/legacy_hierarchy", schematic );
            std::vector<SCH_SHEET_PATH> paths;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                if( path.LastScreen()->GetFileName().EndsWith( "ampli_ht.kicad_sch" ) )
                    paths.push_back( path );
            }

            BOOST_REQUIRE_EQUAL( paths.size(), 2 );
            SCH_SCREEN* screen = paths[0].LastScreen();
            SCH_SYMBOL* symbol = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                symbol = static_cast<SCH_SYMBOL*>( item );
                break;
            }

            BOOST_REQUIRE( symbol );
            SCH_FIELD field( symbol, FIELD_T::USER, "Netclass" );
            field.SetText( "Default" );
            symbol->AddField( field );
            const wxString variant( "Netclass variant" );
            schematic->AddVariant( variant );
            symbol->SetFieldText( "Netclass", "MissingVariantClass", &paths[0], variant );
            screen->Update( symbol, false );
            schematic->SetCurrentVariant( variant );
            const SCH_FIELD* netclass = symbol->GetField( "Netclass" );
            BOOST_REQUIRE( netclass );
            BOOST_REQUIRE_EQUAL( netclass->GetShownText( &paths[0], FOR_NETNAME, variant ),
                                 wxString( "MissingVariantClass" ) );
            BOOST_REQUIRE_EQUAL( netclass->GetShownText( &paths[1], FOR_NETNAME, variant ), wxString( "Default" ) );
            schematic->RebuildConnectivity();

            if( backend )
                schematic->ConnectionGraph()->Reset();

            ERC_TESTER tester( schematic.get() );
            BOOST_CHECK_EQUAL( tester.TestMissingNetclasses(), 1 );
            std::vector<SCH_ITEM*> markers;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
            {
                const auto* marker = static_cast<SCH_MARKER*>( item );
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                if( error->GetErrorCode() != ERCE_UNDEFINED_NETCLASS )
                    continue;

                BOOST_CHECK( error->GetMainItemID() == symbol->m_Uuid );
                BOOST_REQUIRE( error->IsSheetSpecific() );
                BOOST_CHECK( error->GetSpecificSheetPath().PathRef() == paths[0].PathRef() );
                BOOST_CHECK( error->GetErrorMessage( true ).Contains( "MissingVariantClass" ) );
                markers.push_back( item );
            }

            BOOST_CHECK_EQUAL( markers.size(), 1 );

            for( SCH_ITEM* marker : markers )
                screen->DeleteItem( marker );

            schematic->SetCurrentVariant( wxString() );
            schematic->RebuildConnectivity();
            BOOST_CHECK_EQUAL( tester.TestMissingNetclasses(), 0 );
        }
    }
}
