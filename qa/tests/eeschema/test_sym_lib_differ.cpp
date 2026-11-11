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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <diff_merge/sym_item_diff.h>
#include <diff_merge/sym_lib_differ.h>
#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/sch_geometry_extractor.h>

#include <lib_symbol.h>
#include <sch_item.h>
#include <sch_pin.h>
#include <sch_shape.h>

#include <json_common.h>

#include <wx/filename.h>

#include <functional>
#include <set>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( SymLibDiffer )


static wxString getFixturePath()
{
    // qa/data/eeschema/.. -> qa/data/libraries/Device.kicad_sym
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir(), wxEmptyString );
    fn.RemoveLastDir();
    fn.AppendDir( wxS( "libraries" ) );
    fn.SetFullName( wxS( "Device.kicad_sym" ) );
    return fn.GetFullPath();
}


BOOST_AUTO_TEST_CASE( IdenticalLibrariesDiffEmpty )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !mapA.empty() );

    SYM_LIB_DIFFER differ( mapA, mapB, wxS( "Device.kicad_sym" ) );
    DOCUMENT_DIFF  result = differ.Diff();

    BOOST_CHECK( result.Empty() );
    BOOST_CHECK_EQUAL( result.docType.ToStdString(), "kicad_sym" );
}


BOOST_AUTO_TEST_CASE( SymbolNameOnlyInBeforeShowsRemoved )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    // Drop one symbol from after to simulate removal.
    BOOST_REQUIRE( !mapB.empty() );
    wxString victimName = mapB.begin()->first;
    mapB.erase( mapB.begin() );

    SYM_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF  result = differ.Diff();

    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::REMOVED );
    BOOST_CHECK_EQUAL( result.changes[0].typeName.ToStdString(), "LIB_SYMBOL" );
    // Identity for library symbols is name-derived; the KIID_PATH carries a
    // synthetic KIID hash of the name rather than a registered UUID. We
    // only assert the change exists, not the exact id encoding.
    (void) victimName;
}


BOOST_AUTO_TEST_CASE( SymbolChangesCarryDrawableBBoxes )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !ownersB.empty() );
    LIB_SYMBOL* subject = ownersB.front().get();
    BOOST_REQUIRE( !subject->GetDrawItems().empty() );

    SCH_ITEM& firstItem = *subject->GetDrawItems().begin();
    firstItem.Move( VECTOR2I( 1000, 0 ) );

    SYM_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF  result = differ.Diff();

    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::MODIFIED );
    BOOST_CHECK_GT( result.changes[0].bbox.GetWidth(), 0 );
    BOOST_CHECK_GT( result.changes[0].bbox.GetHeight(), 0 );
}


BOOST_AUTO_TEST_CASE( ExtractSymbolGeometryProducesDrawableContext )
{
    auto [owners, map] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !owners.empty() );

    DOCUMENT_GEOMETRY geometry = ExtractSymbolGeometry( *owners.front(), KIGFX::COLOR4D( 0.38, 0.38, 0.38, 0.55 ) );

    BOOST_CHECK( !geometry.Empty() );
    BOOST_CHECK( BBoxFromGeometry( geometry ).has_value() );
}


BOOST_AUTO_TEST_CASE( DiffJsonRoundTrip )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    if( !mapB.empty() )
        mapB.erase( mapB.begin() );

    SYM_LIB_DIFFER differ( mapA, mapB, wxS( "lib.kicad_sym" ) );
    DOCUMENT_DIFF  result = differ.Diff();

    nlohmann::json j = result.ToJson();
    DOCUMENT_DIFF  back = DOCUMENT_DIFF::FromJson( j );
    BOOST_CHECK_EQUAL( back.changes.size(), result.changes.size() );
    BOOST_CHECK_EQUAL( back.docType.ToStdString(), "kicad_sym" );
}


