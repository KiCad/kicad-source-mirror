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

#include <diff_merge/diff_renderer_plotter.h>
#include <diff_merge/diff_scene.h>
#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/kicad_merge_engine.h>

#include <algorithm>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( DiffSceneHelpers )


// CategoryFor / PAINT_ORDER coverage ----------------------------------------

BOOST_AUTO_TEST_CASE( CategoryForMapsAllChangeKinds )
{
    BOOST_CHECK( CategoryFor( CHANGE_KIND::ADDED ) == CATEGORY::ADDED );
    BOOST_CHECK( CategoryFor( CHANGE_KIND::REMOVED ) == CATEGORY::REMOVED );
    BOOST_CHECK( CategoryFor( CHANGE_KIND::MODIFIED ) == CATEGORY::MODIFIED );
    BOOST_CHECK( CategoryFor( CHANGE_KIND::COLLISION ) == CATEGORY::CONFLICT );
    BOOST_CHECK( CategoryFor( CHANGE_KIND::DUPLICATE_UUID ) == CATEGORY::CONFLICT );
}


BOOST_AUTO_TEST_CASE( PaintOrderIsExhaustiveAndUnique )
{
    BOOST_CHECK_EQUAL( PAINT_ORDER.size(), CATEGORY_COUNT );

    std::set<CATEGORY> seen( PAINT_ORDER.begin(), PAINT_ORDER.end() );
    BOOST_CHECK_EQUAL( seen.size(), CATEGORY_COUNT );
}


// ShapesFor ------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( ShapesForReturnsCorrectListPerCategory )
{
    DIFF_SCENE scene;

    SCENE_SHAPE added;
    added.label = wxS( "a" );
    scene.addedShapes.push_back( added );
    SCENE_SHAPE removed;
    removed.label = wxS( "r" );
    scene.removedShapes.push_back( removed );
    SCENE_SHAPE modified;
    modified.label = wxS( "m" );
    scene.modifiedShapes.push_back( modified );
    SCENE_SHAPE conflict;
    conflict.label = wxS( "c" );
    scene.conflictShapes.push_back( conflict );

    BOOST_CHECK( ShapesFor( scene, CATEGORY::ADDED ).at( 0 ).label == wxS( "a" ) );
    BOOST_CHECK( ShapesFor( scene, CATEGORY::REMOVED ).at( 0 ).label == wxS( "r" ) );
    BOOST_CHECK( ShapesFor( scene, CATEGORY::MODIFIED ).at( 0 ).label == wxS( "m" ) );
    BOOST_CHECK( ShapesFor( scene, CATEGORY::CONFLICT ).at( 0 ).label == wxS( "c" ) );
}


// MakeBBoxOutline ------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MakeBBoxOutlineDegenerateBoxIsEmpty )
{
    DOCUMENT_POLYGON poly = MakeBBoxOutline( BOX2I(), KIGFX::COLOR4D::WHITE );
    BOOST_CHECK( poly.outline.empty() );
}


BOOST_AUTO_TEST_CASE( MakeBBoxOutlineFourCornersInOrder )
{
    const BOX2I          box( VECTOR2I( 10, 20 ), VECTOR2I( 100, 50 ) );
    const KIGFX::COLOR4D color( 0.1, 0.2, 0.3, 1.0 );

    DOCUMENT_POLYGON poly = MakeBBoxOutline( box, color, /*lineWidth*/ 3 );

    BOOST_CHECK_EQUAL( poly.outline.size(), 4 );
    BOOST_CHECK( poly.outline[0] == box.GetOrigin() );
    BOOST_CHECK( poly.outline[2] == box.GetEnd() );
    BOOST_CHECK_EQUAL( poly.lineWidth, 3 );
    BOOST_CHECK( !poly.filled );
    BOOST_CHECK( poly.color == color );
}


// Shared presentation helpers -----------------------------------------------

BOOST_AUTO_TEST_CASE( ChangeDisplayLabelIncludesRefdesWhenPresent )
{
    ITEM_CHANGE change;
    change.typeName = wxS( "FOOTPRINT" );
    change.refdes = wxS( "U7" );

    BOOST_CHECK( ChangeDisplayLabel( change ) == wxS( "FOOTPRINT [U7]" ) );
}


BOOST_AUTO_TEST_CASE( ChangeDisplayLabelFallsBackToTypeName )
{
    ITEM_CHANGE change;
    change.typeName = wxS( "ZONE" );

    BOOST_CHECK( ChangeDisplayLabel( change ) == wxS( "ZONE" ) );
}


BOOST_AUTO_TEST_CASE( IsRoutingNetChangeRequiresRoutingTypeAndRefdes )
{
    ITEM_CHANGE track;
    track.typeName = wxS( "PCB_TRACK" );
    track.refdes = wxS( "GND" );
    BOOST_CHECK( IsRoutingNetChange( track ) );

    ITEM_CHANGE pad;
    pad.typeName = wxS( "PAD" );
    pad.refdes = wxS( "GND" );
    BOOST_CHECK( !IsRoutingNetChange( pad ) );

    ITEM_CHANGE noNet;
    noNet.typeName = wxS( "PCB_VIA" );
    BOOST_CHECK( !IsRoutingNetChange( noNet ) );
}


