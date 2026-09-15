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
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <algorithm>
#include <set>
#include <connection_graph.h>
#include <sch_label.h>
#include <sch_marker.h>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_exclusion.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <scoped_set_reset.h>

struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


static int CountErrorCode( SHEETLIST_ERC_ITEMS_PROVIDER& aErrors, int aErrorCode )
{
    int count = 0;

    for( int i = 0; i < aErrors.GetCount(); ++i )
    {
        std::shared_ptr<ERC_ITEM> ercItem = std::static_pointer_cast<ERC_ITEM>( aErrors.GetItem( i ) );

        if( ercItem && ercItem->GetErrorCode() == aErrorCode )
            ++count;
    }

    return count;
}


BOOST_FIXTURE_TEST_CASE( ERCLabelCapitalization, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using rule area netclass directives
    std::vector<std::pair<wxString, int>> tests = { { "issue16897", 3 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_SIMILAR_LABELS] = RPT_SEVERITY_ERROR;
        settings.m_ERCSeverities[ERCE_SIMILAR_POWER] = RPT_SEVERITY_ERROR;
        settings.m_ERCSeverities[ERCE_SIMILAR_LABEL_AND_POWER] = RPT_SEVERITY_ERROR;

        m_schematic->ConnectionGraph()->RunERC();

        ERC_TESTER tester( m_schematic.get() );
        tester.TestSimilarLabels();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );
    }
}


BOOST_FIXTURE_TEST_CASE( ERCSimilarLabels, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using rule area netclass directives
    std::vector<std::pair<wxString, int>> tests = { { "similar_labels", 3 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LABEL_SINGLE_PIN] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_SAME_LOCAL_GLOBAL_LABEL] = RPT_SEVERITY_ERROR;

        m_schematic->ConnectionGraph()->RunERC();

        ERC_TESTER tester( m_schematic.get() );
        tester.TestSimilarLabels();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );
    }
}


BOOST_FIXTURE_TEST_CASE( ERCSameLocalGlobalLabel, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using rule area netclass directives
    std::vector<std::pair<wxString, int>> tests = { { "same_local_global_label", 1 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_SAME_LOCAL_GLOBAL_LABEL] = RPT_SEVERITY_ERROR;

        m_schematic->ConnectionGraph()->RunERC();

        ERC_TESTER tester( m_schematic.get() );
        tester.TestSameLocalGlobalLabel();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );
    }
}


BOOST_FIXTURE_TEST_CASE( ERCSameLocalGlobalPower, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "same_local_global_power", m_schematic );

    ERC_SETTINGS&                settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    settings.m_ERCSeverities[ERCE_SAME_LOCAL_GLOBAL_LABEL] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_SAME_LOCAL_GLOBAL_POWER] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestSameLocalGlobalLabel();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    BOOST_CHECK_MESSAGE( CountErrorCode( errors, ERCE_SAME_LOCAL_GLOBAL_POWER ) == 2,
                         "Expected 2 ERCE_SAME_LOCAL_GLOBAL_POWER violations\n"
                                 << reportWriter.GetTextReport() );

    BOOST_CHECK_MESSAGE( CountErrorCode( errors, ERCE_SAME_LOCAL_GLOBAL_LABEL ) == 0,
                         "Expected 0 ERCE_SAME_LOCAL_GLOBAL_LABEL violations\n"
                                 << reportWriter.GetTextReport() );
}


