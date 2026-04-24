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

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <common.h>
#include <eda_item.h>
#include <eda_text.h>
#include <base_units.h>
#include <text_var_dependency.h>


BOOST_AUTO_TEST_SUITE( TextVarDependency )


BOOST_AUTO_TEST_CASE( KeyFromToken_Plain )
{
    TEXT_VAR_REF_KEY k = TEXT_VAR_REF_KEY::FromToken( wxT( "VALUE" ) );
    BOOST_CHECK( k.kind == TEXT_VAR_REF_KEY::KIND::LOCAL );
    BOOST_CHECK( k.primary == wxT( "VALUE" ) );
    BOOST_CHECK( k.secondary == wxT( "" ) );
}


BOOST_AUTO_TEST_CASE( KeyFromToken_CrossRef )
{
    TEXT_VAR_REF_KEY k = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) );
    BOOST_CHECK( k.kind == TEXT_VAR_REF_KEY::KIND::CROSS_REF );
    BOOST_CHECK( k.primary == wxT( "U1" ) );
    BOOST_CHECK( k.secondary == wxT( "VALUE" ) );
}


BOOST_AUTO_TEST_CASE( KeyFromToken_LeadingColon )
{
    // Leading colon is not a cross-reference; fall back to PLAIN with the full token.
    TEXT_VAR_REF_KEY k = TEXT_VAR_REF_KEY::FromToken( wxT( ":VALUE" ) );
    BOOST_CHECK( k.kind == TEXT_VAR_REF_KEY::KIND::LOCAL );
    BOOST_CHECK( k.primary == wxT( ":VALUE" ) );
}


BOOST_AUTO_TEST_CASE( KeyFromToken_TrailingColon )
{
    TEXT_VAR_REF_KEY k = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:" ) );
    BOOST_CHECK( k.kind == TEXT_VAR_REF_KEY::KIND::LOCAL );
    BOOST_CHECK( k.primary == wxT( "U1:" ) );
}


BOOST_AUTO_TEST_CASE( KeyFromToken_TitleBlockField )
{
    TEXT_VAR_REF_KEY k = TEXT_VAR_REF_KEY::FromToken( wxT( "PROJECTNAME" ) );
    BOOST_CHECK( k.kind == TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK );
    BOOST_CHECK( k.primary == wxT( "PROJECTNAME" ) );

    TEXT_VAR_REF_KEY rev = TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) );
    BOOST_CHECK( rev.kind == TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK );

    TEXT_VAR_REF_KEY c9 = TEXT_VAR_REF_KEY::FromToken( wxT( "COMMENT9" ) );
    BOOST_CHECK( c9.kind == TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK );
}


BOOST_AUTO_TEST_CASE( KeyFromToken_SpecialContextualToken )
{
    TEXT_VAR_REF_KEY k = TEXT_VAR_REF_KEY::FromToken( wxT( "SHEETNAME" ) );
    BOOST_CHECK( k.kind == TEXT_VAR_REF_KEY::KIND::SPECIAL );
    BOOST_CHECK( k.primary == wxT( "SHEETNAME" ) );
}


BOOST_AUTO_TEST_CASE( KeyFromToken_SPICEOperatingPoint )
{
    TEXT_VAR_REF_KEY bare = TEXT_VAR_REF_KEY::FromToken( wxT( "OP" ) );
    BOOST_CHECK( bare.kind == TEXT_VAR_REF_KEY::KIND::OP );

    TEXT_VAR_REF_KEY port = TEXT_VAR_REF_KEY::FromToken( wxT( "OP:1" ) );
    BOOST_CHECK( port.kind == TEXT_VAR_REF_KEY::KIND::OP );
    BOOST_CHECK( port.primary == wxT( "OP" ) );
    BOOST_CHECK( port.secondary == wxT( "1" ) );

    // IsTrackable() gates out OP keys so the tracker never registers them.
    BOOST_CHECK( !bare.IsTrackable() );
    BOOST_CHECK( !port.IsTrackable() );
    BOOST_CHECK( TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) ).IsTrackable() );
}


BOOST_AUTO_TEST_CASE( KeyEquality )
{
    TEXT_VAR_REF_KEY a = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) );
    TEXT_VAR_REF_KEY b = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) );
    TEXT_VAR_REF_KEY c = TEXT_VAR_REF_KEY::FromToken( wxT( "U2:VALUE" ) );
    TEXT_VAR_REF_KEY d = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:MPN" ) );

    BOOST_CHECK( a == b );
    BOOST_CHECK( !( a == c ) );
    BOOST_CHECK( !( a == d ) );
}


