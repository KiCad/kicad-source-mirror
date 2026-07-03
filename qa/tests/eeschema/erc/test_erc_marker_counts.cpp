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
#include <sch_marker.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_MARKER_COUNT_FIXTURE
{
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * SHEETLIST_ERC_ITEMS_PROVIDER caches per-severity counts. Excluding a marker moves it between
 * its error/warning bucket and the exclusion bucket, so the cached counts must follow the
 * transition and stay identical to a fresh full recompute.
 */
BOOST_FIXTURE_TEST_CASE( ERCMarkerCountsExclusion, ERC_MARKER_COUNT_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue10430", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();

    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestMultUnitPinConflicts();
    tester.TestMultiunitFootprints();
    tester.TestNoConnectPins();
    tester.TestPinToPin();
    tester.TestSimilarLabels();
    tester.TestTextVars( nullptr );

    const int allSeverities = RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING | RPT_SEVERITY_EXCLUSION;

    SHEETLIST_ERC_ITEMS_PROVIDER provider( m_schematic.get() );
    provider.SetSeverities( allSeverities );

    BOOST_REQUIRE( provider.GetCount() > 0 );

    const int errorsBefore = provider.GetCount( RPT_SEVERITY_ERROR );
    const int warningsBefore = provider.GetCount( RPT_SEVERITY_WARNING );
    const int exclusionsBefore = provider.GetCount( RPT_SEVERITY_EXCLUSION );

    // Pick a displayed, non-excluded marker to exclude.
    SCH_MARKER* marker = nullptr;
    SEVERITY    markerSeverity = RPT_SEVERITY_UNDEFINED;

    for( int i = 0; i < provider.GetCount(); ++i )
    {
        SCH_MARKER* candidate = static_cast<SCH_MARKER*>( provider.GetItem( i )->GetParent() );

        if( !candidate->IsExcluded() )
        {
            marker = candidate;
            markerSeverity = settings.GetSeverity( candidate->GetRCItem()->GetErrorCode() );
            break;
        }
    }

    BOOST_REQUIRE( marker != nullptr );
    BOOST_REQUIRE( markerSeverity == RPT_SEVERITY_ERROR || markerSeverity == RPT_SEVERITY_WARNING );

    provider.SetMarkerExcluded( marker, true );

    // The exclusion bucket gains one; the marker's original bucket loses one. Total is unchanged.
    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_EXCLUSION ), exclusionsBefore + 1 );

    if( markerSeverity == RPT_SEVERITY_ERROR )
    {
        BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_ERROR ), errorsBefore - 1 );
        BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_WARNING ), warningsBefore );
    }
    else
    {
        BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_WARNING ), warningsBefore - 1 );
        BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_ERROR ), errorsBefore );
    }

    // The incrementally maintained counts must match a fresh authoritative recompute.
    SHEETLIST_ERC_ITEMS_PROVIDER fresh( m_schematic.get() );
    fresh.SetSeverities( allSeverities );

    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_ERROR ), fresh.GetCount( RPT_SEVERITY_ERROR ) );
    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_WARNING ),
                       fresh.GetCount( RPT_SEVERITY_WARNING ) );
    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_EXCLUSION ),
                       fresh.GetCount( RPT_SEVERITY_EXCLUSION ) );

    // Restoring the marker must move it back and keep counts consistent with a recompute.
    provider.SetMarkerExcluded( marker, false );

    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_ERROR ), errorsBefore );
    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_WARNING ), warningsBefore );
    BOOST_CHECK_EQUAL( provider.GetCount( RPT_SEVERITY_EXCLUSION ), exclusionsBefore );
}
