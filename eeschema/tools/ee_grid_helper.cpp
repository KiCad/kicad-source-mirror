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
#include <sch_item.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include "ee_grid_helper.h"


EE_GRID_HELPER::EE_GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
    GRID_HELPER( aToolMgr )
{
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


VECTOR2I EE_GRID_HELPER::BestDragOrigin( const VECTOR2I &aMousePos, int aLayer,
                                         const EE_SELECTION& aItems )
{
    clearAnchors();

    for( EDA_ITEM* item : aItems )
        computeAnchors( static_cast<SCH_ITEM*>( item ), aMousePos, true );

    double worldScale = m_toolMgr->GetView()->GetGAL()->GetWorldScale();
    double lineSnapMinCornerDistance = 50.0 / worldScale;

    ANCHOR* nearestOutline = nearestAnchor( aMousePos, OUTLINE, aLayer );
    ANCHOR* nearestCorner = nearestAnchor( aMousePos, CORNER, aLayer );
    ANCHOR* nearestOrigin = nearestAnchor( aMousePos, ORIGIN, aLayer );
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


VECTOR2I EE_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, int aLayer, SCH_ITEM* aSkip )
{
    EE_SELECTION skipItems;
    skipItems.Add( aSkip );

    return BestSnapAnchor( aOrigin, aLayer, skipItems );
}


VECTOR2I EE_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, int aLayer,
                                         const EE_SELECTION& aSkip )
{
    constexpr int snapRange = SNAP_RANGE * IU_PER_MILS;

    VECTOR2I pt = aOrigin;
    VECTOR2I snapDist( snapRange, snapRange );
    bool     snapLineX = false;
    bool     snapLineY = false;
    bool     snapPoint = false;
    bool     gridChecked = false;

    BOX2I    bb( VECTOR2I( aOrigin.x - snapRange / 2, aOrigin.y - snapRange / 2 ),
                 VECTOR2I( snapRange, snapRange ) );

    clearAnchors();

    for( SCH_ITEM* item : queryVisible( bb, aSkip ) )
        computeAnchors( item, aOrigin );

    ANCHOR*  nearest = nearestAnchor( aOrigin, SNAPPABLE, aLayer );
    VECTOR2I nearestGrid = m_enableGrid ? Align( aOrigin ) : aOrigin;

    if( m_enableSnapLine && m_snapItem && m_skipPoint != VECTOR2I( m_viewSnapLine.GetPosition() ) )
    {
        if( std::abs( m_viewSnapLine.GetPosition().x - aOrigin.x ) < snapDist.x )
        {
            pt.x = m_viewSnapLine.GetPosition().x;
            snapDist.x = std::abs( m_viewSnapLine.GetPosition().x - aOrigin.x );
            snapLineX = true;
        }

        if( std::abs( m_viewSnapLine.GetPosition().y - aOrigin.y ) < snapDist.y )
        {
            pt.y = m_viewSnapLine.GetPosition().y;
            snapDist.y = std::abs( m_viewSnapLine.GetPosition().y - aOrigin.y );
            snapLineY = true;
        }

        if( m_enableGrid && std::abs( nearestGrid.x - aOrigin.x ) < snapDist.x )
        {
            pt.x = nearestGrid.x;
            snapDist.x = std::abs( nearestGrid.x - aOrigin.x );
            snapLineX = false;
        }

        if( m_enableGrid && std::abs( nearestGrid.y - aOrigin.y ) < snapDist.y )
        {
            pt.y = nearestGrid.y;
            snapDist.y = std::abs( nearestGrid.y - aOrigin.y );
            snapLineY = false;
        }

        gridChecked = true;
    }

    if( m_enableSnap && nearest && nearest->Distance( aOrigin ) < snapDist.EuclideanNorm() )
    {
        pt = nearest->pos;
        snapDist.x = std::abs( nearest->pos.x - aOrigin.x );
        snapDist.y = std::abs( nearest->pos.y - aOrigin.y );
        snapLineX = snapLineY = false;
        snapPoint = true;

        if( m_enableGrid && ( nearestGrid - aOrigin ).EuclideanNorm() < snapDist.EuclideanNorm() )
        {
            pt = nearestGrid;
            snapDist.x = std::abs( nearestGrid.x - aOrigin.x );
            snapDist.y = std::abs( nearestGrid.y - aOrigin.y );
            snapPoint = false;
        }

        gridChecked = true;
    }

    if( m_enableGrid && !gridChecked )
        pt = nearestGrid;

    if( snapLineX || snapLineY )
    {
        m_viewSnapLine.SetEndPosition( pt );

        if( m_toolMgr->GetView()->IsVisible( &m_viewSnapLine ) )
            m_toolMgr->GetView()->Update( &m_viewSnapLine, KIGFX::GEOMETRY );
        else
            m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, true );
    }
    else if( snapPoint )
    {
        m_snapItem = nearest;
        m_viewSnapPoint.SetPosition( (wxPoint) pt );
        m_viewSnapLine.SetPosition( (wxPoint) pt );

        m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, false );

        if( m_toolMgr->GetView()->IsVisible( &m_viewSnapPoint ) )
            m_toolMgr->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY);
        else
            m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, true );
    }
    else
    {
        m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
        m_toolMgr->GetView()->SetVisible( &m_viewSnapLine, false );
    }

    return pt;
}


SCH_ITEM* EE_GRID_HELPER::GetSnapped() const
{
    if( !m_snapItem )
        return nullptr;

    return static_cast<SCH_ITEM*>( m_snapItem->item );
}


std::set<SCH_ITEM*> EE_GRID_HELPER::queryVisible( const BOX2I& aArea,
                                                  const EE_SELECTION& aSkipList ) const
{
    std::set<SCH_ITEM*>                       items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    KIGFX::VIEW* view = m_toolMgr->GetView();

    view->Query( aArea, selectedItems );

    for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : selectedItems )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( it.first );

        // The item must be visible and on an active layer
        if( view->IsVisible( item ) && item->ViewGetLOD( it.second, view ) < view->GetScale() )
            items.insert ( item );
    }

    for( EDA_ITEM* skipItem : aSkipList )
        items.erase( static_cast<SCH_ITEM*>( skipItem ) );

    return items;
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

        for( const wxPoint& pt : pts )
            addAnchor( VECTOR2I( pt ), SNAPPABLE | CORNER, aItem );

        break;
    }

    default:
        break;
   }
}


EE_GRID_HELPER::ANCHOR* EE_GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags,
                                                       int aMatchLayer )
{
    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* best = NULL;

    for( ANCHOR& a : m_anchors )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( a.item );

        if( ( aFlags & a.flags ) != aFlags )
            continue;

        if( aMatchLayer == LAYER_CONNECTABLE && !item->IsConnectable() )
            continue;
        else if( aMatchLayer == LAYER_GRAPHICS && item->IsConnectable() )
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