BOOST_AUTO_TEST_CASE( KeyHash_Distinguishes )
{
    TEXT_VAR_REF_KEY_HASH hasher;

    TEXT_VAR_REF_KEY plain;
    plain.kind = TEXT_VAR_REF_KEY::KIND::LOCAL;
    plain.primary = wxT( "U1:VALUE" );

    TEXT_VAR_REF_KEY cross;
    cross.kind = TEXT_VAR_REF_KEY::KIND::CROSS_REF;
    cross.primary = wxT( "U1" );
    cross.secondary = wxT( "VALUE" );

    // A plain token whose primary happens to equal "U1:VALUE" must not collide
    // with the cross-ref key {U1, VALUE}. Different KIND must participate in the hash.
    BOOST_CHECK_NE( hasher( plain ), hasher( cross ) );
}


BOOST_AUTO_TEST_CASE( Extract_NoReferences )
{
    auto refs = ExtractTextVarReferences( wxT( "plain text, nothing to see" ) );
    BOOST_CHECK_EQUAL( refs.size(), 0u );
}


BOOST_AUTO_TEST_CASE( Extract_SinglePlain )
{
    auto refs = ExtractTextVarReferences( wxT( "${VALUE}" ) );
    BOOST_REQUIRE_EQUAL( refs.size(), 1u );
    BOOST_CHECK( refs[0].kind == TEXT_VAR_REF_KEY::KIND::LOCAL );
    BOOST_CHECK( refs[0].primary == wxT( "VALUE" ) );
}


BOOST_AUTO_TEST_CASE( Extract_SingleCrossRef )
{
    auto refs = ExtractTextVarReferences( wxT( "${U1:VALUE}" ) );
    BOOST_REQUIRE_EQUAL( refs.size(), 1u );
    BOOST_CHECK( refs[0].kind == TEXT_VAR_REF_KEY::KIND::CROSS_REF );
    BOOST_CHECK( refs[0].primary == wxT( "U1" ) );
    BOOST_CHECK( refs[0].secondary == wxT( "VALUE" ) );
}


BOOST_AUTO_TEST_CASE( Extract_MultipleReferences )
{
    auto refs = ExtractTextVarReferences( wxT( "R=${R12:VALUE}, C=${C3:VALUE}, sheet=${SHEETNAME}" ) );
    BOOST_REQUIRE_EQUAL( refs.size(), 3u );

    auto hasKey = [&]( TEXT_VAR_REF_KEY::KIND k, const wxString& p, const wxString& s )
    {
        return std::any_of( refs.begin(), refs.end(),
                            [&]( const TEXT_VAR_REF_KEY& ref )
                            { return ref.kind == k && ref.primary == p && ref.secondary == s; } );
    };

    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::CROSS_REF, wxT( "R12" ), wxT( "VALUE" ) ) );
    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::CROSS_REF, wxT( "C3" ), wxT( "VALUE" ) ) );
    BOOST_CHECK( hasKey( TEXT_VAR_REF_KEY::KIND::SPECIAL, wxT( "SHEETNAME" ), wxT( "" ) ) );
}


BOOST_AUTO_TEST_CASE( Extract_EscapedReferenceIgnored )
{
    // `\${VALUE}` must not generate a dependency edge — the literal text is intended.
    auto refs = ExtractTextVarReferences( wxT( "literal \\${VALUE} here" ) );
    BOOST_CHECK_EQUAL( refs.size(), 0u );
}


BOOST_AUTO_TEST_CASE( Extract_MalformedTokenStillCapturesInnerRef )
{
    // `${FOO${BAR}` — outer closing brace missing (premature EOF). The
    // partial body still contains a fully-formed inner ${BAR} that must be
    // captured as a dependency; dropping it would silently lose edges on
    // user-in-progress edits.
    auto refs = ExtractTextVarReferences( wxT( "${FOO${BAR}" ) );

    bool hasBar = std::any_of( refs.begin(), refs.end(),
                               []( const TEXT_VAR_REF_KEY& k )
                               { return k.primary == wxT( "BAR" ); } );

    BOOST_CHECK( hasBar );
}


