/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <macros.h>
#include <gal/graphics_abstraction_layer.h>
#include <sch_group.h>
#include <sch_item.h>
#include <sch_line.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <sch_tool_base.h>
#include <settings/app_settings.h>
#include <trigo.h>
#include <view/view.h>
#include "ee_grid_helper.h"

EE_GRID_HELPER::EE_GRID_HELPER() :
        GRID_HELPER()
{
}


EE_GRID_HELPER::EE_GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
        GRID_HELPER( aToolMgr, LAYER_SCHEMATIC_ANCHOR )
{
    if( !m_toolMgr )
        return;

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
}


EE_GRID_HELPER::~EE_GRID_HELPER()
{
    if( !m_toolMgr )
        return;

    KIGFX::VIEW* view = m_toolMgr->GetView();

    view->Remove( &m_viewAxis );
    view->Remove( &m_viewSnapPoint );
}


VECTOR2I EE_GRID_HELPER::BestDragOrigin( const VECTOR2I& aMousePos, GRID_HELPER_GRIDS aGrid,
                                         const SCH_SELECTION& aItems )
{
    clearAnchors();

    // If we're working with any connectable objects, skip non-connectable objects
    // since they are often off-grid, e.g. text anchors
    bool hasConnectables = false;

    for( EDA_ITEM* item : aItems )
    {
        GRID_HELPER_GRIDS grid = GetItemGrid( static_cast<SCH_ITEM*>( item ) );
        if( grid == GRID_CONNECTABLE || grid == GRID_WIRES )
        {
            hasConnectables = true;
            break;
        }
    }

    for( EDA_ITEM* item : aItems )
        computeAnchors( static_cast<SCH_ITEM*>( item ), aMousePos, true, !hasConnectables );

    double worldScale = m_toolMgr->GetView()->GetGAL()->GetWorldScale();
    double lineSnapMinCornerDistance = 50.0 / worldScale;

    ANCHOR* nearestOutline = nearestAnchor( aMousePos, OUTLINE, aGrid );
    ANCHOR* nearestCorner = nearestAnchor( aMousePos, CORNER, aGrid );
    ANCHOR* nearestOrigin = nearestAnchor( aMousePos, ORIGIN, aGrid );
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


VECTOR2I EE_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, GRID_HELPER_GRIDS aGrid,
                                         SCH_ITEM* aSkip )
{
    SCH_SELECTION skipItems;
    skipItems.Add( aSkip );

    return BestSnapAnchor( aOrigin, aGrid, skipItems );
}


VECTOR2I EE_GRID_HELPER::BestSnapAnchor( const VECTOR2I& aOrigin, GRID_HELPER_GRIDS aGrid,
                                         const SCH_SELECTION& aSkip )
{
    constexpr int snapRange = SNAP_RANGE * schIUScale.IU_PER_MILS;

    VECTOR2I pt = aOrigin;
    VECTOR2I snapDist( snapRange, snapRange );
    bool     gridChecked = false;
    bool     snappedToAnchor = false;

    BOX2I    bb( VECTOR2I( aOrigin.x - snapRange / 2, aOrigin.y - snapRange / 2 ),
                 VECTOR2I( snapRange, snapRange ) );

    clearAnchors();
    m_snapItem = std::nullopt;

    for( SCH_ITEM* item : queryVisible( bb, aSkip ) )
        computeAnchors( item, aOrigin );

    ANCHOR*  nearest = nearestAnchor( aOrigin, SNAPPABLE, aGrid );
    VECTOR2I nearestGrid = Align( aOrigin, aGrid );

    if( KIGFX::ANCHOR_DEBUG* ad = enableAndGetAnchorDebug(); ad )
    {
        ad->ClearAnchors();
        for( const ANCHOR& a : m_anchors )
            ad->AddAnchor( a.pos );

        ad->SetNearest( nearest ? OPT_VECTOR2I( nearest->pos ) : std::nullopt );
        m_toolMgr->GetView()->Update( ad, KIGFX::GEOMETRY );
    }

    showConstructionGeometry( m_enableSnap );

    SNAP_LINE_MANAGER& snapLineManager = getSnapManager().GetSnapLineManager();
    const VECTOR2D     gridSize = GetGridSize( aGrid );

    std::optional<VECTOR2I> guideSnap;

    if( m_enableSnapLine )
        guideSnap = SnapToConstructionLines( aOrigin, nearestGrid, gridSize, snapRange );

    if( m_enableSnap && nearest && nearest->Distance( aOrigin ) < snapDist.EuclideanNorm() )
    {

        if( canUseGrid() && ( nearestGrid - aOrigin ).EuclideanNorm() < snapDist.EuclideanNorm() )
        {
            pt = nearestGrid;
            snapDist.x = std::abs( nearestGrid.x - aOrigin.x );
            snapDist.y = std::abs( nearestGrid.y - aOrigin.y );
            gridChecked = true;
        }
        else
        {
            pt = nearest->pos;
            snapDist.x = std::abs( nearest->pos.x - aOrigin.x );
            snapDist.y = std::abs( nearest->pos.y - aOrigin.y );
            snappedToAnchor = true;
            gridChecked = true;
        }
    }

    if( guideSnap && m_skipPoint != *guideSnap )
    {
        snapLineManager.SetSnapLineEnd( *guideSnap );
        m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );
        m_snapItem = std::nullopt;
        return *guideSnap;
    }

    if( snappedToAnchor )
    {
        m_snapItem = *nearest;
        m_viewSnapPoint.SetPosition( pt );

        snapLineManager.SetSnapLineOrigin( pt );

        if( m_toolMgr->GetView()->IsVisible( &m_viewSnapPoint ) )
            m_toolMgr->GetView()->Update( &m_viewSnapPoint, KIGFX::GEOMETRY);
        else
            m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, true );
    }

    if( !snappedToAnchor )
        m_snapItem = std::nullopt;

    if( canUseGrid() && !gridChecked )
        pt = nearestGrid;

    snapLineManager.SetSnapLineEnd( std::nullopt );
    m_toolMgr->GetView()->SetVisible( &m_viewSnapPoint, false );

    return pt;
}


