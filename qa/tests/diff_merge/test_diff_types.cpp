/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <diff_merge/kicad_diff_types.h>

#include <nlohmann/json.hpp>

#include <cstdio>

#ifndef _WIN32
#include <unistd.h>
#endif


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( DiffTypes )


// Helper: round-trip a value through JSON and verify equality
static void roundTrip( const DIFF_VALUE& aValue )
{
    nlohmann::json   j   = aValue.ToJson();
    DIFF_VALUE       out = DIFF_VALUE::FromJson( j );
    BOOST_CHECK_MESSAGE( out == aValue,
                         "DIFF_VALUE round-trip mismatch for type "
                             << static_cast<int>( aValue.GetType() ) );

    // Bit-identical JSON string is the strongest guarantee — same JSON twice in a row.
    nlohmann::json j2 = out.ToJson();
    BOOST_CHECK_EQUAL( j.dump(), j2.dump() );
}


BOOST_AUTO_TEST_CASE( DiffValueNone )
{
    DIFF_VALUE v;
    BOOST_CHECK( v.GetType() == DIFF_VALUE::T::NONE );
    roundTrip( v );
}


BOOST_AUTO_TEST_CASE( DiffValueBool )
{
    roundTrip( DIFF_VALUE::FromBool( true ) );
    roundTrip( DIFF_VALUE::FromBool( false ) );
    BOOST_CHECK( DIFF_VALUE::FromBool( true ) != DIFF_VALUE::FromBool( false ) );
}


