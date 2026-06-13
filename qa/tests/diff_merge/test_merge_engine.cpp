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

#include <diff_merge/kicad_merge_engine.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <nlohmann/json.hpp>


using namespace KICAD_DIFF;


namespace
{

ITEM_CHANGE makeAdded( const KIID& aUuid, const wxString& aType = wxS( "PCB_TRACK" ) )
{
    ITEM_CHANGE c;
    c.id.push_back( aUuid );
    c.typeName = aType;
    c.kind     = CHANGE_KIND::ADDED;
    return c;
}


ITEM_CHANGE makeRemoved( const KIID& aUuid, const wxString& aType = wxS( "PCB_TRACK" ) )
{
    ITEM_CHANGE c;
    c.id.push_back( aUuid );
    c.typeName = aType;
    c.kind     = CHANGE_KIND::REMOVED;
    return c;
}


ITEM_CHANGE makeModifiedWidth( const KIID& aUuid, int aBefore, int aAfter )
{
    ITEM_CHANGE c;
    c.id.push_back( aUuid );
    c.typeName = wxS( "PCB_TRACK" );
    c.kind     = CHANGE_KIND::MODIFIED;

    PROPERTY_DELTA d;
    d.name   = wxS( "Width" );
    d.before = DIFF_VALUE::FromInt( aBefore );
    d.after  = DIFF_VALUE::FromInt( aAfter );
    c.properties.push_back( std::move( d ) );

    return c;
}


ITEM_CHANGE makeModifiedTwoProps( const KIID& aUuid )
{
    ITEM_CHANGE c;
    c.id.push_back( aUuid );
    c.typeName = wxS( "FOOTPRINT" );
    c.kind     = CHANGE_KIND::MODIFIED;

    PROPERTY_DELTA p1;
    p1.name   = wxS( "Position X" );
    p1.before = DIFF_VALUE::FromInt( 0 );
    p1.after  = DIFF_VALUE::FromInt( 1000 );
    c.properties.push_back( p1 );

    PROPERTY_DELTA p2;
    p2.name   = wxS( "Reference" );
    p2.before = DIFF_VALUE::FromString( wxS( "R1" ) );
    p2.after  = DIFF_VALUE::FromString( wxS( "R2" ) );
    c.properties.push_back( p2 );

    return c;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MergeEngine )


BOOST_AUTO_TEST_CASE( OneSidedChangeAutoTakes )
{
    KIID::SeedGenerator( 200 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back( makeAdded( id ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_REQUIRE_EQUAL( plan.actions.size(), 1u );
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::TAKE_OURS );
    BOOST_CHECK( plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( BothSidesAddSameIdConflicts )
{
    KIID::SeedGenerator( 201 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back( makeAdded( id ) );
    theirs.changes.push_back( makeAdded( id ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_REQUIRE_EQUAL( plan.actions.size(), 1u );
    BOOST_CHECK( !plan.Resolved() );
    BOOST_CHECK_EQUAL( plan.ConflictCount(), 1u );
}


BOOST_AUTO_TEST_CASE( BothSidesDeleteSameIdAutoTakes )
{
    KIID::SeedGenerator( 202 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back( makeRemoved( id ) );
    theirs.changes.push_back( makeRemoved( id ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_REQUIRE_EQUAL( plan.actions.size(), 1u );
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::DELETE_ITEM );
    BOOST_CHECK( plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( DeleteOnOneSideModifyOnOtherConflicts )
{
    KIID::SeedGenerator( 203 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back( makeRemoved( id ) );
    theirs.changes.push_back( makeModifiedWidth( id, 100, 200 ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_REQUIRE_EQUAL( plan.actions.size(), 1u );
    BOOST_CHECK( !plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( OrthogonalPropertyEditsAutoMerge )
{
    KIID::SeedGenerator( 204 );
    KIID id;

    // Ours changes only "Position X"; Theirs changes only "Reference".
    // The two edits don't touch the same property — auto-merge.
    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    ITEM_CHANGE ourChange;
    ourChange.id.push_back( id );
    ourChange.typeName = wxS( "FOOTPRINT" );
    ourChange.kind     = CHANGE_KIND::MODIFIED;
    PROPERTY_DELTA p1;
    p1.name = wxS( "Position X" );
    p1.before = DIFF_VALUE::FromInt( 0 );
    p1.after  = DIFF_VALUE::FromInt( 1000 );
    ourChange.properties.push_back( p1 );
    ours.changes.push_back( ourChange );

    ITEM_CHANGE theirChange;
    theirChange.id.push_back( id );
    theirChange.typeName = wxS( "FOOTPRINT" );
    theirChange.kind     = CHANGE_KIND::MODIFIED;
    PROPERTY_DELTA p2;
    p2.name = wxS( "Reference" );
    p2.before = DIFF_VALUE::FromString( wxS( "R1" ) );
    p2.after  = DIFF_VALUE::FromString( wxS( "R2" ) );
    theirChange.properties.push_back( p2 );
    theirs.changes.push_back( theirChange );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_REQUIRE_EQUAL( plan.actions.size(), 1u );
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::MERGE_PROPS );
    BOOST_CHECK( plan.Resolved() );

    // Two property resolutions: Position X -> OURS, Reference -> THEIRS.
    BOOST_REQUIRE_EQUAL( plan.actions[0].props.size(), 2u );

    bool foundOurs = false, foundTheirs = false;

    for( const PROPERTY_RESOLUTION& p : plan.actions[0].props )
    {
        if( p.name == wxS( "Position X" ) && p.kind == PROP_RES::OURS )
            foundOurs = true;

        if( p.name == wxS( "Reference" ) && p.kind == PROP_RES::THEIRS )
            foundTheirs = true;
    }

    BOOST_CHECK( foundOurs );
    BOOST_CHECK( foundTheirs );
}


BOOST_AUTO_TEST_CASE( SamePropertyDifferentValuesConflicts )
{
    KIID::SeedGenerator( 205 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back(  makeModifiedWidth( id, 100, 200 ) );
    theirs.changes.push_back( makeModifiedWidth( id, 100, 300 ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_REQUIRE_EQUAL( plan.actions.size(), 1u );
    BOOST_CHECK( !plan.Resolved() );
    BOOST_CHECK( plan.actions[0].kind == ITEM_RES::MERGE_PROPS );
}


BOOST_AUTO_TEST_CASE( SamePropertyEqualValuesAutoMerges )
{
    KIID::SeedGenerator( 206 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back(  makeModifiedWidth( id, 100, 250 ) );
    theirs.changes.push_back( makeModifiedWidth( id, 100, 250 ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_CHECK( plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( ZoneEditMarksRequiresZoneRefill )
{
    KIID::SeedGenerator( 207 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    ITEM_CHANGE z;
    z.id.push_back( id );
    z.typeName = wxS( "ZONE" );
    z.kind     = CHANGE_KIND::MODIFIED;
    ours.changes.push_back( z );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_CHECK( plan.requiresZoneRefill );
}


BOOST_AUTO_TEST_CASE( TrackEditMarksConnectivityRebuild )
{
    KIID::SeedGenerator( 208 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    ITEM_CHANGE t;
    t.id.push_back( id );
    t.typeName = wxS( "PCB_TRACK" );
    t.kind     = CHANGE_KIND::MODIFIED;
    ours.changes.push_back( t );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_CHECK( plan.requiresConnectivityRebuild );
}


BOOST_AUTO_TEST_CASE( DuplicateUuidIsAlwaysConflict )
{
    KIID::SeedGenerator( 209 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    ITEM_CHANGE dup;
    dup.id.push_back( id );
    dup.typeName = wxS( "PCB_TRACK" );
    dup.kind     = CHANGE_KIND::DUPLICATE_UUID;
    ours.changes.push_back( dup );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_CHECK( !plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( MergePlanJsonRoundTrip )
{
    KIID::SeedGenerator( 210 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;
    ours.changes.push_back( makeAdded( id ) );
    theirs.changes.push_back( makeModifiedWidth( id, 100, 200 ) );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    nlohmann::json j    = plan.ToJson();
    MERGE_PLAN     back = MERGE_PLAN::FromJson( j );

    // Sizes alone could miss a swap of actions <-> unresolved or a content
    // drop — pin actual id and kind values.
    BOOST_REQUIRE_EQUAL( back.actions.size(), plan.actions.size() );
    for( std::size_t i = 0; i < plan.actions.size(); ++i )
    {
        BOOST_CHECK( back.actions[i].id   == plan.actions[i].id );
        BOOST_CHECK( back.actions[i].kind == plan.actions[i].kind );
    }

    BOOST_REQUIRE_EQUAL( back.unresolved.size(), plan.unresolved.size() );
    for( std::size_t i = 0; i < plan.unresolved.size(); ++i )
        BOOST_CHECK( back.unresolved[i] == plan.unresolved[i] );

    BOOST_CHECK_EQUAL( back.requiresZoneRefill,          plan.requiresZoneRefill );
    BOOST_CHECK_EQUAL( back.requiresConnectivityRebuild, plan.requiresConnectivityRebuild );
}


BOOST_AUTO_TEST_CASE( MergePlanFromJsonMissingRequiresFieldsDefaultsFalse )
{
    // The FromJson contract uses `.value("...", false)` for the two
    // requires* fields so plans serialized before those flags existed
    // load cleanly with conservative defaults. Pin that contract.
    nlohmann::json j;
    j["actions"]    = nlohmann::json::array();
    j["unresolved"] = nlohmann::json::array();

    MERGE_PLAN plan = MERGE_PLAN::FromJson( j );
    BOOST_CHECK( plan.actions.empty() );
    BOOST_CHECK( plan.unresolved.empty() );
    BOOST_CHECK( !plan.requiresZoneRefill );
    BOOST_CHECK( !plan.requiresConnectivityRebuild );
}


BOOST_AUTO_TEST_CASE( ItemResolutionJsonRoundTripAllKinds )
{
    // Every ITEM_RES enum value must round-trip via JSON. A regression in
    // ItemResToString / ItemResFromString would only show up here.
    KIID::SeedGenerator( 211 );
    KIID id;

    const ITEM_RES kinds[] = {
        ITEM_RES::TAKE_OURS,   ITEM_RES::TAKE_THEIRS, ITEM_RES::TAKE_ANCESTOR,
        ITEM_RES::MERGE_PROPS, ITEM_RES::DELETE_ITEM, ITEM_RES::KEEP,
    };

    for( ITEM_RES kind : kinds )
    {
        ITEM_RESOLUTION r;
        r.id   = KIID_PATH( id.AsString() );
        r.kind = kind;

        ITEM_RESOLUTION back = ITEM_RESOLUTION::FromJson( r.ToJson() );
        BOOST_CHECK( back == r );
    }
}


BOOST_AUTO_TEST_CASE( ItemResolutionJsonRoundTripCarriesProps )
{
    // MERGE_PROPS resolutions carry a `props` vector of PROPERTY_RESOLUTIONs
    // that needs to round-trip nested. A serializer that wrote `props`
    // but a reader that ignored them would still pass the kind-only round
    // trip above — pin the nested path here.
    KIID::SeedGenerator( 212 );
    KIID id;

    ITEM_RESOLUTION r;
    r.id   = KIID_PATH( id.AsString() );
    r.kind = ITEM_RES::MERGE_PROPS;

    PROPERTY_RESOLUTION pickOurs;
    pickOurs.name = wxS( "Width" );
    pickOurs.kind = PROP_RES::OURS;
    r.props.push_back( pickOurs );

    PROPERTY_RESOLUTION custom;
    custom.name        = wxS( "Layer" );
    custom.kind        = PROP_RES::CUSTOM;
    custom.customValue = DIFF_VALUE::FromInt( 42 );
    r.props.push_back( custom );

    ITEM_RESOLUTION back = ITEM_RESOLUTION::FromJson( r.ToJson() );
    BOOST_CHECK( back == r );
    BOOST_REQUIRE_EQUAL( back.props.size(), 2u );
    BOOST_CHECK( back.props[0] == pickOurs );
    BOOST_CHECK( back.props[1] == custom );
    // Pin the customValue payload directly. The `==` checks above rely on
    // PROPERTY_RESOLUTION::operator== — if equality stopped comparing
    // customValue, the nested CUSTOM payload could silently drop and the
    // round-trip would still appear to pass.
    BOOST_CHECK( back.props[1].customValue == DIFF_VALUE::FromInt( 42 ) );
}


BOOST_AUTO_TEST_CASE( PropertyResolutionJsonRoundTripAllKinds )
{
    // Every PROP_RES enum value round-trips AND pins its literal kind
    // string in the JSON. Round-trip-only symmetry (FromJson(ToJson()) ==)
    // misses paired serializer/parser regressions where both ends flip
    // to a new spelling — pin each literal explicitly.
    struct CASE
    {
        PROP_RES    kind;
        const char* literal;
    };

    const CASE kinds[] = {
        { PROP_RES::OURS,     "ours"     },
        { PROP_RES::THEIRS,   "theirs"   },
        { PROP_RES::ANCESTOR, "ancestor" },
        { PROP_RES::CUSTOM,   "custom"   },
    };

    for( const CASE& c : kinds )
    {
        BOOST_TEST_CONTEXT( c.literal )
        {
            PROPERTY_RESOLUTION r;
            r.name = wxS( "Width" );
            r.kind = c.kind;

            if( c.kind == PROP_RES::CUSTOM )
                r.customValue = DIFF_VALUE::FromInt( 42 );

            nlohmann::json j = r.ToJson();
            BOOST_CHECK_EQUAL( j.at( "kind" ).get<std::string>(), c.literal );
            BOOST_CHECK( PROPERTY_RESOLUTION::FromJson( j ) == r );

            if( c.kind == PROP_RES::CUSTOM )
            {
                BOOST_CHECK( PROPERTY_RESOLUTION::FromJson( j ).customValue
                             == DIFF_VALUE::FromInt( 42 ) );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( EmptyDiffsProduceEmptyPlan )
{
    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_CHECK( plan.actions.empty() );
    BOOST_CHECK( plan.Resolved() );
    BOOST_CHECK( !plan.requiresZoneRefill );
    BOOST_CHECK( !plan.requiresConnectivityRebuild );
}


BOOST_AUTO_TEST_CASE( ChildChangesAreIndexedRecursively )
{
    KIID::SeedGenerator( 220 );
    KIID parentId, childId;

    // Parent footprint MODIFIED with one child PAD also MODIFIED on ours.
    // Theirs MODIFIEs the same child PAD — without recursive indexing the
    // engine would only see the parent collision and miss the child conflict.
    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    ITEM_CHANGE ourParent;
    ourParent.id.push_back( parentId );
    ourParent.typeName = wxS( "FOOTPRINT" );
    ourParent.kind     = CHANGE_KIND::MODIFIED;

    ITEM_CHANGE ourChild;
    ourChild.id.push_back( parentId );
    ourChild.id.push_back( childId );
    ourChild.typeName = wxS( "PAD" );
    ourChild.kind     = CHANGE_KIND::MODIFIED;
    PROPERTY_DELTA d1;
    d1.name   = wxS( "Number" );
    d1.before = DIFF_VALUE::FromString( wxS( "1" ) );
    d1.after  = DIFF_VALUE::FromString( wxS( "A" ) );
    ourChild.properties.push_back( d1 );
    ourParent.children.push_back( ourChild );

    ours.changes.push_back( ourParent );

    ITEM_CHANGE theirParent = ourParent;
    theirParent.children.clear();
    ITEM_CHANGE theirChild = ourChild;
    theirChild.properties.clear();
    PROPERTY_DELTA d2;
    d2.name   = wxS( "Number" );
    d2.before = DIFF_VALUE::FromString( wxS( "1" ) );
    d2.after  = DIFF_VALUE::FromString( wxS( "B" ) );
    theirChild.properties.push_back( d2 );
    theirParent.children.push_back( theirChild );

    theirs.changes.push_back( theirParent );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    // The child's KIID_PATH should appear in plan.actions because the engine
    // indexed nested children. The actual property values differ, so it should
    // be flagged unresolved.
    bool foundChild = false;

    for( const ITEM_RESOLUTION& r : plan.actions )
    {
        if( r.id.size() == 2 && r.id.back() == childId )
            foundChild = true;
    }

    BOOST_CHECK( foundChild );
    BOOST_CHECK( !plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( PreferAutoMergeFalseConflictsOrthogonalEdits )
{
    KIID::SeedGenerator( 221 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    // Orthogonal property edits — would auto-merge by default, but
    // preferAutoMerge=false should treat them as conflicts.
    ITEM_CHANGE o;
    o.id.push_back( id );
    o.typeName = wxS( "FOOTPRINT" );
    o.kind     = CHANGE_KIND::MODIFIED;
    PROPERTY_DELTA d1;
    d1.name   = wxS( "Position X" );
    d1.before = DIFF_VALUE::FromInt( 0 );
    d1.after  = DIFF_VALUE::FromInt( 1000 );
    o.properties.push_back( d1 );
    ours.changes.push_back( o );

    ITEM_CHANGE t;
    t.id.push_back( id );
    t.typeName = wxS( "FOOTPRINT" );
    t.kind     = CHANGE_KIND::MODIFIED;
    PROPERTY_DELTA d2;
    d2.name   = wxS( "Reference" );
    d2.before = DIFF_VALUE::FromString( wxS( "R1" ) );
    d2.after  = DIFF_VALUE::FromString( wxS( "R2" ) );
    t.properties.push_back( d2 );
    theirs.changes.push_back( t );

    KICAD_MERGE_ENGINE::OPTIONS opts;
    opts.preferAutoMerge = false;
    KICAD_MERGE_ENGINE engine( opts );

    MERGE_PLAN plan = engine.Plan( ours, theirs );
    BOOST_CHECK( !plan.Resolved() );
}


BOOST_AUTO_TEST_CASE( EmptyPropertyDeltasOnBothSidesConflicts )
{
    KIID::SeedGenerator( 222 );
    KIID id;

    DOCUMENT_DIFF ours;
    DOCUMENT_DIFF theirs;

    // MODIFIED records without any property deltas — change came from
    // operator== alone. Engine must NOT silently auto-resolve.
    ITEM_CHANGE c1;
    c1.id.push_back( id );
    c1.typeName = wxS( "PCB_TRACK" );
    c1.kind     = CHANGE_KIND::MODIFIED;
    ours.changes.push_back( c1 );

    ITEM_CHANGE c2 = c1;
    theirs.changes.push_back( c2 );

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( ours, theirs );

    BOOST_CHECK( !plan.Resolved() );
}


// ---------------------------------------------------------------------------
// ResolvePropertyConflict
// ---------------------------------------------------------------------------

namespace
{

PROPERTY_DELTA MakeDelta( const wxString& aName, int aBefore, int aAfter )
{
    PROPERTY_DELTA d;
    d.name   = aName;
    d.before = DIFF_VALUE::FromInt( aBefore );
    d.after  = DIFF_VALUE::FromInt( aAfter );
    return d;
}


KICAD_MERGE_ENGINE::OPTIONS DefaultOptions()
{
    KICAD_MERGE_ENGINE::OPTIONS o;
    return o;   // preferAutoMerge=true, autoResolveEqualValues=true
}


KICAD_MERGE_ENGINE::OPTIONS StrictOptions()
{
    KICAD_MERGE_ENGINE::OPTIONS o;
    o.preferAutoMerge        = false;
    o.autoResolveEqualValues = false;
    return o;
}

} // namespace


BOOST_AUTO_TEST_CASE( ResolveProperty_OnlyOurs_ReturnsOursClean )
{
    PROPERTY_DELTA ours = MakeDelta( wxS( "Width" ), 10, 20 );
    auto outcome = ResolvePropertyConflict( &ours, nullptr, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( !outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_OnlyTheirs_ReturnsTheirsClean )
{
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 30 );
    auto outcome = ResolvePropertyConflict( nullptr, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::THEIRS );
    BOOST_CHECK( !outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_BothNull_PreconditionViolation )
{
    // Both sides null is a precondition violation. CHECK_WX_ASSERT keys off
    // wxDEBUG_LEVEL — when assertions are compiled in (the normal QA build)
    // it expands to BOOST_CHECK_THROW( ..., WX_ASSERT_ERROR ); when stripped
    // (wxDEBUG_LEVEL == 0) it expands to nothing. In the latter case we
    // still want to pin the deterministic OURS fallback so the contract is
    // verified end-to-end regardless of build configuration.
    CHECK_WX_ASSERT( ResolvePropertyConflict( nullptr, nullptr, DefaultOptions() ) );

#if wxDEBUG_LEVEL == 0
    auto outcome = ResolvePropertyConflict( nullptr, nullptr, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( !outcome.isUnresolved );
#endif
}


BOOST_AUTO_TEST_CASE( ResolveProperty_BothSameAfter_AutoResolvesEqual )
{
    // Both sides changed to the same final value — autoResolveEqualValues
    // picks OURS without flagging unresolved.
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 25 );
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 25 );

    auto outcome = ResolvePropertyConflict( &ours, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( !outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_BothSameAfter_StrictReportsConflict )
{
    // With autoResolveEqualValues=false, equal end values still conflict.
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 25 );
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 25 );

    auto outcome = ResolvePropertyConflict( &ours, &theirs, StrictOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_OursNoOp_TakesTheirs )
{
    // Ours' "after" equals its "before" — a no-op edit. With preferAutoMerge,
    // theirs (which actually changed) wins.
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 10 );
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 30 );

    auto outcome = ResolvePropertyConflict( &ours, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::THEIRS );
    BOOST_CHECK( !outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_TheirsNoOp_TakesOurs )
{
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 30 );
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 10 );

    auto outcome = ResolvePropertyConflict( &ours, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( !outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_NoOpDetectionRequiresPreferAutoMerge )
{
    // Even when ours is a no-op, strict options should report a conflict.
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 10 );
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 30 );

    auto outcome = ResolvePropertyConflict( &ours, &theirs, StrictOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_TheirsNoOpRequiresMatchingBefore )
{
    // Mirror of OursNoOpRequiresMatchingBefore — theirs is a no-op against a
    // different baseline. Without the matching-before check on the theirs-
    // no-op branch, this case would silently take OURS (a stale baseline on
    // theirs would override a real edit on ours).
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 30 );  // changed
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 5, 5 );    // no-op, stale before

    auto outcome = ResolvePropertyConflict( &ours, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_OursNoOpRequiresMatchingBefore )
{
    // Ours' before/after are equal (no-op), but ours' before differs from
    // theirs' before — the "ours didn't really change" branch is gated on
    // matching before values, so this falls through to the conflict path.
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 5, 5 );    // no-op
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 30 );  // changed

    auto outcome = ResolvePropertyConflict( &ours, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.isUnresolved );
}


BOOST_AUTO_TEST_CASE( ResolveProperty_BothDivergent_UnresolvedConflict )
{
    PROPERTY_DELTA ours   = MakeDelta( wxS( "Width" ), 10, 20 );
    PROPERTY_DELTA theirs = MakeDelta( wxS( "Width" ), 10, 30 );

    auto outcome = ResolvePropertyConflict( &ours, &theirs, DefaultOptions() );
    BOOST_CHECK( outcome.kind == PROP_RES::OURS );
    BOOST_CHECK( outcome.isUnresolved );
}


BOOST_AUTO_TEST_SUITE_END()