BOOST_AUTO_TEST_CASE( AppendGeometryMovesEveryPrimitiveKind )
{
    DOCUMENT_GEOMETRY dst;
    DOCUMENT_GEOMETRY src;

    src.segments.push_back( DOCUMENT_SEGMENT{} );
    src.polygons.push_back( DOCUMENT_POLYGON{} );
    src.circles.push_back( DOCUMENT_CIRCLE{} );

    AppendGeometry( dst, std::move( src ) );

    BOOST_CHECK_EQUAL( dst.segments.size(), 1u );
    BOOST_CHECK_EQUAL( dst.polygons.size(), 1u );
    BOOST_CHECK_EQUAL( dst.circles.size(), 1u );
}


// HighlightedBBox -----------------------------------------------------------

namespace
{

SCENE_SHAPE makeShape( const wxString& aId, const BOX2I& aBox )
{
    SCENE_SHAPE s;
    s.changeId = KIID_PATH( aId );
    s.bbox = aBox;
    return s;
}

} // namespace


BOOST_AUTO_TEST_CASE( HighlightedBBoxFindsAcrossCategories )
{
    DIFF_SCENE scene;
    scene.addedShapes.push_back( makeShape( wxS( "/aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa" ),
                                            BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) ) );
    scene.modifiedShapes.push_back( makeShape( wxS( "/bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb" ),
                                               BOX2I( VECTOR2I( 100, 100 ), VECTOR2I( 10, 10 ) ) ) );

    std::array<bool, CATEGORY_COUNT> allVisible{ { true, true, true, true } };

    auto found = HighlightedBBox( scene, KIID_PATH( wxS( "/bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb" ) ), allVisible );

    BOOST_REQUIRE( found.has_value() );
    BOOST_CHECK( found->Contains( VECTOR2I( 105, 105 ) ) );
}


BOOST_AUTO_TEST_CASE( HighlightedBBoxIgnoresHiddenCategory )
{
    DIFF_SCENE scene;
    scene.addedShapes.push_back( makeShape( wxS( "/aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa" ),
                                            BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) ) );

    // ADDED hidden, others visible.
    std::array<bool, CATEGORY_COUNT> visible{ { false, true, true, true } };

    auto found = HighlightedBBox( scene, KIID_PATH( wxS( "/aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa" ) ), visible );

    BOOST_CHECK( !found.has_value() );
}


BOOST_AUTO_TEST_CASE( HighlightedBBoxUnionsMultipleMatches )
{
    DIFF_SCENE scene;

    // Same changeId on two shapes — e.g. a DUPLICATE_UUID reported twice with
    // bboxes at different locations. The helper should union them.
    const wxString id = wxS( "/cccccccc-cccc-cccc-cccc-cccccccccccc" );
    scene.conflictShapes.push_back( makeShape( id, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) ) );
    scene.conflictShapes.push_back( makeShape( id, BOX2I( VECTOR2I( 100, 100 ), VECTOR2I( 10, 10 ) ) ) );

    std::array<bool, CATEGORY_COUNT> allVisible{ { true, true, true, true } };
    auto                             found = HighlightedBBox( scene, KIID_PATH( id ), allVisible );

    BOOST_REQUIRE( found.has_value() );
    // Union must contain both centers.
    BOOST_CHECK( found->Contains( VECTOR2I( 5, 5 ) ) );
    BOOST_CHECK( found->Contains( VECTOR2I( 105, 105 ) ) );
}


// CollectChangeBBoxes -------------------------------------------------------

BOOST_AUTO_TEST_CASE( CollectChangeBBoxesFlatList )
{
    DOCUMENT_DIFF diff;

    ITEM_CHANGE c1;
    c1.id = KIID_PATH( wxS( "/11111111-1111-1111-1111-111111111111" ) );
    c1.bbox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );

    ITEM_CHANGE c2;
    c2.id = KIID_PATH( wxS( "/22222222-2222-2222-2222-222222222222" ) );
    c2.bbox = BOX2I( VECTOR2I( 50, 50 ), VECTOR2I( 5, 5 ) );

    // Degenerate bbox — should be skipped.
    ITEM_CHANGE c3;
    c3.id = KIID_PATH( wxS( "/33333333-3333-3333-3333-333333333333" ) );
    c3.bbox = BOX2I();

    diff.changes = { c1, c2, c3 };

    std::map<KIID_PATH, BOX2I> out;
    CollectChangeBBoxes( diff, out );

    BOOST_CHECK_EQUAL( out.size(), 2 );
    BOOST_CHECK( out.count( c1.id ) == 1 );
    BOOST_CHECK( out.count( c2.id ) == 1 );
    BOOST_CHECK( out.count( c3.id ) == 0 );
}


BOOST_AUTO_TEST_CASE( CollectChangeBBoxesRecursesIntoChildren )
{
    DOCUMENT_DIFF diff;

    ITEM_CHANGE parent;
    parent.id = KIID_PATH( wxS( "/aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa" ) );
    parent.bbox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) );

    ITEM_CHANGE child;
    child.id = KIID_PATH( wxS( "/bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb" ) );
    child.bbox = BOX2I( VECTOR2I( 10, 10 ), VECTOR2I( 5, 5 ) );

    parent.children = { child };
    diff.changes = { parent };

    std::map<KIID_PATH, BOX2I> out;
    CollectChangeBBoxes( diff, out );

    BOOST_CHECK_EQUAL( out.size(), 2 );
    BOOST_CHECK( out.count( parent.id ) == 1 );
    BOOST_CHECK( out.count( child.id ) == 1 );
}


