/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <sch_rule_area.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE() : m_settingsManager( true /* headless */ ) {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCRuleAreaNetClasseDirectives, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using rule area netclass directives
    std::vector<std::pair<wxString, int>> tests = { { "RuleAreaOneNetclassDirective", 0 },
                                                    { "RuleAreaTwoNetclassDirectives", 1 },
                                                    { "RuleAreaThreeNetclassDirectives", 2 },
                                                    { "RuleAreaNetclassConflictOnWire_1", 1 },
                                                    { "RuleAreaNetclassConflictOnWire_2", 1 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_NETCLASS_CONFLICT] = RPT_SEVERITY_ERROR;

        std::unordered_set<SCH_SCREEN*> allScreens;
        SCH_SCREENS                     screens( m_schematic->Root() );
        screens.BuildClientSheetPathList();

        for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr;
             screen = screens.GetNext() )
            allScreens.insert( screen );

        SCH_RULE_AREA::UpdateRuleAreasInScreens( allScreens, nullptr );

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
        m_schematic->ConnectionGraph()->Recalculate( sheets, true );
        m_schematic->ConnectionGraph()->RunERC();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MILLIMETRES );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );
    }
}


BOOST_FIXTURE_TEST_CASE( ERCRuleAreaOverlaps, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using rule area netclass directives
    std::vector<std::pair<wxString, int>> tests = { { "RuleAreaNoOverlap", 0 },
                                                    { "RuleAreaOneOverlap", 1 },
                                                    { "RuleAreaOneOverlapTwice", 2 },
                                                    { "RuleAreaTwoOverlaps", 2 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_OVERLAPPING_RULE_AREAS] = RPT_SEVERITY_ERROR;

        std::unordered_set<SCH_SCREEN*> allScreens;
        SCH_SCREENS                     screens( m_schematic->Root() );
        screens.BuildClientSheetPathList();

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
            allScreens.insert( screen );

        SCH_RULE_AREA::UpdateRuleAreasInScreens( allScreens, nullptr );

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
        m_schematic->ConnectionGraph()->Recalculate( sheets, true );

        ERC_TESTER tester( m_schematic.get() );
        tester.RunRuleAreaERC();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MILLIMETRES );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );
    }
}
