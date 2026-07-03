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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <schematic.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_FOOTPRINT_FILTERS_FIXTURE
{
    ERC_FOOTPRINT_FILTERS_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24037
//
// TestFootprintFilters() previously created markers with the
// ERCE_FOOTPRINT_LINK_ISSUES error code instead of ERCE_FOOTPRINT_FILTERS.
// As a consequence, ignoring the "Assigned footprint doesn't match footprint
// filters" violation in the ERC settings had no effect: the filter check
// kept producing markers tagged as link issues, leaving a red error marker
// on the canvas even though the violations counter showed zero.
BOOST_FIXTURE_TEST_CASE( ERCFootprintFiltersUsesCorrectErrorCode,
                         ERC_FOOTPRINT_FILTERS_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue24037/issue24037", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    settings.m_ERCSeverities[ERCE_FOOTPRINT_FILTERS]    = RPT_SEVERITY_WARNING;
    settings.m_ERCSeverities[ERCE_FOOTPRINT_LINK_ISSUES] = RPT_SEVERITY_WARNING;

    ERC_TESTER tester( m_schematic.get() );
    tester.TestFootprintFilters();

    SHEETLIST_ERC_ITEMS_PROVIDER provider( m_schematic.get() );
    provider.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    BOOST_REQUIRE_EQUAL( provider.GetCount(), 1 );

    std::shared_ptr<ERC_ITEM> item = provider.GetERCItem( 0 );
    BOOST_CHECK_EQUAL( item->GetErrorCode(), ERCE_FOOTPRINT_FILTERS );

    // With ERCE_FOOTPRINT_FILTERS set to IGNORE, the marker raised by
    // TestFootprintFilters() must be filtered out of the visible-error set
    // (and counted under the IGNORE bucket instead). The bug was that the
    // marker was tagged ERCE_FOOTPRINT_LINK_ISSUES, so changing the
    // ERCE_FOOTPRINT_FILTERS severity had no effect on it.
    SCH_SCREENS screens( m_schematic->Root() );
    screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );

    settings.m_ERCSeverities[ERCE_FOOTPRINT_FILTERS] = RPT_SEVERITY_IGNORE;

    tester.TestFootprintFilters();

    SHEETLIST_ERC_ITEMS_PROVIDER visible( m_schematic.get() );
    visible.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );
    BOOST_CHECK_EQUAL( visible.GetCount(), 0 );

    SHEETLIST_ERC_ITEMS_PROVIDER ignored( m_schematic.get() );
    ignored.SetSeverities( RPT_SEVERITY_IGNORE );
    BOOST_CHECK_EQUAL( ignored.GetCount(), 1 );

    // The marker for an IGNORE-severity violation must not request any render
    // layers, otherwise it repaints as a red error even though it is ignored.
    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
            BOOST_CHECK( static_cast<SCH_MARKER*>( item )->ViewGetLayers().empty() );
    }
}
