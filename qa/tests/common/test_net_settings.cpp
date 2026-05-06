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
#include <settings/json_settings_internals.h>


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


// m_netClassChainPatternAssignments is derived state rebuilt on every netlist update from
// m_netChainClasses plus board NETINFO.  It must NOT contribute to operator==, otherwise a
// no-op netlist rebuild marks the project dirty even when the user made no edit.  The
// persisted m_netChainClasses map (covered above) is the source of truth for equality.
BOOST_AUTO_TEST_CASE( ChainPatternAssignmentExcludedFromEquality )
{
    NET_SETTINGS a( nullptr, "" );
    NET_SETTINGS b( nullptr, "" );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxS( "HighSpeed" ), false );
    std::shared_ptr<NETCLASS> power = std::make_shared<NETCLASS>( wxS( "Power" ), false );
    std::map<wxString, std::shared_ptr<NETCLASS>> classes;
    classes[wxS( "HighSpeed" )] = highSpeed;
    classes[wxS( "Power" )] = power;

    a.SetNetclasses( classes );
    b.SetNetclasses( classes );

    BOOST_CHECK( a == b );

    a.SetChainPatternAssignment( wxS( "DDR_DQ0" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( a == b );

    a.SetChainPatternAssignment( wxS( "VCC_3V3" ), wxS( "Power" ) );
    b.SetChainPatternAssignment( wxS( "OTHER" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( a == b );

    a.ClearChainPatternAssignments();

    BOOST_CHECK( a == b );
}


// Persisted-state regression guard for the "net_chain_classes" JSON key.  In-memory
// equality tests cannot detect a renamed/missing JSON key or a broken read lambda --
// those failures present as silent loss of every chain->class assignment on project
// reopen.  Exercise the full Store -> serialize -> parse -> Load round trip so any
// drift in the JSON contract fails the suite immediately.
BOOST_AUTO_TEST_CASE( NetChainClassesJsonRoundTrip )
{
    NET_SETTINGS source( nullptr, "" );

    source.SetNetChainClass( wxS( "CHAIN_A" ), wxS( "Default" ) );
    source.SetNetChainClass( wxS( "CHAIN_B" ), wxS( "HighSpeed" ) );
    source.SetNetChainClass( wxS( "CHAIN_WITH_SPACES and: punctuation!" ), wxS( "Power" ) );
    source.SetNetChainClass( wxS( "Unicode_éèê" ), wxS( "RF_µwave" ) );

    BOOST_REQUIRE( source.Store() );

    // Round-trip through the text form so a renamed key, broken serializer, or broken
    // parser all surface as a test failure.  FormatAsString -> parse mirrors what
    // SaveToFile / LoadFromFile do on disk.
    std::string serialized = source.FormatAsString();
    nlohmann::json reparsed = nlohmann::json::parse( serialized );

    BOOST_REQUIRE( reparsed.contains( "net_chain_classes" ) );
    BOOST_REQUIRE( reparsed["net_chain_classes"].is_object() );

    NET_SETTINGS sink( nullptr, "" );

    // Seed the sink with a stale entry so a no-op reader would still leave it behind;
    // the read lambda must clear() before applying the loaded map.
    sink.SetNetChainClass( wxS( "STALE_CHAIN" ), wxS( "Default" ) );

    JSON_SETTINGS_INTERNALS reparsedInternals;
    static_cast<nlohmann::json&>( reparsedInternals ) = reparsed;
    sink.Internals()->CloneFrom( reparsedInternals );
    sink.Load();

    BOOST_CHECK( sink.GetNetChainClasses().size() == source.GetNetChainClasses().size() );
    BOOST_CHECK( sink.GetNetChainClasses() == source.GetNetChainClasses() );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "CHAIN_A" ) ) == wxS( "Default" ) );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "CHAIN_B" ) ) == wxS( "HighSpeed" ) );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "CHAIN_WITH_SPACES and: punctuation!" ) )
                 == wxS( "Power" ) );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "Unicode_éèê" ) ) == wxS( "RF_µwave" ) );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "STALE_CHAIN" ) ).IsEmpty() );
    BOOST_CHECK( source == sink );
}


// An empty m_netChainClasses must serialize as an empty object (not be omitted, not be
// emitted as null) and round-trip back to an empty map without leaving stale entries.
BOOST_AUTO_TEST_CASE( NetChainClassesJsonRoundTripEmpty )
{
    NET_SETTINGS source( nullptr, "" );

    BOOST_REQUIRE( source.Store() );

    std::string serialized = source.FormatAsString();
    nlohmann::json reparsed = nlohmann::json::parse( serialized );

    BOOST_REQUIRE( reparsed.contains( "net_chain_classes" ) );
    BOOST_CHECK( reparsed["net_chain_classes"].is_object() );
    BOOST_CHECK( reparsed["net_chain_classes"].empty() );

    NET_SETTINGS sink( nullptr, "" );

    sink.SetNetChainClass( wxS( "STALE" ), wxS( "Default" ) );

    JSON_SETTINGS_INTERNALS reparsedInternals;
    static_cast<nlohmann::json&>( reparsedInternals ) = reparsed;
    sink.Internals()->CloneFrom( reparsedInternals );
    sink.Load();

    BOOST_CHECK( sink.GetNetChainClasses().empty() );
}


// Empty class strings are how chain assignments are cleared (SetNetChainClass with "")
// and the writer must not emit them.  An attacker-supplied or hand-edited JSON file
// that contains an empty string value must also be ignored on load, matching the
// in-memory clear semantics tested in ChainClassValueAndClearAffectEquality.
BOOST_AUTO_TEST_CASE( NetChainClassesJsonDropsEmptyValues )
{
    NET_SETTINGS source( nullptr, "" );

    source.SetNetChainClass( wxS( "KEEP" ), wxS( "Default" ) );

    BOOST_REQUIRE( source.Store() );

    nlohmann::json reparsed = nlohmann::json::parse( source.FormatAsString() );

    BOOST_REQUIRE( reparsed.contains( "net_chain_classes" ) );
    BOOST_REQUIRE_EQUAL( reparsed["net_chain_classes"].size(), 1u );

    // Inject a hand-crafted empty-value entry to confirm the reader rejects it.
    reparsed["net_chain_classes"]["EMPTY"] = "";

    NET_SETTINGS sink( nullptr, "" );

    JSON_SETTINGS_INTERNALS reparsedInternals;
    static_cast<nlohmann::json&>( reparsedInternals ) = reparsed;
    sink.Internals()->CloneFrom( reparsedInternals );
    sink.Load();

    BOOST_CHECK_EQUAL( sink.GetNetChainClasses().size(), 1u );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "KEEP" ) ) == wxS( "Default" ) );
    BOOST_CHECK( sink.GetNetChainClass( wxS( "EMPTY" ) ).IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