// Library item KIID_PATH must be deterministic across calls -- LibraryItemKiidPath
// is a hash-based synthetic UUID, and a regression that swaps the hash
// algorithm would silently desync differ <-> applier KIID matching.
BOOST_AUTO_TEST_CASE( LibraryItemKiidPathIsDeterministicForSameName )
{
    KIID_PATH a = LibraryItemKiidPath( wxS( "Device:R" ) );
    KIID_PATH b = LibraryItemKiidPath( wxS( "Device:R" ) );

    BOOST_REQUIRE_EQUAL( a.size(), 1u );
    BOOST_REQUIRE_EQUAL( b.size(), 1u );
    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_CASE( LibraryItemKiidPathDiffersForDifferentNames )
{
    KIID_PATH a = LibraryItemKiidPath( wxS( "Device:R" ) );
    KIID_PATH b = LibraryItemKiidPath( wxS( "Device:C" ) );

    BOOST_CHECK( !( a == b ) );
}


// Multi-symbol library: every symbol must surface as its own change record
// when removed.  Currently the smoke test removes one symbol; here we
// remove three and verify all three changes are emitted.
BOOST_AUTO_TEST_CASE( MultipleSymbolRemovalsAllEmitChanges )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE_GE( mapB.size(), 3u );

    std::vector<wxString> victims;

    for( int i = 0; i < 3 && !mapB.empty(); ++i )
    {
        victims.push_back( mapB.begin()->first );
        mapB.erase( mapB.begin() );
    }

    SYM_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF  result = differ.Diff();

    BOOST_CHECK_EQUAL( result.changes.size(), victims.size() );

    for( const ITEM_CHANGE& c : result.changes )
    {
        BOOST_CHECK( c.kind == CHANGE_KIND::REMOVED );
        BOOST_CHECK_EQUAL( c.typeName.ToStdString(), "LIB_SYMBOL" );
    }
}


// Output ordering must be deterministic for the same input pair so JSON
// diffs are byte-stable across runs.
BOOST_AUTO_TEST_CASE( OutputOrderingIsDeterministicAcrossRuns )
{
    auto [ownersA1, mapA1] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB1, mapB1] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersA2, mapA2] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB2, mapB2] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE_GE( mapB1.size(), 2u );

    // Same mutation on both run pairs.
    mapB1.erase( mapB1.begin() );
    mapB2.erase( mapB2.begin() );

    SYM_LIB_DIFFER differ1( mapA1, mapB1 );
    SYM_LIB_DIFFER differ2( mapA2, mapB2 );

    DOCUMENT_DIFF r1 = differ1.Diff();
    DOCUMENT_DIFF r2 = differ2.Diff();

    BOOST_CHECK_EQUAL( r1.ToJson().dump(), r2.ToJson().dump() );
}


BOOST_AUTO_TEST_CASE( ModifiedSymbolCarriesPinChildDelta )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    LIB_SYMBOL* victim = nullptr;

    for( const std::unique_ptr<LIB_SYMBOL>& owner : ownersB )
    {
        if( owner && !owner->IsDerived() && !owner->GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ).empty() )
        {
            victim = owner.get();
            break;
        }
    }

    BOOST_REQUIRE( victim );

    victim->GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ).front()->Move( VECTOR2I( 1270, 0 ) );

    SYM_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF  result = differ.Diff();

    const ITEM_CHANGE* change = nullptr;

    for( const ITEM_CHANGE& c : result.changes )
    {
        if( c.kind == CHANGE_KIND::MODIFIED && c.refdes && *c.refdes == victim->GetName() )
        {
            change = &c;
            break;
        }
    }

    BOOST_REQUIRE( change );

    bool foundPinChild = false;

    for( const ITEM_CHANGE& child : change->children )
    {
        if( child.typeName == wxS( "Pin" ) && !child.properties.empty() )
            foundPinChild = true;
    }

    BOOST_CHECK( foundPinChild );
}


// Symbol compare ignores unit and body style names, so only the deltas show a rename
BOOST_AUTO_TEST_CASE( UnitAndBodyStyleNamesAreDetected )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    auto victim = std::ranges::find_if( ownersB,
                                        []( const std::unique_ptr<LIB_SYMBOL>& aOwner )
                                        {
                                            return aOwner && !aOwner->IsDerived();
                                        } );

    BOOST_REQUIRE( victim != ownersB.end() );

    ( *victim )->GetUnitDisplayNames()[1] = wxS( "QA_Unit" );
    ( *victim )->SetBodyStyleNames( { wxS( "QA_Normal" ), wxS( "QA_Alternate" ) } );

    std::set<wxString> names;

    for( const ITEM_CHANGE& change : SYM_LIB_DIFFER( mapA, mapB ).Diff().changes )
    {
        for( const PROPERTY_DELTA& delta : change.properties )
            names.insert( delta.name );
    }

    BOOST_CHECK( names.contains( wxS( "Unit Names" ) ) );
    BOOST_CHECK( names.contains( wxS( "Body Style Names" ) ) );
}


