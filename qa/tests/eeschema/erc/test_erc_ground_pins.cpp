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

/**
 * @file test_erc_ground_pins.cpp
 * Test suite for the ERC ground pin checks (ERCE_GROUND_PIN_NOT_GROUND)
 *
 * This test verifies that the ERC correctly detects and reports when pins labeled
 * with "GND" are not connected to a ground net while other pins in the same symbol
 * are connected to ground nets.
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


struct ERC_GROUND_PIN_TEST_FIXTURE
{
    ERC_GROUND_PIN_TEST_FIXTURE() :
            m_settingsManager()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test case: Pin with "GND" in its name connected to non-ground net while another pin
 * in the same symbol is connected to a ground net.
 * Expected: 1 ERCE_GROUND_PIN_NOT_GROUND error
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinMismatch, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Test schematic with a symbol that has:
    // - One pin (VCC) connected to +5V net
    // - One pin labeled "GND" connected to non-ground net "OTHER"
    // - A separate GND symbol connected to "GND" net (establishes ground reference)
    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_error", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Skip the symbol library warnings for this test
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Enable the ground pin test
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestGroundPins();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    BOOST_CHECK_MESSAGE( errors.GetCount() == 1,
                         "Expected 1 ERCE_GROUND_PIN_NOT_GROUND error but got "
                         << errors.GetCount() << "\n" << reportWriter.GetTextReport() );

    // Verify the error is the correct type
    bool foundGroundPinError = false;
    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            foundGroundPinError = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundGroundPinError,
                         "Expected to find ERCE_GROUND_PIN_NOT_GROUND error\n"
                         << reportWriter.GetTextReport() );
}


/**
 * Test case: Pin with "GND" in its name correctly connected to ground net.
 * Expected: 0 ERCE_GROUND_PIN_NOT_GROUND errors
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinCorrect, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Test schematic with a symbol where the GND pin is correctly connected to a ground net
    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_ok", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Skip the symbol library warnings
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Enable the ground pin test
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestGroundPins();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    // Should have no ground pin errors since the GND pin is correctly connected
    int groundPinErrors = 0;
    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            groundPinErrors++;
        }
    }

    BOOST_CHECK_MESSAGE( groundPinErrors == 0,
                         "Expected 0 ERCE_GROUND_PIN_NOT_GROUND errors but got "
                         << groundPinErrors << "\n" << reportWriter.GetTextReport() );
}


/**
 * Test case: Symbol with multiple ground pins, some connected correctly, some not.
 * Expected: 1 ERCE_GROUND_PIN_NOT_GROUND error (for the incorrectly connected pin)
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinMixed, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Test schematic with a symbol that has multiple ground-labeled pins:
    // - One GND pin correctly connected to ground net
    // - One GND_ALT pin connected to non-ground net
    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_mixed", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Skip the symbol library warnings
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Enable the ground pin test
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestGroundPins();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    // Should have exactly 1 ground pin error for the GND_ALT pin
    int groundPinErrors = 0;
    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            groundPinErrors++;
        }
    }

    BOOST_CHECK_MESSAGE( groundPinErrors == 1,
                         "Expected 1 ERCE_GROUND_PIN_NOT_GROUND error but got "
                         << groundPinErrors << "\n" << reportWriter.GetTextReport() );
}


/**
 * Test case: Symbol with ground-labeled pin but no ground net anywhere in the schematic.
 * Expected: 0 ERCE_GROUND_PIN_NOT_GROUND errors (condition not triggered)
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinNoGroundNet, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Test schematic with a symbol that has a GND pin but no ground net exists anywhere
    // The error should only trigger when there IS a ground net somewhere but the GND pin
    // is not connected to it
    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_no_ground_net", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Skip the symbol library warnings
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Enable the ground pin test
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestGroundPins();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    // Should have no ground pin errors since there's no ground net to compare against
    int groundPinErrors = 0;
    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            groundPinErrors++;
        }
    }

    BOOST_CHECK_MESSAGE( groundPinErrors == 0,
                         "Expected 0 ERCE_GROUND_PIN_NOT_GROUND errors but got "
                         << groundPinErrors << "\n" << reportWriter.GetTextReport() );
}


/**
 * Test case: Verify that the ground pin check is properly enabled/disabled.
 * Expected: Errors only when the check is enabled
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinToggle, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_error", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Skip the symbol library warnings
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    int groundPinErrors = 0;
    // Disable the ground pin test
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_IGNORE;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestGroundPins();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            groundPinErrors++;
        }
    }

    BOOST_CHECK_MESSAGE( groundPinErrors == 0,
                         "Expected 0 errors when test is disabled but got " << groundPinErrors );

    // Now re-enable the ground pin test and count again
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester2( m_schematic.get() );
    tester2.TestGroundPins();
    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    groundPinErrors = 0;

    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            groundPinErrors++;
        }
    }

    BOOST_CHECK_MESSAGE( groundPinErrors >= 1,
                         "Expected at least 1 error when test is enabled but got " << groundPinErrors );
}


/**
 * Test case: Verify error message content and pin identification.
 * Expected: Error message contains pin name and proper item identification
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinErrorMessage, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_error", m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Skip the symbol library warnings
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Enable the ground pin test
    settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestGroundPins();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    // Find the ground pin error and verify its content
    bool foundGroundPinError = false;
    wxString errorMessage;

    for( unsigned i = 0; i < errors.GetCount(); i++ )
    {
        if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
        {
            foundGroundPinError = true;
            errorMessage = errors.GetItem( i )->GetErrorMessage( false );

            // Verify the error message contains expected content
            BOOST_CHECK_MESSAGE( errorMessage.Contains( "Pin" ),
                                 "Error message should contain 'Pin': " << errorMessage.ToStdString() );

            BOOST_CHECK_MESSAGE( errorMessage.Contains( "GND" ),
                                 "Error message should contain 'GND': " << errorMessage.ToStdString() );

            BOOST_CHECK_MESSAGE( errorMessage.Contains( "not connected to ground net" ),
                                 "Error message should contain expected text: " << errorMessage.ToStdString() );

            // Verify that the error has the pin item associated
            std::shared_ptr<RC_ITEM> ercItem = errors.GetItem( i );
            BOOST_CHECK_MESSAGE( ercItem->GetMainItemID() != niluuid,
                                 "ERC item should have a main item (the pin)" );

            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundGroundPinError,
                         "Should have found a ground pin error to test message content" );
}


/**
 * Comprehensive test with all ground pin related test cases.
 * This demonstrates the expected behavior of the ground pin ERC check.
 */
