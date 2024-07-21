/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pcb_grid_helper.h"

#include <functional>

#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <zone.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/intersection.h>
#include <geometry/oval.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <macros.h>
#include <math/util.h> // for KiROUND
#include <gal/painter.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_tool_base.h>
#include <view/view.h>

PCB_GRID_HELPER::PCB_GRID_HELPER( TOOL_MANAGER* aToolMgr, MAGNETIC_SETTINGS* aMagneticSettings ) :
    GRID_HELPER( aToolMgr ),
    m_magneticSettings( aMagneticSettings )
{
    KIGFX::VIEW*            view = m_toolMgr->GetView();
    KIGFX::RENDER_SETTINGS* settings = view->GetPainter()->GetSettings();
    KIGFX::COLOR4D          auxItemsColor = settings->GetLayerColor( LAYER_AUX_ITEMS );
    KIGFX::COLOR4D          umbilicalColor = settings->GetLayerColor( LAYER_ANCHOR );

    m_viewAxis.SetSize( 20000 );
    m_viewAxis.SetStyle( KIGFX::ORIGIN_VIEWITEM::CROSS );
    m_viewAxis.SetColor( auxItemsColor.WithAlpha( 0.4 ) );
    m_viewAxis.SetDrawAtZero( true );
    view->Add( &m_viewAxis );
    view->SetVisible( &m_viewAxis, false );

    m_viewSnapPoint.SetSize( 10 );
    m_viewSnapPoint.SetStyle( KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS );
    m_viewSnapPoint.SetColor( auxItemsColor );
    m_viewSnapPoint.SetDrawAtZero( true );
    view->Add( &m_viewSnapPoint );
    view->SetVisible( &m_viewSnapPoint, false );

    m_viewSnapLine.SetStyle( KIGFX::ORIGIN_VIEWITEM::DASH_LINE );
    m_viewSnapLine.SetColor( umbilicalColor );
    m_viewSnapLine.SetDrawAtZero( true );
    view->Add( &m_viewSnapLine );
    view->SetVisible( &m_viewSnapLine, false );
}


PCB_GRID_HELPER::~PCB_GRID_HELPER()
{
    KIGFX::VIEW* view = m_toolMgr->GetView();

    view->Remove( &m_viewAxis );
    view->Remove( &m_viewSnapPoint );
    view->Remove( &m_viewSnapLine );
}