BOOST_AUTO_TEST_CASE( DiffValueInt )
{
    roundTrip( DIFF_VALUE::FromInt( 0 ) );
    roundTrip( DIFF_VALUE::FromInt( -123 ) );
    roundTrip( DIFF_VALUE::FromInt( 2'147'483'647 ) );
}


BOOST_AUTO_TEST_CASE( DiffValueInt64 )
{
    roundTrip( DIFF_VALUE::FromInt64( 0 ) );
    roundTrip( DIFF_VALUE::FromInt64( 9'223'372'036'854'775'807LL ) );
    roundTrip( DIFF_VALUE::FromInt64( -9'223'372'036'854'775'807LL ) );
}


BOOST_AUTO_TEST_CASE( DiffValueDouble )
{
    roundTrip( DIFF_VALUE::FromDouble( 0.0 ) );
    roundTrip( DIFF_VALUE::FromDouble( 3.141592653589793 ) );
    roundTrip( DIFF_VALUE::FromDouble( -1.5e10 ) );
}


BOOST_AUTO_TEST_CASE( DiffValueString )
{
    roundTrip( DIFF_VALUE::FromString( std::string( "hello" ) ) );
    roundTrip( DIFF_VALUE::FromString( wxString::FromUTF8( "héllo wörld" ) ) );
    roundTrip( DIFF_VALUE::FromString( std::string( "" ) ) );
}


BOOST_AUTO_TEST_CASE( DiffValueKiid )
{
    KIID::SeedGenerator( 42 );
    KIID id;
    roundTrip( DIFF_VALUE::FromKiid( id ) );
}


BOOST_AUTO_TEST_CASE( DiffValueVector2I )
{
    roundTrip( DIFF_VALUE::FromVector2I( VECTOR2I( 0, 0 ) ) );
    roundTrip( DIFF_VALUE::FromVector2I( VECTOR2I( -10, 20 ) ) );
    roundTrip( DIFF_VALUE::FromVector2I( VECTOR2I( 1'000'000, -1'000'000 ) ) );
}


BOOST_AUTO_TEST_CASE( DiffValueBox2I )
{
    BOX2I b( VECTOR2I( 10, 20 ), VECTOR2I( 100, 200 ) );
    roundTrip( DIFF_VALUE::FromBox2I( b ) );
}


BOOST_AUTO_TEST_CASE( DiffValueColor )
{
    roundTrip( DIFF_VALUE::FromColor( KIGFX::COLOR4D( 0.1, 0.2, 0.3, 0.4 ) ) );
    roundTrip( DIFF_VALUE::FromColor( KIGFX::COLOR4D( 0.0, 0.0, 0.0, 0.0 ) ) );
    roundTrip( DIFF_VALUE::FromColor( KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) ) );
}


BOOST_AUTO_TEST_CASE( DiffValueLayer )
{
    roundTrip( DIFF_VALUE::FromLayer( F_Cu ) );
    roundTrip( DIFF_VALUE::FromLayer( B_Cu ) );
    roundTrip( DIFF_VALUE::FromLayer( Edge_Cuts ) );
}


BOOST_AUTO_TEST_CASE( DiffValueEnum )
{
    roundTrip( DIFF_VALUE::FromEnum( 7, "SEVEN" ) );
    roundTrip( DIFF_VALUE::FromEnum( 0, "" ) );
}


BOOST_AUTO_TEST_CASE( DiffValuePolygonSet )
{
    DIFF_VALUE::PolygonSet ps;

    // One polygon: a square outline with a smaller square hole.
    std::vector<VECTOR2I> outline = { { 0, 0 }, { 100, 0 }, { 100, 100 }, { 0, 100 } };
    std::vector<VECTOR2I> hole = { { 25, 25 }, { 75, 25 }, { 75, 75 }, { 25, 75 } };
    ps.push_back( { outline, hole } );

    // A second, hole-free triangle in a different location.
    ps.push_back( { { { 200, 200 }, { 300, 200 }, { 250, 300 } } } );

    roundTrip( DIFF_VALUE::FromPolygonSet( ps ) );

    DIFF_VALUE v = DIFF_VALUE::FromPolygonSet( ps );
    BOOST_CHECK( v.GetType() == DIFF_VALUE::T::POLYGON_SET );
    BOOST_CHECK( v.AsPolygonSet() == ps );
}


BOOST_AUTO_TEST_CASE( DiffValueEmptyPolygonSet )
{
    roundTrip( DIFF_VALUE::FromPolygonSet( DIFF_VALUE::PolygonSet{} ) );
}


BOOST_AUTO_TEST_CASE( PropertyDeltaRoundTrip )
{
    PROPERTY_DELTA d;
    d.name   = wxS( "Position" );
    d.before = DIFF_VALUE::FromVector2I( VECTOR2I( 0, 0 ) );
    d.after  = DIFF_VALUE::FromVector2I( VECTOR2I( 1000, 2000 ) );

    nlohmann::json j   = d.ToJson();
    PROPERTY_DELTA out = PROPERTY_DELTA::FromJson( j );
    BOOST_CHECK( out == d );
}


BOOST_AUTO_TEST_CASE( PropertyDeltaRoundTripAsymmetric )
{
    // Asymmetric before/after slots: NONE means the property was absent on
    // that side. Each row exercises one real-world delta shape so a
    // serializer that collapses NONE to a default value or drops the type
    // discriminator trips on the per-half GetType() checks.
    struct CASE
    {
        const char*     label;
        wxString        name;
        DIFF_VALUE      before;
        DIFF_VALUE      after;
        DIFF_VALUE::T   beforeType;
        DIFF_VALUE::T   afterType;
    };

    const CASE cases[] = {
        // Optional property newly set.
        { "added",       wxS( "SolderMaskMargin" ), DIFF_VALUE(),
          DIFF_VALUE::FromInt( 50000 ),             DIFF_VALUE::T::NONE,
          DIFF_VALUE::T::INT },
        // Optional property cleared.
        { "removed",     wxS( "SolderMaskMargin" ), DIFF_VALUE::FromInt( 50000 ),
          DIFF_VALUE(),                             DIFF_VALUE::T::INT,
          DIFF_VALUE::T::NONE },
        // Semantic upgrade across types (numeric width to "auto").
        { "type-change", wxS( "Width" ),            DIFF_VALUE::FromInt( 100 ),
          DIFF_VALUE::FromString( wxS( "auto" ) ),  DIFF_VALUE::T::INT,
          DIFF_VALUE::T::STRING },
    };

    for( const CASE& c : cases )
    {
        BOOST_TEST_CONTEXT( c.label )
        {
            PROPERTY_DELTA d;
            d.name   = c.name;
            d.before = c.before;
            d.after  = c.after;

            PROPERTY_DELTA out = PROPERTY_DELTA::FromJson( d.ToJson() );
            BOOST_CHECK( out == d );
            BOOST_CHECK( out.before.GetType() == c.beforeType );
            BOOST_CHECK( out.after.GetType()  == c.afterType );
        }
    }
}


BOOST_AUTO_TEST_CASE( ItemChangeRoundTrip )
{
    KIID::SeedGenerator( 1 );

    ITEM_CHANGE c;
    c.id       = KIID_PATH( wxS( "/" ) + KIID().AsString() );
    c.typeName = wxS( "PCB_TRACK" );
    c.kind     = CHANGE_KIND::MODIFIED;
    c.bbox     = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) );
    c.refdes   = wxS( "R1" );

    PROPERTY_DELTA d;
    d.name   = wxS( "Width" );
    d.before = DIFF_VALUE::FromInt( 100000 );
    d.after  = DIFF_VALUE::FromInt( 200000 );
    c.properties.push_back( d );

    nlohmann::json j   = c.ToJson();
    ITEM_CHANGE    out = ITEM_CHANGE::FromJson( j );
    BOOST_CHECK( out == c );
}