BOOST_FIXTURE_TEST_CASE( ERCGroundPinComprehensive, ERC_GROUND_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Test data: schema name and expected number of ERCE_GROUND_PIN_NOT_GROUND errors
    std::vector<std::pair<wxString, int>> tests =
    {
        { "ground_pin_test_error", 1 },        // GND pin on non-ground net while symbol has ground
        { "ground_pin_test_ok", 0 },           // GND pin correctly connected
        { "ground_pin_test_mixed", 1 },        // Mixed: one correct, one incorrect
        { "ground_pin_test_no_ground_net", 0 } // No ground net anywhere (no error triggered)
    };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS& settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the symbol library warnings
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Enable the ground pin test
        settings.m_ERCSeverities[ERCE_GROUND_PIN_NOT_GROUND] = RPT_SEVERITY_ERROR;

        m_schematic->ConnectionGraph()->RunERC();

        ERC_TESTER tester( m_schematic.get() );

        // Run all ERC tests to ensure ground pin test integrates properly
        tester.TestMultUnitPinConflicts();
        tester.TestMultiunitFootprints();
        tester.TestMissingUnits();
        tester.TestNoConnectPins();
        tester.TestPinToPin();
        tester.TestGroundPins(); // Our test
        tester.TestSimilarLabels();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        // Count only ground pin errors
        int groundPinErrors = 0;
        for( unsigned i = 0; i < errors.GetCount(); i++ )
        {
            if( errors.GetItem( i )->GetErrorCode() == ERCE_GROUND_PIN_NOT_GROUND )
            {
                groundPinErrors++;
            }
        }

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( groundPinErrors == test.second,
                             "Expected " << test.second << " ERCE_GROUND_PIN_NOT_GROUND errors in "
                             << test.first.ToStdString() << " but got " << groundPinErrors
                             << "\n" << reportWriter.GetTextReport() );
    }
}