BOOST_AUTO_TEST_CASE( Extract_NestedReferences )
{
    // ${FOO_${BAR}} — both the outer token (literal form) and the inner BAR
    // must be captured as dependency edges.
    auto refs = ExtractTextVarReferences( wxT( "${FOO_${BAR}}" ) );

    bool hasBar = std::any_of( refs.begin(), refs.end(),
                               []( const TEXT_VAR_REF_KEY& k )
                               { return k.primary == wxT( "BAR" ); } );
    bool hasOuter = std::any_of( refs.begin(), refs.end(),
                                 []( const TEXT_VAR_REF_KEY& k )
                                 { return k.primary.Contains( wxT( "FOO_" ) ); } );

    BOOST_CHECK( hasBar );
    BOOST_CHECK( hasOuter );
}


BOOST_AUTO_TEST_CASE( Extract_ERC_WARNING_IsCaptured )
{
    // Codex finding 3: ExpandTextVars' resolver short-circuits ERC_WARNING/
    // DRC_WARNING tokens when not in an ERC/DRC context, so the old collector
    // approach silently dropped them from the dependency graph. The dedicated
    // lexer captures them regardless.
    auto refs = ExtractTextVarReferences( wxT( "${ERC_WARNING something}" ) );

    BOOST_REQUIRE_EQUAL( refs.size(), 1u );
    BOOST_CHECK( refs[0].primary == wxT( "ERC_WARNING something" ) );
}


BOOST_AUTO_TEST_CASE( Extract_NestedDynamicCrossRef )
{
    // ${U1:NET_NAME(@{${ROW}-2})} — the outer token captures the full body
    // literally, ROW is captured as a nested dependency via the @{} math
    // wrapper.
    auto refs = ExtractTextVarReferences( wxT( "${U1:NET_NAME(@{${ROW}-2})}" ) );

    bool hasRow = std::any_of( refs.begin(), refs.end(),
                               []( const TEXT_VAR_REF_KEY& k )
                               { return k.primary == wxT( "ROW" ); } );
    bool hasOuter = std::any_of( refs.begin(), refs.end(),
                                 []( const TEXT_VAR_REF_KEY& k )
                                 { return k.kind == TEXT_VAR_REF_KEY::KIND::CROSS_REF
                                          && k.primary == wxT( "U1" ); } );

    BOOST_CHECK( hasRow );
    BOOST_CHECK( hasOuter );
}


BOOST_AUTO_TEST_CASE( Extract_MathExpressionInnerVar )
{
    // @{2*${X}} — the inner ${X} is what we depend on; the math wrapper itself
    // is not a named variable source.
    auto refs = ExtractTextVarReferences( wxT( "@{2*${X}}" ) );

    bool hasX = std::any_of( refs.begin(), refs.end(),
                             []( const TEXT_VAR_REF_KEY& k )
                             { return k.primary == wxT( "X" ); } );

    BOOST_CHECK( hasX );
}


// Use a test-local EDA_ITEM stand-in. EDA_ITEM is abstract (NotInSchematic, Clone,
// GetClass pure virtual in subclasses) — we only need a unique pointer identity.
namespace
{
struct FakeItem
{
    int id;
};
}


BOOST_AUTO_TEST_CASE( Index_RegisterAndLookup )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  item{ 1 };
    EDA_ITEM*                 itemPtr = reinterpret_cast<EDA_ITEM*>( &item );

    std::vector<TEXT_VAR_REF_KEY> keys = {
        TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) ),
        TEXT_VAR_REF_KEY::FromToken( wxT( "SHEETNAME" ) )
    };

    index.Register( itemPtr, keys );

    BOOST_CHECK_EQUAL( index.ItemCount(), 1u );
    BOOST_CHECK_EQUAL( index.DependentCount( keys[0] ), 1u );
    BOOST_CHECK_EQUAL( index.DependentCount( keys[1] ), 1u );

    std::vector<EDA_ITEM*> found;
    index.ForEachDependent( keys[0], [&]( EDA_ITEM* i ) { found.push_back( i ); } );

    BOOST_REQUIRE_EQUAL( found.size(), 1u );
    BOOST_CHECK_EQUAL( found[0], itemPtr );
}


