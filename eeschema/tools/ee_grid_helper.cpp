/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/shape_line_chain.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <math/vector2d.h>
#include <sch_item.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <view/view_controls.h>

#include "ee_grid_helper.h"


EE_GRID_HELPER::EE_GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
    m_toolMgr( aToolMgr )
{
    m_enableSnap = true;
    m_enableSnapLine = true;
    m_snapSize = 100;
    m_snapItem = nullptr;
    KIGFX::VIEW* view = m_toolMgr->GetView();

    m_viewAxis.SetSize( 20000 );
    m_viewAxis.SetStyle( KIGFX::ORIGIN_VIEWITEM::CROSS );
    m_viewAxis.SetColor( COLOR4D( 0.0, 0.1, 0.4, 0.8 ) );
    m_viewAxis.SetDrawAtZero( true );
    view->Add( &m_viewAxis );
    view->SetVisible( &m_viewAxis, false );

    m_viewSnapPoint.SetStyle( KIGFX::ORIGIN_VIEWITEM::CIRCLE_CROSS );
    m_viewSnapPoint.SetColor( COLOR4D( 0.0, 0.1, 0.4, 1.0 ) );
    m_viewSnapPoint.SetDrawAtZero( true );
    view->Add( &m_viewSnapPoint );
    view->SetVisible( &m_viewSnapPoint, false );

    m_viewSnapLine.SetStyle( KIGFX::ORIGIN_VIEWITEM::DASH_LINE );
    m_viewSnapLine.SetColor( COLOR4D( 0.33, 0.55, 0.95, 1.0 ) );
    m_viewSnapLine.SetDrawAtZero( true );
    view->Add( &m_viewSnapLine );
    view->SetVisible( &m_viewSnapLine, false );
}


EE_GRID_HELPER::~EE_GRID_HELPER()
{
}


VECTOR2I EE_GRID_HELPER::GetGrid() const
{
    VECTOR2D size = m_toolMgr->GetView()->GetGAL()->GetGridSize();

    return VECTOR2I( KiROUND( size.x ), KiROUND( size.y ) );
}


VECTOR2I EE_GRID_HELPER::GetOrigin() const
{
    VECTOR2D origin = m_toolMgr->GetView()->GetGAL()->GetGridOrigin();

    return VECTOR2I( origin );
}


void EE_GRID_HELPER::SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin )
{
    if( aEnable )
    {
        m_auxAxis = aOrigin;
        m_viewAxis.SetPosition( wxPoint( aOrigin ) );
        m_toolMgr->GetView()->SetVisible( &m_viewAxis, true );
    }
    else
    {
        m_auxAxis = OPT<VECTOR2I>();
        m_toolMgr->GetView()->SetVisible( &m_viewAxis, false );
    }
}


VECTOR2I EE_GRID_HELPER::Align( const VECTOR2I& aPoint ) const
{
    if( !m_toolMgr->GetView()->GetGAL()->GetGridSnapping() )
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


VECTOR2I EE_GRID_HELPER::AlignToWire( const VECTOR2I& aPoint, const SEG& aSeg )
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
    pts[2] = aSeg.IntersectLines( SEG( nearest + VECTOR2I( -1, 0 ), nearest + VECTOR2I( 1, 0 ) ) );
    pts[3] = aSeg.IntersectLines( SEG( nearest + VECTOR2I( 0, -1 ), nearest + VECTOR2I( 0, 1 ) ) );

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

VECTOR2I EE_GRID_HELPER::BestDragOrigin( const VECTOR2I &aMousePos, std::vector<SCH_ITEM*>& aItems )
{
    clearAnchors();

    for( SCH_ITEM* item : aItems )
        computeAnchors( item, aMousePos, true );

    double worldScale = m_toolMgr->GetView()->GetGAL()->GetWorldScale();
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


std::set<SCH_ITEM*> EE_GRID_HELPER::queryVisible( const BOX2I& aArea,
                                                 const std::vector<SCH_ITEM*>& aSkip ) const
{
    std::set<SCH_ITEM*> items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    KIGFX::VIEW*                  view = m_toolMgr->GetView();

    view->Query( aArea, selectedItems );

    for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : selectedItems )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( it.first );

        // The item must be visible and on an active layer
        if( view->IsVisible( item )
                && item->ViewGetLOD( it.second, view ) < view->GetScale() )
        {
            items.insert ( item );
        }
    }


    for( SCH_ITEM* skipItem : aSkip )
        items.erase( skipItem );

    return items;
}