BOOST_AUTO_TEST_CASE( CollectChangeBBoxesPreservesExistingEntries )
{
    DOCUMENT_DIFF diff;

    ITEM_CHANGE c;
    c.id = KIID_PATH( wxS( "/11111111-1111-1111-1111-111111111111" ) );
    c.bbox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );
    diff.changes = { c };

    std::map<KIID_PATH, BOX2I> out;
    out.emplace( c.id, BOX2I( VECTOR2I( 999, 999 ), VECTOR2I( 1, 1 ) ) );

    CollectChangeBBoxes( diff, out );

    // emplace is no-op on existing key — the pre-existing entry wins.
    BOOST_CHECK( out.at( c.id ).GetOrigin() == VECTOR2I( 999, 999 ) );
}


// ExpandBBoxToGeometry ------------------------------------------------------

BOOST_AUTO_TEST_CASE( ExpandBBoxToGeometryNoOpOnEmptyGeometry )
{
    DIFF_SCENE scene;
    scene.documentBBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) );

    const BOX2I before = scene.documentBBox;
    ExpandBBoxToGeometry( scene );

    BOOST_CHECK( scene.documentBBox.GetOrigin() == before.GetOrigin() );
    BOOST_CHECK( scene.documentBBox.GetEnd() == before.GetEnd() );
}


BOOST_AUTO_TEST_CASE( ExpandBBoxToGeometryIncludesSegmentExtent )
{
    DIFF_SCENE scene;
    scene.documentBBox = BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );

    DOCUMENT_SEGMENT seg;
    seg.start = VECTOR2I( -50, -50 );
    seg.end = VECTOR2I( 200, 200 );
    seg.width = 0;
    scene.referenceGeometry.segments.push_back( seg );

    ExpandBBoxToGeometry( scene );

    BOOST_CHECK( scene.documentBBox.Contains( VECTOR2I( -50, -50 ) ) );
    BOOST_CHECK( scene.documentBBox.Contains( VECTOR2I( 200, 200 ) ) );
}


BOOST_AUTO_TEST_CASE( ExpandBBoxToGeometryInflatesByStrokeWidth )
{
    // Thick segment from (0,0) to (0,0) — the stroke width is the entire
    // extent. ExpandBBoxToGeometry must inflate by width/2 so the bbox
    // covers the visible stroke.
    DIFF_SCENE scene;

    DOCUMENT_SEGMENT seg;
    seg.start = VECTOR2I( 0, 0 );
    seg.end = VECTOR2I( 0, 0 );
    seg.width = 40;
    scene.referenceGeometry.segments.push_back( seg );

    ExpandBBoxToGeometry( scene );

    BOOST_CHECK_GE( scene.documentBBox.GetWidth(), 40 );
    BOOST_CHECK_GE( scene.documentBBox.GetHeight(), 40 );
}


BOOST_AUTO_TEST_CASE( ExpandBBoxToGeometryIncludesCircleExtent )
{
    DIFF_SCENE scene;

    DOCUMENT_CIRCLE c;
    c.center = VECTOR2I( 100, 100 );
    c.radius = 50;
    c.lineWidth = 0;
    scene.comparisonGeometry.circles.push_back( c );

    ExpandBBoxToGeometry( scene );

    BOOST_CHECK( scene.documentBBox.Contains( VECTOR2I( 50, 100 ) ) );
    BOOST_CHECK( scene.documentBBox.Contains( VECTOR2I( 150, 100 ) ) );
    BOOST_CHECK( scene.documentBBox.Contains( VECTOR2I( 100, 50 ) ) );
    BOOST_CHECK( scene.documentBBox.Contains( VECTOR2I( 100, 150 ) ) );
}


// ItemRes string round-trip -------------------------------------------------

BOOST_AUTO_TEST_CASE( ItemResStringRoundTrip )
{
    for( ITEM_RES kind : { ITEM_RES::TAKE_OURS, ITEM_RES::TAKE_THEIRS, ITEM_RES::TAKE_ANCESTOR, ITEM_RES::MERGE_PROPS,
                           ITEM_RES::DELETE_ITEM, ITEM_RES::KEEP } )
    {
        const std::string s = ItemResToString( kind );
        const ITEM_RES    out = ItemResFromString( s );
        BOOST_CHECK( out == kind );
    }
}


BOOST_AUTO_TEST_CASE( ItemResFromStringRejectsUnknown )
{
    BOOST_CHECK_THROW( ItemResFromString( "TAKE_OURS" ), std::invalid_argument );
    BOOST_CHECK_THROW( ItemResFromString( "" ), std::invalid_argument );
    BOOST_CHECK_THROW( ItemResFromString( "bogus" ), std::invalid_argument );
}


// PropRes string round-trip -------------------------------------------------

BOOST_AUTO_TEST_CASE( PropResStringRoundTrip )
{
    for( PROP_RES kind : { PROP_RES::OURS, PROP_RES::THEIRS, PROP_RES::ANCESTOR, PROP_RES::CUSTOM } )
    {
        const std::string s = PropResToString( kind );
        const PROP_RES    out = PropResFromString( s );
        BOOST_CHECK( out == kind );
    }
}


