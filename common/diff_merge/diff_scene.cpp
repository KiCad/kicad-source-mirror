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

#include <diff_merge/diff_scene.h>

#include <diff_merge/diff_renderer_plotter.h>
#include <geometry/shape_poly_set.h>
#include <trace_helpers.h>

#include <wx/debug.h>
#include <wx/log.h>

#include <iterator>
#include <map>


namespace KICAD_DIFF
{

KIGFX::COLOR4D ThemeColorFor( const DIFF_COLOR_THEME& aTheme, CATEGORY aCategory )
{
    switch( aCategory )
    {
    case CATEGORY::ADDED: return aTheme.added;
    case CATEGORY::REMOVED: return aTheme.removed;
    case CATEGORY::MODIFIED: return aTheme.modified;
    case CATEGORY::CONFLICT: return aTheme.conflict;
    }

    wxFAIL_MSG( wxS( "Unknown CATEGORY" ) );
    return KIGFX::COLOR4D();
}


namespace
{
    struct NET_SHAPE_GROUP
    {
        KIID_PATH   firstChangeId;
        wxString    netName;
        CHANGE_KIND kind;
        BOX2I       bbox;
        bool        hasBBox = false;
    };


    bool bboxValid( const BOX2I& aBox )
    {
        return aBox.GetWidth() > 0 && aBox.GetHeight() > 0;
    }


    std::vector<SCENE_SHAPE>& mutableShapesFor( DIFF_SCENE& aScene, CATEGORY aCategory )
    {
        // The const-overload caller never returns a reference to a true const
        // SCENE_SHAPE vector — only switches on category — so we can re-use
        // its switch.
        return const_cast<std::vector<SCENE_SHAPE>&>( ShapesFor( aScene, aCategory ) );
    }


    void mergeSceneBBox( const BOX2I& aBBox, BOX2I& aUnionBBox, bool& aFirst )
    {
        if( aFirst )
        {
            aUnionBBox = aBBox;
            aFirst = false;
        }
        else
        {
            aUnionBBox.Merge( aBBox );
        }
    }


    SHAPE_POLY_SET polygonSetFromDiffValue( const DIFF_VALUE& aValue )
    {
        SHAPE_POLY_SET out;

        if( aValue.GetType() != DIFF_VALUE::T::POLYGON_SET )
            return out;

        for( const auto& poly : aValue.AsPolygonSet() )
        {
            if( poly.empty() )
                continue;

            int outlineIdx = out.NewOutline();

            for( const VECTOR2I& pt : poly.front() )
                out.Append( pt.x, pt.y, outlineIdx, /*aHole=*/-1 );

            for( std::size_t h = 1; h < poly.size(); ++h )
            {
                int holeIdx = out.NewHole( outlineIdx );

                for( const VECTOR2I& pt : poly[h] )
                    out.Append( pt.x, pt.y, outlineIdx, holeIdx );
            }
        }

        return out;
    }


    SCENE_SHAPE::PolygonList polygonsFromPolySet( const SHAPE_POLY_SET& aPoly )
    {
        SCENE_SHAPE::PolygonList out;

        for( int o = 0; o < aPoly.OutlineCount(); ++o )
        {
            const auto&                        polyRefs = aPoly.CPolygon( o );
            std::vector<std::vector<VECTOR2I>> contours;

            for( const auto& contour : polyRefs )
            {
                std::vector<VECTOR2I> pts;
                pts.reserve( contour.PointCount() );

                for( int p = 0; p < contour.PointCount(); ++p )
                    pts.push_back( contour.CPoint( p ) );

                contours.push_back( std::move( pts ) );
            }

            out.push_back( std::move( contours ) );
        }

        return out;
    }


