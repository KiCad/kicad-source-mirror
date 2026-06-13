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

#include <diff_merge/pcb_geometry_extractor.h>

#include <board.h>
#include <board_item.h>
#include <eda_shape.h>
#include <footprint.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <layer_ids.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>


namespace KICAD_DIFF
{

namespace
{

    /// Push a polyline as a chain of segments. Used for curved shapes that get
    /// tessellated into straight subdivisions.
    void emitPolyline( const SHAPE_LINE_CHAIN& aChain, int aWidth, const KIGFX::COLOR4D& aColor, const LSET& aLayers,
                       DOCUMENT_GEOMETRY& aOut )
    {
        for( int i = 0; i + 1 < aChain.PointCount(); ++i )
        {
            DOCUMENT_SEGMENT seg;
            seg.start = aChain.CPoint( i );
            seg.end = aChain.CPoint( i + 1 );
            seg.width = aWidth;
            seg.color = aColor;
            seg.layers = aLayers;
            aOut.segments.push_back( seg );
        }
    }


    void addShapeAsGeometry( const PCB_SHAPE& aShape, const KIGFX::COLOR4D& aColor, DOCUMENT_GEOMETRY& aOut )
    {
        const int  width = aShape.GetWidth();
        const bool fill = aShape.IsSolidFill();
        const LSET layers( { aShape.GetLayer() } );

        switch( aShape.GetShape() )
        {
        case SHAPE_T::SEGMENT:
        {
            DOCUMENT_SEGMENT seg;
            seg.start = aShape.GetStart();
            seg.end = aShape.GetEnd();
            seg.width = width;
            seg.color = aColor;
            seg.layers = layers;
            aOut.segments.push_back( seg );
            break;
        }

        case SHAPE_T::RECTANGLE:
        {
            const VECTOR2I   a = aShape.GetStart();
            const VECTOR2I   b = aShape.GetEnd();
            DOCUMENT_POLYGON poly;
            poly.outline = { a, { b.x, a.y }, b, { a.x, b.y } };
            poly.filled = fill;
            poly.lineWidth = width;
            poly.color = aColor;
            poly.layers = layers;
            aOut.polygons.push_back( poly );
            break;
        }

        case SHAPE_T::CIRCLE:
        {
            DOCUMENT_CIRCLE c;
            c.center = aShape.GetCenter();
            c.radius = aShape.GetRadius();
            c.filled = fill;
            c.lineWidth = width;
            c.color = aColor;
            c.layers = layers;
            aOut.circles.push_back( c );
            break;
        }

        case SHAPE_T::POLY:
        {
            const SHAPE_POLY_SET& set = aShape.GetPolyShape();

            for( int outline = 0; outline < set.OutlineCount(); ++outline )
            {
                const SHAPE_LINE_CHAIN& chain = set.COutline( outline );

                DOCUMENT_POLYGON poly;
                poly.filled = fill;
                poly.lineWidth = width;
                poly.color = aColor;
                poly.layers = layers;
                poly.outline.reserve( chain.PointCount() );

                for( int i = 0; i < chain.PointCount(); ++i )
                    poly.outline.push_back( chain.CPoint( i ) );

                aOut.polygons.push_back( std::move( poly ) );

                // Holes — emitted as separate (unfilled) outlines so they're at
                // least visible. True polygon-with-holes rendering would need a
                // tessellator and isn't worth the complexity for a context view.
                for( int hole = 0; hole < set.HoleCount( outline ); ++hole )
                {
                    const SHAPE_LINE_CHAIN& holeChain = set.CHole( outline, hole );

                    DOCUMENT_POLYGON holePoly;
                    holePoly.filled = false;
                    holePoly.lineWidth = width;
                    holePoly.color = aColor;
                    holePoly.layers = layers;
                    holePoly.outline.reserve( holeChain.PointCount() );

                    for( int i = 0; i < holeChain.PointCount(); ++i )
                        holePoly.outline.push_back( holeChain.CPoint( i ) );

                    aOut.polygons.push_back( std::move( holePoly ) );
                }
            }

            break;
        }

        case SHAPE_T::ARC:
        {
            const SHAPE_ARC arc( aShape.GetStart(), aShape.GetArcMid(), aShape.GetEnd(), 0 );
            emitPolyline( arc.ConvertToPolyline(), width, aColor, layers, aOut );
            break;
        }

        case SHAPE_T::BEZIER:
        {
            const std::vector<VECTOR2I>& pts = aShape.GetBezierPoints();

            if( pts.size() >= 2 )
            {
                SHAPE_LINE_CHAIN chain;

                for( const VECTOR2I& pt : pts )
                    chain.Append( pt );

                emitPolyline( chain, width, aColor, layers, aOut );
            }
            else
            {
                // Bezier wasn't tessellated; fall back to start→end chord so
                // the user at least sees a placeholder for the curve.
                DOCUMENT_SEGMENT seg;
                seg.start = aShape.GetStart();
                seg.end = aShape.GetEnd();
                seg.width = width;
                seg.color = aColor;
                seg.layers = layers;
                aOut.segments.push_back( seg );
            }

            break;
        }

        default: break;
        }
    }


