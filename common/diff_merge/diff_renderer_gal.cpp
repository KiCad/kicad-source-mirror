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

#include <diff_merge/diff_renderer_gal.h>

#include <geometry/shape_poly_set.h>
#include <view/view_overlay.h>


namespace KICAD_DIFF
{

namespace
{

constexpr double FILL_ALPHA = 0.35;

// Zone deltas can blanket a large area, so they fill fainter than the bbox
// rectangles to keep the board readable underneath.
constexpr double POLYGON_FILL_ALPHA = 0.20;

const KIGFX::COLOR4D HIDDEN_FILL( 0.5, 0.5, 0.5, 0.10 );


/**
 * Push one document geometry's shapes into a VIEW_OVERLAY. The two polygon
 * passes (every fill, then every stroke) come from the shared iterator so a
 * filled-and-stroked shape gets both and a later filled polygon does not cover
 * an earlier outline; without the stroke pass a filled poly with a non-zero
 * line width would silently lose its outline at the rendered scale.
 */
class OVERLAY_GEOMETRY_SINK : public GEOMETRY_SINK
{
public:
    explicit OVERLAY_GEOMETRY_SINK( KIGFX::VIEW_OVERLAY& aOverlay ) : m_overlay( aOverlay ) {}

    void FillPolygon( const DOCUMENT_POLYGON& aPoly ) override
    {
        if( !aPoly.filled || aPoly.outline.size() < 3 )
            return;

        m_overlay.SetIsFill( true );
        m_overlay.SetIsStroke( false );
        m_overlay.SetFillColor( aPoly.color );

        std::deque<VECTOR2D> pts;

        for( const VECTOR2I& pt : aPoly.outline )
            pts.emplace_back( pt.x, pt.y );

        m_overlay.Polygon( pts );
    }

    void StrokePolygon( const DOCUMENT_POLYGON& aPoly ) override
    {
        // Skip stroke only when the shape is filled AND has no line width;
        // a filled poly with a positive width still wants its outline.
        if( aPoly.outline.size() < 2 || ( aPoly.filled && aPoly.lineWidth == 0 ) )
            return;

        m_overlay.SetIsFill( false );
        m_overlay.SetIsStroke( true );
        m_overlay.SetStrokeColor( aPoly.color );
        m_overlay.SetLineWidth( static_cast<double>( aPoly.lineWidth ) );

        for( std::size_t i = 0; i + 1 < aPoly.outline.size(); ++i )
        {
            m_overlay.Line( VECTOR2D( aPoly.outline[i].x,     aPoly.outline[i].y ),
                            VECTOR2D( aPoly.outline[i + 1].x, aPoly.outline[i + 1].y ) );
        }

        // Close the loop.
        m_overlay.Line( VECTOR2D( aPoly.outline.back().x,  aPoly.outline.back().y ),
                        VECTOR2D( aPoly.outline.front().x, aPoly.outline.front().y ) );
    }

    void DrawSegment( const DOCUMENT_SEGMENT& aSegment ) override
    {
        m_overlay.SetIsFill( false );
        m_overlay.SetIsStroke( true );
        m_overlay.SetStrokeColor( aSegment.color );

        if( aSegment.width > 0 )
        {
            m_overlay.Segment( VECTOR2D( aSegment.start.x, aSegment.start.y ),
                               VECTOR2D( aSegment.end.x, aSegment.end.y ),
                               static_cast<double>( aSegment.width ) );
        }
        else
        {
            m_overlay.SetLineWidth( 0.0 );
            m_overlay.Line( VECTOR2D( aSegment.start.x, aSegment.start.y ),
                            VECTOR2D( aSegment.end.x, aSegment.end.y ) );
        }
    }

