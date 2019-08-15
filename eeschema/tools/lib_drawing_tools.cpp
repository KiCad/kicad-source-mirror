/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <ee_actions.h>
#include <lib_edit_frame.h>
#include <sch_view.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/lib_drawing_tools.h>
#include <tools/lib_pin_tool.h>
#include <class_libentry.h>
#include <bitmaps.h>
#include <lib_text.h>
#include <dialogs/dialog_lib_edit_text.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include "ee_point_editor.h"

static void* g_lastPinWeakPtr;


LIB_DRAWING_TOOLS::LIB_DRAWING_TOOLS() :
    EE_TOOL_BASE<LIB_EDIT_FRAME>( "eeschema.SymbolDrawing" )
{
}


bool LIB_DRAWING_TOOLS::Init()
{
    EE_TOOL_BASE::Init();

    auto isDrawingCondition = [] ( const SELECTION& aSel ) {
        LIB_ITEM* item = (LIB_ITEM*) aSel.Front();
        return item && item->IsNew();
    };

    m_menu.GetMenu().AddItem( EE_ACTIONS::finishDrawing, isDrawingCondition, 2 );

    return true;
}


int LIB_DRAWING_TOOLS::TwoClickPlace( const TOOL_EVENT& aEvent )
{
    KICAD_T       type = aEvent.Parameter<KICAD_T>();
    LIB_PIN_TOOL* pinTool = type == LIB_PIN_T ? m_toolMgr->GetTool<LIB_PIN_TOOL>() : nullptr;
    VECTOR2I      cursorPos;
    EDA_ITEM*     item = nullptr;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        auto cleanup = [&] () {
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
            m_view->ClearPreview();
            delete item;
            item = nullptr;
        };

        if( evt->IsCancelInteractive() )
        {
            if( item )
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( item )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            LIB_PART* part = m_frame->GetCurPart();

            if( !part )
                continue;

            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                switch( type )
                {
                case LIB_PIN_T:
                {
                    item = pinTool->CreatePin( wxPoint( cursorPos.x, -cursorPos.y ), part );
                    g_lastPinWeakPtr = item;
                    break;
                }
                case LIB_TEXT_T:
                {
                    LIB_TEXT* text = new LIB_TEXT( part );
                    text->SetPosition( wxPoint( cursorPos.x, -cursorPos.y ) );
                    text->SetTextSize( wxSize( m_frame->g_LastTextSize, m_frame->g_LastTextSize ) );
                    text->SetTextAngle( m_frame->g_LastTextAngle );

                    DIALOG_LIB_EDIT_TEXT dlg( m_frame, text );

                    if( dlg.ShowModal() != wxID_OK )
                        delete text;
                    else
                        item = text;

                    break;
                }
                default:
                    wxFAIL_MSG( "TwoClickPlace(): unknown type" );
                }

                // Restore cursor after dialog
                getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );

                if( item )
                {
                    item->SetFlags( IS_NEW | IS_MOVED );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                    m_selectionTool->AddItemToSel( item );
                }

                getViewControls()->SetCursorPosition( cursorPos, false );
            }

            // ... and second click places:
            else
            {
                m_frame->SaveCopyInUndoList( part );

                switch( item->Type() )
                {
                case LIB_PIN_T:
                    pinTool->PlacePin( (LIB_PIN*) item );
                    break;
                case LIB_TEXT_T:
                    part->AddDrawItem( (LIB_TEXT*) item );
                    break;
                default:
                    wxFAIL_MSG( "TwoClickPlace(): unknown type" );
                }

                item->ClearEditFlags();
                item = nullptr;
                m_view->ClearPreview();

                m_frame->RebuildView();
                m_frame->OnModify();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }

        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            static_cast<LIB_ITEM*>( item )->SetPosition( wxPoint( cursorPos.x, -cursorPos.y ) );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }

        else
            evt->SetPassEvent();

        // Enable autopanning and cursor capture only when there is an item to be placed
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    return 0;
}


