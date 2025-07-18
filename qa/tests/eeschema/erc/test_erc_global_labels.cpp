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

struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCGlobalLabels, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using global labels
    std::vector<std::pair<wxString, int>> tests =
    {
        { "issue9367",                   13  }
    };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS& settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Skip the unconnected wire endpoints warning
        settings.m_ERCSeverities[ERCE_UNCONNECTED_WIRE_ENDPOINT] = RPT_SEVERITY_IGNORE;

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
        m_schematic->ConnectionGraph()->Recalculate( sheets, true );
        m_schematic->ConnectionGraph()->RunERC();

        ERC_TESTER tester( m_schematic.get() );
        tester.TestConflictingBusAliases();
        tester.TestMultUnitPinConflicts();
        tester.TestMultiunitFootprints();
        tester.TestNoConnectPins();
        tester.TestPinToPin();
        tester.TestSimilarLabels();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );

    }
}


BOOST_FIXTURE_TEST_CASE( ERCSingleGlobalLabels, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using global labels
    std::vector<std::pair<wxString, int>> tests = { { "issue13212", 3 },
                                                    { "erc_label_test", 7 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_LABEL_SINGLE_PIN] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_SINGLE_GLOBAL_LABEL] = RPT_SEVERITY_ERROR;

        // Silence dangling wire errors
        settings.m_ERCSeverities[ERCE_UNCONNECTED_WIRE_ENDPOINT] = RPT_SEVERITY_IGNORE;

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
        m_schematic->ConnectionGraph()->Recalculate( sheets, true );
        m_schematic->ConnectionGraph()->RunERC();

        ERC_TESTER tester( m_schematic.get() );
        tester.TestConflictingBusAliases();
        tester.TestMultUnitPinConflicts();
        tester.TestMultiunitFootprints();
        tester.TestNoConnectPins();
        tester.TestPinToPin();
        tester.TestSimilarLabels();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );

    }
}