BOOST_AUTO_TEST_CASE( DocumentDiffRoundTrip )
{
    DOCUMENT_DIFF d;
    d.path    = wxS( "board.kicad_pcb" );
    d.docType = wxS( "kicad_pcb" );
    BOOST_CHECK( d.Empty() );

    ITEM_CHANGE c;
    c.id       = KIID_PATH();
    c.typeName = wxS( "PCB_VIA" );
    c.kind     = CHANGE_KIND::ADDED;
    c.bbox     = BOX2I();
    d.changes.push_back( c );

    BOOST_CHECK( !d.Empty() );
    BOOST_CHECK_EQUAL( d.Size(), 1u );

    nlohmann::json j   = d.ToJson();
    DOCUMENT_DIFF  out = DOCUMENT_DIFF::FromJson( j );
    BOOST_CHECK_EQUAL( out.path.ToStdString(), d.path.ToStdString() );
    BOOST_CHECK_EQUAL( out.docType.ToStdString(), d.docType.ToStdString() );
    // Size alone could pass on a reader that returned default-constructed
    // ITEM_CHANGEs — pin the actual content via operator==.
    BOOST_REQUIRE_EQUAL( out.changes.size(), d.changes.size() );
    BOOST_CHECK( out.changes[0] == d.changes[0] );
}


BOOST_AUTO_TEST_CASE( ProjectDiffEmpty )
{
    PROJECT_DIFF p;
    BOOST_CHECK( p.Empty() );

    DOCUMENT_DIFF d;
    p.documents.push_back( d );
    BOOST_CHECK( p.Empty() );

    ITEM_CHANGE c;
    c.kind     = CHANGE_KIND::ADDED;
    c.typeName = wxS( "PCB_VIA" );
    p.documents.front().changes.push_back( c );
    BOOST_CHECK( !p.Empty() );
}