BOOST_AUTO_TEST_CASE( PropResStringLiterals )
{
    // Pin the literal spellings — paired serializer/parser regressions
    // that flipped both ends to a new spelling would still round-trip,
    // but downstream tools that read the wire format directly would
    // break. The PROPERTY_RESOLUTION JSON consumers (kicad-cli mergetool
    // scripted resolutions in particular) depend on these strings.
    BOOST_CHECK_EQUAL( std::string( PropResToString( PROP_RES::OURS ) ), "ours" );
    BOOST_CHECK_EQUAL( std::string( PropResToString( PROP_RES::THEIRS ) ), "theirs" );
    BOOST_CHECK_EQUAL( std::string( PropResToString( PROP_RES::ANCESTOR ) ), "ancestor" );
    BOOST_CHECK_EQUAL( std::string( PropResToString( PROP_RES::CUSTOM ) ), "custom" );
}


BOOST_AUTO_TEST_CASE( PropResFromStringRejectsUnknown )
{
    BOOST_CHECK_THROW( PropResFromString( "OURS" ), std::invalid_argument );
    BOOST_CHECK_THROW( PropResFromString( "" ), std::invalid_argument );
    BOOST_CHECK_THROW( PropResFromString( "bogus" ), std::invalid_argument );
}


// BBoxFromGeometry coverage -------------------------------------------------

BOOST_AUTO_TEST_CASE( BBoxFromGeometry_EmptyReturnsNullopt )
{
    DOCUMENT_GEOMETRY g;
    BOOST_CHECK( !BBoxFromGeometry( g ).has_value() );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_SegmentInflatedByHalfStroke )
{
    DOCUMENT_GEOMETRY g;
    DOCUMENT_SEGMENT  s;
    s.start = VECTOR2I( 0, 0 );
    s.end = VECTOR2I( 100, 0 );
    s.width = 20;
    g.segments.push_back( s );

    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    // Each endpoint contributes [-10,-10]..[+10,+10] around itself.
    BOOST_CHECK_EQUAL( bbox->GetLeft(), -10 );
    BOOST_CHECK_EQUAL( bbox->GetRight(), 110 );
    BOOST_CHECK_EQUAL( bbox->GetTop(), -10 );
    BOOST_CHECK_EQUAL( bbox->GetBottom(), 10 );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_NegativeStrokeBecomesHairline )
{
    // width < 0 takes the EffectivePlotWidth path (substitutes the renderer's
    // hairline) so the bbox includes the half-hairline inflation the renderer
    // actually paints.
    DOCUMENT_GEOMETRY g;
    DOCUMENT_SEGMENT  s;
    s.start = VECTOR2I( 50, 50 );
    s.end = VECTOR2I( 50, 50 );
    s.width = -100;
    g.segments.push_back( s );

    const int  halfHairline = PLOT_HAIRLINE_IU / 2;
    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK_EQUAL( bbox->GetLeft(), 50 - halfHairline );
    BOOST_CHECK_EQUAL( bbox->GetRight(), 50 + halfHairline );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_PolygonInflatedByHalfLineWidth )
{
    DOCUMENT_GEOMETRY g;
    DOCUMENT_POLYGON  p;
    p.outline = { VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ), VECTOR2I( 100, 50 ) };
    p.lineWidth = 6;
    g.polygons.push_back( p );

    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK_EQUAL( bbox->GetLeft(), -3 );
    BOOST_CHECK_EQUAL( bbox->GetRight(), 103 );
    BOOST_CHECK_EQUAL( bbox->GetTop(), -3 );
    BOOST_CHECK_EQUAL( bbox->GetBottom(), 53 );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_CircleAccountsForRadiusPlusHalfStroke )
{
    DOCUMENT_GEOMETRY g;
    DOCUMENT_CIRCLE   c;
    c.center = VECTOR2I( 100, 200 );
    c.radius = 30;
    c.lineWidth = 4;
    g.circles.push_back( c );

    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    // reach = radius (30) + half stroke (2) = 32.
    BOOST_CHECK_EQUAL( bbox->GetLeft(), 68 );
    BOOST_CHECK_EQUAL( bbox->GetRight(), 132 );
    BOOST_CHECK_EQUAL( bbox->GetTop(), 168 );
    BOOST_CHECK_EQUAL( bbox->GetBottom(), 232 );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_UnionsAcrossAllShapeKinds )
{
    // Each shape declares width=0 — but BBoxFromGeometry mirrors the
    // renderer's PLOT_HAIRLINE_IU substitution so inflation is hairline/2 = 5
    // on each side. The extents below account for that.
    DOCUMENT_GEOMETRY g;

    DOCUMENT_SEGMENT s;
    s.start = VECTOR2I( -1000, -1000 );
    s.end = VECTOR2I( -900, -900 );
    s.width = 0;
    g.segments.push_back( s );

    DOCUMENT_POLYGON p;
    p.outline = { VECTOR2I( 500, 500 ), VECTOR2I( 600, 500 ), VECTOR2I( 600, 600 ) };
    p.lineWidth = 0;
    g.polygons.push_back( p );

    DOCUMENT_CIRCLE c;
    c.center = VECTOR2I( 0, 1000 );
    c.radius = 50;
    c.lineWidth = 0;
    g.circles.push_back( c );

    const int  halfHairline = PLOT_HAIRLINE_IU / 2;
    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK_EQUAL( bbox->GetLeft(), -1000 - halfHairline );
    BOOST_CHECK_EQUAL( bbox->GetRight(), 600 + halfHairline );
    BOOST_CHECK_EQUAL( bbox->GetTop(), -1000 - halfHairline );
    BOOST_CHECK_EQUAL( bbox->GetBottom(), 1050 + halfHairline );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_WidthZeroSegmentMatchesRendererHairline )
{
    // The bug this commit fixes: a width-0 horizontal segment previously
    // produced a zero-height bbox, but the renderer draws it at
    // PLOT_HAIRLINE_IU pixels. Now the bbox includes half the hairline above
    // and below so the rendered pixels actually fit inside the bbox.
    DOCUMENT_GEOMETRY g;
    DOCUMENT_SEGMENT  s;
    s.start = VECTOR2I( 0, 100 );
    s.end = VECTOR2I( 200, 100 );
    s.width = 0;
    g.segments.push_back( s );

    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK_EQUAL( bbox->GetHeight(), PLOT_HAIRLINE_IU );
    BOOST_CHECK_EQUAL( bbox->GetWidth(), 200 + PLOT_HAIRLINE_IU );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_SinglePointPolygonSkipped )
{
    // A single-point outline draws nothing in any renderer (headless plotter
    // requires >=3, GAL requires >=2), so the bbox skips it.
    DOCUMENT_GEOMETRY g;
    DOCUMENT_POLYGON  p;
    p.outline = { VECTOR2I( 999, 999 ) };
    p.lineWidth = 0;
    g.polygons.push_back( p );

    BOOST_CHECK( !BBoxFromGeometry( g ).has_value() );
}