BOOST_AUTO_TEST_CASE( Index_ReRegisterReplaces )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  item{ 1 };
    EDA_ITEM*                 itemPtr = reinterpret_cast<EDA_ITEM*>( &item );

    TEXT_VAR_REF_KEY oldKey = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) );
    TEXT_VAR_REF_KEY newKey = TEXT_VAR_REF_KEY::FromToken( wxT( "U2:MPN" ) );

    index.Register( itemPtr, { oldKey } );
    BOOST_CHECK_EQUAL( index.DependentCount( oldKey ), 1u );

    index.Register( itemPtr, { newKey } );
    BOOST_CHECK_EQUAL( index.DependentCount( oldKey ), 0u );
    BOOST_CHECK_EQUAL( index.DependentCount( newKey ), 1u );
    BOOST_CHECK_EQUAL( index.ItemCount(), 1u );
}


BOOST_AUTO_TEST_CASE( Index_Unregister )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  item{ 1 };
    EDA_ITEM*                 itemPtr = reinterpret_cast<EDA_ITEM*>( &item );

    TEXT_VAR_REF_KEY key = TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) );
    index.Register( itemPtr, { key } );
    BOOST_CHECK_EQUAL( index.DependentCount( key ), 1u );

    index.Unregister( itemPtr );
    BOOST_CHECK_EQUAL( index.DependentCount( key ), 0u );
    BOOST_CHECK_EQUAL( index.ItemCount(), 0u );

    // Unregister is idempotent — second call is a no-op.
    index.Unregister( itemPtr );
    BOOST_CHECK_EQUAL( index.ItemCount(), 0u );
}


BOOST_AUTO_TEST_CASE( Index_MultipleItemsShareKey )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  a{ 1 };
    FakeItem                  b{ 2 };
    FakeItem                  c{ 3 };
    EDA_ITEM*                 aPtr = reinterpret_cast<EDA_ITEM*>( &a );
    EDA_ITEM*                 bPtr = reinterpret_cast<EDA_ITEM*>( &b );
    EDA_ITEM*                 cPtr = reinterpret_cast<EDA_ITEM*>( &c );

    TEXT_VAR_REF_KEY shared = TEXT_VAR_REF_KEY::FromToken( wxT( "R1:VALUE" ) );
    TEXT_VAR_REF_KEY other  = TEXT_VAR_REF_KEY::FromToken( wxT( "SHEETNAME" ) );

    index.Register( aPtr, { shared } );
    index.Register( bPtr, { shared, other } );
    index.Register( cPtr, { other } );

    BOOST_CHECK_EQUAL( index.DependentCount( shared ), 2u );
    BOOST_CHECK_EQUAL( index.DependentCount( other ), 2u );
    BOOST_CHECK_EQUAL( index.ItemCount(), 3u );

    // Removing one dependent leaves the other.
    index.Unregister( aPtr );
    BOOST_CHECK_EQUAL( index.DependentCount( shared ), 1u );

    std::vector<EDA_ITEM*> remaining;
    index.ForEachDependent( shared, [&]( EDA_ITEM* i ) { remaining.push_back( i ); } );
    BOOST_REQUIRE_EQUAL( remaining.size(), 1u );
    BOOST_CHECK_EQUAL( remaining[0], bPtr );
}


BOOST_AUTO_TEST_CASE( Index_ClearDropsEverything )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  a{ 1 };
    FakeItem                  b{ 2 };
    EDA_ITEM*                 aPtr = reinterpret_cast<EDA_ITEM*>( &a );
    EDA_ITEM*                 bPtr = reinterpret_cast<EDA_ITEM*>( &b );

    TEXT_VAR_REF_KEY k1 = TEXT_VAR_REF_KEY::FromToken( wxT( "A" ) );
    TEXT_VAR_REF_KEY k2 = TEXT_VAR_REF_KEY::FromToken( wxT( "B" ) );

    index.Register( aPtr, { k1 } );
    index.Register( bPtr, { k2 } );

    index.Clear();

    BOOST_CHECK_EQUAL( index.ItemCount(), 0u );
    BOOST_CHECK_EQUAL( index.DependentCount( k1 ), 0u );
    BOOST_CHECK_EQUAL( index.DependentCount( k2 ), 0u );
}


BOOST_AUTO_TEST_CASE( Index_EmptyRegistrationIsUnregister )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  item{ 1 };
    EDA_ITEM*                 itemPtr = reinterpret_cast<EDA_ITEM*>( &item );

    TEXT_VAR_REF_KEY key = TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) );

    index.Register( itemPtr, { key } );
    BOOST_CHECK_EQUAL( index.DependentCount( key ), 1u );

    // Registering with an empty key vector clears the item's edges.
    index.Register( itemPtr, {} );
    BOOST_CHECK_EQUAL( index.DependentCount( key ), 0u );
    BOOST_CHECK_EQUAL( index.ItemCount(), 0u );
}