BOOST_AUTO_TEST_CASE( ProjectDiffJsonRoundTrip )
{
    // PROJECT_DIFF::ToJson/FromJson were previously only exercised via
    // higher-level pipelines. Pin the two-document round-trip directly
    // with non-default bbox / refdes / properties / children payloads so
    // a regression that DROPS any of those (leaving the default values
    // that match across both sides) is still caught by the equality
    // assertion below.
    KIID::SeedGenerator( 700 );
    KIID idA, idB, childKiid;
    const KIID_PATH pathA = KIID_PATH( wxS( "/" ) + idA.AsString() );
    const KIID_PATH pathB = KIID_PATH( wxS( "/" ) + idB.AsString() );

    PROJECT_DIFF p;

    DOCUMENT_DIFF docA;
    docA.path    = wxS( "board.kicad_pcb" );
    docA.docType = wxS( "kicad_pcb" );

    ITEM_CHANGE c1;
    c1.id       = pathA;
    c1.typeName = wxS( "PCB_VIA" );
    c1.kind     = CHANGE_KIND::ADDED;
    c1.bbox     = BOX2I( VECTOR2I( 100, 200 ), VECTOR2I( 50, 50 ) );
    docA.changes.push_back( c1 );
    p.documents.push_back( docA );

    DOCUMENT_DIFF docB;
    docB.path    = wxS( "sheet.kicad_sch" );
    docB.docType = wxS( "kicad_sch" );

    ITEM_CHANGE c2;
    c2.id       = pathB;
    c2.typeName = wxS( "SCH_LABEL" );
    c2.kind     = CHANGE_KIND::MODIFIED;
    c2.bbox     = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 30, 10 ) );
    c2.refdes   = wxS( "NET_A" );

    PROPERTY_DELTA d;
    d.name   = wxS( "Width" );
    d.before = DIFF_VALUE::FromInt( 100 );
    d.after  = DIFF_VALUE::FromInt( 200 );
    c2.properties.push_back( d );

    ITEM_CHANGE childChange;
    childChange.id       = KIID_PATH( wxS( "/" ) + childKiid.AsString() );
    childChange.typeName = wxS( "SCH_PIN" );
    childChange.kind     = CHANGE_KIND::MODIFIED;
    c2.children.push_back( childChange );

    docB.changes.push_back( c2 );
    p.documents.push_back( docB );

    PROJECT_DIFF out = PROJECT_DIFF::FromJson( p.ToJson() );

    BOOST_REQUIRE_EQUAL( out.documents.size(), 2u );
    // Pin path + docType per document so an array-iteration regression
    // (e.g. duplicating documents[0]) trips. Size-only would miss that.
    BOOST_CHECK_EQUAL( out.documents[0].path.ToStdString(),    "board.kicad_pcb" );
    BOOST_CHECK_EQUAL( out.documents[0].docType.ToStdString(), "kicad_pcb" );
    BOOST_CHECK_EQUAL( out.documents[1].path.ToStdString(),    "sheet.kicad_sch" );
    BOOST_CHECK_EQUAL( out.documents[1].docType.ToStdString(), "kicad_sch" );

    // Full ITEM_CHANGE equality subsumes id / typeName / kind / bbox /
    // properties / refdes / children — typeName-only checks would pass if
    // the round-trip dropped/mangled any other field (e.g. MODIFIED
    // collapsing to default ADDED on c2).
    BOOST_REQUIRE_EQUAL( out.documents[0].changes.size(), 1u );
    BOOST_REQUIRE_EQUAL( out.documents[1].changes.size(), 1u );
    BOOST_CHECK( out.documents[0].changes[0] == c1 );
    BOOST_CHECK( out.documents[1].changes[0] == c2 );
}


BOOST_AUTO_TEST_CASE( ChangeKindStringRoundTrip )
{
    BOOST_CHECK( ChangeKindFromString( "added" )   == CHANGE_KIND::ADDED );
    BOOST_CHECK( ChangeKindFromString( "removed" ) == CHANGE_KIND::REMOVED );
    BOOST_CHECK( ChangeKindFromString( "modified" ) == CHANGE_KIND::MODIFIED );
    BOOST_CHECK( ChangeKindFromString( "collision" ) == CHANGE_KIND::COLLISION );
    BOOST_CHECK( ChangeKindFromString( "duplicate_uuid" ) == CHANGE_KIND::DUPLICATE_UUID );

    BOOST_CHECK_THROW( ChangeKindFromString( "garbage" ), std::invalid_argument );
}


BOOST_AUTO_TEST_CASE( DeterministicJsonOutput )
{
    DIFF_VALUE v1 = DIFF_VALUE::FromInt( 42 );
    DIFF_VALUE v2 = DIFF_VALUE::FromInt( 42 );
    BOOST_CHECK_EQUAL( v1.ToJson().dump(), v2.ToJson().dump() );
}


