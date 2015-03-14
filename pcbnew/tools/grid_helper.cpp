/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <wxPcbStruct.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_draw_panel_gal.h>

#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>

#include <geometry/shape_line_chain.h>

#include "grid_helper.h"

GRID_HELPER::GRID_HELPER( PCB_BASE_FRAME* aFrame ) :
    m_frame( aFrame )
{
    m_diagonalAuxAxesEnable = true;
}


GRID_HELPER::~GRID_HELPER()
{
}


void GRID_HELPER::SetGrid( int aSize )
{
    assert( false );
}


void GRID_HELPER::SetOrigin( const VECTOR2I& aOrigin )
{
}


VECTOR2I GRID_HELPER::GetGrid()
{
    PCB_SCREEN* screen = m_frame->GetScreen();

    const wxRealPoint& size = screen->GetGridSize();

    return VECTOR2I ( KiROUND( size.x ), KiROUND( size.y ) );
}


VECTOR2I GRID_HELPER::GetOrigin()
{
    return VECTOR2I( 0, 0 );
}


void GRID_HELPER::SetAuxAxes( bool aEnable, const VECTOR2I aOrigin, bool aEnableDiagonal )
{
    if( aEnable )
        m_auxAxis = aOrigin;
    else
        m_auxAxis = boost::optional<VECTOR2I>();

    m_diagonalAuxAxesEnable = aEnable;
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint )
{
    const VECTOR2D gridOffset( GetOrigin () );
    const VECTOR2D gridSize( GetGrid() );

    VECTOR2I nearest( round( ( aPoint.x - gridOffset.x ) / gridSize.x ) * gridSize.x + gridOffset.x,
                       round( ( aPoint.y - gridOffset.y ) / gridSize.y ) * gridSize.y + gridOffset.y );

    if( !m_auxAxis )
        return nearest;

    if( std::abs( m_auxAxis->x - aPoint.x) < std::abs( nearest.x - aPoint.x ) )
        nearest.x = m_auxAxis->x;

    if( std::abs( m_auxAxis->y - aPoint.y) < std::abs( nearest.y - aPoint.y ) )
        nearest.y = m_auxAxis->y;

    return nearest;
}


VECTOR2I GRID_HELPER::BestDragOrigin( const VECTOR2I &aMousePos, BOARD_ITEM* aItem )
{
    clearAnchors();
    computeAnchors( aItem, aMousePos );

    double worldScale = m_frame->GetGalCanvas()->GetGAL()->GetWorldScale();
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


std::set<BOARD_ITEM*> GRID_HELPER::queryVisible( const BOX2I& aArea )
{
    std::set<BOARD_ITEM*> items;

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR>::iterator it, it_end;

    m_frame->GetGalCanvas()->GetView()->Query( aArea, selectedItems );         // Get the list of selected items

    for( it = selectedItems.begin(), it_end = selectedItems.end(); it != it_end; ++it )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it->first );
        if( item->ViewIsVisible() )
            items.insert ( item );
    }

    return items;
}


VECTOR2I GRID_HELPER::BestSnapAnchor( const VECTOR2I &aOrigin, BOARD_ITEM* aDraggedItem )
{
    double worldScale = m_frame->GetGalCanvas()->GetGAL()->GetWorldScale();
    int snapRange = (int) ( 100.0 / worldScale );

    BOX2I bb( VECTOR2I( aOrigin.x - snapRange / 2, aOrigin.y - snapRange/2 ), VECTOR2I( snapRange, snapRange ) );

    clearAnchors();

    BOOST_FOREACH( BOARD_ITEM* item, queryVisible( bb ) )
    {
        computeAnchors( item, aOrigin );
    }

    LSET layers( aDraggedItem->GetLayer() );
    ANCHOR* nearest = nearestAnchor( aOrigin, CORNER | SNAPPABLE, layers );

    VECTOR2I nearestGrid = Align( aOrigin );
    double gridDist = ( nearestGrid - aOrigin ).EuclideanNorm();
    if( nearest )
    {
        double snapDist = nearest->Distance( aOrigin );

        if( nearest && snapDist < gridDist )
             return nearest->pos;
       }

       return nearestGrid;
}