VECTOR2I EE_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, SCH_ITEM* aDraggedItem )
{
    return BestSnapAnchor( aOrigin, LSET::AllLayersMask(), { aDraggedItem } );
}


VECTOR2I EE_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
                                      const std::vector<SCH_ITEM*>& aSkip )
{
    double worldScale = m_toolMgr->GetView()->GetGAL()->GetWorldScale();
    double snapDist   = m_snapSize / worldScale;
    int snapRange     = KiROUND( snapDist );

    BOX2I bb( VECTOR2I( aOrigin.x - snapRange / 2, aOrigin.y - snapRange / 2 ),
              VECTOR2I( snapRange, snapRange ) );

    clearAnchors();

    for( SCH_ITEM* item : queryVisible( bb, aSkip ) )
        computeAnchors( item, aOrigin );

    ANCHOR*  nearest = nearestAnchor( aOrigin, SNAPPABLE, aLayers );
    VECTOR2I nearestGrid = Align( aOrigin );

    if( nearest )
        snapDist = nearest->Distance( aOrigin );

    if( m_snapItem && m_enableSnapLine && m_enableSnap )
    {
        bool snapLine = false;
        int x_dist = std::abs( m_viewSnapLine.GetPosition().x - aOrigin.x );
        int y_dist = std::abs( m_viewSnapLine.GetPosition().y - aOrigin.y );

        /// Allows de-snapping from the line if you are closer to another snap point
        if( x_dist < snapRange && x_dist < snapDist )
        {
            nearestGrid.x = m_viewSnapLine.GetPosition().x;
            snapLine      = true;
        }

        if( y_dist < snapRange && y_dist < snapDist )
        {
            nearestGrid.y = m_viewSnapLine.GetPosition().y;
            snapLine      = true;
        }

        if( snapLine && m_skipPoint != VECTOR2I( m_viewSnapLine.GetPosition() ) )
        {
            m_viewSnapLine.SetEndPosition( nearestGrid );
            m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );

            if( m_toolMgr->GetView()->IsVisible( &m_viewSnapLine ) )
                m_toolMgr->GetView()->Update( &m_viewSnapLine, KIGFX::GEOMETRY );
            else
                m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, true );

            return nearestGrid;
        }
    }

    if( nearest && m_enableSnap )
    {

        if( snapDist <= snapRange )
        {
            m_viewSnapPoint.SetPosition( wxPoint( nearest->pos ) );
            m_viewSnapLine.SetPosition( wxPoint( nearest->pos ) );
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


SCH_ITEM* EE_GRID_HELPER::GetSnapped() const
{
    if( !m_snapItem )
        return nullptr;

    return m_snapItem->item;
}


void EE_GRID_HELPER::computeAnchors( SCH_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom )
{
    switch( aItem->Type() )
    {
    case SCH_COMPONENT_T:
    case SCH_SHEET_T:
        addAnchor( aItem->GetPosition(), ORIGIN, aItem );
        KI_FALLTHROUGH;
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_LINE_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    case SCH_BUS_WIRE_ENTRY_T:
    {
        std::vector<wxPoint> pts = aItem->GetConnectionPoints();

        for( auto pt : pts )
            addAnchor( VECTOR2I( pt ), SNAPPABLE | CORNER, aItem );

        break;
    }

    default:
        break;
   }
}


EE_GRID_HELPER::ANCHOR* EE_GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags,
                                                 LSET aMatchLayers )
{
    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* best = NULL;

    for( ANCHOR& a : m_anchors )
    {
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