BOOST_AUTO_TEST_CASE( BBoxFromGeometry_TwoPointPolygonIncluded )
{
    // The GAL overlay (diff_renderer_gal.cpp) draws two-point outlines as
    // open segments — the bbox must include them so the interactive zoom-
    // to-fit doesn't crop the rendered shape.
    DOCUMENT_GEOMETRY g;
    DOCUMENT_POLYGON  p;
    p.outline = { VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ) };
    p.lineWidth = 0;
    g.polygons.push_back( p );

    const auto bbox = BBoxFromGeometry( g );
    BOOST_REQUIRE( bbox.has_value() );
    BOOST_CHECK_EQUAL( bbox->GetLeft(), 0 - PLOT_HAIRLINE_IU / 2 );
    BOOST_CHECK_EQUAL( bbox->GetRight(), 100 + PLOT_HAIRLINE_IU / 2 );
}


// Geometry layer filtering ---------------------------------------------------

BOOST_AUTO_TEST_CASE( FilterGeometryByVisibleLayersKeepsLayerlessShapes )
{
    DOCUMENT_GEOMETRY g;

    DOCUMENT_SEGMENT layerless;
    layerless.start = VECTOR2I( 0, 0 );
    layerless.end = VECTOR2I( 100, 0 );
    g.segments.push_back( layerless );

    DOCUMENT_SEGMENT front;
    front.start = VECTOR2I( 0, 10 );
    front.end = VECTOR2I( 100, 10 );
    front.layers = LSET( { F_Cu } );
    g.segments.push_back( front );

    DOCUMENT_GEOMETRY filtered = FilterGeometryByVisibleLayers( g, LSET( { B_Cu } ) );

    BOOST_REQUIRE_EQUAL( filtered.segments.size(), 1u );
    BOOST_CHECK( filtered.segments[0].layers.none() );
}


BOOST_AUTO_TEST_CASE( FilterGeometryByVisibleLayersMatchesAnyLayer )
{
    DOCUMENT_GEOMETRY g;

    DOCUMENT_POLYGON frontBack;
    frontBack.outline = { VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ), VECTOR2I( 100, 100 ) };
    frontBack.layers = LSET( { F_Cu, B_Cu } );
    g.polygons.push_back( frontBack );

    DOCUMENT_CIRCLE silk;
    silk.center = VECTOR2I( 50, 50 );
    silk.radius = 10;
    silk.layers = LSET( { F_SilkS } );
    g.circles.push_back( silk );

    DOCUMENT_GEOMETRY filtered = FilterGeometryByVisibleLayers( g, LSET( { B_Cu } ) );

    BOOST_REQUIRE_EQUAL( filtered.polygons.size(), 1u );
    BOOST_CHECK( ( filtered.polygons[0].layers & LSET( { B_Cu } ) ).any() );
    BOOST_CHECK( filtered.circles.empty() );
}


// EffectivePlotWidth coverage -----------------------------------------------

BOOST_AUTO_TEST_CASE( EffectivePlotWidth_PositivePassThrough )
{
    BOOST_CHECK_EQUAL( EffectivePlotWidth( 1 ), 1 );
    BOOST_CHECK_EQUAL( EffectivePlotWidth( 1000 ), 1000 );
}


BOOST_AUTO_TEST_CASE( EffectivePlotWidth_NonPositiveBecomesHairline )
{
    BOOST_CHECK_EQUAL( EffectivePlotWidth( 0 ), PLOT_HAIRLINE_IU );
    BOOST_CHECK_EQUAL( EffectivePlotWidth( -1 ), PLOT_HAIRLINE_IU );
    BOOST_CHECK_EQUAL( EffectivePlotWidth( -999 ), PLOT_HAIRLINE_IU );
}


