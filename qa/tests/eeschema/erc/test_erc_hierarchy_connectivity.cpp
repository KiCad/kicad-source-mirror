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

#include <wx/filename.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <advanced_config.h>
#include <api/schematic/schematic_rules.pb.h>
#include <erc/erc_exclusion.h>
#include <erc/erc_report.h>
#include <json_common.h>
#include <connection_graph.h>
#include <connectivity/conn_facade.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <locale_io.h>
#include <sch_label.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <map>
#include <set>
#include <tuple>
#include <scoped_set_reset.h>


BOOST_AUTO_TEST_CASE( ERCHierarchyMismatchesUseCapturedConnectivity )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue24201/issue24201", schematic );
    SCH_LABEL_BASE* renamed = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( path.size() > schematic->Hierarchy().front().size() )
        {
            auto labels = path.LastScreen()->Items().OfType( SCH_HIER_LABEL_T );

            if( labels.begin() != labels.end() )
                renamed = static_cast<SCH_LABEL_BASE*>( *labels.begin() );
        }
    }

    BOOST_REQUIRE( renamed );
    renamed->SetText( "ERC_UNMATCHED_PORT" );

    for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
        severity = code == ERCE_HIERACHICAL_LABEL ? RPT_SEVERITY_ERROR : RPT_SEVERITY_IGNORE;

    using DIAGNOSTICS = std::map<std::pair<KIID_PATH, KIID>, VECTOR2I>;
    const auto collect = [&]()
    {
        DIAGNOSTICS result;

        for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_CHECK_EQUAL( error->GetErrorCode(), ERCE_HIERACHICAL_LABEL );
                BOOST_CHECK( error->MainItemHasSheetPath() );
                BOOST_CHECK( !error->AuxItemHasSheetPath() );
                result.emplace( std::pair{ error->GetSpecificSheetPath().PathRef(), error->GetMainItemID() },
                                marker->GetPosition() );
            }
        }

        return result;
    };
    schematic->RebuildConnectivity();
    BOOST_REQUIRE_EQUAL( schematic->ConnectionGraph()->RunERC(), 2 );
    const auto expected = collect();
    BOOST_REQUIRE_EQUAL( expected.size(), 2 );

    const auto clearMarkers = [&]()
    {
        for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
        {
            std::vector<SCH_ITEM*> markers;

            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                markers.push_back( item );

            for( SCH_ITEM* marker : markers )
                path.LastScreen()->DeleteItem( marker );
        }
    };
    clearMarkers();

    enabled = true;
    schematic->RebuildConnectivity();
    schematic->ConnectionGraph()->Reset();
    BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), 2 );
    BOOST_CHECK( collect() == expected );
}


