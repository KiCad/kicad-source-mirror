/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
using namespace std::placeholders;

#include <pcb_edit_frame.h>

#include <class_board.h>
#include <class_dimension.h>
#include <class_draw_panel_gal.h>
#include <class_edge_mod.h>
#include <class_module.h>
#include <class_zone.h>

#include <painter.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>

#include <geometry/shape_line_chain.h>

#include "grid_helper.h"


GRID_HELPER::GRID_HELPER( PCB_BASE_FRAME* aFrame ) :
    m_frame( aFrame )
{
    m_enableSnap = true;
    m_enableGrid = true;
    m_snapSize = 100;
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

    m_viewAxis.SetSize( 20000 );
    m_viewAxis.SetStyle( KIGFX::ORIGIN_VIEWITEM::CROSS );
    m_viewAxis.SetColor( COLOR4D( 1.0, 1.0, 1.0, 0.4 ) );
    m_viewAxis.SetDrawAtZero( true );
    view->Add( &m_viewAxis );
    view->SetVisible( &m_viewAxis, false );

    m_viewSnapPoint.SetStyle( KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS );
    m_viewSnapPoint.SetColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    m_viewSnapPoint.SetDrawAtZero( true );
    view->Add( &m_viewSnapPoint );
    view->SetVisible( &m_viewSnapPoint, false );
}


GRID_HELPER::~GRID_HELPER()
{
}


VECTOR2I GRID_HELPER::GetGrid() const
{
    PCB_SCREEN* screen = m_frame->GetScreen();

    const wxRealPoint& size = screen->GetGridSize();

    return VECTOR2I( KiROUND( size.x ), KiROUND( size.y ) );
}


VECTOR2I GRID_HELPER::GetOrigin() const
{
    return VECTOR2I( m_frame->GetGridOrigin() );
}


void GRID_HELPER::SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin )
{
    if( aEnable )
    {
        m_auxAxis = aOrigin;
        m_viewAxis.SetPosition( aOrigin );
        m_frame->GetCanvas()->GetView()->SetVisible( &m_viewAxis, true );
    }
    else
    {
        m_auxAxis = OPT<VECTOR2I>();
        m_frame->GetCanvas()->GetView()->SetVisible( &m_viewAxis, false );
    }
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint ) const
{
    if( !m_enableGrid )
        return aPoint;

    const VECTOR2D gridOffset( GetOrigin() );
    const VECTOR2D grid( GetGrid() );

    VECTOR2I nearest( KiROUND( ( aPoint.x - gridOffset.x ) / grid.x ) * grid.x + gridOffset.x,
                      KiROUND( ( aPoint.y - gridOffset.y ) / grid.y ) * grid.y + gridOffset.y );

    if( !m_auxAxis )
        return nearest;

    if( std::abs( m_auxAxis->x - aPoint.x ) < std::abs( nearest.x - aPoint.x ) )
        nearest.x = m_auxAxis->x;

    if( std::abs( m_auxAxis->y - aPoint.y ) < std::abs( nearest.y - aPoint.y ) )
        nearest.y = m_auxAxis->y;

    return nearest;
}


VECTOR2I GRID_HELPER::AlignToSegment( const VECTOR2I& aPoint, const SEG& aSeg )
{
    OPT_VECTOR2I pts[6];

    if( !m_enableSnap )
        return aPoint;

    const VECTOR2D gridOffset( GetOrigin() );
    const VECTOR2D gridSize( GetGrid() );

    VECTOR2I nearest( KiROUND( ( aPoint.x - gridOffset.x ) / gridSize.x ) * gridSize.x + gridOffset.x,
                      KiROUND( ( aPoint.y - gridOffset.y ) / gridSize.y ) * gridSize.y + gridOffset.y );

    pts[0] = aSeg.A;
    pts[1] = aSeg.B;
    pts[2] = aSeg.IntersectLines( SEG( nearest + VECTOR2I( -1, 1 ), nearest + VECTOR2I( 1, -1 ) ) );
    pts[3] = aSeg.IntersectLines( SEG( nearest + VECTOR2I( -1, -1 ), nearest + VECTOR2I( 1, 1 ) ) );

    int min_d = std::numeric_limits<int>::max();

    for( int i = 0; i < 4; i++ )
    {
        if( pts[i] && aSeg.Contains( *pts[i] ) )
        {
            int d = (*pts[i] - aPoint).EuclideanNorm();

            if( d < min_d )
            {
                min_d = d;
                nearest = *pts[i];
            }
        }
    }

    return nearest;
}