// Compile-time guarantee that the substituted hairline width is genuinely
// positive — SVG treats width=0 as "no stroke", so the contract documented
// in diff_renderer_plotter.h would silently break if a future edit zeroed
// PLOT_HAIRLINE_IU.
static_assert( PLOT_HAIRLINE_IU > 0, "hairline width must be positive" );


// ThemeColorFor coverage -----------------------------------------------------

BOOST_AUTO_TEST_CASE( ThemeColorFor_MapsEveryCategory )
{
    DIFF_COLOR_THEME theme;
    // Override defaults with distinct sentinel colors so an inverted mapping
    // (e.g. returning theme.added for CATEGORY::REMOVED) trips the test.
    theme.added = KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 );
    theme.removed = KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 );
    theme.modified = KIGFX::COLOR4D( 0.0, 0.0, 1.0, 1.0 );
    theme.conflict = KIGFX::COLOR4D( 1.0, 1.0, 0.0, 1.0 );

    BOOST_CHECK( ThemeColorFor( theme, CATEGORY::ADDED ) == theme.added );
    BOOST_CHECK( ThemeColorFor( theme, CATEGORY::REMOVED ) == theme.removed );
    BOOST_CHECK( ThemeColorFor( theme, CATEGORY::MODIFIED ) == theme.modified );
    BOOST_CHECK( ThemeColorFor( theme, CATEGORY::CONFLICT ) == theme.conflict );
}


// BuildScene coverage --------------------------------------------------------

namespace
{

ITEM_CHANGE MakeChangeWithBBox( const wxString& aType, CHANGE_KIND aKind, const BOX2I& aBBox )
{
    ITEM_CHANGE c;
    c.typeName = aType;
    c.kind = aKind;
    c.id = KIID_PATH( wxS( "/" ) + KIID().AsString() );
    c.bbox = aBBox;
    return c;
}

} // namespace


BOOST_AUTO_TEST_CASE( BuildScene_EmptyDiffProducesEmptyScene )
{
    DOCUMENT_DIFF    diff;
    DIFF_COLOR_THEME theme;

    DIFF_SCENE scene = BuildScene( diff, theme );

    BOOST_CHECK( scene.addedShapes.empty() );
    BOOST_CHECK( scene.removedShapes.empty() );
    BOOST_CHECK( scene.modifiedShapes.empty() );
    BOOST_CHECK( scene.conflictShapes.empty() );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetWidth(), 0 );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetHeight(), 0 );
}


BOOST_AUTO_TEST_CASE( BuildScene_RoutesByChangeKind )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::REMOVED, BOX2I( VECTOR2I( 20, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back( MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::MODIFIED,
                                                BOX2I( VECTOR2I( 40, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back( MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::COLLISION,
                                                BOX2I( VECTOR2I( 60, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back( MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::DUPLICATE_UUID,
                                                BOX2I( VECTOR2I( 80, 0 ), VECTOR2I( 10, 10 ) ) ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_CHECK_EQUAL( scene.addedShapes.size(), 1u );
    BOOST_CHECK_EQUAL( scene.removedShapes.size(), 1u );
    BOOST_CHECK_EQUAL( scene.modifiedShapes.size(), 1u );
    // COLLISION and DUPLICATE_UUID both map to CONFLICT.
    BOOST_CHECK_EQUAL( scene.conflictShapes.size(), 2u );
}


BOOST_AUTO_TEST_CASE( BuildScene_AppliesThemeColorsPerCategory )
{
    // Use distinct sentinel colors on every category so an inverted mapping
    // (e.g. returning theme.added for CATEGORY::REMOVED) trips even though
    // both sides are valid COLOR4D values. Default-constructed COLOR4D
    // values are identical, so the previous version of this test would have
    // passed even with the dispatch entirely broken.
    DIFF_COLOR_THEME theme;
    theme.added = KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 );
    theme.removed = KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 );
    theme.modified = KIGFX::COLOR4D( 0.0, 0.0, 1.0, 1.0 );
    theme.conflict = KIGFX::COLOR4D( 1.0, 1.0, 0.0, 1.0 );

    DOCUMENT_DIFF diff;
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::REMOVED, BOX2I( VECTOR2I( 20, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back( MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::MODIFIED,
                                                BOX2I( VECTOR2I( 40, 0 ), VECTOR2I( 10, 10 ) ) ) );
    diff.changes.push_back( MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::COLLISION,
                                                BOX2I( VECTOR2I( 60, 0 ), VECTOR2I( 10, 10 ) ) ) );

    DIFF_SCENE scene = BuildScene( diff, theme );

    BOOST_REQUIRE_EQUAL( scene.addedShapes.size(), 1u );
    BOOST_REQUIRE_EQUAL( scene.removedShapes.size(), 1u );
    BOOST_REQUIRE_EQUAL( scene.modifiedShapes.size(), 1u );
    BOOST_REQUIRE_EQUAL( scene.conflictShapes.size(), 1u );
    BOOST_CHECK( scene.addedShapes[0].color == theme.added );
    BOOST_CHECK( scene.removedShapes[0].color == theme.removed );
    BOOST_CHECK( scene.modifiedShapes[0].color == theme.modified );
    BOOST_CHECK( scene.conflictShapes[0].color == theme.conflict );
}


BOOST_AUTO_TEST_CASE( BuildScene_SkipsItemsWithDegenerateBBox )
{
    // BuildScene calls bboxValid which requires BOTH dimensions > 0 — a
    // zero-area bbox at the origin or elsewhere should be skipped, and so
    // should bboxes with only one zero dimension (a horizontal or vertical
    // sliver). The (0,0) origin case alone would pass even if the predicate
    // only checked the origin field, so test all three shapes.
    DOCUMENT_DIFF diff;
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 50, 50 ), VECTOR2I( 0, 0 ) ) ) );
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 100, 100 ), VECTOR2I( 0, 50 ) ) ) );
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 200, 200 ), VECTOR2I( 50, 0 ) ) ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_CHECK( scene.addedShapes.empty() );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetWidth(), 0 );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetHeight(), 0 );
}