VECTOR2D EE_GRID_HELPER::GetGridSize( GRID_HELPER_GRIDS aGrid ) const
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
        g = grid.grids[idx].ToDouble( schIUScale );

    return g;
}


SCH_ITEM* EE_GRID_HELPER::GetSnapped() const
{
    if( !m_snapItem )
        return nullptr;

    if( m_snapItem->items.empty() )
        return nullptr;

    return static_cast<SCH_ITEM*>( m_snapItem->items[0] );
}


std::set<SCH_ITEM*> EE_GRID_HELPER::queryVisible( const BOX2I& aArea,
                                                  const SCH_SELECTION& aSkipList ) const
{
    std::set<SCH_ITEM*>                       items;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_toolMgr->GetToolHolder() );
    KIGFX::VIEW*    view = m_toolMgr->GetView();

    view->Query( aArea, selectedItems );

    for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : selectedItems )
    {
        if( !it.first->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( it.first );

        if( frame && frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        {
            // If we are in the symbol editor, don't use the symbol itself
            if( item->Type() == LIB_SYMBOL_T )
                continue;
        }
        else
        {
            // If we are not in the symbol editor, don't use symbol-editor-private items
            if( item->IsPrivate() )
                continue;
        }

        // The item must be visible and on an active layer
        if( view->IsVisible( item ) && item->ViewGetLOD( it.second, view ) < view->GetScale() )
            items.insert ( item );
    }

    for( EDA_ITEM* skipItem : aSkipList )
        items.erase( static_cast<SCH_ITEM*>( skipItem ) );

    return items;
}


GRID_HELPER_GRIDS EE_GRID_HELPER::GetSelectionGrid( const SELECTION& aSelection ) const
{
    GRID_HELPER_GRIDS grid = GetItemGrid( aSelection.Front() );

    // Find the largest grid of all the items and use that
    for( EDA_ITEM* item : aSelection )
    {
        GRID_HELPER_GRIDS itemGrid = GetItemGrid( item );

        if( GetGridSize( itemGrid ) > GetGridSize( grid ) )
            grid = itemGrid;
    }

    return grid;
}


GRID_HELPER_GRIDS EE_GRID_HELPER::GetItemGrid( const EDA_ITEM* aItem ) const
{
    if( !aItem )
        return GRID_CURRENT;

    switch( aItem->Type() )
    {
    case LIB_SYMBOL_T:
    case SCH_SYMBOL_T:
    case SCH_PIN_T:
    case SCH_SHEET_PIN_T:
    case SCH_SHEET_T:
    case SCH_NO_CONNECT_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_RULE_AREA_T:
        return GRID_CONNECTABLE;

    case SCH_FIELD_T:
    case SCH_TEXT_T:
        return GRID_TEXT;

    case SCH_SHAPE_T:
    // The text box's border lines are what need to be on the graphic grid
    case SCH_TEXTBOX_T:
    case SCH_BITMAP_T:
        return GRID_GRAPHICS;

    case SCH_JUNCTION_T:
        return GRID_WIRES;

    case SCH_LINE_T:
        if( static_cast<const SCH_LINE*>( aItem )->IsConnectable() )
            return GRID_WIRES;
        else
            return GRID_GRAPHICS;

    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
        return GRID_WIRES;

    // Groups need to get the grid of their children
    case SCH_GROUP_T:
    {
        const SCH_GROUP* group = static_cast<const SCH_GROUP*>( aItem );

        // Shouldn't happen
        if( group->GetItems().empty() )
            return GRID_CURRENT;

        GRID_HELPER_GRIDS grid = GetItemGrid( *group->GetItems().begin() );

        for( EDA_ITEM* item : static_cast<const SCH_GROUP*>( aItem )->GetItems() )
        {
            GRID_HELPER_GRIDS itemGrid = GetItemGrid( item );

            if( GetGridSize( itemGrid ) > GetGridSize( grid ) )
                grid = itemGrid;
        }

        return grid;
    }

    default:
        return GRID_CURRENT;
    }
}


void EE_GRID_HELPER::computeAnchors( SCH_ITEM *aItem, const VECTOR2I &aRefPos, bool aFrom,
                                     bool aIncludeText )
{
    bool isGraphicLine =
            aItem->Type() == SCH_LINE_T && static_cast<SCH_LINE*>( aItem )->IsGraphicLine();

    switch( aItem->Type() )
    {
    case SCH_TEXT_T:
    case SCH_FIELD_T:
    {
        if( aIncludeText )
            addAnchor( aItem->GetPosition(), ORIGIN, aItem );

        break;
    }

    case SCH_TABLE_T:
    {
        if( aIncludeText )
        {
            addAnchor( aItem->GetPosition(), SNAPPABLE | CORNER, aItem );
            addAnchor( static_cast<SCH_TABLE*>( aItem )->GetEnd(), SNAPPABLE | CORNER, aItem );
        }

        break;
    }

    case SCH_TEXTBOX_T:
    case SCH_TABLECELL_T:
    {
        if( aIncludeText )
        {
            addAnchor( aItem->GetPosition(), SNAPPABLE | CORNER, aItem );
            addAnchor( static_cast<SCH_SHAPE*>( aItem )->GetEnd(), SNAPPABLE | CORNER, aItem );
        }

        break;
    }

    case SCH_SYMBOL_T:
    case SCH_SHEET_T:
        addAnchor( aItem->GetPosition(), ORIGIN, aItem );
        KI_FALLTHROUGH;

    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_LINE_T:
        // Don't add anchors for graphic lines unless we're including text,
        // they may be on a non-connectable grid
        if( isGraphicLine && !aIncludeText )
            break;

        KI_FALLTHROUGH;
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_SHEET_PIN_T:
    {
        std::vector<VECTOR2I> pts = aItem->GetConnectionPoints();

        for( const VECTOR2I& pt : pts )
            addAnchor( VECTOR2I( pt ), SNAPPABLE | CORNER, aItem );

        break;
    }
    case SCH_PIN_T:
    {
        SCH_PIN* pin = static_cast<SCH_PIN*>( aItem );
        addAnchor( pin->GetPosition(), SNAPPABLE | ORIGIN, aItem );
        break;
    }

    case SCH_GROUP_T:
        for( EDA_ITEM* item : static_cast<SCH_GROUP*>( aItem )->GetItems() )
        {
            computeAnchors( static_cast<SCH_ITEM*>( item ), aRefPos, aFrom, aIncludeText );
        }

        break;

    default:
        break;
    }

    // Don't add anchors for graphic lines unless we're including text,
    // they may be on a non-connectable grid
    if( aItem->Type() == SCH_LINE_T && ( aIncludeText || !isGraphicLine ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aItem );
        VECTOR2I  pt = Align( aRefPos );

        if( line->GetStartPoint().x == line->GetEndPoint().x )
        {
            VECTOR2I possible( line->GetStartPoint().x, pt.y );

            if( TestSegmentHit( possible, line->GetStartPoint(), line->GetEndPoint(), 0 ) )
                addAnchor( possible, SNAPPABLE | VERTICAL, aItem );
        }
        else if( line->GetStartPoint().y == line->GetEndPoint().y )
        {
            VECTOR2I possible( pt.x, line->GetStartPoint().y );

            if( TestSegmentHit( possible, line->GetStartPoint(), line->GetEndPoint(), 0 ) )
                addAnchor( possible, SNAPPABLE | HORIZONTAL, aItem );
        }
    }
}


EE_GRID_HELPER::ANCHOR* EE_GRID_HELPER::nearestAnchor( const VECTOR2I& aPos, int aFlags,
                                                       GRID_HELPER_GRIDS aGrid )
{
    double  minDist = std::numeric_limits<double>::max();
    ANCHOR* best = nullptr;

    for( ANCHOR& a : m_anchors )
    {
        if( ( aFlags & a.flags ) != aFlags )
            continue;

        // A "virtual" anchor with no real items associated shouldn't be filtered out
        if( !a.items.empty() )
        {
            // Filter using the first item
            SCH_ITEM* item = static_cast<SCH_ITEM*>( a.items[0] );

            if( aGrid == GRID_CONNECTABLE && !item->IsConnectable() )
                continue;
            else if( aGrid == GRID_GRAPHICS && item->IsConnectable() )
                continue;
        }

        double dist = a.Distance( aPos );

        if( dist < minDist )
        {
            minDist = dist;
            best = &a;
        }
    }

    return best;
}