// Tracker-level tests use direct index manipulation rather than exercising the
// OnItemsAdded() entry point, because that entry point dynamic_cast<>s to
// EDA_TEXT* which requires EDA_ITEM's typeinfo across a DSO boundary the
// standalone qa_common executable does not see. Integration tests that wire a
// real BOARD_LISTENER / SCHEMATIC_LISTENER live with the module tests.


BOOST_AUTO_TEST_CASE( Tracker_InvalidateKeyFansOutToAllDependents )
{
    TEXT_VAR_TRACKER tracker;
    FakeItem         a{ 1 };
    FakeItem         b{ 2 };
    FakeItem         unrelated{ 3 };

    EDA_ITEM* aPtr = reinterpret_cast<EDA_ITEM*>( &a );
    EDA_ITEM* bPtr = reinterpret_cast<EDA_ITEM*>( &b );
    EDA_ITEM* unrelatedPtr = reinterpret_cast<EDA_ITEM*>( &unrelated );

    TEXT_VAR_REF_KEY shared = TEXT_VAR_REF_KEY::FromToken( wxT( "SHEETNAME" ) );
    TEXT_VAR_REF_KEY other  = TEXT_VAR_REF_KEY::FromToken( wxT( "OTHER" ) );

    tracker.Index().Register( aPtr, { shared } );
    tracker.Index().Register( bPtr, { shared } );
    tracker.Index().Register( unrelatedPtr, { other } );

    std::vector<EDA_ITEM*> hit;
    (void) tracker.AddInvalidateListener(
            [&]( EDA_ITEM* item, const TEXT_VAR_REF_KEY& ) { hit.push_back( item ); } );

    tracker.InvalidateKey( shared );

    BOOST_CHECK_EQUAL( hit.size(), 2u );
    BOOST_CHECK( std::find( hit.begin(), hit.end(), aPtr ) != hit.end() );
    BOOST_CHECK( std::find( hit.begin(), hit.end(), bPtr ) != hit.end() );
    BOOST_CHECK( std::find( hit.begin(), hit.end(), unrelatedPtr ) == hit.end() );
}


BOOST_AUTO_TEST_CASE( Tracker_SourceChangeFansOutExtractedKeys )
{
    TEXT_VAR_TRACKER tracker;
    FakeItem         dependent{ 1 };
    FakeItem         source{ 2 };

    EDA_ITEM* dependentPtr = reinterpret_cast<EDA_ITEM*>( &dependent );
    EDA_ITEM* sourcePtr    = reinterpret_cast<EDA_ITEM*>( &source );

    TEXT_VAR_REF_KEY crossRef = TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) );
    tracker.Index().Register( dependentPtr, { crossRef } );

    tracker.SetSourceKeyExtractor(
            [&]( EDA_ITEM* item ) -> std::vector<TEXT_VAR_REF_KEY>
            {
                if( item == sourcePtr )
                    return { crossRef };

                return {};
            } );

    std::vector<std::pair<EDA_ITEM*, TEXT_VAR_REF_KEY>> invalidations;
    (void) tracker.AddInvalidateListener(
            [&]( EDA_ITEM* item, const TEXT_VAR_REF_KEY& key )
            { invalidations.emplace_back( item, key ); } );

    // HandleItemChanged re-registers (we pass empty keys since source is not
    // itself a dependent) then fans out source keys via the extractor.
    tracker.HandleItemChanged( sourcePtr, {} );

    BOOST_REQUIRE_EQUAL( invalidations.size(), 1u );
    BOOST_CHECK_EQUAL( invalidations[0].first, dependentPtr );
    BOOST_CHECK( invalidations[0].second == crossRef );
}


BOOST_AUTO_TEST_CASE( Tracker_NoInvalidateWithoutCallback )
{
    TEXT_VAR_TRACKER tracker;
    FakeItem         item{ 1 };
    EDA_ITEM*        itemPtr = reinterpret_cast<EDA_ITEM*>( &item );

    tracker.Index().Register( itemPtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) } );

    // Without a registered invalidate callback, InvalidateKey is a no-op and
    // must not crash.
    tracker.InvalidateKey( TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) );
    BOOST_CHECK_EQUAL( tracker.Index().ItemCount(), 1u );
}