    void pushDeltaShape( DIFF_SCENE& aScene, const DIFF_COLOR_THEME& aTheme, CATEGORY aCategory,
                         const ITEM_CHANGE& aChange, const SHAPE_POLY_SET& aRegion, BOX2I& aUnionBBox, bool& aFirst )
    {
        if( aRegion.OutlineCount() == 0 )
            return;

        SCENE_SHAPE shape;
        shape.bbox = aRegion.BBox();
        shape.changeId = aChange.id;
        shape.color = ThemeColorFor( aTheme, aCategory );
        shape.label = ChangeDisplayLabel( aChange );
        shape.polygons = polygonsFromPolySet( aRegion );

        mutableShapesFor( aScene, aCategory ).push_back( std::move( shape ) );
        mergeSceneBBox( aRegion.BBox(), aUnionBBox, aFirst );
    }


    bool emitOutlineDelta( const ITEM_CHANGE& aChange, const DIFF_COLOR_THEME& aTheme, DIFF_SCENE& aScene,
                           BOX2I& aUnionBBox, bool& aFirst )
    {
        bool emitted = false;

        for( const PROPERTY_DELTA& d : aChange.properties )
        {
            if( d.before.GetType() != DIFF_VALUE::T::POLYGON_SET || d.after.GetType() != DIFF_VALUE::T::POLYGON_SET )
            {
                continue;
            }

            if( d.name != wxS( "Outline" ) && !d.name.StartsWith( wxS( "Filled Area" ) ) )
                continue;

            SHAPE_POLY_SET before = polygonSetFromDiffValue( d.before );
            SHAPE_POLY_SET after = polygonSetFromDiffValue( d.after );

            SHAPE_POLY_SET added = after;
            added.BooleanSubtract( before );

            SHAPE_POLY_SET removed = before;
            removed.BooleanSubtract( after );

            pushDeltaShape( aScene, aTheme, CATEGORY::ADDED, aChange, added, aUnionBBox, aFirst );
            pushDeltaShape( aScene, aTheme, CATEGORY::REMOVED, aChange, removed, aUnionBBox, aFirst );

            if( added.OutlineCount() > 0 || removed.OutlineCount() > 0 )
                emitted = true;
        }

        return emitted;
    }


    void addChangeShape( const ITEM_CHANGE& aChange, const DIFF_COLOR_THEME& aTheme, DIFF_SCENE& aScene,
                         BOX2I& aUnionBBox, bool& aFirst )
    {
        if( emitOutlineDelta( aChange, aTheme, aScene, aUnionBBox, aFirst ) )
            return;

        if( !bboxValid( aChange.bbox ) )
            return;

        const CATEGORY cat = CategoryFor( aChange.kind );

        SCENE_SHAPE shape;
        shape.bbox = aChange.bbox;
        shape.changeId = aChange.id;
        shape.color = ThemeColorFor( aTheme, cat );

        shape.label = ChangeDisplayLabel( aChange );

        mutableShapesFor( aScene, cat ).push_back( shape );
        mergeSceneBBox( aChange.bbox, aUnionBBox, aFirst );
    }


    void collectShapes( const ITEM_CHANGE& aChange, const DIFF_COLOR_THEME& aTheme, DIFF_SCENE& aScene,
                        BOX2I& aUnionBBox, bool& aFirst,
                        std::map<std::pair<CHANGE_KIND, wxString>, NET_SHAPE_GROUP>& aNetGroups )
    {
        if( IsRoutingNetChange( aChange ) )
        {
            auto& group = aNetGroups[std::make_pair( aChange.kind, *aChange.refdes )];

            if( !group.hasBBox )
            {
                group.firstChangeId = aChange.id;
                group.netName = *aChange.refdes;
                group.kind = aChange.kind;
                group.bbox = aChange.bbox;
                group.hasBBox = bboxValid( aChange.bbox );
            }
            else if( bboxValid( aChange.bbox ) )
            {
                group.bbox.Merge( aChange.bbox );
            }
        }
        else
        {
            addChangeShape( aChange, aTheme, aScene, aUnionBBox, aFirst );
        }

        for( const ITEM_CHANGE& child : aChange.children )
            collectShapes( child, aTheme, aScene, aUnionBBox, aFirst, aNetGroups );
    }


