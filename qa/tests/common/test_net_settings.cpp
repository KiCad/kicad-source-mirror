/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <boost/test/unit_test.hpp>

#include <project/net_settings.h>
#include <netclass.h>


BOOST_AUTO_TEST_SUITE( NetSettingsTests )


// Regression guard for the dirty-check used by the project save framework.
// Without m_netChainClasses included in operator==, edits to chain-class
// assignments returned "no change" and were silently dropped on close.
BOOST_AUTO_TEST_CASE( ChainClassAssignmentAffectsEquality )
{
    NET_SETTINGS a( nullptr, "" );
    NET_SETTINGS b( nullptr, "" );

    BOOST_CHECK( a == b );

    a.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "Default" ) );

    BOOST_CHECK( a != b );

    b.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "Default" ) );

    BOOST_CHECK( a == b );
}


// Verify that differing chain-class values (same chain key) compare unequal,
// and that clearing an assignment via empty class string restores equality.
BOOST_AUTO_TEST_CASE( ChainClassValueAndClearAffectEquality )
{
    NET_SETTINGS a( nullptr, "" );
    NET_SETTINGS b( nullptr, "" );

    a.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "Default" ) );
    b.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( a != b );

    b.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "Default" ) );

    BOOST_CHECK( a == b );

    a.SetNetChainClass( wxS( "CHAIN_A" ), wxString() );

    BOOST_CHECK( a != b );

    b.SetNetChainClass( wxS( "CHAIN_A" ), wxString() );

    BOOST_CHECK( a == b );
}


// Asymmetric-size guard for the std::equal calls in operator==. A 3-iterator
// std::equal returns true when the LHS is a prefix of the RHS, which silently
// dropped one-sided edits.
BOOST_AUTO_TEST_CASE( AssignmentSizeDifferencesAffectEquality )
{
    NET_SETTINGS a( nullptr, "" );
    NET_SETTINGS b( nullptr, "" );

    b.SetNetclassLabelAssignment( wxS( "NET_A" ), { wxS( "Default" ) } );

    BOOST_CHECK( a != b );
    BOOST_CHECK( b != a );
}


// Returns true if the effective netclass for aNetName resolves to a netclass named aExpected,
// either directly or as a constituent of a composite effective netclass.
static bool resolvesToNetclass( NET_SETTINGS& aSettings, const wxString& aNetName,
                                const wxString& aExpected )
{
    std::shared_ptr<NETCLASS> resolved = aSettings.GetEffectiveNetClass( aNetName );

    if( !resolved )
        return false;

    if( resolved->GetName() == aExpected )
        return true;

    for( NETCLASS* constituent : resolved->GetConstituentNetclasses() )
    {
        if( constituent && constituent->GetName() == aExpected )
            return true;
    }

    return false;
}


// Chain-derived netclass pattern assignments must contribute to effective netclass resolution
// alongside user-authored patterns, but must be cleanable independently so stale chain entries
// do not persist across netlist updates.
BOOST_AUTO_TEST_CASE( ChainPatternAssignmentResolvesAndClears )
{
    NET_SETTINGS settings( nullptr, "" );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxS( "HighSpeed" ), false );
    std::map<wxString, std::shared_ptr<NETCLASS>> classes;
    classes[wxS( "HighSpeed" )] = highSpeed;
    settings.SetNetclasses( classes );

    settings.SetChainPatternAssignment( wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( resolvesToNetclass( settings, wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) ) );

    settings.ClearChainPatternAssignments();

    BOOST_CHECK( !resolvesToNetclass( settings, wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) ) );
}


// Clearing chain-derived patterns must not disturb user-authored patterns.
BOOST_AUTO_TEST_CASE( ClearChainPatternAssignmentsLeavesUserPatterns )
{
    NET_SETTINGS settings( nullptr, "" );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxS( "HighSpeed" ), false );
    std::shared_ptr<NETCLASS> power = std::make_shared<NETCLASS>( wxS( "Power" ), false );
    std::map<wxString, std::shared_ptr<NETCLASS>> classes;
    classes[wxS( "HighSpeed" )] = highSpeed;
    classes[wxS( "Power" )] = power;
    settings.SetNetclasses( classes );

    settings.SetNetclassPatternAssignment( wxS( "VCC_*" ), wxS( "Power" ) );
    settings.SetChainPatternAssignment( wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( resolvesToNetclass( settings, wxS( "VCC_3V3" ), wxS( "Power" ) ) );
    BOOST_CHECK( resolvesToNetclass( settings, wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) ) );

    settings.ClearChainPatternAssignments();

    BOOST_CHECK( resolvesToNetclass( settings, wxS( "VCC_3V3" ), wxS( "Power" ) ) );
    BOOST_CHECK( !resolvesToNetclass( settings, wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) ) );
}


// ClearNetChainClasses must remove all chain->class entries.
BOOST_AUTO_TEST_CASE( ClearNetChainClassesRemovesAllEntries )
{
    NET_SETTINGS settings( nullptr, "" );

    settings.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "Default" ) );
    settings.SetNetChainClass( wxS( "CHAIN_B" ), wxS( "HighSpeed" ) );

    BOOST_CHECK_EQUAL( settings.GetNetChainClasses().size(), 2u );

    settings.ClearNetChainClasses();

    BOOST_CHECK( settings.GetNetChainClasses().empty() );
    BOOST_CHECK( settings.GetNetChainClass( wxS( "CHAIN_A" ) ).IsEmpty() );
}


// Regression guard for operator== coverage of m_netClassChainPatternAssignments.
// The container holds unique_ptrs, so naive std::equal would compare pointer identity
// and silently mark two settings instances built from identical input as unequal.
BOOST_AUTO_TEST_CASE( ChainPatternAssignmentAffectsEquality )
{
    NET_SETTINGS a( nullptr, "" );
    NET_SETTINGS b( nullptr, "" );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxS( "HighSpeed" ), false );
    std::map<wxString, std::shared_ptr<NETCLASS>> classes;
    classes[wxS( "HighSpeed" )] = highSpeed;

    a.SetNetclasses( classes );
    b.SetNetclasses( classes );

    BOOST_CHECK( a == b );

    a.SetChainPatternAssignment( wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( a != b );

    b.SetChainPatternAssignment( wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( a == b );

    a.ClearChainPatternAssignments();

    BOOST_CHECK( a != b );

    b.ClearChainPatternAssignments();

    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_SUITE_END()
