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

#include <functional>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <zone.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <macros.h>
#include <math/util.h> // for KiROUND
#include <painter.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_tool_base.h>
#include <view/view.h>
#include "pcb_grid_helper.h"


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


VECTOR2I PCB_GRID_HELPER::AlignToNearestPad( const VECTOR2I& aMousePos, PADS& aPads )
{
    clearAnchors();

    for( BOARD_ITEM* item : aPads )
        computeAnchors( item, aMousePos, true );

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
                                          const SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    clearAnchors();

    for( BOARD_ITEM* item : aItems )
        computeAnchors( item, aMousePos, true, aSelectionFilter );

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


VECTOR2I PCB_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, BOARD_ITEM* aReferenceItem )
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

    return BestSnapAnchor( aOrigin, layers, item );
}


VECTOR2I PCB_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
                                          const std::vector<BOARD_ITEM*>& aSkip )
{
    // Tuning constant: snap radius in screen space
    const int snapSize = 25;

    // Snapping distance is in screen space, clamped to the current grid to ensure that the grid
    // points that are visible can always be snapped to.
    // see https://gitlab.com/kicad/code/kicad/-/issues/5638
    // see https://gitlab.com/kicad/code/kicad/-/issues/7125
    // see https://gitlab.com/kicad/code/kicad/-/issues/12303
    int snapScale = KiROUND( snapSize / m_toolMgr->GetView()->GetGAL()->GetWorldScale() );
    int snapRange = m_enableGrid ? std::min( snapScale, GetVisibleGrid().x ) : snapScale;
    int snapDist = snapRange;

    //Respect limits of coordinates representation
    BOX2I bb;
    bb.SetOrigin( GetClampedCoords<double, int>( VECTOR2D( aOrigin ) - snapRange / 2.0 ) );
    bb.SetEnd( GetClampedCoords<double, int>( VECTOR2D( aOrigin ) + snapRange / 2.0 ) );

    clearAnchors();

    for( BOARD_ITEM* item : queryVisible( bb, aSkip ) )
        computeAnchors( item, aOrigin );

    ANCHOR*  nearest = nearestAnchor( aOrigin, SNAPPABLE, aLayers );
    VECTOR2I nearestGrid = Align( aOrigin );

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

            if( m_toolMgr->GetView()->IsVisible( &m_viewSnapPoint ) )
                m_toolMgr->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY);
            else
                m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, true );

            m_snapItem = nearest;
            return nearest->pos;
        }
    }

    m_snapItem = nullptr;
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


std::set<BOARD_ITEM*> PCB_GRID_HELPER::queryVisible( const BOX2I& aArea,
                                                     const std::vector<BOARD_ITEM*>& aSkip ) const
{
    std::set<BOARD_ITEM*> items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    bool                 isHighContrast = settings->GetHighContrast();

    view->Query( aArea, selectedItems );

    for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : selectedItems )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

        // If we are in the footprint editor, don't use the footprint itself
        if( static_cast<PCB_TOOL_BASE*>( m_toolMgr->GetCurrentTool() )->IsFootprintEditor()
                && item->Type() == PCB_FOOTPRINT_T )
        {
            continue;
        }

        // The item must be visible and on an active layer
        if( view->IsVisible( item )
                && ( !isHighContrast || activeLayers.count( it.second ) )
                && item->ViewGetLOD( it.second, view ) < view->GetScale() )
        {
            items.insert ( item );
        }
    }

    for( BOARD_ITEM* skipItem : aSkip )
        items.erase( skipItem );

    return items;
}