    void DrawCircle( const DOCUMENT_CIRCLE& aCircle ) override
    {
        if( aCircle.filled )
        {
            m_overlay.SetIsFill( true );
            m_overlay.SetIsStroke( false );
            m_overlay.SetFillColor( aCircle.color );
        }
        else
        {
            m_overlay.SetIsFill( false );
            m_overlay.SetIsStroke( true );
            m_overlay.SetStrokeColor( aCircle.color );
            m_overlay.SetLineWidth( static_cast<double>( aCircle.lineWidth ) );
        }

        m_overlay.Circle( VECTOR2D( aCircle.center.x, aCircle.center.y ),
                          static_cast<double>( aCircle.radius ) );
    }

private:
    KIGFX::VIEW_OVERLAY& m_overlay;
};


void renderDocumentGeometry( KIGFX::VIEW_OVERLAY& aOverlay, const DOCUMENT_GEOMETRY& aGeometry )
{
    OVERLAY_GEOMETRY_SINK sink( aOverlay );
    IterateDocumentGeometry( aGeometry, sink );
}

} // namespace


void RenderSceneToOverlay( KIGFX::VIEW_OVERLAY& aOverlay, const DIFF_SCENE& aScene,
                           const std::array<bool, CATEGORY_COUNT>& aVisible, const std::optional<KIID_PATH>& aHighlight,
                           const std::set<KIID_PATH>& aHidden )
{
    aOverlay.Clear();

    // Background geometry — drawn first so the diff bbox rectangles sit on
    // top. Reference side first so comparison-only items don't get washed out
    // when both sides cover the same area.
    renderDocumentGeometry( aOverlay, aScene.referenceGeometry );
    renderDocumentGeometry( aOverlay, aScene.comparisonGeometry );

    const bool nativeContext = aScene.referenceGeometry.Empty() && aScene.comparisonGeometry.Empty();

    // Bounding boxes of the changed items (footprints, tracks, vias) in the
    // visible categories. Zone heat fills are punched with these so an item
    // added inside a changed zone shows through instead of being washed by the
    // same-color fill.
    SHAPE_POLY_SET itemBoxes;

    for( CATEGORY cat : PAINT_ORDER )
    {
        if( !aVisible[static_cast<std::size_t>( cat )] )
            continue;

        for( const SCENE_SHAPE& s : ShapesFor( aScene, cat ) )
        {
            if( !s.polygons.empty() || s.bbox.GetWidth() <= 0 || s.bbox.GetHeight() <= 0 )
                continue;

            int o = itemBoxes.NewOutline();
            itemBoxes.Append( s.bbox.GetOrigin().x, s.bbox.GetOrigin().y, o, -1 );
            itemBoxes.Append( s.bbox.GetEnd().x, s.bbox.GetOrigin().y, o, -1 );
            itemBoxes.Append( s.bbox.GetEnd().x, s.bbox.GetEnd().y, o, -1 );
            itemBoxes.Append( s.bbox.GetOrigin().x, s.bbox.GetEnd().y, o, -1 );
        }
    }

    for( CATEGORY cat : PAINT_ORDER )
    {
        if( !aVisible[static_cast<std::size_t>( cat )] )
            continue;

        for( const SCENE_SHAPE& s : ShapesFor( aScene, cat ) )
        {
            KIGFX::COLOR4D fill = s.color;
            fill.a = FILL_ALPHA;

            const bool hidden = !aHidden.empty() && aHidden.count( s.changeId ) > 0;

            if( hidden )
                fill = HIDDEN_FILL;

            if( !s.polygons.empty() )
            {
                if( !hidden )
                    fill.a = POLYGON_FILL_ALPHA;

                SHAPE_POLY_SET set = PolySetFromPolygonList( s.polygons );

                if( set.OutlineCount() > 0 )
                {
                    SHAPE_POLY_SET heat = set;

                    // Only punch item holes when the zone is muted. Shown zones
                    // keep a solid heat fill.
                    if( hidden && itemBoxes.OutlineCount() > 0 )
                        heat.BooleanSubtract( itemBoxes );

                    aOverlay.SetIsFill( true );
                    aOverlay.SetIsStroke( false );
                    aOverlay.SetFillColor( fill );
                    aOverlay.Polygon( heat );

                    // Stroke the full region boundary so it stays crisp where
                    // the punched holes break the fill.
                    KIGFX::COLOR4D border = s.color;
                    border.a = hidden ? HIDDEN_FILL.a : 1.0;

                    aOverlay.SetIsFill( false );
                    aOverlay.SetIsStroke( true );
                    aOverlay.SetStrokeColor( border );
                    aOverlay.SetLineWidth( 0.0 );
                    aOverlay.Polygon( set );
                }

                continue;
            }

            if( nativeContext )
                continue;

            aOverlay.SetIsFill( true );
            aOverlay.SetIsStroke( false );
            aOverlay.SetFillColor( fill );
            aOverlay.Rectangle( s.bbox.GetOrigin(), s.bbox.GetEnd() );
        }
    }
}

} // namespace KICAD_DIFF