VECTOR2I GRID_HELPER::BestDragOrigin( const VECTOR2I &aMousePos, BOARD_ITEM* aItem )
{
    clearAnchors();
    computeAnchors( aItem, aMousePos, true );

    double worldScale = m_frame->GetCanvas()->GetGAL()->GetWorldScale();
    double lineSnapMinCornerDistance = 50.0 / worldScale;

    ANCHOR* nearestOutline = nearestAnchor( aMousePos, OUTLINE, LSET::AllLayersMask() );
    ANCHOR* nearestCorner = nearestAnchor( aMousePos, CORNER, LSET::AllLayersMask() );
    ANCHOR* nearestOrigin = nearestAnchor( aMousePos, ORIGIN, LSET::AllLayersMask() );
    ANCHOR* best = NULL;
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


std::set<BOARD_ITEM*> GRID_HELPER::queryVisible( const BOX2I& aArea,
        const std::vector<BOARD_ITEM*> aSkip ) const
{
    std::set<BOARD_ITEM*> items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    auto view = m_frame->GetCanvas()->GetView();
    auto activeLayers = view->GetPainter()->GetSettings()->GetActiveLayers();
    bool isHighContrast = view->GetPainter()->GetSettings()->GetHighContrast();
    view->Query( aArea, selectedItems );

    for( auto it : selectedItems )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

        // The item must be visible and on an active layer
        if( view->IsVisible( item ) && ( !isHighContrast || activeLayers.count( it.second ) ) )
            items.insert ( item );
    }


    for( auto ii : aSkip )
        items.erase( ii );

    return items;
}


VECTOR2I GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, BOARD_ITEM* aDraggedItem )
{
    LSET layers;
    std::vector<BOARD_ITEM*> item;

    if( aDraggedItem )
    {
        layers = aDraggedItem->GetLayerSet();
        item.push_back( aDraggedItem );
    }
    else
        layers = LSET::AllLayersMask();

    return BestSnapAnchor( aOrigin, layers, item );
}


VECTOR2I GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
        const std::vector<BOARD_ITEM*> aSkip )
{
    double worldScale = m_frame->GetCanvas()->GetGAL()->GetWorldScale();
    int snapRange = (int) ( m_snapSize / worldScale );

    BOX2I bb( VECTOR2I( aOrigin.x - snapRange / 2, aOrigin.y - snapRange / 2 ), VECTOR2I( snapRange, snapRange ) );

    clearAnchors();

    for( BOARD_ITEM* item : queryVisible( bb, aSkip ) )
        computeAnchors( item, aOrigin );

    ANCHOR* nearest = nearestAnchor( aOrigin, SNAPPABLE, aLayers );
    VECTOR2I nearestGrid = Align( aOrigin );
    double gridDist = ( nearestGrid - aOrigin ).EuclideanNorm();

    if( nearest && m_enableSnap )
    {
        double snapDist = nearest->Distance( aOrigin );

        if( !m_enableGrid || snapDist <= gridDist )
        {
            m_viewSnapPoint.SetPosition( nearest->pos );

            if( m_frame->GetCanvas()->GetView()->IsVisible( &m_viewSnapPoint ) )
                m_frame->GetCanvas()->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY);
            else
                m_frame->GetCanvas()->GetView()->SetVisible( &m_viewSnapPoint, true );

            m_snapItem = nearest;
            return nearest->pos;
        }
    }

    m_snapItem = nullptr;
    m_frame->GetCanvas()->GetView()->SetVisible( &m_viewSnapPoint, false );
    return nearestGrid;
}


BOARD_ITEM* GRID_HELPER::GetSnapped( void ) const
{
    if( !m_snapItem )
        return nullptr;

    return m_snapItem->item;
}


