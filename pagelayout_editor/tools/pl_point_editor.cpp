/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/log.h>
#include <fmt/format.h>

#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <preview_items/angle_item.h>
#include <confirm.h>
#include <bitmaps.h>
#include <status_popup.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include "pl_editor_frame.h"
#include "pl_point_editor.h"
#include "properties_frame.h"
#include "tools/pl_selection_tool.h"

// Few constants to avoid using bare numbers for point indices
enum RECTANGLE_POINTS
{
    RECT_TOPLEFT, RECT_TOPRIGHT, RECT_BOTLEFT, RECT_BOTRIGHT
};


enum LINE_POINTS
{
    LINE_START, LINE_END
};


class EDIT_POINTS_FACTORY
{
public:
    static std::shared_ptr<EDIT_POINTS> Make( EDA_ITEM* aItem )
    {
        std::shared_ptr<EDIT_POINTS> points = std::make_shared<EDIT_POINTS>( aItem );

        if( !aItem )
            return points;

        // Generate list of edit points based on the item type
        switch( aItem->Type() )
        {
        case WSG_LINE_T:
        {
            DS_DRAW_ITEM_LINE* line = static_cast<DS_DRAW_ITEM_LINE*>( aItem );
            points->AddPoint( line->GetStart() );
            points->AddPoint( line->GetEnd() );
            break;
        }

        case WSG_RECT_T:
        {
            DS_DRAW_ITEM_RECT* rect = static_cast<DS_DRAW_ITEM_RECT*>( aItem );
            VECTOR2I           topLeft = rect->GetStart();
            VECTOR2I           botRight = rect->GetEnd();

            if( topLeft.y > botRight.y )
                std::swap( topLeft.y, botRight.y );

            if( topLeft.x > botRight.x )
                std::swap( topLeft.x, botRight.x );

            points->AddPoint( topLeft );
            points->AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
            points->AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
            points->AddPoint( botRight );
            break;
        }

        default:
            points.reset();
            break;
        }

        return points;
    }

private:
    EDIT_POINTS_FACTORY() {};
};


PL_POINT_EDITOR::PL_POINT_EDITOR() :
    TOOL_INTERACTIVE( "plEditor.PointEditor" ),
    m_frame( nullptr ),
    m_selectionTool( nullptr ),
    m_editedPoint( nullptr )
{
}


void PL_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
    {
        // Init variables used by every drawing tool
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
    }

    if( KIGFX::VIEW* view = getView() )
    {
        if( m_angleItem )
            view->Remove( m_angleItem.get() );

        if( m_editPoints )
            view->Remove( m_editPoints.get() );
    }

    m_angleItem.reset();
    m_editPoints.reset();
}


bool PL_POINT_EDITOR::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();
    return true;
}


void PL_POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    EDIT_POINT* point = m_editedPoint;

    if( aEvent.IsMotion() )
        point = m_editPoints->FindPoint( aEvent.Position(), getView() );
    else if( aEvent.IsDrag( BUT_LEFT ) )
        point = m_editPoints->FindPoint( aEvent.DragOrigin(), getView() );
    else
        point = m_editPoints->FindPoint( getViewControls()->GetCursorPosition(), getView() );

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int PL_POINT_EDITOR::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    const PL_SELECTION&   selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || !selection.Front()->IsType( { WSG_LINE_T, WSG_RECT_T } ) )
        return 0;

    EDA_ITEM* item = (EDA_ITEM*) selection.Front();

    // Wait till drawing tool is done
    if( item->IsNew() )
        return 0;

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );

    m_editPoints = EDIT_POINTS_FACTORY::Make( item );

    if( !m_editPoints )
        return 0;

    m_angleItem = std::make_unique<KIGFX::PREVIEW::ANGLE_ITEM>( m_editPoints );

    getView()->Add( m_editPoints.get() );
    getView()->Add( m_angleItem.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;
    bool modified = false;

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !m_editPoints || evt->IsSelectionEvent() )
            break;

        if ( !inDrag )
            updateEditedPoint( *evt );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            if( !inDrag )
            {
                try
                {
                    m_frame->SaveCopyInUndoList();
                }
                catch( const fmt::format_error& exc )
                {
                    wxLogWarning( wxS( "Exception \"%s\" serializing string ocurred." ),
                                  exc.what() );
                    return 1;
                }

                controls->ForceCursorPosition( false );
                inDrag = true;
                modified = true;
            }

            m_editedPoint->SetPosition( controls->GetCursorPosition( !evt->DisableGridSnapping() ) );

            updateItem();
            updatePoints();
        }

        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            controls->SetAutoPan( false );
            inDrag = false;
        }

        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
                m_frame->RollbackFromUndo();
                inDrag = false;
                modified = false;
            }
            else if( evt->IsCancelInteractive() )
            {
                break;
            }

            if( evt->IsActivate() && !evt->IsMoveTool() )
                break;
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( inDrag );
        controls->CaptureCursor( inDrag );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );

    if( m_editPoints )
    {
        getView()->Remove( m_editPoints.get() );
        getView()->Remove( m_angleItem.get() );

        if( modified )
            m_frame->OnModify();

        m_editPoints.reset();
        m_angleItem.reset();
        m_frame->GetCanvas()->Refresh();
    }

    return 0;
}