BOOST_AUTO_TEST_CASE( PinNameChangeIsDetected )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    LIB_SYMBOL* victim = nullptr;

    for( const std::unique_ptr<LIB_SYMBOL>& owner : ownersB )
    {
        if( owner && !owner->IsDerived() && !owner->GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ).empty() )
        {
            victim = owner.get();
            break;
        }
    }

    BOOST_REQUIRE( victim );

    victim->GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ).front()->SetName( wxS( "QA_RENAMED_PIN" ) );

    SYM_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF  result = differ.Diff();

    const ITEM_CHANGE* change = nullptr;

    for( const ITEM_CHANGE& c : result.changes )
    {
        if( c.kind == CHANGE_KIND::MODIFIED && c.refdes && *c.refdes == victim->GetName() )
        {
            change = &c;
            break;
        }
    }

    BOOST_REQUIRE( change );

    bool foundPinDelta = false;

    for( const ITEM_CHANGE& child : change->children )
    {
        if( child.typeName == wxS( "Pin" ) && !child.properties.empty() )
            foundPinDelta = true;
    }

    BOOST_CHECK( foundPinDelta );
}


BOOST_AUTO_TEST_CASE( AddedGraphicYieldsSingleAddedElement )
{
    auto [ownersA, mapA] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    LIB_SYMBOL* victim = nullptr;

    for( const std::unique_ptr<LIB_SYMBOL>& owner : ownersB )
    {
        if( owner && !owner->IsDerived() )
        {
            victim = owner.get();
            break;
        }
    }

    BOOST_REQUIRE( victim );

    SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 5080, 5080 ) );
    rect->SetEnd( VECTOR2I( 7620, 7620 ) );
    victim->AddDrawItem( rect );

    SYM_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF  result = differ.Diff();

    const ITEM_CHANGE* change = nullptr;

    for( const ITEM_CHANGE& c : result.changes )
    {
        if( c.kind == CHANGE_KIND::MODIFIED && c.refdes && *c.refdes == victim->GetName() )
        {
            change = &c;
            break;
        }
    }

    BOOST_REQUIRE( change );

    int added = 0;
    int removed = 0;
    int modified = 0;

    for( const ITEM_CHANGE& child : change->children )
    {
        if( child.kind == CHANGE_KIND::ADDED )
            added++;
        else if( child.kind == CHANGE_KIND::REMOVED )
            removed++;
        else if( child.kind == CHANGE_KIND::MODIFIED )
            modified++;
    }

    BOOST_CHECK_EQUAL( added, 1 );
    BOOST_CHECK_EQUAL( removed, 0 );
    BOOST_CHECK_EQUAL( modified, 0 );
}


// Edit the index'th plain symbol with pins, the same symbol in every load of one library
static void editSymbol( const std::vector<std::unique_ptr<LIB_SYMBOL>>& aOwners, size_t aIndex,
                        const std::function<void( LIB_SYMBOL& )>& aEdit )
{
    for( const std::unique_ptr<LIB_SYMBOL>& owner : aOwners )
    {
        if( !owner || owner->IsDerived() || owner->GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ).empty() )
            continue;

        if( aIndex-- == 0 )
        {
            aEdit( *owner );
            return;
        }
    }

    BOOST_FAIL( "the library has too few symbols with pins" );
}


// A merge that drops either side's edit to a symbol the other side left alone must not pass for clean
BOOST_AUTO_TEST_CASE( MergeKeepsBothSidesRejectsADroppedEdit )
{
    auto [ancestorOwners, ancestor] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [oursOwners, ours] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [theirsOwners, theirs] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [mergedOwners, merged] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    auto movePin = []( LIB_SYMBOL& aSymbol )
    {
        aSymbol.GetGraphicalPins( ALL_UNITS, ALL_BODY_STYLES ).front()->Move( VECTOR2I( 1270, 0 ) );
    };

    // Footprint filters are neither a registered property nor an element
    auto setFilters = []( LIB_SYMBOL& aSymbol )
    {
        wxString str_filter( "QA_Filter_*" );
        aSymbol.SetFPFilters( wxArrayString( 1, &str_filter ) );
    };

    editSymbol( oursOwners, 0, movePin );
    editSymbol( theirsOwners, 1, setFilters );
    editSymbol( mergedOwners, 0, movePin );
    editSymbol( mergedOwners, 1, setFilters );

    BOOST_CHECK( SYM_LIB_DIFFER::MergeKeepsBothSides( ancestor, ours, theirs, merged ) );
    BOOST_CHECK( !SYM_LIB_DIFFER::MergeKeepsBothSides( ancestor, ours, theirs, ours ) );
}