VECTOR2I PCB_GRID_HELPER::AlignToSegment( const VECTOR2I& aPoint, const SEG& aSeg )
{
    const int c_gridSnapEpsilon_sq = 4;

    VECTOR2I aligned = Align( aPoint );

    if( !m_enableSnap )
        return aligned;

    std::vector<VECTOR2I> points;

    const SEG testSegments[] = { SEG( aligned, aligned + VECTOR2( 1, 0 ) ),
                                 SEG( aligned, aligned + VECTOR2( 0, 1 ) ),
                                 SEG( aligned, aligned + VECTOR2( 1, 1 ) ),
                                 SEG( aligned, aligned + VECTOR2( 1, -1 ) ) };

    for( const SEG& seg : testSegments )
    {
        OPT_VECTOR2I vec = aSeg.IntersectLines( seg );

        if( vec && aSeg.SquaredDistance( *vec ) <= c_gridSnapEpsilon_sq )
            points.push_back( *vec );
    }

    VECTOR2I    nearest = aligned;
    SEG::ecoord min_d_sq = VECTOR2I::ECOORD_MAX;

    // Snap by distance between pointer and endpoints
    for( const VECTOR2I& pt : { aSeg.A, aSeg.B } )
    {
        SEG::ecoord d_sq = ( pt - aPoint ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    // Snap by distance between aligned cursor and intersections
    for( const VECTOR2I& pt : points )
    {
        SEG::ecoord d_sq = ( pt - aligned ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    return nearest;
}


VECTOR2I PCB_GRID_HELPER::AlignToArc( const VECTOR2I& aPoint, const SHAPE_ARC& aArc )
{
    VECTOR2I aligned = Align( aPoint );

    if( !m_enableSnap )
        return aligned;

    std::vector<VECTOR2I> points;

    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 1, 0 ) ), &points );
    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 0, 1 ) ), &points );
    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 1, 1 ) ), &points );
    aArc.IntersectLine( SEG( aligned, aligned + VECTOR2( 1, -1 ) ), &points );

    VECTOR2I    nearest = aligned;
    SEG::ecoord min_d_sq = VECTOR2I::ECOORD_MAX;

    // Snap by distance between pointer and endpoints
    for( const VECTOR2I& pt : { aArc.GetP0(), aArc.GetP1() } )
    {
        SEG::ecoord d_sq = ( pt - aPoint ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    // Snap by distance between aligned cursor and intersections
    for( const VECTOR2I& pt : points )
    {
        SEG::ecoord d_sq = ( pt - aligned ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    return nearest;
}


VECTOR2I PCB_GRID_HELPER::AlignToNearestPad( const VECTOR2I& aMousePos, std::deque<PAD*>& aPads )
{
    clearAnchors();

    for( BOARD_ITEM* item : aPads )
        computeAnchors( item, aMousePos, true, nullptr );

    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* nearestOrigin = nullptr;

    for( ANCHOR& a : m_anchors )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( a.item );

        if( ( ORIGIN & a.flags ) != ORIGIN )
            continue;

        if( !item->HitTest( aMousePos ) )
            continue;

        double dist = a.Distance( aMousePos );

        if( dist < minDist )
        {
            minDist = dist;
            nearestOrigin = &a;
        }
    }

    return nearestOrigin ? nearestOrigin->pos : aMousePos;
}


VECTOR2I PCB_GRID_HELPER::BestDragOrigin( const VECTOR2I &aMousePos,
                                          std::vector<BOARD_ITEM*>& aItems,
                                          GRID_HELPER_GRIDS aGrid,
                                          const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    clearAnchors();

    computeAnchors( aItems, aMousePos, true, aSelectionFilter );

    double worldScale = m_toolMgr->GetView()->GetGAL()->GetWorldScale();
    double lineSnapMinCornerDistance = 50.0 / worldScale;

    ANCHOR* nearestOutline = nearestAnchor( aMousePos, OUTLINE, LSET::AllLayersMask() );
    ANCHOR* nearestCorner = nearestAnchor( aMousePos, CORNER, LSET::AllLayersMask() );
    ANCHOR* nearestOrigin = nearestAnchor( aMousePos, ORIGIN, LSET::AllLayersMask() );
    ANCHOR* best = nullptr;
    double minDist = std::numeric_limits<double>::max();

    if( nearestOrigin )
    {
        minDist = nearestOrigin->Distance( aMousePos );
        best = nearestOrigin;
    }

    if( nearestCorner )
    {
        double dist = nearestCorner->Distance( aMousePos );

        if( dist < minDist )
        {
            minDist = dist;
            best = nearestCorner;
        }
    }

    if( nearestOutline )
    {
        double dist = nearestOutline->Distance( aMousePos );

        if( minDist > lineSnapMinCornerDistance && dist < minDist )
            best = nearestOutline;
    }

    return best ? best->pos : aMousePos;
}


VECTOR2I PCB_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, BOARD_ITEM* aReferenceItem,
                                          GRID_HELPER_GRIDS aGrid )
{
    LSET layers;
    std::vector<BOARD_ITEM*> item;

    if( aReferenceItem )
    {
        layers = aReferenceItem->GetLayerSet();
        item.push_back( aReferenceItem );
    }
    else
    {
        layers = LSET::AllLayersMask();
    }

    return BestSnapAnchor( aOrigin, layers, aGrid, item );
}

VECTOR2I PCB_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
                                          GRID_HELPER_GRIDS               aGrid,
                                          const std::vector<BOARD_ITEM*>& aSkip )
{
    // Tuning constant: snap radius in screen space
    const int snapSize = 25;

    // Snapping distance is in screen space, clamped to the current grid to ensure that the grid
    // points that are visible can always be snapped to.
    // see https://gitlab.com/kicad/code/kicad/-/issues/5638
    // see https://gitlab.com/kicad/code/kicad/-/issues/7125
    // see https://gitlab.com/kicad/code/kicad/-/issues/12303
    double snapScale = snapSize / m_toolMgr->GetView()->GetGAL()->GetWorldScale();
    // warning: GetVisibleGrid().x sometimes returns a value > INT_MAX. Intermediate calculation
    // needs double.
    int snapRange = KiROUND( m_enableGrid ? std::min( snapScale, GetVisibleGrid().x ) : snapScale );
    int snapDist = snapRange;

    //Respect limits of coordinates representation
    BOX2I bb;
    bb.SetOrigin( GetClampedCoords<double, int>( VECTOR2D( aOrigin ) - snapRange / 2.0 ) );
    bb.SetEnd( GetClampedCoords<double, int>( VECTOR2D( aOrigin ) + snapRange / 2.0 ) );

    clearAnchors();

    const std::vector<BOARD_ITEM*> visibleItems = queryVisible( bb, aSkip );
    computeAnchors( visibleItems, aOrigin, false, nullptr );

    ANCHOR*  nearest = nearestAnchor( aOrigin, SNAPPABLE, aLayers );
    VECTOR2I nearestGrid = Align( aOrigin, aGrid );

    if( nearest )
        snapDist = nearest->Distance( aOrigin );

    // Existing snap lines need priority over new snaps
    if( m_snapItem && m_enableSnapLine && m_enableSnap )
    {
        bool snapLine = false;
        int x_dist = std::abs( m_viewSnapLine.GetPosition().x - aOrigin.x );
        int y_dist = std::abs( m_viewSnapLine.GetPosition().y - aOrigin.y );

        /// Allows de-snapping from the line if you are closer to another snap point
        if( x_dist < snapRange && ( !nearest || snapDist > snapRange ) )
        {
            nearestGrid.x = m_viewSnapLine.GetPosition().x;
            snapLine      = true;
        }

        if( y_dist < snapRange && ( !nearest || snapDist > snapRange ) )
        {
            nearestGrid.y = m_viewSnapLine.GetPosition().y;
            snapLine      = true;
        }

        if( snapLine && m_skipPoint != VECTOR2I( m_viewSnapLine.GetPosition() ) )
        {
            m_viewSnapLine.SetEndPosition( nearestGrid );

            if( m_toolMgr->GetView()->IsVisible( &m_viewSnapLine ) )
                m_toolMgr->GetView()->Update( &m_viewSnapLine, KIGFX::GEOMETRY );
            else
                m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, true );

            return nearestGrid;
        }
    }

    if( nearest && m_enableSnap )
    {
        if( nearest->Distance( aOrigin ) <= snapRange )
        {
            m_viewSnapPoint.SetPosition( nearest->pos );
            m_viewSnapLine.SetPosition( nearest->pos );
            m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, false );

            m_viewSnapPoint.SetSnapTypes( nearest->pointTypes );

            if( m_toolMgr->GetView()->IsVisible( &m_viewSnapPoint ) )
                m_toolMgr->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY);
            else
                m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, true );

            m_snapItem = *nearest;
            return nearest->pos;
        }
    }

    m_snapItem = std::nullopt;
    m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
    m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, false );
    return nearestGrid;
}