BOOST_AUTO_TEST_CASE( JsonEncodesUtf8NotLocale )
{
    // Non-ASCII content (UTF-8) must round-trip through JSON regardless of the
    // host locale. Using ToStdString() would invoke locale-dependent encoding
    // on Windows; we standardize on UTF-8 for the wire format.
    DIFF_VALUE v = DIFF_VALUE::FromString( wxString::FromUTF8( "héllo wörld µ" ) );

    nlohmann::json j = v.ToJson();

    // The JSON dump should contain the UTF-8 bytes verbatim (escaped by JSON
    // for non-ASCII as \u sequences, but representing the same logical string).
    DIFF_VALUE back = DIFF_VALUE::FromJson( j );
    BOOST_CHECK( back == v );

    PROPERTY_DELTA d;
    d.name   = wxString::FromUTF8( "Réf" );
    d.before = v;
    d.after  = DIFF_VALUE::FromString( wxString::FromUTF8( "föo" ) );
    PROPERTY_DELTA dback = PROPERTY_DELTA::FromJson( d.ToJson() );
    BOOST_CHECK( dback == d );
}


#ifndef _WIN32
BOOST_AUTO_TEST_CASE( WriteDiffOutputStdoutWritesContent )
{
    fflush( stdout );

    int pipeFd[2];
    BOOST_REQUIRE_EQUAL( pipe( pipeFd ), 0 );

    const int savedStdout = dup( STDOUT_FILENO );
    BOOST_REQUIRE( savedStdout >= 0 );

    BOOST_REQUIRE_EQUAL( dup2( pipeFd[1], STDOUT_FILENO ), STDOUT_FILENO );
    close( pipeFd[1] );

    const std::string content = "diff A B\nhéllo\n";
    BOOST_CHECK( WriteDiffOutput( content, wxEmptyString ) );
    fflush( stdout );

    BOOST_REQUIRE_EQUAL( dup2( savedStdout, STDOUT_FILENO ), STDOUT_FILENO );
    close( savedStdout );

    char buffer[256] = {};
    ssize_t bytesRead = read( pipeFd[0], buffer, sizeof( buffer ) - 1 );
    close( pipeFd[0] );

    BOOST_REQUIRE( bytesRead >= 0 );
    BOOST_CHECK_EQUAL( std::string( buffer, static_cast<std::size_t>( bytesRead ) ), content );
}
#endif


// DIFF_VALUE inequality coverage --------------------------------------------
//
// Round-trip tests above exercise `operator==` on identical values. None
// (except Bool) pin that DIFFERENT values compare unequal. A regression
// where operator== returns true for everything would corrupt every
// downstream consumer (PROPERTY_DELTA / ITEM_CHANGE / merge engine
// auto-resolve) — pin each type explicitly.

BOOST_AUTO_TEST_CASE( DiffValueInequalityAcrossPayloads )
{
    BOOST_CHECK( DIFF_VALUE::FromInt( 1 )           != DIFF_VALUE::FromInt( 2 ) );
    BOOST_CHECK( DIFF_VALUE::FromInt64( 1LL )       != DIFF_VALUE::FromInt64( 2LL ) );
    BOOST_CHECK( DIFF_VALUE::FromDouble( 1.0 )      != DIFF_VALUE::FromDouble( 2.0 ) );
    BOOST_CHECK( DIFF_VALUE::FromString( wxS( "a" ) )
                 != DIFF_VALUE::FromString( wxS( "b" ) ) );

    KIID::SeedGenerator( 600 );
    KIID k1, k2;
    // Defensive: confirm the seeded generator produced distinct KIIDs so the
    // KIID-inequality assertion below tests what it claims to test.
    BOOST_REQUIRE( k1 != k2 );
    BOOST_CHECK( DIFF_VALUE::FromKiid( k1 ) != DIFF_VALUE::FromKiid( k2 ) );

    BOOST_CHECK( DIFF_VALUE::FromVector2I( VECTOR2I( 1, 2 ) )
                 != DIFF_VALUE::FromVector2I( VECTOR2I( 3, 4 ) ) );
    BOOST_CHECK( DIFF_VALUE::FromBox2I( BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) )
                 != DIFF_VALUE::FromBox2I( BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 20, 20 ) ) ) );
    BOOST_CHECK( DIFF_VALUE::FromColor( KIGFX::COLOR4D( 1, 0, 0, 1 ) )
                 != DIFF_VALUE::FromColor( KIGFX::COLOR4D( 0, 1, 0, 1 ) ) );
    BOOST_CHECK( DIFF_VALUE::FromLayer( F_Cu ) != DIFF_VALUE::FromLayer( B_Cu ) );
    BOOST_CHECK( DIFF_VALUE::FromEnum( 1, "A" ) != DIFF_VALUE::FromEnum( 2, "B" ) );
}