BOOST_AUTO_TEST_CASE( ERCRootHierarchyLabelsKeepEveryItemAndRespectSeverity )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue24201/issue24201", schematic );
    const SCH_SHEET_PATH root = schematic->Hierarchy().front();
    SCH_LABEL_BASE* source = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            source = static_cast<SCH_LABEL_BASE*>( item );
    }

    BOOST_REQUIRE( source );
    std::set<KIID> expected;

    for( int index = 0; index < 2; ++index )
    {
        auto* label = static_cast<SCH_LABEL_BASE*>( source->Duplicate( false ) );
        const VECTOR2I position( 100000000 + index * 1000000, 100000000 );
        label->SetText( "ERC_ROOT_PORT" );
        label->Move( position - label->GetPosition() );
        root.LastScreen()->Append( label );
        expected.insert( label->m_Uuid );
    }

    enabled = false;

    for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
        severity = code == ERCE_PIN_NOT_CONNECTED ? RPT_SEVERITY_ERROR : RPT_SEVERITY_IGNORE;

    schematic->RebuildConnectivity();
    schematic->ConnectionGraph()->RunERC();
    std::vector<SCH_ITEM*> legacyMarkers;

    for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_MARKER_T ) )
    {
        auto* marker = static_cast<SCH_MARKER*>( item );
        legacyMarkers.push_back( marker );

        if( expected.contains( marker->GetRCItem()->GetMainItemID() ) )
            marker->SetExcluded( true, "Retained root port" );
    }

    schematic->RecordERCExclusions();
    BOOST_REQUIRE_EQUAL( schematic->ErcSettings().m_ErcExclusions.size(), 2 );

    for( SCH_ITEM* marker : legacyMarkers )
        root.LastScreen()->DeleteItem( marker );

    using SIGNATURE = std::tuple<KIID, int, bool, KIID_PATH, bool, size_t, int, int, bool, wxString>;

    for( bool pinEnabled : { false, true } )
    {
        for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
        {
            severity = code == ERCE_HIERACHICAL_LABEL || ( pinEnabled && code == ERCE_PIN_NOT_CONNECTED )
                    ? RPT_SEVERITY_ERROR : RPT_SEVERITY_IGNORE;
        }

        std::multiset<SIGNATURE> legacy;

        for( bool backend : { false, true } )
        {
            BOOST_TEST_CONTEXT( "pinEnabled=" << pinEnabled << "; backend=" << backend )
            {
                enabled = backend;
                schematic->RebuildConnectivity();
                schematic->ConnectionGraph()->RunERC();

                if( pinEnabled )
                    schematic->ResolveERCExclusionsPostUpdate();

                std::multiset<SIGNATURE> actual;
                size_t pinMarkers = 0;
                std::vector<SCH_ITEM*> markers;

                for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    markers.push_back( marker );
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                    // Legacy creates root-label markers even when their severity hides them from users
                    if( !expected.contains( error->GetMainItemID() )
                        || schematic->ErcSettings().GetSeverity( error->GetErrorCode() ) == RPT_SEVERITY_IGNORE )
                        continue;

                    if( error->GetErrorCode() == ERCE_PIN_NOT_CONNECTED )
                    {
                        BOOST_CHECK( marker->IsExcluded() );
                        BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained root port" ) );
                        ++pinMarkers;
                    }

                    actual.emplace( error->GetMainItemID(), error->GetErrorCode(), error->IsSheetSpecific(),
                                    error->IsSheetSpecific() ? error->GetSpecificSheetPath().PathRef() : KIID_PATH(),
                                    error->MainItemHasSheetPath(), error->GetIDs().size(), marker->GetPosition().x,
                                    marker->GetPosition().y, marker->IsExcluded(), marker->GetComment() );
                }

                if( !backend )
                    legacy = actual;

                BOOST_CHECK_EQUAL( pinMarkers, pinEnabled ? expected.size() : 0 );
                BOOST_CHECK( actual == legacy );

                for( SCH_ITEM* marker : markers )
                    root.LastScreen()->DeleteItem( marker );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( ERCHierarchySheetPinDanglingUsesCapturedContacts )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue24201/issue24201", schematic );
    const SCH_SHEET_PATH root = schematic->Hierarchy().front();
    auto sheets = root.LastScreen()->Items().OfType( SCH_SHEET_T );
    BOOST_REQUIRE( sheets.begin() != sheets.end() );
    auto* sheet = static_cast<SCH_SHEET*>( *sheets.begin() );
    BOOST_REQUIRE( !sheet->GetPins().empty() );
    SCH_SHEET_PIN* pin = sheet->GetPins().front();
    sheet->Move( VECTOR2I( 100000000, 100000000 ) );

    for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
        severity = code == ERCE_PIN_NOT_CONNECTED ? RPT_SEVERITY_ERROR : RPT_SEVERITY_IGNORE;

    schematic->RebuildConnectivity();
    schematic->ConnectionGraph()->RunERC();
    std::vector<SCH_ITEM*> legacyMarkers;

    for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_MARKER_T ) )
    {
        auto* marker = static_cast<SCH_MARKER*>( item );
        legacyMarkers.push_back( marker );

        if( marker->GetRCItem()->GetMainItemID() == pin->m_Uuid )
            marker->SetExcluded( true, "Retained sheet pin" );
    }

    schematic->RecordERCExclusions();
    BOOST_REQUIRE_EQUAL( schematic->ErcSettings().m_ErcExclusions.size(), 1 );

    for( SCH_ITEM* marker : legacyMarkers )
        root.LastScreen()->DeleteItem( marker );

    using SIGNATURE = std::tuple<int, wxString, int, int, bool, KIID_PATH, bool, bool, size_t, bool, wxString>;
    std::multiset<SIGNATURE> legacy;

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "backend=" << backend )
        {
            enabled = backend;
            schematic->RebuildConnectivity();
            schematic->ConnectionGraph()->RunERC();
            schematic->ResolveERCExclusionsPostUpdate();
            std::multiset<SIGNATURE> actual;
            std::vector<SCH_ITEM*> markers;

            for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );
                markers.push_back( marker );
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                if( error->GetMainItemID() != pin->m_Uuid )
                    continue;

                BOOST_CHECK( marker->IsExcluded() );
                BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained sheet pin" ) );
                actual.emplace( error->GetErrorCode(), error->GetErrorMessage( true ), marker->GetPosition().x,
                                marker->GetPosition().y, error->IsSheetSpecific(),
                                error->IsSheetSpecific() ? error->GetSpecificSheetPath().PathRef() : KIID_PATH(),
                                error->MainItemHasSheetPath(), error->AuxItemHasSheetPath(), error->GetIDs().size(),
                                marker->IsExcluded(), marker->GetComment() );
            }

            if( !backend )
                legacy = actual;

            BOOST_CHECK_EQUAL( actual.size(), 1 );
            BOOST_CHECK( actual == legacy );

            for( SCH_ITEM* marker : markers )
                root.LastScreen()->DeleteItem( marker );
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCSimulationModelsBindAffectedSharedInstances )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "legacy_hierarchy/legacy_hierarchy", schematic );
    schematic->ErcSettings().SetSeverity( ERCE_SIMULATION_MODEL, RPT_SEVERITY_WARNING );
    const auto refresh = [&]
    {
        schematic->RebuildConnectivity();

        if( enabled )
            schematic->Connectivity().PrepareSimulationModels( *schematic );
    };
    std::vector<SCH_SHEET_PATH> paths;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "ampli_ht.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2 );
    BOOST_REQUIRE( paths[0].LastScreen() == paths[1].LastScreen() );
    paths[0].SetPageNumber( wxS( "2" ) );
    paths[1].SetPageNumber( wxS( "3" ) );
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : paths[0].LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        if( candidate->GetRef( &paths[0] ).StartsWith( wxS( "D" ) ) )
        {
            symbol = candidate;
            break;
        }
    }

    BOOST_REQUIRE( symbol );
    const wxString library = wxFileName( wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                                        + wxS( "spice_netlists/rectifier/diode.lib" ) ).GetFullPath();

    for( const auto& [name, value] : std::vector<std::pair<wxString, wxString>>{
            { wxS( "Sim.Device" ), wxS( "D" ) }, { wxS( "Sim.Library" ), library },
            { wxS( "Sim.Name" ), wxS( "DIODE1" ) }, { wxS( "Sim.Pins" ), wxS( "1=K 2=A" ) } } )
    {
        if( !symbol->GetField( name ) )
            symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, name ) );

        symbol->GetField( name )->SetText( value );
    }

    for( bool backend : { false, true } )
    {
        enabled = backend;
        std::map<KIID_PATH, int> validCounts;

        for( const wxString& name : { wxString( "DIODE1" ), wxString( "MISSING_NATIVE_MODEL" ),
                                     wxString( "DIODE${#}" ) } )
        {
            symbol->GetField( wxS( "Sim.Name" ) )->SetText( name );
            refresh();

            for( const SCH_SHEET_PATH& displayed : paths )
            {
                schematic->SetCurrentSheet( displayed );
                ERC_TESTER tester( schematic.get() );
                const int reported = tester.TestSimModelIssues();
                std::set<KIID_PATH> affected;
                size_t count = 0;
                std::vector<SCH_MARKER*> markers;

                for( SCH_ITEM* item : paths[0].LastScreen()->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    markers.push_back( marker );
                    auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                    if( error->GetErrorCode() != ERCE_SIMULATION_MODEL || error->GetMainItemID() != symbol->m_Uuid )
                        continue;

                    ++count;
                    BOOST_CHECK( error->IsSheetSpecific() );
                    BOOST_CHECK( error->MainItemHasSheetPath() );
                    BOOST_CHECK( !error->AuxItemHasSheetPath() );
                    BOOST_CHECK( marker->GetPosition() == symbol->GetPosition() );

                    if( error->IsSheetSpecific() && error->MainItemHasSheetPath() )
                    {
                        BOOST_CHECK( error->GetSpecificSheetPath() == error->GetMainItemSheetPath() );
                        BOOST_CHECK( affected.insert( error->GetSpecificSheetPath().PathRef() ).second );
                    }
                }

                const size_t expected = name == wxS( "DIODE1" ) ? 0 : name == wxS( "DIODE${#}" ) ? 1 : 2;
                BOOST_CHECK_EQUAL( count, expected );
                BOOST_CHECK_EQUAL( affected.size(), expected );

                if( expected == 2 )
                    BOOST_CHECK( ( affected == std::set<KIID_PATH>{ paths[0].PathRef(), paths[1].PathRef() } ) );

                if( name == wxS( "DIODE1" ) )
                    validCounts[displayed.PathRef()] = reported;
                else
                    BOOST_CHECK_EQUAL( reported - validCounts.at( displayed.PathRef() ), expected );

                if( name == wxS( "DIODE${#}" ) )
                    BOOST_CHECK( affected.contains( paths[1].PathRef() ) );

                for( SCH_MARKER* marker : markers )
                    paths[0].LastScreen()->DeleteItem( marker );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCDuplicateSheetsBindSharedParentInstances )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue25112/issue25112", schematic );
    schematic->ErcSettings().SetSeverity( ERCE_DUPLICATE_SHEET_NAME, RPT_SEVERITY_ERROR );
    std::vector<SCH_SHEET_PATH> parents;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( wxFileName( path.LastScreen()->GetFileName() ).GetFullName() == wxS( "level1.kicad_sch" ) )
            parents.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( parents.size(), 2 );
    BOOST_REQUIRE( parents[0].LastScreen() == parents[1].LastScreen() );
    SCH_SCREEN* screen = parents[0].LastScreen();
    std::vector<SCH_SHEET*> children;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
        children.push_back( static_cast<SCH_SHEET*>( item ) );

    BOOST_REQUIRE_EQUAL( children.size(), 2 );
    const std::set<KIID_PATH> expectedPaths{ parents[0].PathRef(), parents[1].PathRef() };
    const auto checkReport = [&]( size_t expected )
    {
        ERC_REPORT report( schematic.get(), EDA_UNITS::MM );
        wxString text = report.GetTextReport();
        BOOST_CHECK( text.Contains( children[0]->GetName() ) );
        const wxString message = ERC_ITEM::Create( ERCE_DUPLICATE_SHEET_NAME )->GetErrorMessage( true );
        BOOST_CHECK_EQUAL( text.Replace( message, wxEmptyString ), expected );
    };

    for( bool backend : { false, true } )
    {
        enabled = backend;
        children[0]->SetName( wxS( "DuplicateChild" ) );
        children[1]->SetName( wxS( "DistinctChild" ) );
        schematic->RebuildConnectivity();
        ERC_TESTER valid( schematic.get() );
        BOOST_CHECK_EQUAL( valid.TestDuplicateSheetNames( false ), 0 );
        BOOST_CHECK_EQUAL( valid.TestDuplicateSheetNames( true ), 0 );
        auto initialMarkers = screen->Items().OfType( SCH_MARKER_T );
        BOOST_CHECK( initialMarkers.begin() == initialMarkers.end() );
        children[1]->SetName( wxS( "duplicatechild" ) );

        // Export and highlight preflight checks run before connectivity refresh
        ERC_TESTER preflight( schematic.get() );
        BOOST_CHECK_EQUAL( preflight.TestDuplicateSheetNames( false ), 2 );
        auto preflightMarkers = screen->Items().OfType( SCH_MARKER_T );
        BOOST_CHECK( preflightMarkers.begin() == preflightMarkers.end() );
        schematic->RebuildConnectivity();

        for( const SCH_SHEET_PATH& displayed : parents )
        {
            schematic->SetCurrentSheet( displayed );
            ERC_TESTER tester( schematic.get() );
            BOOST_CHECK_EQUAL( tester.TestDuplicateSheetNames( true ), 2 );
            std::set<KIID_PATH> actualPaths;
            std::vector<SCH_MARKER*> markers;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );
                markers.push_back( marker );
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_CHECK_EQUAL( error->GetErrorCode(), ERCE_DUPLICATE_SHEET_NAME );
                BOOST_CHECK( error->GetMainItemID() == children[0]->m_Uuid );
                BOOST_CHECK( error->GetAuxItemID() == children[1]->m_Uuid );
                BOOST_CHECK( error->IsSheetSpecific() );
                BOOST_CHECK( error->MainItemHasSheetPath() );
                BOOST_CHECK( error->AuxItemHasSheetPath() );
                BOOST_CHECK( marker->GetPosition() == children[0]->GetPosition() );

                if( error->IsSheetSpecific() && error->MainItemHasSheetPath() && error->AuxItemHasSheetPath() )
                {
                    BOOST_CHECK( error->GetSpecificSheetPath() == error->GetMainItemSheetPath() );
                    BOOST_CHECK( error->GetSpecificSheetPath() == error->GetAuxItemSheetPath() );
                    BOOST_CHECK( actualPaths.insert( error->GetSpecificSheetPath().PathRef() ).second );
                }
            }

            BOOST_CHECK_EQUAL( markers.size(), 2 );
            checkReport( 2 );
            BOOST_CHECK( actualPaths == expectedPaths );
            BOOST_CHECK( schematic->CurrentSheet() == displayed );

            for( SCH_MARKER* marker : markers )
                screen->DeleteItem( marker );
        }

        children[0]->SetName( wxS( "Child42" ) );
        children[1]->SetName( wxS( "Child${#}" ) );

        for( size_t i = 0; i < parents.size(); ++i )
        {
            SCH_SHEET_PATH childPath = parents[i];
            childPath.push_back( children[1] );
            childPath.SetPageNumber( i == 0 ? wxString( "42" ) : wxString( "43" ) );
            BOOST_REQUIRE_EQUAL( children[1]->GetField( FIELD_T::SHEET_NAME )->GetShownText( &parents[i], RESOLVED ),
                                 i == 0 ? wxString( "Child42" ) : wxString( "Child43" ) );
        }

        schematic->RebuildConnectivity();

        for( const SCH_SHEET_PATH& displayed : parents )
        {
            schematic->SetCurrentSheet( displayed );
            ERC_TESTER tester( schematic.get() );
            BOOST_CHECK_EQUAL( tester.TestDuplicateSheetNames( false ), 1 );
            BOOST_CHECK_EQUAL( tester.TestDuplicateSheetNames( true ), 1 );
            std::vector<SCH_MARKER*> markers;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );
                markers.push_back( marker );
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_CHECK( marker->GetPosition() == children[0]->GetPosition() );
                BOOST_CHECK( error->IsSheetSpecific() );

                if( error->IsSheetSpecific() )
                    BOOST_CHECK( error->GetSpecificSheetPath().PathRef() == parents[0].PathRef() );
            }

            BOOST_CHECK_EQUAL( markers.size(), 1 );
            checkReport( 1 );
            BOOST_CHECK( schematic->CurrentSheet() == displayed );

            for( SCH_MARKER* marker : markers )
                screen->DeleteItem( marker );
        }
    }

    children[0]->SetName( wxS( "DuplicateChild" ) );
    children[1]->SetName( wxS( "duplicatechild" ) );
    const auto targetMarkers = [&]
    {
        std::vector<SCH_MARKER*> result;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == ERCE_DUPLICATE_SHEET_NAME
                && marker->GetRCItem()->GetMainItemID() == children[0]->m_Uuid )
            {
                result.push_back( marker );
            }
        }

        return result;
    };
    auto& exclusions = schematic->ErcSettings().m_ErcExclusions;

    for( const SCH_SHEET_PATH& target : parents )
    {
        for( int format : { 0, 1, 2, 3, 4 } )
        {
            const bool pathless = format != 0;
            const bool mixed = format == 2;
            const bool legacyString = format >= 3;
            exclusions.clear();
            schematic->ErcSettings().m_ErcExclusionsLegacy.clear();
            enabled = false;
            schematic->RebuildConnectivity();
            ERC_TESTER tester( schematic.get() );
            tester.TestDuplicateSheetNames( true );
            BOOST_REQUIRE_EQUAL( targetMarkers().size(), 2 );

            for( SCH_MARKER* marker : targetMarkers() )
            {
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_REQUIRE( error->IsSheetSpecific() );

                if( error->GetSpecificSheetPath().PathRef() != target.PathRef() )
                    continue;

                auto proto = ERC_EXCLUSION::FromMarker( *marker ).ToProto();

                if( mixed )
                {
                    auto scoped = ERC_EXCLUSION::FromProto( proto );
                    scoped.SetComment( wxS( "Scoped duplicate-sheet exclusion" ) );
                    const nlohmann::json saved = scoped;
                    exclusions.insert( saved.get<ERC_EXCLUSION>() );
                }

                if( pathless )
                {
                    proto.mutable_marker()->clear_sheet_specific_path();
                    proto.mutable_marker()->clear_main_item_sheet_path();
                    proto.mutable_marker()->clear_aux_item_sheet_path();
                }

                auto exclusion = ERC_EXCLUSION::FromProto( proto );
                exclusion.SetComment( wxS( "Retained duplicate-sheet exclusion" ) );
                if( legacyString )
                {
                    wxString data = wxString::Format( wxS( "duplicate_sheet_names|%d|%d|%s|%s" ),
                            children[0]->GetPosition().x, children[0]->GetPosition().y,
                            children[0]->m_Uuid.AsString(), children[1]->m_Uuid.AsString() );

                    if( format == 3 )
                        data += wxS( "|||" );

                    BOOST_CHECK( ERC_EXCLUSION::FromLegacyStrings( schematic->Hierarchy(), data,
                                                                   exclusion.GetComment() ) == exclusion );
                    schematic->ErcSettings().m_ErcExclusionsLegacy.emplace( data, exclusion.GetComment() );
                }
                else
                {
                    const nlohmann::json saved = exclusion;
                    exclusions.insert( saved.get<ERC_EXCLUSION>() );
                }
            }

            BOOST_REQUIRE_EQUAL( exclusions.size(), legacyString ? 0 : mixed ? 2 : 1 );
            const auto savedExclusions = exclusions;
            const auto savedLegacy = schematic->ErcSettings().m_ErcExclusionsLegacy;

            for( SCH_MARKER* marker : targetMarkers() )
                screen->DeleteItem( marker );

            for( bool backend : { true, false } )
            {
                exclusions = savedExclusions;
                schematic->ErcSettings().m_ErcExclusionsLegacy = savedLegacy;
                enabled = backend;
                schematic->RebuildConnectivity();
                ERC_TESTER rescan( schematic.get() );
                rescan.TestDuplicateSheetNames( true );
                schematic->ResolveERCExclusionsPostUpdate();
                BOOST_CHECK_EQUAL( targetMarkers().size(), 2 );
                size_t excluded = 0;

                for( SCH_MARKER* marker : targetMarkers() )
                {
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                    BOOST_CHECK( error->IsSheetSpecific() );
                    BOOST_CHECK( error->MainItemHasSheetPath() );
                    BOOST_CHECK( error->AuxItemHasSheetPath() );

                    if( error->IsSheetSpecific() && error->MainItemHasSheetPath() && error->AuxItemHasSheetPath() )
                    {
                        BOOST_CHECK( error->GetMainItemSheetPath() == error->GetSpecificSheetPath() );
                        BOOST_CHECK( error->GetAuxItemSheetPath() == error->GetSpecificSheetPath() );
                        const bool expected = pathless || error->GetSpecificSheetPath() == target;
                        BOOST_CHECK_EQUAL( marker->IsExcluded(), expected );
                        const wxString comment = mixed && error->GetSpecificSheetPath() == target
                                ? wxString( "Scoped duplicate-sheet exclusion" )
                                : expected ? wxString( "Retained duplicate-sheet exclusion" ) : wxString();
                        BOOST_CHECK_EQUAL( marker->GetComment(), comment );
                    }

                    excluded += marker->IsExcluded();
                }

                BOOST_CHECK_EQUAL( excluded, pathless ? 2 : 1 );

                for( SCH_MARKER* marker : targetMarkers() )
                    screen->DeleteItem( marker );
            }
        }
    }
}
