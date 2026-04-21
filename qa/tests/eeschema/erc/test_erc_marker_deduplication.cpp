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

#include <base_units.h>
#include <connection_graph.h>
#include <schematic.h>
#include <sch_marker.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <erc/erc_item.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_MARKER_DEDUP_FIXTURE
{
    ERC_MARKER_DEDUP_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Regression test for issue 23672. SHEETLIST_ERC_ITEMS_PROVIDER::visitMarkers
// collects markers into a std::set keyed by marker position. The position
// comparator formerly fell through to VECTOR2::operator<, which compared by
// squared magnitude. Two markers at mirrored coordinates (a, b) and (b, a)
// have the same squared magnitude and were therefore treated as duplicates,
// silently dropping one marker. The multinetclasses schematic triggers this
// exact collision: an isolated_pin_label at (121.92, 77.47) and a
// pin_not_connected at (77.47, 121.92) share squared magnitude 21394.9153.
BOOST_FIXTURE_TEST_CASE( ERCMarkerDeduplicationMultinetclasses, ERC_MARKER_DEDUP_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, wxS( "netlists/multinetclasses/multinetclasses" ),
                            m_schematic );

    ERC_SETTINGS&                settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestMultUnitPinConflicts();
    tester.TestMultiunitFootprints();
    tester.TestNoConnectPins();
    tester.TestPinToPin();
    tester.TestSimilarLabels();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    // The mirrored positions that triggered the squared-magnitude collision.
    const VECTOR2I expectedPinNotConnected( schIUScale.mmToIU( 77.47 ),
                                            schIUScale.mmToIU( 121.92 ) );
    const VECTOR2I expectedIsolatedLabel( schIUScale.mmToIU( 121.92 ),
                                          schIUScale.mmToIU( 77.47 ) );

    int  pinNotConnected = 0;
    int  isolatedPinLabel = 0;
    bool sawExpectedPinNotConnected = false;
    bool sawExpectedIsolatedLabel = false;

    for( int ii = 0; ii < errors.GetCount(); ++ii )
    {
        std::shared_ptr<RC_ITEM> item = errors.GetItem( ii );
        SCH_MARKER* marker = static_cast<SCH_MARKER*>( item->GetParent() );
        int errorCode = item->GetErrorCode();

        BOOST_REQUIRE( marker );

        if( errorCode == ERCE_PIN_NOT_CONNECTED )
        {
            pinNotConnected++;
            sawExpectedPinNotConnected |= marker->GetPosition() == expectedPinNotConnected;
        }
        else if( errorCode == ERCE_LABEL_SINGLE_PIN )
        {
            isolatedPinLabel++;
            sawExpectedIsolatedLabel |= marker->GetPosition() == expectedIsolatedLabel;
        }
    }

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    // Before the fix, one pin_not_connected marker (R7 Pad1) was dropped
    // because its position collided with an isolated_pin_label marker.
    BOOST_CHECK_MESSAGE( pinNotConnected == 12,
                         "Expected 12 pin_not_connected violations but got " << pinNotConnected
                                                                             << "\n"
                                                                             << reportWriter.GetTextReport() );

    BOOST_CHECK_MESSAGE( isolatedPinLabel >= 1,
                         "Expected at least 1 isolated_pin_label violation but got "
                                 << isolatedPinLabel << "\n"
                                 << reportWriter.GetTextReport() );

    // The two specific markers from the bug report must both be present.
    BOOST_CHECK_MESSAGE( sawExpectedPinNotConnected,
                         "Expected pin_not_connected marker at (77.47, 121.92) mm was missing\n"
                                 << reportWriter.GetTextReport() );

    BOOST_CHECK_MESSAGE( sawExpectedIsolatedLabel,
                         "Expected isolated_pin_label marker at (121.92, 77.47) mm was missing\n"
                                 << reportWriter.GetTextReport() );
}