    void addTrackAsGeometry( const PCB_TRACK& aTrack, const KIGFX::COLOR4D& aColor, DOCUMENT_GEOMETRY& aOut )
    {
        if( aTrack.Type() == PCB_VIA_T )
        {
            const PCB_VIA& via = static_cast<const PCB_VIA&>( aTrack );

            DOCUMENT_CIRCLE c;
            c.center = via.GetPosition();
            c.radius = via.GetWidth( PADSTACK::ALL_LAYERS ) / 2;
            c.filled = true;
            c.lineWidth = 0;
            c.color = aColor;
            c.layers = via.GetLayerSet();
            aOut.circles.push_back( c );
            return;
        }

        DOCUMENT_SEGMENT seg;
        seg.start = aTrack.GetStart();
        seg.end = aTrack.GetEnd();
        seg.width = aTrack.GetWidth();
        seg.color = aColor;
        seg.layers = aTrack.GetLayerSet();
        aOut.segments.push_back( seg );
    }


    void addBBoxAsGeometry( const BOARD_ITEM& aItem, const KIGFX::COLOR4D& aColor, DOCUMENT_GEOMETRY& aOut )
    {
        DOCUMENT_POLYGON poly = MakeBBoxOutline( aItem.GetBoundingBox(), aColor );

        if( poly.outline.empty() )
            return;

        poly.layers = aItem.GetLayerSet();
        aOut.polygons.push_back( std::move( poly ) );
    }

} // namespace


DOCUMENT_GEOMETRY ExtractBoardGeometry( const BOARD& aBoard, const KIGFX::COLOR4D& aColor )
{
    DOCUMENT_GEOMETRY out;

    // Board-level drawings: pull anything on Edge.Cuts to anchor the outline,
    // then user-doc layers so the user has a sense of the board shape.
    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( !item )
            continue;

        const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>( item );

        if( !shape )
            continue;

        const PCB_LAYER_ID layer = shape->GetLayer();
        const bool         isOutline = ( layer == Edge_Cuts );
        const bool isDoc = ( layer == Dwgs_User || layer == Cmts_User || layer == Eco1_User || layer == Eco2_User
                             || layer == F_SilkS || layer == B_SilkS );

        if( !isOutline && !isDoc )
            continue;

        addShapeAsGeometry( *shape, aColor, out );
    }

    // Copper tracks and vias give the context view its board-shaped content
    // and let layer filtering actually isolate routing layers.
    for( const PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track )
            addTrackAsGeometry( *track, aColor, out );
    }

    // Pads and zones are shown as bbox outlines for now. This keeps the
    // geometry model cheap but still gives layer-aware context around common
    // copper objects.
    for( const PAD* pad : aBoard.GetPads() )
    {
        if( pad )
            addBBoxAsGeometry( *pad, aColor, out );
    }

    for( const ZONE* zone : aBoard.Zones() )
    {
        if( zone )
            addBBoxAsGeometry( *zone, aColor, out );
    }

    // Footprint bounding boxes anchor component placement.
    for( const FOOTPRINT* fp : aBoard.Footprints() )
    {
        if( !fp )
            continue;

        DOCUMENT_POLYGON poly = MakeBBoxOutline( fp->GetBoundingBox( false ), aColor );

        if( poly.outline.empty() )
            continue;

        poly.layers = LSET( { fp->GetLayer() } );
        out.polygons.push_back( std::move( poly ) );
    }

    return out;
}


DOCUMENT_GEOMETRY ExtractFootprintGeometry( const FOOTPRINT& aFootprint, const KIGFX::COLOR4D& aColor )
{
    DOCUMENT_GEOMETRY out;

    for( const PAD* pad : aFootprint.Pads() )
    {
        if( pad )
            addBBoxAsGeometry( *pad, aColor, out );
    }

    for( const BOARD_ITEM* item : aFootprint.GraphicalItems() )
    {
        if( !item )
            continue;

        if( const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>( item ) )
            addShapeAsGeometry( *shape, aColor, out );
        else
            addBBoxAsGeometry( *item, aColor, out );
    }

    for( const ZONE* zone : aFootprint.Zones() )
    {
        if( zone )
            addBBoxAsGeometry( *zone, aColor, out );
    }

    DOCUMENT_POLYGON poly = MakeBBoxOutline( aFootprint.GetBoundingBox( false ), aColor );

    if( !poly.outline.empty() )
    {
        poly.layers = LSET( { aFootprint.GetLayer() } );
        out.polygons.push_back( std::move( poly ) );
    }

    return out;
}

} // namespace KICAD_DIFF