BOOST_AUTO_TEST_CASE( Tracker_InvalidateVariantScopedFiresCrossRefAndLocal )
{
    TEXT_VAR_TRACKER tracker;
    FakeItem         crossDep{ 1 };
    FakeItem         localDep{ 2 };
    FakeItem         titleDep{ 3 };

    EDA_ITEM* crossPtr = reinterpret_cast<EDA_ITEM*>( &crossDep );
    EDA_ITEM* localPtr = reinterpret_cast<EDA_ITEM*>( &localDep );
    EDA_ITEM* titlePtr = reinterpret_cast<EDA_ITEM*>( &titleDep );

    tracker.Index().Register( crossPtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) ) } );
    tracker.Index().Register( localPtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "VALUE" ) ) } );
    tracker.Index().Register( titlePtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) ) } );

    std::vector<EDA_ITEM*> hit;
    (void) tracker.AddInvalidateListener(
            [&]( EDA_ITEM* item, const TEXT_VAR_REF_KEY& ) { hit.push_back( item ); } );

    // Variant switches change ${REFDES:FIELD} and own-field values but do
    // NOT affect title-block sources.
    tracker.InvalidateVariantScoped();

    BOOST_CHECK_EQUAL( hit.size(), 2u );
    BOOST_CHECK( std::find( hit.begin(), hit.end(), crossPtr ) != hit.end() );
    BOOST_CHECK( std::find( hit.begin(), hit.end(), localPtr ) != hit.end() );
    BOOST_CHECK( std::find( hit.begin(), hit.end(), titlePtr ) == hit.end() );
}


BOOST_AUTO_TEST_CASE( Tracker_InvalidateByKindFiltersKeys )
{
    TEXT_VAR_TRACKER tracker;
    FakeItem         titleDep{ 1 };
    FakeItem         crossDep{ 2 };

    EDA_ITEM* tPtr = reinterpret_cast<EDA_ITEM*>( &titleDep );
    EDA_ITEM* cPtr = reinterpret_cast<EDA_ITEM*>( &crossDep );

    tracker.Index().Register( tPtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "REVISION" ) ) } );
    tracker.Index().Register( cPtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "U1:VALUE" ) ) } );

    std::vector<EDA_ITEM*> hit;
    (void) tracker.AddInvalidateListener(
            [&]( EDA_ITEM* item, const TEXT_VAR_REF_KEY& ) { hit.push_back( item ); } );

    // A TITLE_BLOCK/SPECIAL-scoped fan-out must invalidate the title dep and
    // leave the cross-ref dep alone — those are driven by per-item commits.
    tracker.InvalidateByKind( { TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK,
                                TEXT_VAR_REF_KEY::KIND::SPECIAL } );

    BOOST_REQUIRE_EQUAL( hit.size(), 1u );
    BOOST_CHECK_EQUAL( hit[0], tPtr );
}


BOOST_AUTO_TEST_CASE( Index_GetRegisteredKeysEnumeratesAll )
{
    TEXT_VAR_DEPENDENCY_INDEX index;
    FakeItem                  a{ 1 };
    FakeItem                  b{ 2 };

    EDA_ITEM* aPtr = reinterpret_cast<EDA_ITEM*>( &a );
    EDA_ITEM* bPtr = reinterpret_cast<EDA_ITEM*>( &b );

    TEXT_VAR_REF_KEY k1 = TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) );
    TEXT_VAR_REF_KEY k2 = TEXT_VAR_REF_KEY::FromToken( wxT( "Y" ) );
    TEXT_VAR_REF_KEY k3 = TEXT_VAR_REF_KEY::FromToken( wxT( "Z" ) );

    index.Register( aPtr, { k1, k2 } );
    index.Register( bPtr, { k2, k3 } );

    std::vector<TEXT_VAR_REF_KEY> keys = index.GetRegisteredKeys();
    BOOST_CHECK_EQUAL( keys.size(), 3u );   // k2 is shared, counted once
}


BOOST_AUTO_TEST_CASE( Tracker_ClearDropsIndex )
{
    TEXT_VAR_TRACKER tracker;
    FakeItem         item{ 1 };
    EDA_ITEM*        itemPtr = reinterpret_cast<EDA_ITEM*>( &item );

    tracker.Index().Register( itemPtr, { TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) } );
    BOOST_CHECK_EQUAL( tracker.Index().ItemCount(), 1u );

    tracker.Clear();
    BOOST_CHECK_EQUAL( tracker.Index().ItemCount(), 0u );
}


BOOST_AUTO_TEST_SUITE_END()
