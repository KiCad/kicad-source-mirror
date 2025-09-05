/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <netlist_reader/pcb_netlist.h>
#include <lib_id.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <wx/log.h>
#include <qa_utils/utility_registry.h>
#include <reporter.h>

BOOST_AUTO_TEST_SUITE( StackedPinNetlist )


/**
 * Test that COMPONENT::GetNet properly handles stacked pin notation like [8,9,10]
 * and finds individual pin numbers within the stacked group.
 */
BOOST_AUTO_TEST_CASE( TestStackedPinNetMatch )
{
    // Create a component with a stacked pin notation
    LIB_ID fpid( wxT( "TestLib" ), wxT( "TestFootprint" ) );
    wxString reference = wxT( "U1" );
    wxString value = wxT( "TestIC" );
    KIID_PATH path;
    std::vector<KIID> kiids;

    COMPONENT component( fpid, reference, value, path, kiids );

    // Add a net with stacked pin notation [8,9,10]
    component.AddNet( wxT( "[8,9,10]" ), wxT( "DATA_BUS" ), wxT( "bidirectional" ), wxT( "bidirectional" ) );

    // Test that individual pins within the stack are found
    const COMPONENT_NET& net8 = component.GetNet( wxT( "8" ) );
    const COMPONENT_NET& net9 = component.GetNet( wxT( "9" ) );
    const COMPONENT_NET& net10 = component.GetNet( wxT( "10" ) );

    BOOST_CHECK( net8.IsValid() );
    BOOST_CHECK( net9.IsValid() );
    BOOST_CHECK( net10.IsValid() );

    BOOST_CHECK_EQUAL( net8.GetNetName(), wxString( "DATA_BUS" ) );
    BOOST_CHECK_EQUAL( net9.GetNetName(), wxString( "DATA_BUS" ) );
    BOOST_CHECK_EQUAL( net10.GetNetName(), wxString( "DATA_BUS" ) );

    // Test that pins outside the stack are not found
    const COMPONENT_NET& net7 = component.GetNet( wxT( "7" ) );
    const COMPONENT_NET& net11 = component.GetNet( wxT( "11" ) );

    BOOST_CHECK( !net7.IsValid() );
    BOOST_CHECK( !net11.IsValid() );
}


/**
 * Test stacked pin notation with range syntax [1-4]
 */
BOOST_AUTO_TEST_CASE( TestStackedPinRangeMatch )
{
    LIB_ID fpid( wxT( "TestLib" ), wxT( "TestFootprint" ) );
    wxString reference = wxT( "U2" );
    wxString value = wxT( "TestIC" );
    KIID_PATH path;
    std::vector<KIID> kiids;

    COMPONENT component( fpid, reference, value, path, kiids );

    // Add a net with range notation [1-4]
    component.AddNet( wxT( "[1-4]" ), wxT( "POWER_BUS" ), wxT( "power_in" ), wxT( "power_in" ) );

    // Test that all pins in the range are found
    for( int i = 1; i <= 4; i++ )
    {
        const COMPONENT_NET& net = component.GetNet( wxString::Format( wxT( "%d" ), i ) );
        BOOST_CHECK( net.IsValid() );
        BOOST_CHECK_EQUAL( net.GetNetName(), wxString( "POWER_BUS" ) );
    }

    // Test pins outside the range
    const COMPONENT_NET& net0 = component.GetNet( wxT( "0" ) );
    const COMPONENT_NET& net5 = component.GetNet( wxT( "5" ) );

    BOOST_CHECK( !net0.IsValid() );
    BOOST_CHECK( !net5.IsValid() );
}


/**
 * Test mixed notation [1,3,5-7]
 */
BOOST_AUTO_TEST_CASE( TestStackedPinMixedMatch )
{
    LIB_ID fpid( wxT( "TestLib" ), wxT( "TestFootprint" ) );
    wxString reference = wxT( "U3" );
    wxString value = wxT( "TestIC" );
    KIID_PATH path;
    std::vector<KIID> kiids;

    COMPONENT component( fpid, reference, value, path, kiids );

    // Add a net with mixed notation [1,3,5-7]
    component.AddNet( wxT( "[1,3,5-7]" ), wxT( "CONTROL_BUS" ), wxT( "output" ), wxT( "output" ) );

    // Test individual pins and ranges
    std::vector<int> expectedPins = { 1, 3, 5, 6, 7 };
    for( int pin : expectedPins )
    {
        const COMPONENT_NET& net = component.GetNet( wxString::Format( wxT( "%d" ), pin ) );
        BOOST_CHECK( net.IsValid() );
        BOOST_CHECK_EQUAL( net.GetNetName(), wxString( "CONTROL_BUS" ) );
    }

    // Test pins that should not be found
    std::vector<int> unexpectedPins = { 2, 4, 8 };
    for( int pin : unexpectedPins )
    {
        const COMPONENT_NET& net = component.GetNet( wxString::Format( wxT( "%d" ), pin ) );
        BOOST_CHECK( !net.IsValid() );
    }
}


/**
 * Test that regular (non-stacked) pin names still work
 */
