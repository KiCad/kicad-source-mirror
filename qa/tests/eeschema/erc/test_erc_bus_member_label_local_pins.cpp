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

#include <connection_graph.h>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_BUS_MEMBER_LABEL_TEST_FIXTURE
{
    ERC_BUS_MEMBER_LABEL_TEST_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCBusMemberLabelLocalPins, ERC_BUS_MEMBER_LABEL_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Test case for https://gitlab.com/kicad/code/kicad/-/issues/19646
    //
    // When a hierarchical bus connects sheets, local labels that are bus members may only connect
    // to pins through the hierarchy (in other sheets). Previously, ERC counted pins from all sheets
    // connected through the hierarchy, which masked labels that lacked local connections.
    //
    // The fix tracks local pin count separately for bus member labels. If a bus member label has no
    // local pin connections, it is flagged as unconnected even if the net has pins in other sheets
    // through the hierarchical bus.
    //
    // Test data has 5 bus member labels (RES.EXC, RES.PWM, RES.SIN, RES.COS, RES.EN) in the MCU
    // sheet that only connect to pins through the hierarchical bus to the Resolver sheet. All 5
    // should be flagged as unconnected.

    KI_TEST::LoadSchematic( m_settingsManager, "issue19646/issue19646", m_schematic );

    ERC_SETTINGS&                settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

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

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    // Count ERCE_LABEL_NOT_CONNECTED errors specifically
    int labelNotConnectedCount = 0;

    for( int i = 0; i < errors.GetCount(); i++ )
    {
        std::shared_ptr<ERC_ITEM> ercItem =
                std::static_pointer_cast<ERC_ITEM>( errors.GetItem( i ) );

        if( ercItem->GetErrorCode() == ERCE_LABEL_NOT_CONNECTED )
            labelNotConnectedCount++;
    }

    BOOST_CHECK_MESSAGE( labelNotConnectedCount == 5,
                         "Expected 5 ERCE_LABEL_NOT_CONNECTED errors for bus member labels "
                         "without local pin connections, but got " << labelNotConnectedCount
                         << "\n" << reportWriter.GetTextReport() );
}
