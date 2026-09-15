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

#include <algorithm>
#include <set>
#include <advanced_config.h>
#include <api/schematic/schematic_rules.pb.h>
#include <connectivity/conn_facade.h>
#include <erc/erc.h>
#include <erc/erc_exclusion.h>
#include <json_common.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <lib_symbol.h>
#include <locale_io.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_CASE( ERCDuplicateUnitsDoNotHideMissingUnits )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue1768/issue1768", schematic );
    const SCH_SHEET_PATH path = schematic->Hierarchy().front();
    SCH_SCREEN& screen = *path.LastScreen();
    std::vector<SCH_SYMBOL*> symbols;

    for( SCH_ITEM* item : screen.Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->IsMultiUnit() )
        {
            symbol->SetRef( &path, "ERC_DUPLICATE_UNITS" );
            symbol->SetUnitSelection( &path, 1 );
            symbols.push_back( symbol );
        }
    }

    BOOST_REQUIRE( !symbols.empty() );
    BOOST_REQUIRE_EQUAL( symbols.size(), symbols.front()->GetLibSymbolRef()->GetUnitCount() );

    for( const SCH_SYMBOL* symbol : symbols )
        BOOST_REQUIRE( symbol->GetLibId() == symbols.front()->GetLibId() );

    schematic->ErcSettings().SetSeverity( ERCE_MISSING_UNIT, RPT_SEVERITY_WARNING );

    for( bool backend : { true, false } )
    {
        BOOST_TEST_CONTEXT( "backend=" << backend )
        {
            enabled = backend;
            schematic->RebuildConnectivity();
            ERC_TESTER tester( schematic.get() );
            tester.TestMissingUnits();
            size_t missing = 0;
            std::vector<SCH_MARKER*> markers;

            for( SCH_ITEM* item : screen.Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );

                if( marker->GetRCItem()->GetErrorCode() == ERCE_MISSING_UNIT )
                    ++missing;

                markers.push_back( marker );
            }

            BOOST_CHECK_EQUAL( missing, 1 );

            for( SCH_MARKER* marker : markers )
                screen.DeleteItem( marker );
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCMultiUnitFootprintsBindSharedInstancePaths )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue23840/BusAndVectors", schematic );
    std::vector<SCH_SHEET_PATH> paths;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "LEDs.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2 );
    SCH_SCREEN* screen = paths[0].LastScreen();
    BOOST_REQUIRE( screen == paths[1].LastScreen() );
    SETTINGS_MANAGER sourceSettings;
    std::unique_ptr<SCHEMATIC> source;
    KI_TEST::LoadSchematic( sourceSettings, "issue1768/issue1768", source );
    SCH_SYMBOL* first = nullptr;
    SCH_SYMBOL* second = nullptr;
    SCH_SHEET_PATH root = schematic->Hierarchy().front();

    for( SCH_ITEM* item : source->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( !symbol->IsMultiUnit() )
            continue;

        if( !first && symbol->GetUnit() == 1 )
        {
            first = static_cast<SCH_SYMBOL*>( symbol->Duplicate( false ) );
            root.LastScreen()->Append( first );
        }
        else if( !second && symbol->GetUnit() == 2 )
        {
            second = static_cast<SCH_SYMBOL*>( symbol->Duplicate( false ) );
            screen->Append( second );
        }
    }

    BOOST_REQUIRE( first );
    BOOST_REQUIRE( second );
    BOOST_REQUIRE( first->m_Uuid != second->m_Uuid );
    first->SetRef( &root, "ERC_FP" );
    first->SetUnitSelection( &root, 1 );
    first->SetFootprintFieldText( "Acceptance:First" );
    second->SetFootprintFieldText( "Acceptance:Second" );

    for( const SCH_SHEET_PATH& path : paths )
    {
        second->SetRef( &path, "ERC_FP" );
        second->SetUnitSelection( &path, 2 );
    }

    for( bool backend : { true, false } )
    {
        enabled = backend;
        schematic->RebuildConnectivity();

        for( const SCH_SHEET_PATH& displayed : paths )
        {
            BOOST_TEST_CONTEXT( "backend=" << backend << "; displayed=" << displayed.GetPageNumber() )
            {
                schematic->SetCurrentSheet( displayed );
                ERC_TESTER tester( schematic.get() );
                tester.TestMultiunitFootprints();
                std::vector<SCH_MARKER*> markers;
                std::set<KIID_PATH> seen;

                for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                    if( error->GetErrorCode() != ERCE_DIFFERENT_UNIT_FP
                        || error->GetMainItemID() != first->m_Uuid || error->GetAuxItemID() != second->m_Uuid )
                    {
                        continue;
                    }

                    markers.push_back( marker );
                    BOOST_CHECK( marker->GetPosition() == second->GetPosition() );
                    BOOST_CHECK( error->IsSheetSpecific() );
                    BOOST_CHECK( error->MainItemHasSheetPath() );
                    BOOST_CHECK( error->AuxItemHasSheetPath() );

                    if( error->MainItemHasSheetPath() )
                        BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == root.PathRef() );

                    if( error->IsSheetSpecific() && error->AuxItemHasSheetPath() )
                    {
                        BOOST_CHECK( error->GetAuxItemSheetPath().PathRef()
                                     == error->GetSpecificSheetPath().PathRef() );
                        BOOST_CHECK( seen.insert( error->GetSpecificSheetPath().PathRef() ).second );
                    }
                }

                BOOST_CHECK_EQUAL( markers.size(), 2 );
                const std::set<KIID_PATH> expected{ paths[0].PathRef(), paths[1].PathRef() };
                BOOST_CHECK( seen == expected );

                for( SCH_MARKER* marker : markers )
                    screen->DeleteItem( marker );
            }
        }
    }

    const auto targetMarkers = [&]
    {
        std::vector<SCH_MARKER*> result;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );
            const auto error = marker->GetRCItem();

            if( error->GetErrorCode() == ERCE_DIFFERENT_UNIT_FP
                && error->GetMainItemID() == first->m_Uuid && error->GetAuxItemID() == second->m_Uuid )
            {
                result.push_back( marker );
            }
        }

        return result;
    };
    auto& exclusions = schematic->ErcSettings().m_ErcExclusions;

    for( bool legacy : { false, true } )
    {
        exclusions.clear();
        ERC_TESTER tester( schematic.get() );
        tester.TestMultiunitFootprints();
        BOOST_REQUIRE_EQUAL( targetMarkers().size(), 2 );

        for( SCH_MARKER* marker : targetMarkers() )
        {
            const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
            BOOST_REQUIRE( error->IsSheetSpecific() );

            if( error->GetSpecificSheetPath().PathRef() != paths[0].PathRef() )
                continue;

            auto proto = ERC_EXCLUSION::FromMarker( *marker ).ToProto();

            if( legacy )
            {
                proto.mutable_marker()->clear_sheet_specific_path();
                proto.mutable_marker()->clear_main_item_sheet_path();
                proto.mutable_marker()->clear_aux_item_sheet_path();
            }

            auto exclusion = ERC_EXCLUSION::FromProto( proto );
            exclusion.SetComment( "Retained multi-unit footprint exclusion" );
            const nlohmann::json saved = exclusion;
            exclusions.insert( saved.get<ERC_EXCLUSION>() );
        }

        BOOST_REQUIRE_EQUAL( exclusions.size(), 1 );
        const auto savedExclusions = exclusions;

        for( SCH_MARKER* marker : targetMarkers() )
            screen->DeleteItem( marker );

        for( bool backend : { true, false } )
        {
            BOOST_TEST_CONTEXT( "legacy exclusion=" << legacy << "; backend=" << backend )
            {
                enabled = backend;
                exclusions = savedExclusions;
                schematic->RebuildConnectivity();
                ERC_TESTER rescan( schematic.get() );
                rescan.TestMultiunitFootprints();
                schematic->ResolveERCExclusionsPostUpdate();
                std::vector<SCH_MARKER*> rootMarkers;

                for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    const auto error = marker->GetRCItem();

                    if( error->GetErrorCode() == ERCE_DIFFERENT_UNIT_FP
                        && error->GetMainItemID() == first->m_Uuid && error->GetAuxItemID() == second->m_Uuid )
                    {
                        rootMarkers.push_back( marker );
                    }
                }

                BOOST_CHECK( rootMarkers.empty() );

                for( SCH_MARKER* marker : rootMarkers )
                    root.LastScreen()->DeleteItem( marker );

                BOOST_CHECK_EQUAL( targetMarkers().size(), 2 );
                size_t excluded = 0;

                for( SCH_MARKER* marker : targetMarkers() )
                {
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                    BOOST_CHECK( error->IsSheetSpecific() );

                    if( !error->IsSheetSpecific() )
                        continue;

                    const bool expected = legacy || error->GetSpecificSheetPath().PathRef() == paths[0].PathRef();
                    BOOST_CHECK_EQUAL( marker->IsExcluded(), expected );
                    BOOST_CHECK_EQUAL( marker->GetComment(), expected
                            ? wxString( "Retained multi-unit footprint exclusion" ) : wxString() );
                    excluded += marker->IsExcluded();
                }

                BOOST_CHECK_EQUAL( excluded, legacy ? 2 : 1 );

                for( SCH_MARKER* marker : targetMarkers() )
                    screen->DeleteItem( marker );
            }
        }
    }

    BOOST_REQUIRE( root.LastScreen()->Remove( first, false ) );
    BOOST_REQUIRE( screen->Remove( second, false ) );
    screen->Append( first, false );
    root.LastScreen()->Append( second, false );
    second->SetRef( &root, "ERC_FP" );
    second->SetUnitSelection( &root, 2 );
    root.SetPageNumber( "100" );

    for( const SCH_SHEET_PATH& path : paths )
    {
        first->SetRef( &path, "ERC_FP" );
        first->SetUnitSelection( &path, 1 );
    }

    const KIID_PATH canonical = std::min( paths[0].PathRef(), paths[1].PathRef() );

    for( bool variable : { false, true } )
    {
        first->SetFootprintFieldText( variable ? "Acceptance:${#}" : "Acceptance:First" );

        for( bool backend : { true, false } )
        {
            enabled = backend;
            exclusions.clear();
            nlohmann::json saved;

            for( bool reverse : { false, true } )
            {
                BOOST_TEST_CONTEXT( "variable=" << variable << "; backend=" << backend << "; reverse=" << reverse )
                {
                    paths[0].SetPageNumber( reverse ? "3" : "2" );
                    paths[1].SetPageNumber( reverse ? "2" : "3" );

                    if( backend && reverse )
                        schematic->Connectivity().Recalculate( *schematic );
                    else
                        schematic->RebuildConnectivity();

                    ERC_TESTER tester( schematic.get() );
                    tester.TestMultiunitFootprints();

                    if( !variable && reverse )
                    {
                        exclusions.insert( saved.get<ERC_EXCLUSION>() );
                        schematic->ResolveERCExclusionsPostUpdate();
                    }

                    size_t count = 0;

                    for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                    {
                        auto* marker = static_cast<SCH_MARKER*>( item );
                        const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                        if( error->GetErrorCode() != ERCE_DIFFERENT_UNIT_FP
                            || error->GetMainItemID() != first->m_Uuid || error->GetAuxItemID() != second->m_Uuid )
                        {
                            continue;
                        }

                        ++count;
                        BOOST_REQUIRE( error->MainItemHasSheetPath() );
                        const SCH_SHEET_PATH& witness = error->GetMainItemSheetPath();

                        if( variable )
                        {
                            BOOST_CHECK( first->GetFootprintFieldText( &witness, RESOLVED )
                                         == "Acceptance:2" );
                        }
                        else
                        {
                            BOOST_CHECK( witness.PathRef() == canonical );

                            if( reverse )
                            {
                                BOOST_CHECK( marker->IsExcluded() );
                                BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Page order retained" ) );
                            }
                            else
                            {
                                auto exclusion = ERC_EXCLUSION::FromMarker( *marker );
                                exclusion.SetComment( "Page order retained" );
                                saved = exclusion;
                            }
                        }
                    }

                    BOOST_CHECK_EQUAL( count, 1 );

                    for( SCH_SCREEN* current : { root.LastScreen(), screen } )
                    {
                        std::vector<SCH_MARKER*> markers;

                        for( SCH_ITEM* item : current->Items().OfType( SCH_MARKER_T ) )
                            markers.push_back( static_cast<SCH_MARKER*>( item ) );

                        for( SCH_MARKER* marker : markers )
                            current->DeleteItem( marker );
                    }
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCFootprintLinksBindSharedInstancePaths )
{
    struct FOOTPRINT_FACE : KIFACE
    {
        bool OnKifaceStart( PGM_BASE*, int, KIWAY* ) override { return true; }
        void OnKifaceEnd() override {}
        void Reset() override {}
        wxWindow* CreateKiWindow( wxWindow*, int, KIWAY*, int ) override { return nullptr; }
        void GetActions( std::vector<TOOL_ACTION*>& ) const override {}

        static int Check( const wxString& aFootprint, PROJECT* )
        {
            if( aFootprint == "NoLibrary:2" )
                return KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY;

            if( aFootprint == "Disabled:2" )
                return KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED;

            if( aFootprint == "Missing:2" )
                return KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT;

            return 0;
        }

        void* IfaceOrAddress( int aId ) override
        {
            BOOST_REQUIRE_EQUAL( aId, KIFACE_TEST_FOOTPRINT_LINK );
            return reinterpret_cast<void*>( &Check );
        }
    } face;

    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
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
    BOOST_REQUIRE( screen == paths[1].LastScreen() );
    paths[0].SetPageNumber( "2" );
    paths[1].SetPageNumber( "3" );
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        if( candidate->GetLibSymbolRef() && !candidate->GetLibSymbolRef()->IsPower() )
        {
            symbol = candidate;
            break;
        }
    }

    BOOST_REQUIRE( symbol );

    for( const wxString& footprint : { wxString( "NoLibrary:2" ), wxString( "NoLibrary:${#}" ),
                                      wxString( "Disabled:${#}" ), wxString( "Missing:${#}" ),
                                      wxString( "Present:${#}" ) } )
    {
        symbol->SetFootprintFieldText( footprint );
        const bool bothInstances = footprint == "NoLibrary:2";
        const size_t expected = bothInstances ? 2 : footprint == "Present:${#}" ? 0 : 1;

        for( bool backend : { true, false } )
        {
            enabled = backend;
            schematic->RebuildConnectivity();

            for( const SCH_SHEET_PATH& displayed : paths )
            {
                BOOST_TEST_CONTEXT( "footprint=" << footprint << "; backend=" << backend
                                    << "; displayed=" << displayed.GetPageNumber() )
                {
                    schematic->SetCurrentSheet( displayed );
                    ERC_TESTER tester( schematic.get() );
                    BOOST_CHECK_EQUAL( tester.TestFootprintLinkIssues( &face, &schematic->Project() ), expected );
                    size_t count = 0;
                    std::set<KIID_PATH> seen;
                    std::vector<SCH_MARKER*> markers;

                    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                    {
                        auto* marker = static_cast<SCH_MARKER*>( item );
                        markers.push_back( marker );
                        const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                        if( error->GetErrorCode() != ERCE_FOOTPRINT_LINK_ISSUES
                            || error->GetMainItemID() != symbol->m_Uuid )
                        {
                            continue;
                        }

                        ++count;
                        BOOST_CHECK( marker->GetPosition() == symbol->GetPosition() );
                        BOOST_CHECK( error->IsSheetSpecific() );
                        BOOST_CHECK( error->MainItemHasSheetPath() );
                        BOOST_CHECK( !error->AuxItemHasSheetPath() );

                        if( error->IsSheetSpecific() && error->MainItemHasSheetPath() )
                        {
                            const KIID_PATH& path = error->GetSpecificSheetPath().PathRef();
                            BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == path );
                            BOOST_CHECK( seen.insert( path ).second );
                            BOOST_CHECK( path == paths[0].PathRef()
                                         || ( bothInstances && path == paths[1].PathRef() ) );
                        }
                    }

                    BOOST_CHECK_EQUAL( count, expected );
                    BOOST_CHECK_EQUAL( seen.size(), expected );

                    for( SCH_MARKER* marker : markers )
                        screen->DeleteItem( marker );
                }
            }
        }
    }

    symbol->SetFootprintFieldText( "NoLibrary:2" );
    const auto targetMarkers = [&]
    {
        std::vector<SCH_MARKER*> result;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == ERCE_FOOTPRINT_LINK_ISSUES
                && marker->GetRCItem()->GetMainItemID() == symbol->m_Uuid )
            {
                result.push_back( marker );
            }
        }

        return result;
    };
    auto& exclusions = schematic->ErcSettings().m_ErcExclusions;

    for( const SCH_SHEET_PATH& target : paths )
    {
        for( bool pathlessExclusion : { false, true } )
        {
            enabled = false;
            exclusions.clear();
            schematic->RebuildConnectivity();
            ERC_TESTER tester( schematic.get() );
            tester.TestFootprintLinkIssues( &face, &schematic->Project() );
            BOOST_REQUIRE_EQUAL( targetMarkers().size(), 2 );

            for( SCH_MARKER* marker : targetMarkers() )
            {
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_REQUIRE( error->IsSheetSpecific() );

                if( error->GetSpecificSheetPath().PathRef() != target.PathRef() )
                    continue;

                auto proto = ERC_EXCLUSION::FromMarker( *marker ).ToProto();

                if( pathlessExclusion )
                {
                    proto.mutable_marker()->clear_sheet_specific_path();
                    proto.mutable_marker()->clear_main_item_sheet_path();
                }

                auto exclusion = ERC_EXCLUSION::FromProto( proto );
                exclusion.SetComment( "Retained footprint link exclusion" );
                const nlohmann::json saved = exclusion;
                exclusions.insert( saved.get<ERC_EXCLUSION>() );
            }

            BOOST_REQUIRE_EQUAL( exclusions.size(), 1 );
            const auto savedExclusions = exclusions;

            for( SCH_MARKER* marker : targetMarkers() )
                screen->DeleteItem( marker );

            for( bool backend : { true, false } )
            {
                BOOST_TEST_CONTEXT( "pathless exclusion=" << pathlessExclusion << "; backend=" << backend
                                        << "; target=" << target.GetPageNumber() )
                {
                    enabled = backend;
                    exclusions = savedExclusions;
                    schematic->RebuildConnectivity();
                    ERC_TESTER rescan( schematic.get() );
                    rescan.TestFootprintLinkIssues( &face, &schematic->Project() );
                    schematic->ResolveERCExclusionsPostUpdate();
                    BOOST_CHECK_EQUAL( targetMarkers().size(), 2 );
                    size_t excluded = 0;

                    for( SCH_MARKER* marker : targetMarkers() )
                    {
                        const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                        BOOST_CHECK( error->IsSheetSpecific() );
                        BOOST_CHECK( error->MainItemHasSheetPath() );

                        if( !error->IsSheetSpecific() )
                            continue;

                        const bool expected = pathlessExclusion
                                || error->GetSpecificSheetPath().PathRef() == target.PathRef();
                        BOOST_CHECK_EQUAL( marker->IsExcluded(), expected );
                        BOOST_CHECK_EQUAL( marker->GetComment(), expected
                                ? wxString( "Retained footprint link exclusion" ) : wxString() );
                        excluded += marker->IsExcluded();
                    }

                    BOOST_CHECK_EQUAL( excluded, pathlessExclusion ? 2 : 1 );

                    for( SCH_MARKER* marker : targetMarkers() )
                        screen->DeleteItem( marker );
                }
            }
        }
    }

    symbol->SetFootprintFieldText( "NoLibrary:${#}" );

    for( const auto& [backend, legacyString] : { std::pair{ true, false }, std::pair{ false, false },
                                               std::pair{ true, true }, std::pair{ false, true } } )
    {
        BOOST_TEST_CONTEXT( "deleted exclusion instance; backend=" << backend << "; legacy=" << legacyString )
        {
            enabled = backend;
            exclusions.clear();
            schematic->SetCurrentSheet( schematic->Hierarchy().front() );
            schematic->RebuildConnectivity();
            ERC_TESTER tester( schematic.get() );
            tester.TestFootprintLinkIssues( &face, &schematic->Project() );
            BOOST_REQUIRE_EQUAL( targetMarkers().size(), 1 );
            SCH_MARKER* original = targetMarkers().front();
            const auto error = std::static_pointer_cast<ERC_ITEM>( original->GetRCItem() );
            BOOST_REQUIRE( error->IsSheetSpecific() );
            BOOST_REQUIRE( error->GetSpecificSheetPath().PathRef() == paths[0].PathRef() );
            original->SetExcluded( true, "Keep the removed instance exclusion" );
            schematic->RecordERCExclusions();
            BOOST_REQUIRE_EQUAL( exclusions.size(), 1 );
            const nlohmann::json saved = *exclusions.begin();
            const wxString originalComment = original->GetComment();
            const wxString legacyData = wxString::Format( wxS( "%s|%d|%d|%s|%s|%s|%s|" ),
                    error->GetSettingsKey(), original->GetPosition().x, original->GetPosition().y,
                    error->GetMainItemID().AsString(), niluuid.AsString(),
                    error->GetSpecificSheetPath().PathRef().AsString(),
                    error->GetMainItemSheetPath().PathRef().AsString() );
            screen->DeleteItem( original );
            exclusions.clear();

            if( legacyString )
                schematic->ErcSettings().m_ErcExclusionsLegacy.emplace( legacyData, originalComment );
            else
                exclusions.insert( saved.get<ERC_EXCLUSION>() );

            SCH_SHEET* removed = paths[0].Last();
            schematic->RootScreen()->Remove( removed );
            std::unique_ptr<SCH_SHEET> undoOwnedSheet( removed );
            schematic->RefreshHierarchy();
            BOOST_REQUIRE( !schematic->Hierarchy().GetSheetPathByKIIDPath( paths[0].PathRef() ) );
            BOOST_REQUIRE( schematic->Hierarchy().GetSheetPathByKIIDPath( paths[1].PathRef() ) );
            BOOST_REQUIRE( schematic->ResolveItem( symbol->m_Uuid, nullptr, true ) == symbol );

            schematic->SetCurrentSheet( paths[1] );
            schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK( schematic->ErcSettings().m_ErcExclusionsLegacy.empty() );

            for( SCH_MARKER* marker : targetMarkers() )
            {
                const auto retained = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_CHECK( retained->IsSheetSpecific() );

                if( retained->IsSheetSpecific() )
                    BOOST_CHECK( retained->GetSpecificSheetPath().PathRef() == paths[0].PathRef() );
            }

            BOOST_CHECK_EQUAL( exclusions.size(), 1 );

            if( exclusions.size() == 1 )
                BOOST_CHECK( nlohmann::json( *exclusions.begin() ) == saved );

            BOOST_CHECK_EQUAL( schematic->GetUnresolvedERCExclusionCount(), 1 );

            if( !legacyString )
            {
                schematic->ClearUnresolvedERCExclusions( ERCE_PIN_NOT_CONNECTED );
                BOOST_CHECK_EQUAL( schematic->GetUnresolvedERCExclusionCount(), 1 );
                schematic->ClearUnresolvedERCExclusions();
                schematic->RecordERCExclusions();
                schematic->ResolveERCExclusionsPostUpdate();
                BOOST_CHECK( exclusions.empty() );
                BOOST_CHECK_EQUAL( schematic->GetUnresolvedERCExclusionCount(), 0 );
                exclusions.insert( saved.get<ERC_EXCLUSION>() );
                schematic->ResolveERCExclusionsPostUpdate();
            }

            schematic->RootScreen()->Append( undoOwnedSheet.release() );
            schematic->RefreshHierarchy();
            schematic->SetCurrentSheet( schematic->Hierarchy().front() );
            schematic->RecordERCExclusions();
            BOOST_CHECK_EQUAL( exclusions.size(), 1 );

            if( exclusions.size() == 1 )
                BOOST_CHECK( nlohmann::json( *exclusions.begin() ) == saved );

            for( SCH_MARKER* marker : targetMarkers() )
                screen->DeleteItem( marker );

            schematic->RebuildConnectivity();
            ERC_TESTER restored( schematic.get() );
            restored.TestFootprintLinkIssues( &face, &schematic->Project() );
            exclusions.clear();
            schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK_EQUAL( targetMarkers().size(), 1 );
            BOOST_CHECK_EQUAL( schematic->GetUnresolvedERCExclusionCount(), 0 );

            for( SCH_MARKER* marker : targetMarkers() )
            {
                const auto restoredError = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_CHECK( marker->IsExcluded() );
                BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Keep the removed instance exclusion" ) );
                BOOST_CHECK( restoredError->IsSheetSpecific() );

                if( restoredError->IsSheetSpecific() )
                    BOOST_CHECK( restoredError->GetSpecificSheetPath().PathRef() == paths[0].PathRef() );

                marker->SetExcluded( false );
                schematic->RecordERCExclusions();
                BOOST_CHECK( exclusions.empty() );
                screen->DeleteItem( marker );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCFootprintFiltersBindSharedInstancePaths )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );

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
    BOOST_REQUIRE( screen == paths[1].LastScreen() );
    BOOST_REQUIRE( paths[0].GetPageNumber() != paths[1].GetPageNumber() );
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        if( candidate->GetLibSymbolRef() && !candidate->GetLibSymbolRef()->IsPower() )
        {
            symbol = candidate;
            break;
        }
    }

    BOOST_REQUIRE( symbol );
    symbol->SetFootprintFieldText( "Acceptance:${#}" );

    const wxString firstPage = paths[0].GetPageNumber();
    const wxString secondPage = paths[1].GetPageNumber();
    wxArrayString pageFilters;
    pageFilters.Add( wxString( "Acceptance:" ) + firstPage );
    symbol->GetLibSymbolRef()->SetFPFilters( pageFilters );
    auto collectPaths = [&]()
    {
        ERC_TESTER tester( schematic.get() );
        tester.TestFootprintFilters();
        std::set<KIID_PATH> found;
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );
            const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

            if( error->GetErrorCode() == ERCE_FOOTPRINT_FILTERS && error->GetMainItemID() == symbol->m_Uuid )
            {
                BOOST_REQUIRE( error->IsSheetSpecific() );
                BOOST_CHECK( error->MainItemHasSheetPath() );
                BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == error->GetSpecificSheetPath().PathRef() );
                BOOST_CHECK( found.insert( error->GetSpecificSheetPath().PathRef() ).second );
            }

            markers.push_back( marker );
        }

        for( SCH_MARKER* marker : markers )
            screen->DeleteItem( marker );

        return found;
    };

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "backend=" << backend )
        {
            enabled = backend;
            schematic->RebuildConnectivity();
            const auto beforePages = collectPaths();
            BOOST_REQUIRE( beforePages == std::set<KIID_PATH>{ paths[1].PathRef() } );
            paths[0].SetPageNumber( secondPage );
            paths[1].SetPageNumber( firstPage );

            // Only the engine refreshes incrementally, and that refresh must see page renumbering
            if( backend )
                schematic->Connectivity().Recalculate( *schematic );

            const auto afterPages = collectPaths();
            BOOST_REQUIRE( afterPages == std::set<KIID_PATH>{ paths[0].PathRef() } );
            schematic->RebuildConnectivity();
            BOOST_CHECK( collectPaths() == afterPages );
            paths[0].SetPageNumber( firstPage );
            paths[1].SetPageNumber( secondPage );

            if( backend )
                schematic->Connectivity().Recalculate( *schematic );

            BOOST_CHECK( collectPaths() == beforePages );
            schematic->SetCurrentSheet( schematic->Hierarchy().front() );
            SCH_SHEET* removed = paths[1].Last();
            schematic->RootScreen()->Remove( removed );
            std::unique_ptr<SCH_SHEET> undoOwnedSheet( removed );
            schematic->RefreshHierarchy();
            BOOST_CHECK( collectPaths().empty() );
            schematic->RootScreen()->Append( undoOwnedSheet.release() );
            schematic->RefreshHierarchy();
            BOOST_CHECK( collectPaths() == beforePages );
        }
    }
}