BOOST_AUTO_TEST_CASE( TestRegularPinMatch )
{
    LIB_ID fpid( wxT( "TestLib" ), wxT( "TestFootprint" ) );
    wxString reference = wxT( "R1" );
    wxString value = wxT( "1k" );
    KIID_PATH path;
    std::vector<KIID> kiids;

    COMPONENT component( fpid, reference, value, path, kiids );

    // Add regular pins
    component.AddNet( wxT( "1" ), wxT( "VCC" ), wxT( "passive" ), wxT( "passive" ) );
    component.AddNet( wxT( "2" ), wxT( "GND" ), wxT( "passive" ), wxT( "passive" ) );

    const COMPONENT_NET& net1 = component.GetNet( wxT( "1" ) );
    const COMPONENT_NET& net2 = component.GetNet( wxT( "2" ) );

    BOOST_CHECK( net1.IsValid() );
    BOOST_CHECK( net2.IsValid() );
    BOOST_CHECK_EQUAL( net1.GetNetName(), wxString( "VCC" ) );
    BOOST_CHECK_EQUAL( net2.GetNetName(), wxString( "GND" ) );
}


/**
 * This test creates a mock netlist that matches the stacked project structure and
 * validates that PCB pad lookups work correctly with stacked pin notation.
 */
BOOST_AUTO_TEST_CASE( TestStackedProjectNetlistUpdate )
{
    BOOST_TEST_MESSAGE( "Testing stacked pin project netlist functionality" );

    // Create a component that matches the R1 component from the stacked project
    LIB_ID fpid( wxT( "Connector" ), wxT( "Tag-Connect_TC2050-IDC-FP_2x05_P1.27mm_Vertical" ) );
    wxString reference = wxT( "R1" );
    wxString value = wxT( "R" );
    KIID_PATH path;
    std::vector<KIID> kiids;

    COMPONENT component( fpid, reference, value, path, kiids );

    // Add nets matching the stacked project
    // The schematic has two stacked pin groups: [1-5] and [6,7,9-11]
    component.AddNet( wxT( "[1-5]" ), wxT( "Net-(R1-Pad1)" ), wxT( "passive" ), wxT( "passive" ) );
    component.AddNet( wxT( "[6,7,9-11]" ), wxT( "Net-(R1-Pad6)" ), wxT( "passive" ), wxT( "passive" ) );

    BOOST_TEST_MESSAGE( "Created R1 component with stacked pins [1-5] and [6,7,9-11]" );

    // Log all nets for the component
    BOOST_TEST_MESSAGE( "R1 component nets:" );
    for( unsigned i = 0; i < component.GetNetCount(); i++ )
    {
        const COMPONENT_NET& net = component.GetNet( i );
        BOOST_TEST_MESSAGE( "  Pin: " + net.GetPinName() + " -> Net: " + net.GetNetName() );
    }

    // Test individual pin lookups
    // Pins 1-5 should be found (they're in the [1-5] stacked group)
    for( int pin = 1; pin <= 5; pin++ )
    {
        wxString pinStr = wxString::Format( wxT( "%d" ), pin );
        const COMPONENT_NET& net = component.GetNet( pinStr );

        BOOST_CHECK_MESSAGE( net.IsValid(),
                            "Pin " + pinStr + " should be found in stacked group [1-5]" );

        if( net.IsValid() )
        {
            BOOST_CHECK_EQUAL( net.GetNetName(), wxString( "Net-(R1-Pad1)" ) );
            BOOST_TEST_MESSAGE( "Pin " + pinStr + " found with net: " + net.GetNetName() );
        }
        else
        {
            BOOST_TEST_MESSAGE( "Pin " + pinStr + " NOT found (should be in [1-5])" );
        }
    }

    // Pins 6,7,9,10,11 should be found (they're in the [6,7,9-11] stacked group)
    std::vector<int> groupTwoPins = { 6, 7, 9, 10, 11 };
    for( int pin : groupTwoPins )
    {
        wxString pinStr = wxString::Format( wxT( "%d" ), pin );
        const COMPONENT_NET& net = component.GetNet( pinStr );

        BOOST_CHECK_MESSAGE( net.IsValid(),
                            "Pin " + pinStr + " should be found in stacked group [6,7,9-11]" );

        if( net.IsValid() )
        {
            BOOST_CHECK_EQUAL( net.GetNetName(), wxString( "Net-(R1-Pad6)" ) );
            BOOST_TEST_MESSAGE( "Pin " + pinStr + " found with net: " + net.GetNetName() );
        }
        else
        {
            BOOST_TEST_MESSAGE( "Pin " + pinStr + " NOT found (should be in [6,7,9-11])" );
        }
    }

    // Pin 8 should NOT be found (it's not in either stacked group)
    const COMPONENT_NET& net8 = component.GetNet( wxT( "8" ) );
    BOOST_CHECK_MESSAGE( !net8.IsValid(),
                        "Pin 8 should NOT be found (not in any stacked group)" );

    if( net8.IsValid() )
    {
        BOOST_TEST_MESSAGE( "Pin 8 unexpectedly found with net: " + net8.GetNetName() );
    }
    else
    {
        BOOST_TEST_MESSAGE( "Pin 8 correctly NOT found (expected behavior)" );
    }
}


BOOST_AUTO_TEST_SUITE_END()