BOOST_AUTO_TEST_CASE( ERCLabelNameChecksMatchLegacy )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    struct CASE
    {
        const char* fixture;
        bool        similar;
        size_t      count;
    };

    for( const CASE& test : { CASE{ "similar_labels", true, 3 },
                             CASE{ "same_local_global_label", false, 1 },
                             CASE{ "same_local_global_power", false, 2 } } )
    {
        BOOST_TEST_CONTEXT( test.fixture )
        {
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            enabled = false;
            KI_TEST::LoadSchematic( settings, test.fixture, schematic );
            schematic->RebuildConnectivity();
            ERC_TESTER tester( schematic.get() );
            const auto run = [&]()
            {
                if( test.similar )
                    tester.TestSimilarLabels();
                else
                    tester.TestSameLocalGlobalLabel();

                std::vector<std::string> markers;

                for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                {
                    std::vector<SCH_ITEM*> remove;

                    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                    {
                        markers.push_back(
                                ERC_EXCLUSION::FromMarker( *static_cast<SCH_MARKER*>( item ) ).GetSortKey() );
                        remove.push_back( item );
                    }

                    for( SCH_ITEM* item : remove )
                        path.LastScreen()->DeleteItem( item );
                }

                std::sort( markers.begin(), markers.end() );
                return markers;
            };
            const auto legacy = run();
            BOOST_REQUIRE_EQUAL( legacy.size(), test.count );
            enabled = true;
            schematic->RebuildConnectivity();
            schematic->ConnectionGraph()->Reset();
            const auto engine = run();
            BOOST_TEST( engine == legacy, boost::test_tools::per_element() );
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCLabelNamesPreferShortestSharedInstance )
{
    LOCALE_IO locale;
    auto&     enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue9673/issue9673", schematic );
    std::vector<SCH_SHEET*> sheets;

    for( SCH_ITEM* item : schematic->RootScreen()->Items().OfType( SCH_SHEET_T ) )
        sheets.push_back( static_cast<SCH_SHEET*>( item ) );

    BOOST_REQUIRE_EQUAL( sheets.size(), 2u );
    std::ranges::sort( sheets,
                       []( const SCH_SHEET* a, const SCH_SHEET* b )
                       {
                           return a->m_Uuid < b->m_Uuid;
                       } );
    SCH_SHEET* parent = sheets.front();
    SCH_SHEET* target = sheets.back();
    auto*      nested = static_cast<SCH_SHEET*>( target->Duplicate( false ) );

    for( SCH_SHEET_PIN* pin : nested->GetPins() )
        const_cast<KIID&>( pin->m_Uuid ) = KIID();

    parent->GetScreen()->Append( nested );
    schematic->RefreshHierarchy();
    std::set<KIID_PATH> paths;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( path.LastScreen() == target->GetScreen() )
            paths.insert( path.Path() );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    const KIID_PATH shallow = *paths.begin();
    const KIID_PATH deep = *paths.rbegin();
    BOOST_REQUIRE( shallow.size() < deep.size() );
    BOOST_REQUIRE( deep.AsString() < shallow.AsString() );
    SETTINGS_MANAGER           sourceSettings;
    std::unique_ptr<SCHEMATIC> source;
    KI_TEST::LoadSchematic( sourceSettings, "similar_labels", source );
    SCH_LABEL_BASE* localSource = nullptr;
    SCH_LABEL_BASE* globalSource = nullptr;

    for( SCH_ITEM* item : source->RootScreen()->Items() )
    {
        if( item->Type() == SCH_LABEL_T && ( !localSource || item->m_Uuid < localSource->m_Uuid ) )
            localSource = static_cast<SCH_LABEL_BASE*>( item );
        else if( item->Type() == SCH_GLOBAL_LABEL_T && ( !globalSource || item->m_Uuid < globalSource->m_Uuid ) )
            globalSource = static_cast<SCH_LABEL_BASE*>( item );
    }

    BOOST_REQUIRE( localSource );
    BOOST_REQUIRE( globalSource );
    auto* local = static_cast<SCH_LABEL_BASE*>( localSource->Duplicate( false ) );
    auto* global = static_cast<SCH_LABEL_BASE*>( globalSource->Duplicate( false ) );
    auto* lower = static_cast<SCH_LABEL_BASE*>( globalSource->Duplicate( false ) );
    local->SetText( wxS( "ERC_DEPTH_ORDER" ) );
    global->SetText( wxS( "ERC_DEPTH_ORDER" ) );
    lower->SetText( wxS( "erc_depth_order" ) );
    lower->Move( globalSource->GetPosition() );
    BOOST_REQUIRE( lower->GetPosition() != global->GetPosition() );
    target->GetScreen()->Append( local );
    target->GetScreen()->Append( global );
    target->GetScreen()->Append( lower );
    const std::set<KIID> labels{ local->m_Uuid, global->m_Uuid, lower->m_Uuid };

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine " << backend )
        {
            enabled = backend;
            schematic->RebuildConnectivity();
            SCH_SCREENS screens( schematic->Root() );
            screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );
            ERC_TESTER tester( schematic.get() );
            tester.TestSameLocalGlobalLabel();
            size_t same = 0;

            for( SCH_ITEM* item : target->GetScreen()->Items().OfType( SCH_MARKER_T ) )
            {
                const auto error = std::static_pointer_cast<ERC_ITEM>( static_cast<SCH_MARKER*>( item )->GetRCItem() );

                if( error->GetMainItemID() == global->m_Uuid && error->GetAuxItemID() == local->m_Uuid )
                {
                    BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == shallow );
                    BOOST_CHECK( error->GetAuxItemSheetPath().PathRef() == shallow );
                    ++same;
                }
            }

            BOOST_CHECK_EQUAL( same, 1u );
            screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );
            tester.TestSimilarLabels();
            size_t crossSheet = 0;
            size_t sameSheet = 0;

            for( SCH_ITEM* item : target->GetScreen()->Items().OfType( SCH_MARKER_T ) )
            {
                const auto error = std::static_pointer_cast<ERC_ITEM>( static_cast<SCH_MARKER*>( item )->GetRCItem() );

                if( !labels.contains( error->GetMainItemID() ) || !labels.contains( error->GetAuxItemID() ) )
                    continue;

                BOOST_CHECK_EQUAL( error->GetErrorCode(), ERCE_SIMILAR_LABELS );
                const auto& main = error->GetMainItemSheetPath().PathRef();
                const auto& aux = error->GetAuxItemSheetPath().PathRef();

                // Saved exclusions depend on the main item following path text order
                if( main != aux )
                {
                    BOOST_CHECK( main == deep );
                    BOOST_CHECK( aux == shallow );
                    ++crossSheet;
                }
                else
                {
                    ++sameSheet;
                }
            }

            BOOST_CHECK_EQUAL( crossSheet, 4u );
            BOOST_CHECK_EQUAL( sameSheet, 4u );
            screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );
        }
    }
}