void pinEditedCorner( int editedPointIndex, int minWidth, int minHeight, VECTOR2I& topLeft,
                      VECTOR2I& topRight, VECTOR2I& botLeft, VECTOR2I& botRight )
{
    switch( editedPointIndex )
    {
    case RECT_TOPLEFT:
        // pin edited point within opposite corner
        topLeft.x = std::min( topLeft.x, botRight.x - minWidth );
        topLeft.y = std::min( topLeft.y, botRight.y - minHeight );

        // push edited point edges to adjacent corners
        topRight.y = topLeft.y;
        botLeft.x = topLeft.x;

        break;

    case RECT_TOPRIGHT:
        // pin edited point within opposite corner
        topRight.x = std::max( topRight.x, botLeft.x + minWidth );
        topRight.y = std::min( topRight.y, botLeft.y - minHeight );

        // push edited point edges to adjacent corners
        topLeft.y = topRight.y;
        botRight.x = topRight.x;

        break;

    case RECT_BOTLEFT:
        // pin edited point within opposite corner
        botLeft.x = std::min( botLeft.x, topRight.x - minWidth );
        botLeft.y = std::max( botLeft.y, topRight.y + minHeight );

        // push edited point edges to adjacent corners
        botRight.y = botLeft.y;
        topLeft.x = botLeft.x;

        break;

    case RECT_BOTRIGHT:
        // pin edited point within opposite corner
        botRight.x = std::max( botRight.x, topLeft.x + minWidth );
        botRight.y = std::max( botRight.y, topLeft.y + minHeight );

        // push edited point edges to adjacent corners
        botLeft.y = botRight.y;
        topRight.x = botRight.x;

        break;
    }
}