BOOST_AUTO_TEST_CASE( BuildScene_DocumentBBoxIsUnionOfValidChangeBBoxes )
{
    DOCUMENT_DIFF diff;
    diff.changes.push_back(
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ) );
    diff.changes.push_back( MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::REMOVED,
                                                BOX2I( VECTOR2I( 200, 200 ), VECTOR2I( 50, 50 ) ) ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_CHECK_EQUAL( scene.documentBBox.GetLeft(), 0 );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetTop(), 0 );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetRight(), 250 );
    BOOST_CHECK_EQUAL( scene.documentBBox.GetBottom(), 250 );
}


BOOST_AUTO_TEST_CASE( BuildScene_NestedChildrenContributeShapes )
{
    // A footprint change with nested pad children: each child must produce
    // its own SCENE_SHAPE with the child's changeId — per-pad highlight in
    // the dialog uses changeId to map the click target to the source item.
    // Counts alone could be satisfied by emitting duplicate parent shapes.
    DOCUMENT_DIFF diff;
    ITEM_CHANGE   fp = MakeChangeWithBBox( wxS( "FOOTPRINT" ), CHANGE_KIND::MODIFIED,
                                           BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) );
    fp.children.push_back( MakeChangeWithBBox( wxS( "PAD" ), CHANGE_KIND::MODIFIED,
                                               BOX2I( VECTOR2I( 10, 10 ), VECTOR2I( 20, 20 ) ) ) );
    fp.children.push_back(
            MakeChangeWithBBox( wxS( "PAD" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 60, 60 ), VECTOR2I( 20, 20 ) ) ) );

    // Snapshot the ids before moving the parent into the diff.
    const KIID_PATH parentId = fp.id;
    const KIID_PATH modPadId = fp.children[0].id;
    const KIID_PATH addPadId = fp.children[1].id;

    diff.changes.push_back( std::move( fp ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_REQUIRE_EQUAL( scene.modifiedShapes.size(), 2u );
    BOOST_REQUIRE_EQUAL( scene.addedShapes.size(), 1u );

    auto hasId = []( const std::vector<SCENE_SHAPE>& aShapes, const KIID_PATH& aId )
    {
        return std::any_of( aShapes.begin(), aShapes.end(),
                            [&aId]( const SCENE_SHAPE& aShape )
                            {
                                return aShape.changeId == aId;
                            } );
    };

    BOOST_CHECK( hasId( scene.modifiedShapes, parentId ) );
    BOOST_CHECK( hasId( scene.modifiedShapes, modPadId ) );
    BOOST_CHECK( hasId( scene.addedShapes, addPadId ) );
}


BOOST_AUTO_TEST_CASE( BuildScene_CollapsesSameNetRoutingChanges )
{
    DOCUMENT_DIFF diff;

    ITEM_CHANGE track = MakeChangeWithBBox( wxS( "PCB_TRACK" ), CHANGE_KIND::MODIFIED,
                                            BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) );
    track.refdes = wxS( "Net-(U1-Pad1)" );

    ITEM_CHANGE via = MakeChangeWithBBox( wxS( "PCB_VIA" ), CHANGE_KIND::MODIFIED,
                                          BOX2I( VECTOR2I( 20, 20 ), VECTOR2I( 10, 10 ) ) );
    via.refdes = wxS( "Net-(U1-Pad1)" );

    diff.changes.push_back( std::move( track ) );
    diff.changes.push_back( std::move( via ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_REQUIRE_EQUAL( scene.modifiedShapes.size(), 1u );
    BOOST_CHECK( scene.modifiedShapes[0].label == wxS( "NET [Net-(U1-Pad1)]" ) );
    BOOST_CHECK_EQUAL( scene.modifiedShapes[0].bbox.GetLeft(), 0 );
    BOOST_CHECK_EQUAL( scene.modifiedShapes[0].bbox.GetTop(), 0 );
    BOOST_CHECK_EQUAL( scene.modifiedShapes[0].bbox.GetRight(), 30 );
    BOOST_CHECK_EQUAL( scene.modifiedShapes[0].bbox.GetBottom(), 30 );
}


BOOST_AUTO_TEST_CASE( BuildScene_LabelIncludesRefdesWhenPresent )
{
    DOCUMENT_DIFF diff;
    ITEM_CHANGE   c = MakeChangeWithBBox( wxS( "FOOTPRINT" ), CHANGE_KIND::MODIFIED,
                                          BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) );
    c.refdes = wxS( "U7" );
    diff.changes.push_back( std::move( c ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_REQUIRE_EQUAL( scene.modifiedShapes.size(), 1u );
    // Pin the exact format ("%s [%s]" with typeName + refdes) so a label
    // refactor that flips field order or drops the brackets fails the test
    // explicitly. Substring matches would still pass on "U7FOOTPRINT" or
    // "[FOOTPRINT/U7]" which both break the dialog's display contract.
    BOOST_CHECK( scene.modifiedShapes[0].label == wxS( "FOOTPRINT [U7]" ) );
}


BOOST_AUTO_TEST_CASE( BuildScene_LabelIsTypeNameWhenRefdesAbsent )
{
    DOCUMENT_DIFF diff;
    ITEM_CHANGE   c =
            MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::ADDED, BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) ) );
    diff.changes.push_back( std::move( c ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_REQUIRE_EQUAL( scene.addedShapes.size(), 1u );
    BOOST_CHECK( scene.addedShapes[0].label == wxS( "ZONE" ) );
}


static DIFF_VALUE RectPolygonSet( const BOX2I& aBox )
{
    DIFF_VALUE::PolygonSet ps;
    ps.push_back( { { aBox.GetOrigin(),
                      { aBox.GetEnd().x, aBox.GetOrigin().y },
                      aBox.GetEnd(),
                      { aBox.GetOrigin().x, aBox.GetEnd().y } } } );
    return DIFF_VALUE::FromPolygonSet( std::move( ps ) );
}


// A grown ZONE whose old outline is fully inside the new one produces an
// added ring and nothing removed. The delta region must replace the
// whole-zone MODIFIED bbox rectangle.
BOOST_AUTO_TEST_CASE( BuildScene_ZoneOutlineDeltaEmitsAddedRingNotBBox )
{
    BOX2I before( VECTOR2I( 10, 10 ), VECTOR2I( 80, 80 ) ); // 10,10 .. 90,90
    BOX2I after( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) );  // 0,0 .. 100,100

    ITEM_CHANGE c = MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::MODIFIED, after );

    PROPERTY_DELTA d;
    d.name = wxS( "Outline" );
    d.before = RectPolygonSet( before );
    d.after = RectPolygonSet( after );
    c.properties.push_back( std::move( d ) );

    DOCUMENT_DIFF diff;
    diff.changes.push_back( std::move( c ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_CHECK_EQUAL( scene.modifiedShapes.size(), 0u );
    BOOST_REQUIRE_EQUAL( scene.addedShapes.size(), 1u );
    BOOST_CHECK( !scene.addedShapes[0].polygons.empty() );
    BOOST_CHECK_EQUAL( scene.removedShapes.size(), 0u );
}


// A shifted ZONE outline leaves a removed strip on one side and an added
// strip on the other. Both shapes carry real polygon geometry.
BOOST_AUTO_TEST_CASE( BuildScene_ZoneOutlineDeltaEmitsBothAddedAndRemoved )
{
    BOX2I before( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ); // 0,0 .. 100,100
    BOX2I after( VECTOR2I( 50, 0 ), VECTOR2I( 100, 100 ) ); // 50,0 .. 150,100

    ITEM_CHANGE c = MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::MODIFIED, after );

    PROPERTY_DELTA d;
    d.name = wxS( "Outline" );
    d.before = RectPolygonSet( before );
    d.after = RectPolygonSet( after );
    c.properties.push_back( std::move( d ) );

    DOCUMENT_DIFF diff;
    diff.changes.push_back( std::move( c ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_CHECK_EQUAL( scene.modifiedShapes.size(), 0u );
    BOOST_REQUIRE_EQUAL( scene.addedShapes.size(), 1u );
    BOOST_CHECK( !scene.addedShapes[0].polygons.empty() );
    BOOST_REQUIRE_EQUAL( scene.removedShapes.size(), 1u );
    BOOST_CHECK( !scene.removedShapes[0].polygons.empty() );
}


// A "Filled Area (<layer>)" delta drives the same boolean-subtract path as
// "Outline"; the StartsWith predicate must accept it.
BOOST_AUTO_TEST_CASE( BuildScene_ZoneFilledAreaDeltaAlsoEmitsRegions )
{
    BOX2I before( VECTOR2I( 10, 10 ), VECTOR2I( 80, 80 ) );
    BOX2I after( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) );

    ITEM_CHANGE c = MakeChangeWithBBox( wxS( "ZONE" ), CHANGE_KIND::MODIFIED, after );

    PROPERTY_DELTA d;
    d.name = wxS( "Filled Area (F.Cu)" );
    d.before = RectPolygonSet( before );
    d.after = RectPolygonSet( after );
    c.properties.push_back( std::move( d ) );

    DOCUMENT_DIFF diff;
    diff.changes.push_back( std::move( c ) );

    DIFF_SCENE scene = BuildScene( diff, DIFF_COLOR_THEME{} );

    BOOST_CHECK_EQUAL( scene.modifiedShapes.size(), 0u );
    BOOST_REQUIRE_EQUAL( scene.addedShapes.size(), 1u );
    BOOST_CHECK( !scene.addedShapes[0].polygons.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