void GRID_HELPER::computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom )
{
    VECTOR2I origin;

    switch( aItem->Type() )
    {
        case PCB_MODULE_T:
        {
            MODULE* mod = static_cast<MODULE*>( aItem );

            for( auto pad : mod->Pads() )
            {
                if( ( aFrom || m_frame->Settings().m_MagneticPads == CAPTURE_ALWAYS )
                    && pad->GetBoundingBox().Contains( wxPoint( aRefPos.x, aRefPos.y ) ) )
                {
                    addAnchor( pad->GetPosition(), CORNER | SNAPPABLE, pad );
                    break;
                }
            }

            // if the cursor is not over a pad, then drag the module by its origin
            addAnchor( mod->GetPosition(), ORIGIN | SNAPPABLE, mod );
            break;
        }

        case PCB_PAD_T:
        {
            if( aFrom || m_frame->Settings().m_MagneticPads == CAPTURE_ALWAYS )
            {
                D_PAD* pad = static_cast<D_PAD*>( aItem );
                addAnchor( pad->GetPosition(), CORNER | SNAPPABLE, pad );
            }

            break;
        }

        case PCB_MODULE_EDGE_T:
        case PCB_LINE_T:
        {
            if( !m_frame->Settings().m_MagneticGraphics )
                break;

            DRAWSEGMENT* dseg = static_cast<DRAWSEGMENT*>( aItem );
            VECTOR2I start = dseg->GetStart();
            VECTOR2I end = dseg->GetEnd();

            switch( dseg->GetShape() )
            {
                case S_CIRCLE:
                {
                    int r = ( start - end ).EuclideanNorm();

                    addAnchor( start, ORIGIN | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I( -r, 0 ), OUTLINE | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I( r, 0 ), OUTLINE | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I( 0, -r ), OUTLINE | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I( 0, r ), OUTLINE | SNAPPABLE, dseg );
                    break;
                }

                case S_ARC:
                    origin = dseg->GetCenter();
                    addAnchor( dseg->GetArcStart(), CORNER | SNAPPABLE, dseg );
                    addAnchor( dseg->GetArcEnd(), CORNER | SNAPPABLE, dseg );
                    addAnchor( origin, ORIGIN | SNAPPABLE, dseg );
                    break;

                case S_RECT:
                    addAnchor( start, CORNER | SNAPPABLE, dseg );
                    addAnchor( VECTOR2I( end.x, start.y ), CORNER | SNAPPABLE, dseg );
                    addAnchor( VECTOR2I( start.x, end.y ), CORNER | SNAPPABLE, dseg );
                    addAnchor( end, CORNER | SNAPPABLE, dseg );
                    break;

                case S_SEGMENT:
                    origin.x = start.x + ( start.x - end.x ) / 2;
                    origin.y = start.y + ( start.y - end.y ) / 2;
                    addAnchor( start, CORNER | SNAPPABLE, dseg );
                    addAnchor( end, CORNER | SNAPPABLE, dseg );
                    addAnchor( origin, ORIGIN, dseg );
                    break;

                case S_POLYGON:
                    for( const auto& p : dseg->BuildPolyPointsList() )
                        addAnchor( p, CORNER | SNAPPABLE, dseg );

                    break;

                case S_CURVE:
                    addAnchor( start, CORNER | SNAPPABLE, dseg );
                    addAnchor( end, CORNER | SNAPPABLE, dseg );
                    //Fallthrough
                default:
                    origin = dseg->GetStart();
                    addAnchor( origin, ORIGIN | SNAPPABLE, dseg );
                    break;
            }
            break;
        }

        case PCB_TRACE_T:
        {
            if( aFrom || m_frame->Settings().m_MagneticTracks == CAPTURE_ALWAYS )
            {
                TRACK* track = static_cast<TRACK*>( aItem );
                VECTOR2I start = track->GetStart();
                VECTOR2I end = track->GetEnd();
                origin.x = start.x + ( start.x - end.x ) / 2;
                origin.y = start.y + ( start.y - end.y ) / 2;
                addAnchor( start, CORNER | SNAPPABLE, track );
                addAnchor( end, CORNER | SNAPPABLE, track );
                addAnchor( origin, ORIGIN, track);
            }

            break;
        }

        case PCB_MARKER_T:
        case PCB_TARGET_T:
            addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem );
            break;

        case PCB_VIA_T:
        {
            if( aFrom || m_frame->Settings().m_MagneticTracks == CAPTURE_ALWAYS )
                addAnchor( aItem->GetPosition(), ORIGIN | CORNER | SNAPPABLE, aItem );

            break;
        }

        case PCB_ZONE_AREA_T:
        {
            const SHAPE_POLY_SET* outline = static_cast<const ZONE_CONTAINER*>( aItem )->Outline();

            SHAPE_LINE_CHAIN lc;
            lc.SetClosed( true );

            for( auto iter = outline->CIterateWithHoles(); iter; iter++ )
            {
                addAnchor( *iter, CORNER, aItem );
                lc.Append( *iter );
            }

            addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );

            break;
        }

        case PCB_DIMENSION_T:
        {
            const DIMENSION* dim = static_cast<const DIMENSION*>( aItem );
            addAnchor( dim->m_crossBarF, CORNER | SNAPPABLE, aItem );
            addAnchor( dim->m_crossBarO, CORNER | SNAPPABLE, aItem );
            addAnchor( dim->m_featureLineGO, CORNER | SNAPPABLE, aItem );
            addAnchor( dim->m_featureLineDO, CORNER | SNAPPABLE, aItem );
            break;
        }

        case PCB_MODULE_TEXT_T:
        case PCB_TEXT_T:
            addAnchor( aItem->GetPosition(), ORIGIN, aItem );
            break;

        default:
            break;
   }
}


GRID_HELPER::ANCHOR* GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags, LSET aMatchLayers )
{
    double minDist = std::numeric_limits<double>::max();
    ANCHOR* best = NULL;

    for( ANCHOR& a : m_anchors )
    {
        if( ( aMatchLayers & a.item->GetLayerSet() ) == 0 )
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
