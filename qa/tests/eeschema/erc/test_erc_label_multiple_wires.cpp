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

#include <connection_graph.h>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <advanced_config.h>
#include <sch_marker.h>
#include <sch_screen.h>

#include <algorithm>
#include <functional>
#include <scoped_set_reset.h>

struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCLabelMultipleWires, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when labels overlap multiple wires/buses
    std::vector<std::pair<wxString, int>> tests =
    {
        { "issue18346",                   1  }
    };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS& settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        settings.m_ERCSeverities[ERCE_LABEL_MULTIPLE_WIRES] = RPT_SEVERITY_ERROR;

        ERC_TESTER tester( m_schematic.get() );
        tester.TestLabelMultipleWires();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );

    }
}


BOOST_AUTO_TEST_CASE( ERCMultipleWireExclusionsSurviveCanonicalWitnesses )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue18346", schematic );
    SCH_SCREEN* screen = schematic->RootScreen();
    ERC_TESTER tester( schematic.get() );
    BOOST_REQUIRE_EQUAL( tester.TestLabelMultipleWires(), 1 );
    std::vector<SCH_ITEM*> markers;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
    {
        auto* marker = static_cast<SCH_MARKER*>( item );
        auto ids = marker->GetRCItem()->GetIDs();
        BOOST_REQUIRE_GE( ids.size(), 3 );
        std::sort( ids.begin() + 1, ids.end(), std::greater<KIID>() );
        marker->GetRCItem()->SetItems( ids );
        marker->SetExcluded( true, "Retained crossing" );
        markers.push_back( item );
    }

    schematic->RecordERCExclusions();

    for( SCH_ITEM* item : markers )
        screen->DeleteItem( item );

    enabled = true;
    schematic->RebuildConnectivity();
    BOOST_REQUIRE_EQUAL( tester.TestLabelMultipleWires(), 1 );
    schematic->ResolveERCExclusionsPostUpdate();
    size_t count = 0;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
    {
        const auto* marker = static_cast<SCH_MARKER*>( item );
        BOOST_CHECK( marker->IsExcluded() );
        BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained crossing" ) );
        ++count;
    }

    BOOST_CHECK_EQUAL( count, 1 );
}