// State the change record misses must still block the combine, or the merge drops it silently
BOOST_AUTO_TEST_CASE( CombineSymbolEditsRejectsUnreportedState )
{
    auto [owners, symbols] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );
    const LIB_SYMBOL* ancestor = nullptr;

    for( const auto& [name, symbol] : symbols )
    {
        if( !symbol->IsDerived() )
        {
            ancestor = symbol;
            break;
        }
    }

    BOOST_REQUIRE( ancestor );

    LIB_SYMBOL ours( *ancestor );
    LIB_SYMBOL theirs( *ancestor );

    ours.GetDatasheetField().SetText( wxS( "https://example.com/qa.pdf" ) );
    theirs.GetFootprintField().SetText( wxS( "QA:Footprint" ) );

    auto fieldEdit = []( const wxString& aField )
    {
        ITEM_CHANGE child;
        child.typeName = _( "Field" );
        child.refdes = aField;

        ITEM_CHANGE change;
        change.children.push_back( std::move( child ) );
        return change;
    };

    const ITEM_CHANGE ourChange = fieldEdit( wxS( "Datasheet" ) );
    const ITEM_CHANGE theirChange = fieldEdit( wxS( "Footprint" ) );

    std::unique_ptr<LIB_SYMBOL> merged = CombineSymbolEdits( *ancestor, ours, theirs, ourChange, theirChange );

    BOOST_REQUIRE( merged );
    BOOST_CHECK_EQUAL( merged->GetFootprintField().GetText(), wxS( "QA:Footprint" ) );

    theirs.SetBodyStyleNames( { wxS( "QA_Normal" ), wxS( "QA_Alternate" ) } );

    BOOST_CHECK( !CombineSymbolEdits( *ancestor, ours, theirs, ourChange, theirChange ) );
}


// A lower unit count from one side must not combine with an edit the other side makes to a dropped unit
BOOST_AUTO_TEST_CASE( CombineSymbolEditsKeepsTheOtherSidesEdits )
{
    auto [owners, symbols] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    auto plain = std::ranges::find_if( owners,
                                       []( const std::unique_ptr<LIB_SYMBOL>& aOwner )
                                       {
                                           return aOwner && !aOwner->IsDerived() && aOwner->GetUnitCount() == 1;
                                       } );

    BOOST_REQUIRE( plain != owners.end() );

    LIB_SYMBOL ancestor( **plain );
    ancestor.SetUnitCount( 2, false );

    LIB_SYMBOL ours( ancestor );
    LIB_SYMBOL theirs( ancestor );

    SCH_PIN* pin = new SCH_PIN( &ours );
    pin->SetUnit( 2 );
    pin->SetNumber( wxS( "QA" ) );
    ours.AddDrawItem( pin );

    theirs.SetUnitCount( 1, false );

    const wxString name = ancestor.GetName();

    auto changeTo = [&]( const LIB_SYMBOL& aAfter )
    {
        DOCUMENT_DIFF diff = SYM_LIB_DIFFER( { { name, &ancestor } }, { { name, &aAfter } } ).Diff();
        BOOST_REQUIRE_EQUAL( diff.changes.size(), 1 );
        return diff.changes.front();
    };

    const ITEM_CHANGE ourChange = changeTo( ours );
    const ITEM_CHANGE theirChange = changeTo( theirs );

    BOOST_CHECK( !CombineSymbolEdits( ancestor, ours, theirs, ourChange, theirChange ) );

    // The unit count keeps a dropped unit's name in memory but not in the file
    LIB_SYMBOL named( ancestor );
    named.GetUnitDisplayNames()[2] = wxS( "QA_Unit" );

    BOOST_CHECK( !CombineSymbolEdits( ancestor, named, theirs, changeTo( named ), theirChange ) );
}


BOOST_AUTO_TEST_CASE( DerivedSymbolKeepsResolvedParentAfterLoad )
{
    auto [owners, map] = SYM_LIB_DIFFER::LoadLibrary( getFixturePath() );

    int checked = 0;

    for( const std::unique_ptr<LIB_SYMBOL>& sym : owners )
    {
        if( sym->GetParentName().IsEmpty() )
            continue;

        BOOST_TEST_INFO( sym->GetName().ToStdString() );
        BOOST_CHECK( sym->IsDerived() );

        std::shared_ptr<LIB_SYMBOL> parent = sym->GetParent().lock();
        BOOST_REQUIRE( parent );
        BOOST_CHECK_EQUAL( parent->GetName(), sym->GetParentName() );

        checked++;
    }

    BOOST_CHECK( checked > 0 );
}


BOOST_AUTO_TEST_SUITE_END()