    void addNetShapes( const DIFF_COLOR_THEME& aTheme, DIFF_SCENE& aScene, BOX2I& aUnionBBox, bool& aFirst,
                       const std::map<std::pair<CHANGE_KIND, wxString>, NET_SHAPE_GROUP>& aNetGroups )
    {
        for( const auto& [key, group] : aNetGroups )
        {
            if( !group.hasBBox )
                continue;

            const CATEGORY cat = CategoryFor( group.kind );

            SCENE_SHAPE shape;
            shape.bbox = group.bbox;
            shape.changeId = group.firstChangeId;
            shape.color = ThemeColorFor( aTheme, cat );
            shape.label = wxS( "NET [" ) + group.netName + wxS( "]" );

            mutableShapesFor( aScene, cat ).push_back( shape );
            mergeSceneBBox( group.bbox, aUnionBBox, aFirst );
        }
    }

} // namespace


std::optional<BOX2I> BBoxFromGeometry( const DOCUMENT_GEOMETRY& aGeometry )
{
    std::optional<BOX2I> bbox;

    auto addPoint = [&]( const VECTOR2I& aPt )
    {
        if( !bbox.has_value() )
            bbox = BOX2I( aPt, VECTOR2I( 0, 0 ) );
        else
            bbox->Merge( aPt );
    };

    auto addPointInflated = [&]( const VECTOR2I& aPt, int aHalfStroke )
    {
        const VECTOR2I pad( aHalfStroke, aHalfStroke );
        addPoint( aPt - pad );
        addPoint( aPt + pad );
    };

    // Mirror the renderer's stroke-width substitution: a primitive declared
    // with width <= 0 is drawn at PLOT_HAIRLINE_IU by RenderSceneTo{Png,Svg},
    // so the bbox must inflate by the same effective half-stroke or a
    // width-0 horizontal/vertical primitive collapses to a degenerate slice
    // and the renderer's hairline pixels spill outside the reported bbox.

    for( const DOCUMENT_SEGMENT& s : aGeometry.segments )
    {
        const int half = EffectivePlotWidth( s.width ) / 2;
        addPointInflated( s.start, half );
        addPointInflated( s.end, half );
    }

    for( const DOCUMENT_POLYGON& p : aGeometry.polygons )
    {
        // Match the most-permissive renderer's polygon skip. The headless
        // plotter requires outline.size() >= 3 for closed shapes, but the
        // GAL renderer (diff_renderer_gal.cpp) draws two-point outlines as
        // open segments, so the bbox must include them too. Only a strictly
        // single-point outline draws nothing anywhere — skip just that.
        if( p.outline.size() < 2 )
            continue;

        const int half = EffectivePlotWidth( p.lineWidth ) / 2;

        for( const VECTOR2I& pt : p.outline )
            addPointInflated( pt, half );
    }

    for( const DOCUMENT_CIRCLE& c : aGeometry.circles )
    {
        const int      reach = c.radius + EffectivePlotWidth( c.lineWidth ) / 2;
        const VECTOR2I r( reach, reach );
        addPoint( c.center - r );
        addPoint( c.center + r );
    }

    return bbox;
}


LSET GeometryLayerSet( const DOCUMENT_GEOMETRY& aGeometry )
{
    LSET layers;

    for( const DOCUMENT_SEGMENT& s : aGeometry.segments )
        layers |= s.layers;

    for( const DOCUMENT_POLYGON& p : aGeometry.polygons )
        layers |= p.layers;

    for( const DOCUMENT_CIRCLE& c : aGeometry.circles )
        layers |= c.layers;

    return layers;
}


DOCUMENT_GEOMETRY FilterGeometryByVisibleLayers( const DOCUMENT_GEOMETRY& aGeometry, const LSET& aVisibleLayers )
{
    DOCUMENT_GEOMETRY out;

    auto visible = [&]( const LSET& aLayers )
    {
        return aLayers.none() || ( aLayers & aVisibleLayers ).any();
    };

    for( const DOCUMENT_SEGMENT& s : aGeometry.segments )
    {
        if( visible( s.layers ) )
            out.segments.push_back( s );
    }

    for( const DOCUMENT_POLYGON& p : aGeometry.polygons )
    {
        if( visible( p.layers ) )
            out.polygons.push_back( p );
    }

    for( const DOCUMENT_CIRCLE& c : aGeometry.circles )
    {
        if( visible( c.layers ) )
            out.circles.push_back( c );
    }

    return out;
}


void AppendGeometry( DOCUMENT_GEOMETRY& aDst, DOCUMENT_GEOMETRY&& aSrc )
{
    aDst.segments.insert( aDst.segments.end(), std::make_move_iterator( aSrc.segments.begin() ),
                          std::make_move_iterator( aSrc.segments.end() ) );
    aDst.polygons.insert( aDst.polygons.end(), std::make_move_iterator( aSrc.polygons.begin() ),
                          std::make_move_iterator( aSrc.polygons.end() ) );
    aDst.circles.insert( aDst.circles.end(), std::make_move_iterator( aSrc.circles.begin() ),
                         std::make_move_iterator( aSrc.circles.end() ) );
}


bool IsRoutingNetChange( const ITEM_CHANGE& aChange )
{
    return aChange.refdes.has_value() && !aChange.refdes->IsEmpty()
           && ( aChange.typeName == wxS( "PCB_TRACK" ) || aChange.typeName == wxS( "PCB_ARC" )
                || aChange.typeName == wxS( "PCB_VIA" ) );
}


wxString ChangeDisplayLabel( const ITEM_CHANGE& aChange )
{
    wxString label = aChange.typeName;

    if( aChange.refdes.has_value() )
        label += wxS( " [" ) + *aChange.refdes + wxS( "]" );

    return label;
}


DIFF_SCENE BuildScene( const DOCUMENT_DIFF& aDiff, const DIFF_COLOR_THEME& aTheme )
{
    DIFF_SCENE                                                  scene;
    BOX2I                                                       unionBBox;
    bool                                                        first = true;
    std::map<std::pair<CHANGE_KIND, wxString>, NET_SHAPE_GROUP> netGroups;

    for( const ITEM_CHANGE& c : aDiff.changes )
        collectShapes( c, aTheme, scene, unionBBox, first, netGroups );

    addNetShapes( aTheme, scene, unionBBox, first, netGroups );

    if( !first )
        scene.documentBBox = unionBBox;

    wxLogTrace( traceDiffMerge, wxT( "BuildScene: %zu changes -> added=%zu removed=%zu modified=%zu conflict=%zu" ),
                aDiff.changes.size(), scene.addedShapes.size(), scene.removedShapes.size(), scene.modifiedShapes.size(),
                scene.conflictShapes.size() );

    return scene;
}


void ExpandBBoxToGeometry( DIFF_SCENE& aScene )
{
    std::optional<BOX2I> unionBBox;

    if( aScene.documentBBox.GetWidth() > 0 || aScene.documentBBox.GetHeight() > 0 )
        unionBBox = aScene.documentBBox;

    auto merge = [&]( const std::optional<BOX2I>& aSrc )
    {
        if( !aSrc.has_value() )
            return;

        if( !unionBBox.has_value() )
            unionBBox = *aSrc;
        else
            unionBBox->Merge( *aSrc );
    };

    merge( BBoxFromGeometry( aScene.referenceGeometry ) );
    merge( BBoxFromGeometry( aScene.comparisonGeometry ) );

    if( unionBBox.has_value() )
        aScene.documentBBox = *unionBBox;
}


void CollectChangeBBoxes( const DOCUMENT_DIFF& aDiff, std::map<KIID_PATH, BOX2I>& aOut )
{
    // IndexChangesByKiid already does the recursive walk; reuse it so the
    // tree-flatten logic lives in one place. emplace preserves any
    // pre-existing entries the caller seeded.
    for( const auto& [id, change] : IndexChangesByKiid( aDiff ) )
    {
        if( change->bbox.GetWidth() > 0 || change->bbox.GetHeight() > 0 )
            aOut.emplace( id, change->bbox );
    }
}


DOCUMENT_POLYGON MakeBBoxOutline( const BOX2I& aBBox, const KIGFX::COLOR4D& aColor, int aLineWidth )
{
    DOCUMENT_POLYGON poly;
    poly.filled = false;
    poly.lineWidth = aLineWidth;
    poly.color = aColor;

    if( !aBBox.GetWidth() || !aBBox.GetHeight() )
        return poly;

    poly.outline = { aBBox.GetOrigin(),
                     { aBBox.GetEnd().x, aBBox.GetOrigin().y },
                     aBBox.GetEnd(),
                     { aBBox.GetOrigin().x, aBBox.GetEnd().y } };
    return poly;
}


SHAPE_POLY_SET PolySetFromPolygonList( const SCENE_SHAPE::PolygonList& aPolygons )
{
    SHAPE_POLY_SET out;

    for( const auto& poly : aPolygons )
    {
        if( poly.empty() || poly.front().size() < 3 )
            continue;

        int outlineIdx = out.NewOutline();

        for( const VECTOR2I& pt : poly.front() )
            out.Append( pt.x, pt.y, outlineIdx, /*aHole=*/-1 );

        for( std::size_t h = 1; h < poly.size(); ++h )
        {
            int holeIdx = out.NewHole( outlineIdx );

            for( const VECTOR2I& pt : poly[h] )
                out.Append( pt.x, pt.y, outlineIdx, holeIdx );
        }
    }

    return out;
}


CATEGORY CategoryFor( CHANGE_KIND aKind )
{
    switch( aKind )
    {
    case CHANGE_KIND::ADDED: return CATEGORY::ADDED;
    case CHANGE_KIND::REMOVED: return CATEGORY::REMOVED;
    case CHANGE_KIND::MODIFIED: return CATEGORY::MODIFIED;
    case CHANGE_KIND::COLLISION:
    case CHANGE_KIND::DUPLICATE_UUID: return CATEGORY::CONFLICT;
    }

    wxFAIL_MSG( wxS( "Unknown CHANGE_KIND" ) );
    return CATEGORY::MODIFIED;
}


const std::vector<SCENE_SHAPE>& ShapesFor( const DIFF_SCENE& aScene, CATEGORY aCategory )
{
    switch( aCategory )
    {
    case CATEGORY::ADDED: return aScene.addedShapes;
    case CATEGORY::REMOVED: return aScene.removedShapes;
    case CATEGORY::MODIFIED: return aScene.modifiedShapes;
    case CATEGORY::CONFLICT: return aScene.conflictShapes;
    }

    wxFAIL_MSG( wxS( "Unknown CATEGORY" ) );
    return aScene.modifiedShapes;
}


std::optional<BOX2I> HighlightedBBox( const DIFF_SCENE& aScene, const KIID_PATH& aChangeId,
                                      const std::array<bool, CATEGORY_COUNT>& aCategoryVisible )
{
    std::optional<BOX2I> result;

    for( CATEGORY cat : PAINT_ORDER )
    {
        if( !aCategoryVisible[static_cast<std::size_t>( cat )] )
            continue;

        for( const SCENE_SHAPE& s : ShapesFor( aScene, cat ) )
        {
            if( s.changeId != aChangeId )
                continue;

            if( !result )
                result = s.bbox;
            else
                result->Merge( s.bbox );
        }
    }

    return result;
}

} // namespace KICAD_DIFF