BOARD_ITEM* PCB_GRID_HELPER::GetSnapped() const
{
    if( !m_snapItem )
        return nullptr;

    return static_cast<BOARD_ITEM*>( m_snapItem->item );
}


GRID_HELPER_GRIDS PCB_GRID_HELPER::GetItemGrid( const EDA_ITEM* aItem ) const
{
    if( !aItem )
        return GRID_CURRENT;

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
    case PCB_PAD_T:
        return GRID_CONNECTABLE;

    case PCB_TEXT_T:
    case PCB_FIELD_T:
        return GRID_TEXT;

    case PCB_SHAPE_T:
    case PCB_DIMENSION_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_TEXTBOX_T:
        return GRID_GRAPHICS;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        return GRID_WIRES;

    case PCB_VIA_T:
        return GRID_VIAS;

    default:
        return GRID_CURRENT;
    }
}


VECTOR2D PCB_GRID_HELPER::GetGridSize( GRID_HELPER_GRIDS aGrid ) const
{
    const GRID_SETTINGS& grid = m_toolMgr->GetSettings()->m_Window.grid;
    int                  idx = -1;

    VECTOR2D g = m_toolMgr->GetView()->GetGAL()->GetGridSize();

    if( !grid.overrides_enabled )
        return g;

    switch( aGrid )
    {
    case GRID_CONNECTABLE:
        if( grid.override_connected )
            idx = grid.override_connected_idx;

        break;

    case GRID_WIRES:
        if( grid.override_wires )
            idx = grid.override_wires_idx;

        break;

    case GRID_VIAS:
        if( grid.override_vias )
            idx = grid.override_vias_idx;

        break;

    case GRID_TEXT:
        if( grid.override_text )
            idx = grid.override_text_idx;

        break;

    case GRID_GRAPHICS:
        if( grid.override_graphics )
            idx = grid.override_graphics_idx;

        break;

    default:
        break;
    }

    if( idx >= 0 && idx < (int) grid.grids.size() )
        g = grid.grids[idx].ToDouble( pcbIUScale );

    return g;
}