int LIB_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    KICAD_T          type = aEvent.Parameter<KICAD_T>();
    EE_POINT_EDITOR* pointEditor = m_toolMgr->GetTool<EE_POINT_EDITOR>();

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    LIB_PART* part = m_frame->GetCurPart();
    LIB_ITEM* item = nullptr;

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !pointEditor->HasPoint() )
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        auto cleanup = [&] () {
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
            m_view->ClearPreview();
            delete item;
            item = nullptr;
        };

        if( evt->IsCancelInteractive() )
        {
            if( item )
                cleanup();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }

        else if( evt->IsActivate() )
        {
            if( item )
                cleanup();

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) && !item )
        {
            if( !part )
                continue;

            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            switch( type )
            {
            case LIB_ARC_T:       item = new LIB_ARC( part );       break;
            case LIB_CIRCLE_T:    item = new LIB_CIRCLE( part );    break;
            case LIB_POLYLINE_T:  item = new LIB_POLYLINE( part );  break;
            case LIB_RECTANGLE_T: item = new LIB_RECTANGLE( part ); break;
            default: break;     // keep compiler quiet
            }

            wxASSERT( item );

            item->SetWidth( LIB_EDIT_FRAME::g_LastLineWidth );
            item->SetFillMode( LIB_EDIT_FRAME::g_LastFillStyle );
            item->SetFlags( IS_NEW );
            item->BeginEdit( wxPoint( cursorPos.x, -cursorPos.y ) );

            if( m_frame->m_DrawSpecificUnit )
                item->SetUnit( m_frame->GetUnit() );

            if( m_frame->m_DrawSpecificConvert )
                item->SetConvert( m_frame->GetConvert() );

            m_selectionTool->AddItemToSel( item );
        }

        else if( item && ( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                        || evt->IsAction( &EE_ACTIONS::finishDrawing ) ) )
        {
            if( evt->IsDblClick( BUT_LEFT ) || evt->IsAction( &EE_ACTIONS::finishDrawing )
                    || !item->ContinueEdit( wxPoint( cursorPos.x, -cursorPos.y ) ) )
            {
                item->EndEdit();
                item->ClearEditFlags();
                m_view->ClearPreview();

                m_frame->SaveCopyInUndoList( part );
                part->AddDrawItem( item );
                item = nullptr;

                m_frame->RebuildView();
                m_frame->OnModify();
                m_toolMgr->RunAction( ACTIONS::activatePointEditor );
            }
        }

        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->CalcEdit( wxPoint( cursorPos.x, -cursorPos.y) );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }

        else if( evt->IsDblClick( BUT_LEFT ) && !item )
        {
            m_toolMgr->RunAction( EE_ACTIONS::properties, true );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }

        else
            evt->SetPassEvent();

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    return 0;
}


int LIB_DRAWING_TOOLS::PlaceAnchor( const TOOL_EVENT& aEvent )
{
    getViewControls()->ShowCursor( true );
    getViewControls()->SetSnapping( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_BULLSEYE );

        if( evt->IsCancelInteractive() )
        {
            m_frame->PopTool( tool );
            break;
        }
        else if( evt->IsActivate() )
        {
            m_frame->PopTool( tool );
            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            LIB_PART* part = m_frame->GetCurPart();

            if( !part )
                continue;

            VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );
            wxPoint  offset( -cursorPos.x, cursorPos.y );

            part->SetOffset( offset );

            // Refresh the view without changing the viewport
            auto center = m_view->GetCenter();
            center.x += offset.x;
            center.y -= offset.y;
            m_view->SetCenter( center );
            m_view->RecacheAllItems();
            m_frame->OnModify();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else
            evt->SetPassEvent();
    }

    return 0;
}


int LIB_DRAWING_TOOLS::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    LIB_PIN_TOOL* pinTool = m_toolMgr->GetTool<LIB_PIN_TOOL>();
    LIB_PART*     part = m_frame->GetCurPart();
    LIB_PIN*      sourcePin = nullptr;

    if( !part )
        return 0;

    // See if we have a pin matching our weak ptr
    for( LIB_PIN* test = part->GetNextPin();  test;  test = part->GetNextPin( test ) )
    {
        if( (void*) test == g_lastPinWeakPtr )
            sourcePin = test;
    }

    if( sourcePin )
    {
        LIB_PIN* pin = pinTool->RepeatPin( sourcePin );
        g_lastPinWeakPtr = pin;

        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        if( pin )
            m_toolMgr->RunAction( EE_ACTIONS::addItemToSel, true, pin );
    }

    return 0;
}


void LIB_DRAWING_TOOLS::setTransitions()
{
    Go( &LIB_DRAWING_TOOLS::TwoClickPlace,        EE_ACTIONS::placeSymbolPin.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::TwoClickPlace,        EE_ACTIONS::placeSymbolText.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::DrawShape,            EE_ACTIONS::drawSymbolRectangle.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::DrawShape,            EE_ACTIONS::drawSymbolCircle.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::DrawShape,            EE_ACTIONS::drawSymbolArc.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::DrawShape,            EE_ACTIONS::drawSymbolLines.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::PlaceAnchor,          EE_ACTIONS::placeSymbolAnchor.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::RepeatDrawItem,       EE_ACTIONS::repeatDrawItem.MakeEvent() );
}