void PL_POINT_EDITOR::updateItem() const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    DS_DATA_ITEM* dataItem = static_cast<DS_DRAW_ITEM_BASE*>( item )->GetPeer();

    // The current item is perhaps not the main item if we have a set of repeated items.
    // So we change the coordinate references in dataItem using move vectors of the start and
    // end points that are the same for each repeated item.

    switch( item->Type() )
    {
    case WSG_LINE_T:
    {
        DS_DRAW_ITEM_LINE* line = static_cast<DS_DRAW_ITEM_LINE*>( item );

        VECTOR2I move_startpoint = m_editPoints->Point( LINE_START ).GetPosition() - line->GetStart();
        VECTOR2I move_endpoint = m_editPoints->Point( LINE_END ).GetPosition() - line->GetEnd();

        dataItem->MoveStartPointToIU( dataItem->GetStartPosIU() + move_startpoint );
        dataItem->MoveEndPointToIU( dataItem->GetEndPosIU() + move_endpoint );

        for( DS_DRAW_ITEM_BASE* draw_item : dataItem->GetDrawItems() )
        {
            DS_DRAW_ITEM_LINE* draw_line = static_cast<DS_DRAW_ITEM_LINE*>( draw_item );

            draw_line->SetStart( draw_line->GetStart() + move_startpoint );
            draw_line->SetEnd( draw_line->GetEnd() + move_endpoint );
            getView()->Update( draw_item );
        }

        break;
    }

    case WSG_RECT_T:
    {
        DS_DRAW_ITEM_RECT* rect = static_cast<DS_DRAW_ITEM_RECT*>( item );
        VECTOR2I           topLeft = m_editPoints->Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I           topRight = m_editPoints->Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I           botLeft = m_editPoints->Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I           botRight = m_editPoints->Point( RECT_BOTRIGHT ).GetPosition();

        pinEditedCorner( getEditedPointIndex(), drawSheetIUScale.MilsToIU( 1 ),
                         drawSheetIUScale.MilsToIU( 1 ),
                         topLeft, topRight, botLeft, botRight );

        VECTOR2I start_delta, end_delta;

        if( rect->GetStart().y > rect->GetEnd().y )
        {
            start_delta.y = botRight.y - rect->GetStart().y;
            end_delta.y = topLeft.y - rect->GetEnd().y;
        }
        else
        {
            start_delta.y = topLeft.y - rect->GetStart().y;
            end_delta.y = botRight.y - rect->GetEnd().y;
        }

        if( rect->GetStart().x > rect->GetEnd().x )
        {
            start_delta.x = botRight.x - rect->GetStart().x;
            end_delta.x = topLeft.x - rect->GetEnd().x;
        }
        else
        {
            start_delta.x = topLeft.x - rect->GetStart().x;
            end_delta.x = botRight.x - rect->GetEnd().x;
        }

        dataItem->MoveStartPointToIU( dataItem->GetStartPosIU() + start_delta );
        dataItem->MoveEndPointToIU( dataItem->GetEndPosIU() + end_delta );

        for( DS_DRAW_ITEM_BASE* draw_item : dataItem->GetDrawItems() )
        {
            DS_DRAW_ITEM_RECT* draw_rect = static_cast<DS_DRAW_ITEM_RECT*>( draw_item );

            draw_rect->SetStart( draw_rect->GetStart() + start_delta );
            draw_rect->SetEnd( draw_rect->GetEnd() + end_delta );
            getView()->Update( draw_item );
        }

        break;
    }

    default:
        break;
    }

    m_frame->SetMsgPanel( item );

    // The Properties frame will be updated. Avoid flicker during update:
    m_frame->GetPropertiesFrame()->Freeze();
    m_frame->GetPropertiesFrame()->CopyPrmsFromItemToPanel( dataItem );
    m_frame->GetPropertiesFrame()->Thaw();
}


void PL_POINT_EDITOR::updatePoints()
{
    if( !m_editPoints )
        return;

    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    switch( item->Type() )
    {
    case WSG_LINE_T:
    {
        DS_DRAW_ITEM_LINE* line = static_cast<DS_DRAW_ITEM_LINE*>( item );

        m_editPoints->Point( LINE_START ).SetPosition( line->GetStart() );
        m_editPoints->Point( LINE_END ).SetPosition( line->GetEnd() );
        break;
    }

    case WSG_RECT_T:
    {
        DS_DRAW_ITEM_RECT* rect = static_cast<DS_DRAW_ITEM_RECT*>( item );
        VECTOR2I           topLeft = rect->GetPosition();
        VECTOR2I           botRight = rect->GetEnd();

        if( topLeft.y > botRight.y )
            std::swap( topLeft.y, botRight.y );

        if( topLeft.x > botRight.x )
            std::swap( topLeft.x, botRight.x );

        m_editPoints->Point( RECT_TOPLEFT ).SetPosition( (VECTOR2I) topLeft );
        m_editPoints->Point( RECT_TOPRIGHT ).SetPosition( VECTOR2I( botRight.x, topLeft.y ) );
        m_editPoints->Point( RECT_BOTLEFT ).SetPosition( VECTOR2I( topLeft.x, botRight.y ) );
        m_editPoints->Point( RECT_BOTRIGHT ).SetPosition( (VECTOR2I) botRight );
        break;
    }

    default:
        break;
    }

    getView()->Update( m_editPoints.get() );
    getView()->Update( m_angleItem.get() );
}


void PL_POINT_EDITOR::setEditedPoint( EDIT_POINT* aPoint )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    if( aPoint )
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
        controls->ForceCursorPosition( true, aPoint->GetPosition() );
        controls->ShowCursor( true );
    }
    else
    {
        if( m_frame->ToolStackIsEmpty() )
            controls->ShowCursor( false );

        controls->ForceCursorPosition( false );
    }

    m_editedPoint = aPoint;
}


int PL_POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


void PL_POINT_EDITOR::setTransitions()
{
    Go( &PL_POINT_EDITOR::Main,                EVENTS::SelectedEvent );
    Go( &PL_POINT_EDITOR::Main,                ACTIONS::activatePointEditor.MakeEvent() );
    Go( &PL_POINT_EDITOR::modifiedSelection,   EVENTS::SelectedItemsModified );
}