std::vector<BOARD_ITEM*>
PCB_GRID_HELPER::queryVisible( const BOX2I& aArea, const std::vector<BOARD_ITEM*>& aSkip ) const
{
    std::set<BOARD_ITEM*> items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    PCB_TOOL_BASE*       currentTool = static_cast<PCB_TOOL_BASE*>( m_toolMgr->GetCurrentTool() );
    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    bool                 isHighContrast = settings->GetHighContrast();

    view->Query( aArea, selectedItems );

    for( const auto& [ viewItem, layer ] : selectedItems )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( viewItem );

        if( currentTool->IsFootprintEditor() )
        {
            // If we are in the footprint editor, don't use the footprint itself
            if( boardItem->Type() == PCB_FOOTPRINT_T )
                continue;
        }
        else
        {
            // If we are not in the footprint editor, don't use footprint-editor-private items
            if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
            {
                if( IsPcbLayer( layer ) && parentFP->GetPrivateLayers().test( layer ) )
                    continue;
            }
        }

        // The boardItem must be visible and on an active layer
        if( view->IsVisible( boardItem )
                && ( !isHighContrast || activeLayers.count( layer ) )
                && boardItem->ViewGetLOD( layer, view ) < view->GetScale() )
        {
            items.insert ( boardItem );
        }
    }

    std::function<void( BOARD_ITEM* )> skipItem =
            [&]( BOARD_ITEM* aItem )
            {
                items.erase( aItem );

                aItem->RunOnDescendants(
                        [&]( BOARD_ITEM* aChild )
                        {
                            skipItem( aChild );
                        } );
            };

    for( BOARD_ITEM* item : aSkip )
        skipItem( item );

    return {items.begin(), items.end()};
}


struct PCB_INTERSECTABLE
{
    BOARD_ITEM*        Item;
    INTERSECTABLE_GEOM Geometry;

    // Clang wants this constructor
    PCB_INTERSECTABLE( BOARD_ITEM* aItem, INTERSECTABLE_GEOM aSeg ) :
            Item( aItem ), Geometry( std::move( aSeg ) )
    {
    }
};


void PCB_GRID_HELPER::computeAnchors( const std::vector<BOARD_ITEM*>& aItems,
                                      const VECTOR2I& aRefPos, bool aFrom,
                                      const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    std::vector<PCB_INTERSECTABLE> intersectables;

    // This c/should come from a more granular snap filter
    const bool computeIntersections = true;
    const bool excludeGraphics = aSelectionFilter && !aSelectionFilter->graphics;
    const bool excludeTracks = aSelectionFilter && !aSelectionFilter->tracks;

    for( BOARD_ITEM* item : aItems )
    {
        // First, add all the key points of the item itself
        computeAnchors( item, aRefPos, aFrom, aSelectionFilter );

        // If we are computing intersections, construct the relevant intersectables
        if( computeIntersections )
        {
            if( !excludeGraphics && item->Type() == PCB_SHAPE_T )
            {
                PCB_SHAPE& shape = static_cast<PCB_SHAPE&>( *item );

                switch( shape.GetShape() )
                {
                case SHAPE_T::SEGMENT:
                {
                    intersectables.emplace_back( &shape, SEG{ shape.GetStart(), shape.GetEnd() } );
                    break;
                }
                case SHAPE_T::CIRCLE:
                {
                    intersectables.emplace_back( &shape,
                                                 CIRCLE{ shape.GetCenter(), shape.GetRadius() } );
                    break;
                }
                case SHAPE_T::ARC:
                {
                    intersectables.emplace_back(
                            &shape,
                            SHAPE_ARC{ shape.GetStart(), shape.GetArcMid(), shape.GetEnd(), 0 } );
                    break;
                }
                case SHAPE_T::RECTANGLE:
                {
                    intersectables.emplace_back( &shape,
                                                 SHAPE_RECT{ shape.GetStart(), shape.GetEnd() } );
                    break;
                }
                default:
                    // Ignore other shapes
                    break;
                }
            }
            else if( !excludeTracks )
            {
                switch( item->Type() )
                {
                case PCB_TRACE_T:
                {
                    PCB_TRACK& track = static_cast<PCB_TRACK&>( *item );

                    intersectables.emplace_back( &track, SEG{ track.GetStart(), track.GetEnd() } );
                    break;
                }
                case PCB_ARC_T:
                {
                    PCB_ARC& arc = static_cast<PCB_ARC&>( *item );

                    intersectables.emplace_back(
                            &arc, SHAPE_ARC{ arc.GetStart(), arc.GetMid(), arc.GetEnd(), 0 } );
                    break;
                }
                default:
                    // Ignore other items
                    break;
                }
            }
        }
    }

    // Now, add all the intersections between the items
    // This is obviously quadratic, so performance may be a concern for large selections
    // But, so far up to ~20k comparisons seems not to be an issue with run times in the ms range

    for( size_t ii = 0; ii < intersectables.size(); ++ii )
    {
        std::vector<VECTOR2I> intersections;
        const PCB_INTERSECTABLE& intersectableA = intersectables[ii];

        const INTERSECTION_VISITOR visitor{ intersectableA.Geometry, intersections };

        for( size_t jj = ii + 1; jj < intersectables.size(); ++jj )
        {
            const PCB_INTERSECTABLE& intersectableB = intersectables[jj];
            std::visit( visitor, intersectableB.Geometry );
        }

        // For each intersection, add an intersection snap anchor
        for( const VECTOR2I& intersection : intersections )
        {
            addAnchor( intersection, SNAPPABLE, intersectableA.Item, POINT_TYPE::PT_INTERSECTION );
        }
    }
}