BOOST_AUTO_TEST_CASE( DiffValueInequalityAcrossTypes )
{
    // Same logical "1" but different DIFF_VALUE::T discriminator must NOT
    // compare equal — otherwise the merge engine could auto-resolve a
    // type change as a no-op.
    BOOST_CHECK( DIFF_VALUE::FromInt( 1 )    != DIFF_VALUE::FromInt64( 1LL ) );
    BOOST_CHECK( DIFF_VALUE::FromInt( 1 )    != DIFF_VALUE::FromDouble( 1.0 ) );
    BOOST_CHECK( DIFF_VALUE::FromBool( true) != DIFF_VALUE::FromInt( 1 ) );
    BOOST_CHECK( DIFF_VALUE::FromString( wxS( "1" ) ) != DIFF_VALUE::FromInt( 1 ) );

    DIFF_VALUE none;
    BOOST_CHECK( none != DIFF_VALUE::FromInt( 0 ) );
    BOOST_CHECK( none != DIFF_VALUE::FromBool( false ) );
}


BOOST_AUTO_TEST_CASE( DiffValueNoneEqualsNone )
{
    // Two default-constructed values must compare equal — the merge engine
    // relies on this to skip "absent on both sides" properties.
    DIFF_VALUE a, b;
    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_CASE( PropertyDeltaEqualityFieldSensitive )
{
    PROPERTY_DELTA a;
    a.name   = wxS( "Width" );
    a.before = DIFF_VALUE::FromInt( 10 );
    a.after  = DIFF_VALUE::FromInt( 20 );

    PROPERTY_DELTA b = a;
    BOOST_CHECK( a == b );

    b.name = wxS( "Layer" );
    BOOST_CHECK( a != b );

    b      = a;
    b.before = DIFF_VALUE::FromInt( 99 );
    BOOST_CHECK( a != b );

    b      = a;
    b.after  = DIFF_VALUE::FromInt( 99 );
    BOOST_CHECK( a != b );
}


BOOST_AUTO_TEST_CASE( ItemChangeEqualityChildrenSensitive )
{
    // ITEM_CHANGE::operator== must recurse into children — without that,
    // a footprint edit that only changed a nested pad would compare equal
    // to one that didn't. Three layers of pin:
    //   1. no children vs one child -> unequal (catches size-blind eq)
    //   2. one child on each side with DIFFERENT content -> unequal
    //      (catches size-only eq that doesn't recurse)
    //   3. matching children -> equal again
    KIID::SeedGenerator( 601 );
    KIID parent, padIdA, padIdB;

    ITEM_CHANGE a;
    a.id       = KIID_PATH( wxS( "/" ) + parent.AsString() );
    a.typeName = wxS( "FOOTPRINT" );
    a.kind     = CHANGE_KIND::MODIFIED;

    ITEM_CHANGE b = a;
    BOOST_CHECK( a == b );

    ITEM_CHANGE child;
    child.id       = KIID_PATH( wxS( "/" ) + padIdA.AsString() );
    child.typeName = wxS( "PAD" );
    child.kind     = CHANGE_KIND::MODIFIED;
    b.children.push_back( child );
    BOOST_CHECK( a != b );

    // Both sides now have ONE child, but the children differ. A size-only
    // operator== would falsely report equal here — the recursive content
    // comparison is what catches the divergence.
    ITEM_CHANGE differentChild;
    differentChild.id       = KIID_PATH( wxS( "/" ) + padIdB.AsString() );
    differentChild.typeName = wxS( "PAD" );
    differentChild.kind     = CHANGE_KIND::MODIFIED;
    a.children.push_back( differentChild );
    BOOST_CHECK( a != b );

    // Now make A's child match B's: equal again.
    a.children.back() = child;
    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_SUITE_END()