void GRID_HELPER::computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos )
{
    VECTOR2I origin;

    switch( aItem->Type() )
    {
        case PCB_MODULE_T:
        {
            MODULE* mod = static_cast<MODULE*>( aItem );
            addAnchor( mod->GetPosition(), ORIGIN | SNAPPABLE, mod );

            for( D_PAD* pad = mod->Pads(); pad; pad = pad->Next() )
                addAnchor( pad->GetPosition(), CORNER | SNAPPABLE, pad );

            break;
        }


        case PCB_PAD_T:
        {
            D_PAD* pad = static_cast<D_PAD*>( aItem );
            addAnchor( pad->GetPosition(), CORNER | SNAPPABLE, pad );

            break;
        }

        case PCB_MODULE_EDGE_T:
        case PCB_LINE_T:
        {
            DRAWSEGMENT* dseg = static_cast<DRAWSEGMENT*>( aItem );
            VECTOR2I start = dseg->GetStart();
            VECTOR2I end = dseg->GetEnd();
            //LAYER_ID layer = dseg->GetLayer();

            switch( dseg->GetShape() )
            {
                case S_CIRCLE:
                {
                    int r = ( start - end ).EuclideanNorm();

                    addAnchor( start, ORIGIN | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I ( -r, 0 ), OUTLINE | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I ( r, 0 ), OUTLINE | SNAPPABLE, dseg );
                    addAnchor( start + VECTOR2I ( 0, -r ), OUTLINE | SNAPPABLE, dseg);
                    addAnchor( start + VECTOR2I ( 0, r ), OUTLINE | SNAPPABLE, dseg );
                    break;
                }

                case S_ARC:
                {
                    origin = dseg->GetCenter();
                    addAnchor( dseg->GetArcStart(), CORNER | SNAPPABLE, dseg );
                    addAnchor( dseg->GetArcEnd(), CORNER | SNAPPABLE, dseg );
                    addAnchor( origin, ORIGIN | SNAPPABLE, dseg );
                    break;
                }

                case S_SEGMENT:
                {
                    origin.x = start.x + ( start.x - end.x ) / 2;
                    origin.y = start.y + ( start.y - end.y ) / 2;
                    addAnchor( start, CORNER | SNAPPABLE, dseg );
                    addAnchor( end, CORNER | SNAPPABLE, dseg );
                    addAnchor( origin, ORIGIN, dseg );
                    break;
                }

                default:
                {
                    origin = dseg->GetStart();
                    addAnchor( origin, ORIGIN | SNAPPABLE, dseg );
                    break;
                }
            }
            break;
        }

        case PCB_TRACE_T:
        {
            TRACK* track = static_cast<TRACK*>( aItem );
            VECTOR2I start = track->GetStart();
            VECTOR2I end = track->GetEnd();
            origin.x = start.x + ( start.x - end.x ) / 2;
            origin.y = start.y + ( start.y - end.y ) / 2;
            addAnchor( start, CORNER | SNAPPABLE, track );
            addAnchor( end, CORNER | SNAPPABLE, track );
            addAnchor( origin, ORIGIN, track);
            break;
        }

        case PCB_ZONE_AREA_T:
        {
            const CPolyLine* outline = static_cast<const ZONE_CONTAINER*>( aItem )->Outline();
            int cornersCount = outline->GetCornersCount();

            SHAPE_LINE_CHAIN lc;
            lc.SetClosed( true );

            for( int i = 0; i < cornersCount; ++i )
            {
                const VECTOR2I p ( outline->GetPos( i ) );
                addAnchor( p, CORNER, aItem );
                lc.Append( p );
            }

            addAnchor( lc.NearestPoint( aRefPos ), OUTLINE, aItem );

            break;
        }

        case PCB_MODULE_TEXT_T:
        case PCB_TEXT_T:
            addAnchor( aItem->GetPosition(), ORIGIN, aItem );
        default:

        break;
   }
}


GRID_HELPER::ANCHOR* GRID_HELPER::nearestAnchor( VECTOR2I aPos, int aFlags, LSET aMatchLayers )
{
     double minDist = std::numeric_limits<double>::max();
     ANCHOR* best = NULL;

     BOOST_FOREACH( ANCHOR& a, m_anchors )
     {
         if( !aMatchLayers[a.item->GetLayer()] )
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
