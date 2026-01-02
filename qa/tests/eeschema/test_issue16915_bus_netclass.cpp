/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue16915_bus_netclass.cpp
 *
 * Tests for issue 16915: Bus label fails to match Netclass assignment pattern.
 *
 * When a netclass pattern like "/IN[0..7]" is defined, it should match the individual
 * bus member nets (IN0, IN1, ..., IN7) and assign them the corresponding netclass.
 * This affects both the coloring of wires in the schematic and the netclass assignment
 * used for DRC and other operations.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/eeschema_test_utils.h>

#include <project/net_settings.h>
#include <project/project_file.h>


class TEST_ISSUE16915_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{};


BOOST_FIXTURE_TEST_SUITE( Issue16915BusNetclass, TEST_ISSUE16915_FIXTURE )


/**
 * Test that ForEachBusMember correctly expands bus patterns.
 */
BOOST_AUTO_TEST_CASE( ForEachBusMemberExpansion )
{
    std::vector<wxString> collected;

    // Test vector bus expansion
    collected.clear();
    NET_SETTINGS::ForEachBusMember( wxS( "DATA[0..3]" ),
                                    [&]( const wxString& member )
                                    {
                                        collected.push_back( member );
                                    } );

    std::vector<wxString> expected = { wxS( "DATA0" ), wxS( "DATA1" ), wxS( "DATA2" ),
                                       wxS( "DATA3" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( collected.begin(), collected.end(), expected.begin(),
                                   expected.end() );

    // Test bus group with nested vector buses
    collected.clear();
    NET_SETTINGS::ForEachBusMember( wxS( "BUS{A[0..1] B[2..3]}" ),
                                    [&]( const wxString& member )
                                    {
                                        collected.push_back( member );
                                    } );

    expected = { wxS( "A0" ), wxS( "A1" ), wxS( "B2" ), wxS( "B3" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( collected.begin(), collected.end(), expected.begin(),
                                   expected.end() );

    // Test non-bus pattern (should return as-is)
    collected.clear();
    NET_SETTINGS::ForEachBusMember( wxS( "SIMPLE_NET" ),
                                    [&]( const wxString& member )
                                    {
                                        collected.push_back( member );
                                    } );

    expected = { wxS( "SIMPLE_NET" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( collected.begin(), collected.end(), expected.begin(),
                                   expected.end() );
}


/**
 * Test that ParseBusVector correctly expands bus notation patterns.
 */
BOOST_AUTO_TEST_CASE( ParseBusVectorExpansion )
{
    wxString              name;
    std::vector<wxString> members;

    // Test basic vector bus pattern
    BOOST_CHECK( NET_SETTINGS::ParseBusVector( wxS( "IN[0..7]" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxS( "IN" ) );
    BOOST_CHECK_EQUAL( members.size(), 8 );

    std::vector<wxString> expected = { wxS( "IN0" ), wxS( "IN1" ), wxS( "IN2" ), wxS( "IN3" ),
                                       wxS( "IN4" ), wxS( "IN5" ), wxS( "IN6" ), wxS( "IN7" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );

    // Test with path prefix
    members.clear();
    BOOST_CHECK( NET_SETTINGS::ParseBusVector( wxS( "/IN[0..7]" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxS( "/IN" ) );
    BOOST_CHECK_EQUAL( members.size(), 8 );
}


/**
 * Test that netclass patterns with bus notation match individual bus member nets.
 *
 * This is the core test for issue 16915: patterns like "/IN[0..7]" should match
 * nets named "/IN0", "/IN1", etc.
 */
BOOST_AUTO_TEST_CASE( BusPatternMatchesBusMembers )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // The project has patterns:
    //   "/IN[0..7]" -> "Input" netclass
    //   "/OUT[7..0]" -> "Output" netclass

    // Test that individual bus member nets get the correct netclass
    // Input bus members should get "Input" netclass
    for( int i = 0; i <= 7; i++ )
    {
        wxString netName = wxString::Format( "/IN%d", i );
        std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( netName );

        BOOST_TEST_INFO( "Checking netclass for " << netName );
        BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );
    }

    // Output bus members should get "Output" netclass
    for( int i = 0; i <= 7; i++ )
    {
        wxString netName = wxString::Format( "/OUT%d", i );
        std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( netName );

        BOOST_TEST_INFO( "Checking netclass for " << netName );
        BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Output" ) );
    }
}


/**
 * Test that netclass schematic colors are correctly applied to bus members.
 *
 * This tests the secondary aspect of issue 16915: bus member wires should be
 * colored according to their netclass.
 */
BOOST_AUTO_TEST_CASE( BusMemberNetclassHasCorrectColor )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // Check that the Input netclass has the expected color (magenta: rgb(255, 0, 255))
    std::shared_ptr<NETCLASS> inputNc = netSettings->GetEffectiveNetClass( "/IN0" );
    KIGFX::COLOR4D inputColor = inputNc->GetSchematicColor();

    // The project defines Input netclass with schematic_color "rgb(255, 0, 255)"
    BOOST_CHECK_EQUAL( inputColor.r, 1.0 );
    BOOST_CHECK_EQUAL( inputColor.g, 0.0 );
    BOOST_CHECK_EQUAL( inputColor.b, 1.0 );

    // Check that the Output netclass has the expected color (orange: rgb(255, 153, 0))
    std::shared_ptr<NETCLASS> outputNc = netSettings->GetEffectiveNetClass( "/OUT0" );
    KIGFX::COLOR4D outputColor = outputNc->GetSchematicColor();

    // The project defines Output netclass with schematic_color "rgb(255, 153, 0)"
    BOOST_CHECK_EQUAL( outputColor.r, 1.0 );
    BOOST_CHECK_CLOSE( outputColor.g, 0.6, 1.0 );  // 153/255 ≈ 0.6
    BOOST_CHECK_EQUAL( outputColor.b, 0.0 );
}


/**
 * Test that bus wires inherit netclass from their members when all members share the same netclass.
 *
 * This is the core visual fix for issue 16915: when looking up the netclass for a bus
 * (e.g., "/IN[0..7]"), if all its member nets share the same netclass, the bus should
 * inherit that netclass for coloring purposes.
 */
BOOST_AUTO_TEST_CASE( BusWireInheritsNetclassFromMembers )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // The bus "/IN[0..7]" should inherit the "Input" netclass since all its members
    // (/IN0, /IN1, ..., /IN7) have the "Input" netclass assigned via the pattern.
    std::shared_ptr<NETCLASS> busNc = netSettings->GetEffectiveNetClass( "/IN[0..7]" );
    BOOST_CHECK_EQUAL( busNc->GetName(), wxS( "Input" ) );

    // Verify the bus has the correct color (magenta)
    KIGFX::COLOR4D busColor = busNc->GetSchematicColor();
    BOOST_CHECK_EQUAL( busColor.r, 1.0 );
    BOOST_CHECK_EQUAL( busColor.g, 0.0 );
    BOOST_CHECK_EQUAL( busColor.b, 1.0 );

    // Similarly, "/OUT[7..0]" should inherit the "Output" netclass
    busNc = netSettings->GetEffectiveNetClass( "/OUT[7..0]" );
    BOOST_CHECK_EQUAL( busNc->GetName(), wxS( "Output" ) );

    // Verify the bus has the correct color (orange)
    busColor = busNc->GetSchematicColor();
    BOOST_CHECK_EQUAL( busColor.r, 1.0 );
    BOOST_CHECK_CLOSE( busColor.g, 0.6, 1.0 );  // 153/255 ≈ 0.6
    BOOST_CHECK_EQUAL( busColor.b, 0.0 );
}


/**
 * Test that bus group patterns are correctly expanded.
 *
 * For patterns like "/PCI{North[0..2] South[3..5]}", the group should be expanded
 * and each member vector should be further expanded to individual nets.
 */
BOOST_AUTO_TEST_CASE( BusGroupPatternExpansion )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // Add a bus group pattern - members are vector buses that need expansion
    netSettings->SetNetclassPatternAssignment( wxS( "PCI{North[0..2] South[3..5]}" ),
                                                wxS( "Input" ) );

    // Check that individual member nets from the expanded group get the netclass
    // North[0..2] should expand to North0, North1, North2
    for( int i = 0; i <= 2; i++ )
    {
        wxString netName = wxString::Format( "North%d", i );
        std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( netName );

        BOOST_TEST_INFO( "Checking netclass for " << netName );
        BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );
    }

    // South[3..5] should expand to South3, South4, South5
    for( int i = 3; i <= 5; i++ )
    {
        wxString netName = wxString::Format( "South%d", i );
        std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( netName );

        BOOST_TEST_INFO( "Checking netclass for " << netName );
        BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );
    }
}


/**
 * Test that bus with mixed netclass members falls back to default.
 *
 * When bus members have different netclasses, the bus should use the default netclass.
 */
BOOST_AUTO_TEST_CASE( BusWithMixedNetclassesFallsBackToDefault )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // Assign only some members of a test bus to a netclass
    netSettings->SetNetclassPatternAssignment( wxS( "MIXED0" ), wxS( "Input" ) );
    netSettings->SetNetclassPatternAssignment( wxS( "MIXED1" ), wxS( "Output" ) );

    // The bus "MIXED[0..1]" has members with different netclasses, so it should
    // fall back to the default netclass
    std::shared_ptr<NETCLASS> busNc = netSettings->GetEffectiveNetClass( "MIXED[0..1]" );
    BOOST_CHECK_EQUAL( busNc->GetName(), wxS( "Default" ) );
}


/**
 * Test that non-bus patterns still work correctly.
 *
 * Regression test to ensure bus pattern expansion doesn't break regular patterns.
 */
BOOST_AUTO_TEST_CASE( NonBusPatternsStillWork )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // Add a simple wildcard pattern for testing
    netSettings->SetNetclassPatternAssignment( wxS( "/TEST*" ), wxS( "Input" ) );

    // The wildcard pattern should still work
    std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( "/TEST_NET" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );

    nc = netSettings->GetEffectiveNetClass( "/TESTXYZ" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );
}


/**
 * Test that ClearCacheForNet properly invalidates cached netclass lookups.
 *
 * This is related to issue 17891: when a net name changes, the cache for both
 * old and new net names must be cleared so that netclass assignments are
 * correctly updated.
 */
BOOST_AUTO_TEST_CASE( NetclassCacheInvalidation )
{
    LoadSchematic( SchematicQAPath( "issue16915" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    // Initial lookup - should get "Input" netclass from the pattern "/IN[0..7]"
    std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( "/IN0" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );

    // Verify the lookup is cached
    BOOST_CHECK( netSettings->HasEffectiveNetClass( "/IN0" ) );

    // Clear the cache for this specific net
    netSettings->ClearCacheForNet( "/IN0" );

    // Cache should be cleared
    BOOST_CHECK( !netSettings->HasEffectiveNetClass( "/IN0" ) );

    // Looking up again should still return "Input" (pattern still matches)
    nc = netSettings->GetEffectiveNetClass( "/IN0" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );

    // Now add a pattern that matches a renamed net
    // Simulate: net was "/OLD_NET", now it's "/NEW_NET"
    netSettings->SetNetclassPatternAssignment( wxS( "/OLD_NET" ), wxS( "Input" ) );
    netSettings->SetNetclassPatternAssignment( wxS( "/NEW_NET" ), wxS( "Output" ) );

    // Look up both - they should get different netclasses
    nc = netSettings->GetEffectiveNetClass( "/OLD_NET" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );

    nc = netSettings->GetEffectiveNetClass( "/NEW_NET" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Output" ) );

    // Clear all caches (simulating what happens on major connectivity changes)
    netSettings->ClearAllCaches();

    // Both should be recalculated correctly
    nc = netSettings->GetEffectiveNetClass( "/OLD_NET" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Input" ) );

    nc = netSettings->GetEffectiveNetClass( "/NEW_NET" );
    BOOST_CHECK_EQUAL( nc->GetName(), wxS( "Output" ) );
}


BOOST_AUTO_TEST_SUITE_END()