void PCB_GRID_HELPER::computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom,
                                      const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    bool                 isHighContrast = settings->GetHighContrast();

    auto checkVisibility =
            [&]( BOARD_ITEM* item )
            {
                if( !view->IsVisible( item ) )
                    return false;

                bool onActiveLayer = !isHighContrast;
                bool isLODVisible = false;

                for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
                {
                    if( !onActiveLayer && activeLayers.count( layer ) )
                        onActiveLayer = true;

                    if( !isLODVisible && item->ViewGetLOD( layer, view ) < view->GetScale() )
                        isLODVisible = true;

                    if( onActiveLayer && isLODVisible )
                        return true;
                }

                return false;
            };

    // As defaults, these are probably reasonable to avoid spamming key points
    const OVAL_KEY_POINT_FLAGS ovalKeyPointFlags = OVAL_CENTER
                                                    | OVAL_CAP_TIPS
                                                    | OVAL_SIDE_MIDPOINTS
                                                    | OVAL_CARDINAL_EXTREMES;

    // The key points of a circle centred around (0, 0) with the given radius
    auto getCircleKeyPoints = []( int radius, bool aIncludeCenter )
    {
        std::vector<TYPED_POINT2I> points = {
            { { -radius, 0 }, POINT_TYPE::PT_QUADRANT },
            { { radius, 0 }, POINT_TYPE::PT_QUADRANT },
            { { 0, -radius }, POINT_TYPE::PT_QUADRANT },
            { { 0, radius }, POINT_TYPE::PT_QUADRANT },
        };

        if( aIncludeCenter )
            points.push_back( { { 0, 0 }, POINT_TYPE::PT_CENTER } );

        return points;
    };

    auto handlePadShape = [&]( PAD* aPad )
    {
        addAnchor( aPad->GetPosition(), ORIGIN | SNAPPABLE, aPad, POINT_TYPE::PT_CENTER );

        /// If we are getting a drag point, we don't want to center the edge of pads
        if( aFrom )
            return;

        switch( aPad->GetShape() )
        {
        case PAD_SHAPE::CIRCLE:
            for( const TYPED_POINT2I& pt : getCircleKeyPoints( aPad->GetSizeX() / 2, false ) )
            {
                // Transform to the pad positon
                addAnchor( aPad->ShapePos() + pt.m_point, OUTLINE | SNAPPABLE, aPad, pt.m_types );
            }

            break;

        case PAD_SHAPE::OVAL:
            for( const TYPED_POINT2I& pt :
                 GetOvalKeyPoints( aPad->GetSize(), aPad->GetOrientation(), ovalKeyPointFlags ) )
            {
                // Transform to the pad positon
                addAnchor( aPad->ShapePos() + pt.m_point, OUTLINE | SNAPPABLE, aPad, pt.m_types );
            }

            break;

        case PAD_SHAPE::RECTANGLE:
        case PAD_SHAPE::TRAPEZOID:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::CHAMFERED_RECT:
        {
            VECTOR2I half_size( aPad->GetSize() / 2 );
            VECTOR2I trap_delta( 0, 0 );

            if( aPad->GetShape() == PAD_SHAPE::TRAPEZOID )
                trap_delta = aPad->GetDelta() / 2;

            SHAPE_LINE_CHAIN corners;

            corners.Append( -half_size.x - trap_delta.y, half_size.y + trap_delta.x );
            corners.Append( half_size.x + trap_delta.y, half_size.y - trap_delta.x );
            corners.Append( half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
            corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );
            corners.SetClosed( true );

            corners.Rotate( aPad->GetOrientation() );
            corners.Move( aPad->ShapePos() );

            for( size_t ii = 0; ii < corners.GetSegmentCount(); ++ii )
            {
                const SEG& seg = corners.GetSegment( ii );
                addAnchor( seg.A, OUTLINE | SNAPPABLE, aPad, POINT_TYPE::PT_CORNER );
                addAnchor( seg.Center(), OUTLINE | SNAPPABLE, aPad, POINT_TYPE::PT_MID );

                if( ii == corners.GetSegmentCount() - 1 )
                    addAnchor( seg.B, OUTLINE | SNAPPABLE, aPad, POINT_TYPE::PT_CORNER );
            }

            break;
        }

        default:
        {
            const auto& outline = aPad->GetEffectivePolygon( ERROR_INSIDE );

            if( !outline->IsEmpty() )
            {
                for( const VECTOR2I& pt : outline->Outline( 0 ).CPoints() )
                    addAnchor( pt, OUTLINE | SNAPPABLE, aPad );
            }

            break;
        }
        }

        if( aPad->HasHole() )
        {
            // Holes are at the pad centre (it's the shape that may be offset)
            const VECTOR2I hole_pos = aPad->GetPosition();
            const VECTOR2I hole_size = aPad->GetDrillSize();

            std::vector<TYPED_POINT2I> snap_pts;

            if( hole_size.x == hole_size.y )
            {
                // Circle
                snap_pts = getCircleKeyPoints( hole_size.x / 2, true );
            }
            else
            {
                // Oval

                // For now there's no way to have an off-angle hole, so this is the
                // same as the pad. In future, this may not be true:
                // https://gitlab.com/kicad/code/kicad/-/issues/4124
                snap_pts = GetOvalKeyPoints( hole_size, aPad->GetOrientation(), ovalKeyPointFlags );
            }

            for( const TYPED_POINT2I& snap_pt : snap_pts )
                addAnchor( hole_pos + snap_pt.m_point, OUTLINE | SNAPPABLE, aPad, snap_pt.m_types );
        }
    };

    auto handleShape =
            [&]( PCB_SHAPE* shape )
            {
                VECTOR2I   start = shape->GetStart();
                VECTOR2I   end = shape->GetEnd();

                switch( shape->GetShape() )
                {
                    case SHAPE_T::CIRCLE:
                    {
                        const int r = ( start - end ).EuclideanNorm();

                        addAnchor( start, ORIGIN | SNAPPABLE, shape, POINT_TYPE::PT_CENTER );

                        addAnchor( start + VECTOR2I( -r, 0 ), OUTLINE | SNAPPABLE, shape,
                                   POINT_TYPE::PT_QUADRANT );
                        addAnchor( start + VECTOR2I( r, 0 ), OUTLINE | SNAPPABLE, shape,
                                   POINT_TYPE::PT_QUADRANT );
                        addAnchor( start + VECTOR2I( 0, -r ), OUTLINE | SNAPPABLE, shape,
                                   POINT_TYPE::PT_QUADRANT );
                        addAnchor( start + VECTOR2I( 0, r ), OUTLINE | SNAPPABLE, shape,
                                   POINT_TYPE::PT_QUADRANT );
                        break;
                    }

                    case SHAPE_T::ARC:
                        addAnchor( shape->GetStart(), CORNER | SNAPPABLE, shape,
                                   POINT_TYPE::PT_END );
                        addAnchor( shape->GetEnd(), CORNER | SNAPPABLE, shape,
                                   POINT_TYPE::PT_CORNER );
                        addAnchor( shape->GetArcMid(), CORNER | SNAPPABLE, shape,
                                   POINT_TYPE::PT_MID );
                        addAnchor( shape->GetCenter(), ORIGIN | SNAPPABLE, shape,
                                   POINT_TYPE::PT_CENTER );
                        break;

                    case SHAPE_T::RECTANGLE:
                    {
                        VECTOR2I point2( end.x, start.y );
                        VECTOR2I point3( start.x, end.y );
                        SEG first( start, point2 );
                        SEG second( point2, end );
                        SEG third( end, point3 );
                        SEG fourth( point3, start );

                        const int snapFlags = CORNER | SNAPPABLE;

                        addAnchor( shape->GetCenter(), snapFlags, shape, POINT_TYPE::PT_CENTER );

                        addAnchor( first.A,         snapFlags, shape, POINT_TYPE::PT_CORNER );
                        addAnchor( first.Center(),  snapFlags, shape, POINT_TYPE::PT_MID );
                        addAnchor( second.A,        snapFlags, shape, POINT_TYPE::PT_CORNER );
                        addAnchor( second.Center(), snapFlags, shape, POINT_TYPE::PT_MID );
                        addAnchor( third.A,         snapFlags, shape, POINT_TYPE::PT_CORNER );
                        addAnchor( third.Center(),  snapFlags, shape, POINT_TYPE::PT_MID );
                        addAnchor( fourth.A,        snapFlags, shape, POINT_TYPE::PT_CORNER );
                        addAnchor( fourth.Center(), snapFlags, shape, POINT_TYPE::PT_MID );
                        break;
                    }

                    case SHAPE_T::SEGMENT:
                        addAnchor( start, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                        addAnchor( end, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                        addAnchor( shape->GetCenter(), CORNER | SNAPPABLE, shape,
                                   POINT_TYPE::PT_MID );
                        break;

                    case SHAPE_T::POLY:
                    {
                        SHAPE_LINE_CHAIN lc;
                        lc.SetClosed( true );
                        std::vector<VECTOR2I> poly;
                        shape->DupPolyPointsList( poly );

                        for( const VECTOR2I& p : poly )
                        {
                            addAnchor( p, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_CORNER );
                            lc.Append( p );
                        }

                        addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );
                        break;
                    }

                    case SHAPE_T::BEZIER:
                        addAnchor( start, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                        addAnchor( end, CORNER | SNAPPABLE, shape, POINT_TYPE::PT_END );
                        KI_FALLTHROUGH;

                    default:
                        addAnchor( shape->GetPosition(), ORIGIN | SNAPPABLE, shape );
                        break;
                }
            };

    switch( aItem->Type() )
    {
        case PCB_FOOTPRINT_T:
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );

            for( PAD* pad : footprint->Pads() )
            {
                if( aFrom )
                {
                    if( aSelectionFilter && !aSelectionFilter->pads )
                        continue;
                }
                else
                {
                    if( m_magneticSettings->pads != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                        continue;
                }

                if( !checkVisibility( pad ) )
                    continue;

                if( !pad->GetBoundingBox().Contains( aRefPos ) )
                    continue;

                handlePadShape( pad );
            }

            if( aFrom && aSelectionFilter && !aSelectionFilter->footprints )
                break;

            // If the cursor is not over a pad, snap to the anchor (if visible) or the center
            // (if markedly different from the anchor).
            VECTOR2I position = footprint->GetPosition();
            VECTOR2I center = footprint->GetBoundingBox( false, false ).Centre();
            VECTOR2I grid( GetGrid() );

            if( view->IsLayerVisible( LAYER_ANCHOR ) )
                addAnchor( position, ORIGIN | SNAPPABLE, footprint, POINT_TYPE::PT_CENTER );

            if( ( center - position ).SquaredEuclideanNorm() > grid.SquaredEuclideanNorm() )
                addAnchor( center, ORIGIN | SNAPPABLE, footprint, POINT_TYPE::PT_CENTER );

            break;
        }

        case PCB_PAD_T:
            if( aFrom )
            {
                if( aSelectionFilter && !aSelectionFilter->pads )
                    break;
            }
            else
            {
                if( m_magneticSettings->pads != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                    break;
            }

            if( checkVisibility( aItem ) )
                handlePadShape( static_cast<PAD*>( aItem ) );

            break;

        case PCB_TEXTBOX_T:
            if( aFrom )
            {
                if( aSelectionFilter && !aSelectionFilter->text )
                    break;
            }
            else
            {
                if( !m_magneticSettings->graphics )
                    break;
            }

            if( checkVisibility( aItem ) )
                handleShape( static_cast<PCB_SHAPE*>( aItem ) );

            break;

        case PCB_SHAPE_T:
            if( aFrom )
            {
                if( aSelectionFilter && !aSelectionFilter->graphics )
                    break;
            }
            else
            {
                if( !m_magneticSettings->graphics )
                    break;
            }

            if( checkVisibility( aItem ) )
                handleShape( static_cast<PCB_SHAPE*>( aItem ) );

            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
            if( aFrom )
            {
                if( aSelectionFilter && !aSelectionFilter->tracks )
                    break;
            }
            else
            {
                if( m_magneticSettings->tracks != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                    break;
            }

            if( checkVisibility( aItem ) )
            {
                PCB_TRACK* track = static_cast<PCB_TRACK*>( aItem );

                addAnchor( track->GetStart(), CORNER | SNAPPABLE, track, POINT_TYPE::PT_END );
                addAnchor( track->GetEnd(), CORNER | SNAPPABLE, track, POINT_TYPE::PT_END );
                addAnchor( track->GetCenter(), ORIGIN, track, POINT_TYPE::PT_MID );
            }

            break;

        case PCB_MARKER_T:
        case PCB_TARGET_T:
            addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem,
                       POINT_TYPE::PT_CENTER );
            break;

        case PCB_VIA_T:
            if( aFrom )
            {
                if( aSelectionFilter && !aSelectionFilter->vias )
                    break;
            }
            else
            {
                if( m_magneticSettings->tracks != MAGNETIC_OPTIONS::CAPTURE_ALWAYS )
                    break;
            }

            if( checkVisibility( aItem ) )
                addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem,
                           POINT_TYPE::PT_CENTER );

            break;

        case PCB_ZONE_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->zones )
                break;

            if( checkVisibility( aItem ) )
            {
                const SHAPE_POLY_SET* outline = static_cast<const ZONE*>( aItem )->Outline();

                SHAPE_LINE_CHAIN lc;
                lc.SetClosed( true );

                for( auto iter = outline->CIterateWithHoles(); iter; iter++ )
                {
                    addAnchor( *iter, CORNER | SNAPPABLE, aItem, POINT_TYPE::PT_CORNER );
                    lc.Append( *iter );
                }

                addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );
            }

            break;

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_ORTHOGONAL_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            if( checkVisibility( aItem ) )
            {
                const PCB_DIM_ALIGNED* dim = static_cast<const PCB_DIM_ALIGNED*>( aItem );
                addAnchor( dim->GetCrossbarStart(), CORNER | SNAPPABLE, aItem );
                addAnchor( dim->GetCrossbarEnd(), CORNER | SNAPPABLE, aItem );
                addAnchor( dim->GetStart(), CORNER | SNAPPABLE, aItem );
                addAnchor( dim->GetEnd(), CORNER | SNAPPABLE, aItem );
            }

            break;

        case PCB_DIM_CENTER_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            if( checkVisibility( aItem ) )
            {
                const PCB_DIM_CENTER* dim = static_cast<const PCB_DIM_CENTER*>( aItem );
                addAnchor( dim->GetStart(), CORNER | SNAPPABLE, aItem );
                addAnchor( dim->GetEnd(), CORNER | SNAPPABLE, aItem );

                VECTOR2I start( dim->GetStart() );
                VECTOR2I radial( dim->GetEnd() - dim->GetStart() );

                for( int i = 0; i < 2; i++ )
                {
                    RotatePoint( radial, -ANGLE_90 );
                    addAnchor( start + radial, CORNER | SNAPPABLE, aItem );
                }
            }

            break;

        case PCB_DIM_RADIAL_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            if( checkVisibility( aItem ) )
            {
                const PCB_DIM_RADIAL* radialDim = static_cast<const PCB_DIM_RADIAL*>( aItem );
                addAnchor( radialDim->GetStart(), CORNER | SNAPPABLE, aItem );
                addAnchor( radialDim->GetEnd(), CORNER | SNAPPABLE, aItem );
                addAnchor( radialDim->GetKnee(), CORNER | SNAPPABLE, aItem );
                addAnchor( radialDim->GetTextPos(), CORNER | SNAPPABLE, aItem );
            }

            break;

        case PCB_DIM_LEADER_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            if( checkVisibility( aItem ) )
            {
                const PCB_DIM_LEADER* leader = static_cast<const PCB_DIM_LEADER*>( aItem );
                addAnchor( leader->GetStart(), CORNER | SNAPPABLE, aItem );
                addAnchor( leader->GetEnd(), CORNER | SNAPPABLE, aItem );
                addAnchor( leader->GetTextPos(), CORNER | SNAPPABLE, aItem );
            }

            break;

        case PCB_FIELD_T:
        case PCB_TEXT_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->text )
                break;

            if( checkVisibility( aItem ) )
                addAnchor( aItem->GetPosition(), ORIGIN, aItem );

            break;

        case PCB_GROUP_T:
            for( BOARD_ITEM* item : static_cast<const PCB_GROUP*>( aItem )->GetItems() )
            {
                if( checkVisibility( item ) )
                    computeAnchors( item, aRefPos, aFrom, nullptr );
            }

            break;

        default:
            break;
   }
}


PCB_GRID_HELPER::ANCHOR* PCB_GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags,
                                                         LSET aMatchLayers )
{
    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* best = nullptr;

    for( ANCHOR& a : m_anchors )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( a.item );

        if( !m_magneticSettings->allLayers && ( ( aMatchLayers & item->GetLayerSet() ).none() ) )
            continue;

        if( ( aFlags & a.flags ) != aFlags )
            continue;

        double dist = a.Distance( aPos );

        if( dist < minDist )
        {
            minDist = dist;
            best = &a;
        }
    }

    return best;
}
