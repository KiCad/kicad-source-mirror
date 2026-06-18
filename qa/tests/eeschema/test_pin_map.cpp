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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <core/utf8.h>
#include <pin_map.h>
#include <lib_symbol.h>


namespace
{
LIB_ID makeFp( const wxString& aLibNick, const wxString& aName )
{
    LIB_ID id;
    id.SetLibNickname( aLibNick );
    id.SetLibItemName( aName );
    return id;
}
} // namespace


BOOST_AUTO_TEST_SUITE( PinMap )


BOOST_AUTO_TEST_CASE( EntrySetGetClear )
{
    PIN_MAP map( wxS( "STD-8" ) );

    BOOST_CHECK( map.IsEmpty() );
    BOOST_CHECK( !map.HasEntry( wxS( "1" ) ) );

    map.SetEntry( wxS( "1" ), wxS( "1" ) );
    map.SetEntry( wxS( "4" ), wxS( "[4,9]" ) );

    BOOST_CHECK( map.HasEntry( wxS( "1" ) ) );
    BOOST_CHECK_EQUAL( map.GetPadNumber( wxS( "1" ) ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( map.GetPadNumber( wxS( "4" ) ), wxS( "[4,9]" ) );
    BOOST_CHECK_EQUAL( map.GetEntries().size(), 2u );

    // GetPadNumber on a missing pin returns empty, never throws.
    BOOST_CHECK( map.GetPadNumber( wxS( "99" ) ).IsEmpty() );

    // Replace, not duplicate.
    map.SetEntry( wxS( "1" ), wxS( "8" ) );
    BOOST_CHECK_EQUAL( map.GetEntries().size(), 2u );
    BOOST_CHECK_EQUAL( map.GetPadNumber( wxS( "1" ) ), wxS( "8" ) );

    map.ClearEntry( wxS( "1" ) );
    BOOST_CHECK( !map.HasEntry( wxS( "1" ) ) );
    BOOST_CHECK_EQUAL( map.GetEntries().size(), 1u );

    // Clearing a missing entry is a no-op.
    map.ClearEntry( wxS( "99" ) );
    BOOST_CHECK_EQUAL( map.GetEntries().size(), 1u );
}


BOOST_AUTO_TEST_CASE( IsIdentity )
{
    const std::vector<wxString> pins = { wxS( "1" ), wxS( "2" ), wxS( "4" ) };

    PIN_MAP empty( wxS( "Empty" ) );
    BOOST_CHECK( empty.IsIdentity( pins ) );

    PIN_MAP straight( wxS( "Straight" ) );
    straight.SetEntry( wxS( "1" ), wxS( "1" ) );
    straight.SetEntry( wxS( "2" ), wxS( "2" ) );
    BOOST_CHECK( straight.IsIdentity( pins ) );

    PIN_MAP swap( wxS( "Swap" ) );
    swap.SetEntry( wxS( "1" ), wxS( "2" ) );
    swap.SetEntry( wxS( "2" ), wxS( "1" ) );
    BOOST_CHECK( !swap.IsIdentity( pins ) );

    // A bracketed multi-pad entry is never identity even when one expanded pad
    // matches the pin number.
    PIN_MAP stacked( wxS( "Stacked" ) );
    stacked.SetEntry( wxS( "4" ), wxS( "[4,9]" ) );
    BOOST_CHECK( !stacked.IsIdentity( pins ) );
}


BOOST_AUTO_TEST_CASE( SetAddOrReplaceAndFind )
{
    PIN_MAP_SET set;

    set.AddOrReplace( PIN_MAP( wxS( "STD-8" ) ) );
    set.AddOrReplace( PIN_MAP( wxS( "DFN-8-EP" ) ) );

    BOOST_CHECK_EQUAL( set.GetAll().size(), 2u );
    BOOST_REQUIRE( set.FindByName( wxS( "STD-8" ) ) );
    BOOST_CHECK_EQUAL( set.FindByName( wxS( "STD-8" ) )->GetName(), wxS( "STD-8" ) );
    BOOST_CHECK( set.FindByName( wxS( "missing" ) ) == nullptr );

    // Replace by name preserves count and order, and overwrites the entries.
    PIN_MAP replacement( wxS( "STD-8" ) );
    replacement.SetEntry( wxS( "1" ), wxS( "1" ) );
    set.AddOrReplace( std::move( replacement ) );
    BOOST_CHECK_EQUAL( set.GetAll().size(), 2u );
    BOOST_CHECK_EQUAL( set.GetAll().front().GetName(), wxS( "STD-8" ) );
    BOOST_CHECK( set.FindByName( wxS( "STD-8" ) )->HasEntry( wxS( "1" ) ) );

    set.Remove( wxS( "DFN-8-EP" ) );
    BOOST_CHECK_EQUAL( set.GetAll().size(), 1u );
    BOOST_CHECK( set.FindByName( wxS( "DFN-8-EP" ) ) == nullptr );
}


BOOST_AUTO_TEST_CASE( SharedNamedMapReuse )
{
    // Two associated footprints can bind to one named map by name; editing the
    // map changes both bindings because the map is stored once.
    PIN_MAP_SET set;
    set.AddOrReplace( PIN_MAP( wxS( "STD-8" ) ) );

    ASSOCIATED_FOOTPRINT soic{ makeFp( wxS( "Package_SO" ), wxS( "SOIC-8" ) ), wxS( "STD-8" ) };
    ASSOCIATED_FOOTPRINT vssop{ makeFp( wxS( "Package_SO" ), wxS( "VSSOP-8" ) ), wxS( "STD-8" ) };

    BOOST_CHECK_EQUAL( soic.m_MapName, vssop.m_MapName );

    set.FindByName( wxS( "STD-8" ) )->SetEntry( wxS( "1" ), wxS( "1" ) );

    BOOST_CHECK( set.FindByName( soic.m_MapName )->HasEntry( wxS( "1" ) ) );
    BOOST_CHECK( set.FindByName( vssop.m_MapName )->HasEntry( wxS( "1" ) ) );
}


BOOST_AUTO_TEST_CASE( LegacyPinMapConversion )
{
    // The database and HTTP backends both feed equal payloads through MakeLegacyPinMap, so they
    // build identical PIN_MAPs.  Multi-pad assignments become bracketed stacked notation.
    std::unordered_map<wxString, std::vector<wxString>> assignments;
    assignments[wxS( "1" )] = { wxS( "1" ) };
    assignments[wxS( "4" )] = { wxS( "4" ), wxS( "9" ) };
    assignments[wxS( "7" )] = {};   // empty assignment is skipped

    PIN_MAP map = MakeLegacyPinMap( wxS( "Database" ), assignments );

    BOOST_CHECK_EQUAL( map.GetName(), wxS( "Database" ) );
    BOOST_CHECK_EQUAL( map.GetPadNumber( wxS( "1" ) ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( map.GetPadNumber( wxS( "4" ) ), wxS( "[4,9]" ) );
    BOOST_CHECK( !map.HasEntry( wxS( "7" ) ) );
    BOOST_CHECK_EQUAL( map.GetEntries().size(), 2u );

    PIN_MAP httpMap = MakeLegacyPinMap( wxS( "Database" ), assignments );
    BOOST_CHECK( map == httpMap );
}


BOOST_AUTO_TEST_CASE( OverrideDefaultsAndDelegate )
{
    PIN_MAP_INSTANCE_OVERRIDE ov;
    BOOST_CHECK( ov.IsDefault() );
    BOOST_CHECK( !ov.IsDelegate() );

    ov.m_Edits.push_back( { wxS( "1" ), wxS( "8" ) } );
    BOOST_CHECK( !ov.IsDefault() );

    ov.m_Edits.clear();
    ov.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    ov.m_ActiveMapName = wxS( "X" );
    BOOST_CHECK( !ov.IsDefault() );
    BOOST_CHECK( !ov.IsDelegate() );

    ov.m_Mode = PIN_MAP_OVERRIDE_MODE::DELEGATE_TO_UNIT_1;
    BOOST_CHECK( ov.IsDelegate() );
    BOOST_CHECK( !ov.IsDefault() );
}


BOOST_AUTO_TEST_CASE( EqualityIsValueBased )
{
    PIN_MAP a( wxS( "M" ) );
    PIN_MAP b( wxS( "M" ) );
    BOOST_CHECK( a == b );

    a.SetEntry( wxS( "1" ), wxS( "1" ) );
    BOOST_CHECK( !( a == b ) );

    b.SetEntry( wxS( "1" ), wxS( "1" ) );
    BOOST_CHECK( a == b );

    // The map name is part of identity.
    b.SetName( wxS( "N" ) );
    BOOST_CHECK( !( a == b ) );

    // Associated footprints compare by both LIB_ID and map name.
    ASSOCIATED_FOOTPRINT fa{ makeFp( wxS( "L" ), wxS( "F" ) ), wxS( "STD" ) };
    ASSOCIATED_FOOTPRINT fb{ makeFp( wxS( "L" ), wxS( "F" ) ), wxS( "STD" ) };
    BOOST_CHECK( fa == fb );

    fb.m_MapName = wxS( "OTHER" );
    BOOST_CHECK( !( fa == fb ) );
}


namespace
{
PIN_MAP_SET makeSet( const wxString& aMapName )
{
    PIN_MAP_SET set;
    PIN_MAP     map( aMapName );
    map.SetEntry( wxS( "1" ), wxS( "8" ) );
    set.AddOrReplace( std::move( map ) );
    return set;
}
} // namespace


BOOST_AUTO_TEST_CASE( CoupledInheritanceThreeLevels )
{
    // root holds the bundle; mid and leaf are empty and inherit it as one unit.
    LIB_SYMBOL root( wxS( "root" ) );
    root.SetPinMaps( makeSet( wxS( "ROOT-MAP" ) ) );
    root.SetAssociatedFootprints( { { makeFp( wxS( "Package_SO" ), wxS( "SOIC-8" ) ), wxS( "ROOT-MAP" ) } } );

    LIB_SYMBOL mid( wxS( "mid" ) );
    mid.SetParent( &root );

    LIB_SYMBOL leaf( wxS( "leaf" ) );
    leaf.SetParent( &mid );

    BOOST_REQUIRE( leaf.IsDerived() );

    // Empty leaf inherits both halves of the bundle from the root.
    BOOST_REQUIRE( leaf.GetEffectivePinMaps().FindByName( wxS( "ROOT-MAP" ) ) );
    BOOST_CHECK_EQUAL( leaf.GetEffectiveAssociatedFootprints().size(), 1u );
    BOOST_CHECK_EQUAL( leaf.GetEffectiveAssociatedFootprints().front().m_MapName, wxS( "ROOT-MAP" ) );

    // The leaf defines only its own association (no maps).  Because the bundle is coupled, the
    // leaf's (empty) map set replaces the parent's maps too - it does not keep ROOT-MAP.
    leaf.SetAssociatedFootprints( { { makeFp( wxS( "Package_SO" ), wxS( "VSSOP-8" ) ), wxS( "LEAF-MAP" ) } } );

    BOOST_CHECK( leaf.GetEffectivePinMaps().IsEmpty() );
    BOOST_CHECK_EQUAL( leaf.GetEffectiveAssociatedFootprints().size(), 1u );
    BOOST_CHECK_EQUAL( leaf.GetEffectiveAssociatedFootprints().front().m_MapName, wxS( "LEAF-MAP" ) );

    // mid still inherits the unchanged root bundle.
    BOOST_REQUIRE( mid.GetEffectivePinMaps().FindByName( wxS( "ROOT-MAP" ) ) );
}


BOOST_AUTO_TEST_CASE( FlattenIsSelfContained )
{
    LIB_SYMBOL root( wxS( "root" ) );
    root.SetPinMaps( makeSet( wxS( "ROOT-MAP" ) ) );
    root.SetAssociatedFootprints( { { makeFp( wxS( "Package_SO" ), wxS( "SOIC-8" ) ), wxS( "ROOT-MAP" ) } } );

    LIB_SYMBOL leaf( wxS( "leaf" ) );
    leaf.SetParent( &root );

    std::unique_ptr<LIB_SYMBOL> flat = leaf.Flatten();

    BOOST_REQUIRE( flat );
    BOOST_CHECK( !flat->IsDerived() );
    BOOST_REQUIRE( flat->GetPinMaps().FindByName( wxS( "ROOT-MAP" ) ) );
    BOOST_CHECK_EQUAL( flat->GetAssociatedFootprints().size(), 1u );
}


BOOST_AUTO_TEST_CASE( SharedMapReuseOnSymbol )
{
    LIB_SYMBOL sym( wxS( "dual" ) );
    sym.PinMaps().AddOrReplace( PIN_MAP( wxS( "STD-8" ) ) );
    sym.SetAssociatedFootprints( {
            { makeFp( wxS( "Package_SO" ), wxS( "SOIC-8" ) ), wxS( "STD-8" ) },
            { makeFp( wxS( "Package_SO" ), wxS( "VSSOP-8" ) ), wxS( "STD-8" ) },
    } );

    // Editing the one named map is seen through both associations.
    sym.PinMaps().FindByName( wxS( "STD-8" ) )->SetEntry( wxS( "1" ), wxS( "1" ) );

    for( const ASSOCIATED_FOOTPRINT& assoc : sym.GetAssociatedFootprints() )
        BOOST_CHECK( sym.GetPinMaps().FindByName( assoc.m_MapName )->HasEntry( wxS( "1" ) ) );
}


BOOST_AUTO_TEST_CASE( CompareDetectsBundleDifferences )
{
    // Compare a symbol against an exact copy so the baseline is genuinely equal; then mutate the
    // pin-map bundle and confirm Compare() now reports a difference (otherwise rescue/diff/
    // library-mismatch ERC would silently ignore map edits).
    LIB_SYMBOL a( wxS( "part" ) );
    a.SetPinMaps( makeSet( wxS( "M" ) ) );

    LIB_SYMBOL b( a );
    BOOST_CHECK_EQUAL( a.Compare( b ), 0 );

    a.PinMaps().FindByName( wxS( "M" ) )->SetEntry( wxS( "2" ), wxS( "7" ) );
    BOOST_CHECK( a.Compare( b ) != 0 );

    LIB_SYMBOL c( b );
    BOOST_CHECK_EQUAL( b.Compare( c ), 0 );

    c.SetAssociatedFootprints( { { makeFp( wxS( "L" ), wxS( "F" ) ), wxS( "M" ) } } );
    BOOST_CHECK( b.Compare( c ) != 0 );
}


BOOST_AUTO_TEST_SUITE_END()