void PCB_GRID_HELPER::computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom,
                                      const SELECTION_FILTER_OPTIONS* aSelectionFilter )
{
    KIGFX::VIEW*         view = m_toolMgr->GetView();
    RENDER_SETTINGS*     settings = view->GetPainter()->GetSettings();
    const std::set<int>& activeLayers = settings->GetHighContrastLayers();
    bool                 isHighContrast = settings->GetHighContrast();

    auto handlePadShape =
            [&]( PAD* aPad )
            {
                addAnchor( aPad->GetPosition(), ORIGIN | SNAPPABLE, aPad );

                /// If we are getting a drag point, we don't want to center the edge of pads
                if( aFrom )
                    return;

                switch( aPad->GetShape() )
                {
                case PAD_SHAPE::CIRCLE:
                {
                    int      r      = aPad->GetSizeX() / 2;
                    VECTOR2I center = aPad->ShapePos();

                    addAnchor( center + VECTOR2I( -r, 0 ), OUTLINE | SNAPPABLE, aPad );
                    addAnchor( center + VECTOR2I( r, 0 ), OUTLINE | SNAPPABLE, aPad );
                    addAnchor( center + VECTOR2I( 0, -r ), OUTLINE | SNAPPABLE, aPad );
                    addAnchor( center + VECTOR2I( 0, r ), OUTLINE | SNAPPABLE, aPad );
                    break;
                }

                case PAD_SHAPE::OVAL:
                {
                    VECTOR2I pos = aPad->ShapePos();
                    VECTOR2I half_size = aPad->GetSize() / 2;
                    int      half_width = std::min( half_size.x, half_size.y );
                    VECTOR2I half_len( half_size.x - half_width, half_size.y - half_width );

                    RotatePoint( half_len, aPad->GetOrientation() );

                    VECTOR2I a( pos - half_len );
                    VECTOR2I b( pos + half_len );
                    VECTOR2I normal = b - a;
                    normal.Resize( half_width );
                    RotatePoint( normal, ANGLE_90 );

                    addAnchor( a + normal, OUTLINE | SNAPPABLE, aPad );
                    addAnchor( a - normal, OUTLINE | SNAPPABLE, aPad );
                    addAnchor( b + normal, OUTLINE | SNAPPABLE, aPad );
                    addAnchor( b - normal, OUTLINE | SNAPPABLE, aPad );
                    addAnchor( pos + normal, OUTLINE | SNAPPABLE, aPad );
                    addAnchor( pos - normal, OUTLINE | SNAPPABLE, aPad );

                    RotatePoint( normal, -ANGLE_90 );

                    addAnchor( a - normal, OUTLINE | SNAPPABLE, aPad );
                    addAnchor( b + normal, OUTLINE | SNAPPABLE, aPad );
                    break;
                }

                case PAD_SHAPE::RECT:
                case PAD_SHAPE::TRAPEZOID:
                case PAD_SHAPE::ROUNDRECT:
                case PAD_SHAPE::CHAMFERED_RECT:
                {
                    VECTOR2I half_size( aPad->GetSize() / 2 );
                    VECTOR2I trap_delta( 0, 0 );

                    if( aPad->GetShape() == PAD_SHAPE::TRAPEZOID )
                        trap_delta = aPad->GetDelta() / 2;

                    SHAPE_LINE_CHAIN corners;

                    corners.Append( -half_size.x - trap_delta.y,  half_size.y + trap_delta.x );
                    corners.Append(  half_size.x + trap_delta.y,  half_size.y - trap_delta.x );
                    corners.Append(  half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
                    corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );
                    corners.SetClosed( true );

                    corners.Rotate( aPad->GetOrientation() );
                    corners.Move( aPad->ShapePos() );

                    for( size_t ii = 0; ii < corners.GetSegmentCount(); ++ii )
                    {
                        const SEG& seg = corners.GetSegment( ii );
                        addAnchor( seg.A, OUTLINE | SNAPPABLE, aPad );
                        addAnchor( seg.Center(), OUTLINE | SNAPPABLE, aPad );

                        if( ii == corners.GetSegmentCount() - 1 )
                            addAnchor( seg.B, OUTLINE | SNAPPABLE, aPad );
                    }

                    break;
                }

                default:
                {
                    const std::shared_ptr<SHAPE_POLY_SET>& outline = aPad->GetEffectivePolygon();

                    if( !outline->IsEmpty() )
                    {
                        for( const VECTOR2I& pt : outline->Outline( 0 ).CPoints() )
                            addAnchor( pt, OUTLINE | SNAPPABLE, aPad );
                    }

                    break;
                }
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
                        int r = ( start - end ).EuclideanNorm();

                        addAnchor( start, ORIGIN | SNAPPABLE, shape );
                        addAnchor( start + VECTOR2I( -r, 0 ), OUTLINE | SNAPPABLE, shape );
                        addAnchor( start + VECTOR2I( r, 0 ), OUTLINE | SNAPPABLE, shape );
                        addAnchor( start + VECTOR2I( 0, -r ), OUTLINE | SNAPPABLE, shape );
                        addAnchor( start + VECTOR2I( 0, r ), OUTLINE | SNAPPABLE, shape );
                        break;
                    }

                    case SHAPE_T::ARC:
                        addAnchor( shape->GetStart(), CORNER | SNAPPABLE, shape );
                        addAnchor( shape->GetEnd(), CORNER | SNAPPABLE, shape );
                        addAnchor( shape->GetArcMid(), CORNER | SNAPPABLE, shape );
                        addAnchor( shape->GetCenter(), ORIGIN | SNAPPABLE, shape );
                        break;

                    case SHAPE_T::RECT:
                    {
                        VECTOR2I point2( end.x, start.y );
                        VECTOR2I point3( start.x, end.y );
                        SEG first( start, point2 );
                        SEG second( point2, end );
                        SEG third( end, point3 );
                        SEG fourth( point3, start );

                        addAnchor( first.A,         CORNER | SNAPPABLE, shape );
                        addAnchor( first.Center(),  CORNER | SNAPPABLE, shape );
                        addAnchor( second.A,        CORNER | SNAPPABLE, shape );
                        addAnchor( second.Center(), CORNER | SNAPPABLE, shape );
                        addAnchor( third.A,         CORNER | SNAPPABLE, shape );
                        addAnchor( third.Center(),  CORNER | SNAPPABLE, shape );
                        addAnchor( fourth.A,        CORNER | SNAPPABLE, shape );
                        addAnchor( fourth.Center(), CORNER | SNAPPABLE, shape );
                        break;
                    }

                    case SHAPE_T::SEGMENT:
                        addAnchor( start, CORNER | SNAPPABLE, shape );
                        addAnchor( end, CORNER | SNAPPABLE, shape );
                        addAnchor( shape->GetCenter(), CORNER | SNAPPABLE, shape );
                        break;

                    case SHAPE_T::POLY:
                    {
                        SHAPE_LINE_CHAIN lc;
                        lc.SetClosed( true );
                        std::vector<VECTOR2I> poly;
                        shape->DupPolyPointsList( poly );

                        for( const VECTOR2I& p : poly )
                        {
                            addAnchor( p, CORNER | SNAPPABLE, shape );
                            lc.Append( p );
                        }

                        addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );
                        break;
                    }

                    case SHAPE_T::BEZIER:
                        addAnchor( start, CORNER | SNAPPABLE, shape );
                        addAnchor( end, CORNER | SNAPPABLE, shape );
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

                if( !view->IsVisible( pad ) || !pad->GetBoundingBox().Contains( aRefPos ) )
                    continue;

                // Getting pads from a footprint requires re-checking that the pads are shown
                bool onActiveLayer = !isHighContrast;
                bool isLODVisible = false;

                for( PCB_LAYER_ID layer : pad->GetLayerSet().Seq() )
                {
                    if( !onActiveLayer && activeLayers.count( layer ) )
                        onActiveLayer = true;

                    if( !isLODVisible && pad->ViewGetLOD( layer, view ) < view->GetScale() )
                        isLODVisible = true;

                    if( onActiveLayer && isLODVisible )
                    {
                        handlePadShape( pad );
                        break;
                    }
                }
            }

            if( aFrom && aSelectionFilter && !aSelectionFilter->footprints )
                break;

            // if the cursor is not over a pad, then drag the footprint by its origin
            VECTOR2I position = footprint->GetPosition();
            addAnchor( position, ORIGIN | SNAPPABLE, footprint );

            // Add the footprint center point if it is markedly different from the origin
            VECTOR2I center = footprint->GetBoundingBox( false, false ).Centre();
            VECTOR2I grid( GetGrid() );

            if( ( center - position ).SquaredEuclideanNorm() > grid.SquaredEuclideanNorm() )
                addAnchor( center, ORIGIN | SNAPPABLE, footprint );

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

            handleShape( static_cast<PCB_SHAPE*>( aItem ) );
            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
        {
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

            PCB_TRACK* track = static_cast<PCB_TRACK*>( aItem );

            addAnchor( track->GetStart(), CORNER | SNAPPABLE, track );
            addAnchor( track->GetEnd(), CORNER | SNAPPABLE, track );
            addAnchor( track->GetCenter(), ORIGIN, track);

            break;
        }

        case PCB_MARKER_T:
        case PCB_TARGET_T:
            addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem );
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

            addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem );

            break;

        case PCB_ZONE_T:
        {
            if( aFrom && aSelectionFilter && !aSelectionFilter->zones )
                break;

            const SHAPE_POLY_SET* outline = static_cast<const ZONE*>( aItem )->Outline();

            SHAPE_LINE_CHAIN lc;
            lc.SetClosed( true );

            for( auto iter = outline->CIterateWithHoles(); iter; iter++ )
            {
                addAnchor( *iter, CORNER | SNAPPABLE, aItem );
                lc.Append( *iter );
            }

            addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );

            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_ORTHOGONAL_T:
        {
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            const PCB_DIM_ALIGNED* dim = static_cast<const PCB_DIM_ALIGNED*>( aItem );
            addAnchor( dim->GetCrossbarStart(), CORNER | SNAPPABLE, aItem );
            addAnchor( dim->GetCrossbarEnd(), CORNER | SNAPPABLE, aItem );
            addAnchor( dim->GetStart(), CORNER | SNAPPABLE, aItem );
            addAnchor( dim->GetEnd(), CORNER | SNAPPABLE, aItem );
            break;
        }

        case PCB_DIM_CENTER_T:
        {
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

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

            break;
        }

        case PCB_DIM_RADIAL_T:
        {
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            const PCB_DIM_RADIAL* radialDim = static_cast<const PCB_DIM_RADIAL*>( aItem );
            addAnchor( radialDim->GetStart(), CORNER | SNAPPABLE, aItem );
            addAnchor( radialDim->GetEnd(), CORNER | SNAPPABLE, aItem );
            addAnchor( radialDim->GetKnee(), CORNER | SNAPPABLE, aItem );
            addAnchor( radialDim->GetTextPos(), CORNER | SNAPPABLE, aItem );
            break;
        }

        case PCB_DIM_LEADER_T:
        {
            if( aFrom && aSelectionFilter && !aSelectionFilter->dimensions )
                break;

            const PCB_DIM_LEADER* leader = static_cast<const PCB_DIM_LEADER*>( aItem );
            addAnchor( leader->GetStart(), CORNER | SNAPPABLE, aItem );
            addAnchor( leader->GetEnd(), CORNER | SNAPPABLE, aItem );
            addAnchor( leader->GetTextPos(), CORNER | SNAPPABLE, aItem );
            break;
        }

        case PCB_TEXT_T:
            if( aFrom && aSelectionFilter && !aSelectionFilter->text )
                break;

            addAnchor( aItem->GetPosition(), ORIGIN, aItem );
            break;

        case PCB_GROUP_T:
        {
            const PCB_GROUP* group = static_cast<const PCB_GROUP*>( aItem );

            for( BOARD_ITEM* item : group->GetItems() )
                computeAnchors( item, aRefPos, aFrom );

            break;
        }

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

        if( ( aMatchLayers & item->GetLayerSet() ) == 0 )
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
