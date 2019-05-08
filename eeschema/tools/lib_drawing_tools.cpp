/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "lib_drawing_tools.h"
#include "lib_pin_tool.h"
#include <sch_actions.h>
#include <lib_edit_frame.h>
#include <sch_view.h>
#include <class_draw_panel_gal.h>
#include <project.h>
#include <id.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <view/view_group.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <hotkeys.h>
#include <class_libentry.h>
#include <bitmaps.h>
#include <lib_text.h>
#include <dialogs/dialog_lib_edit_text.h>

// Drawing tool actions
TOOL_ACTION SCH_ACTIONS::placeSymbolPin( "libedit.InteractiveDrawing.placeSymbolPin",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LIBEDIT_CREATE_PIN ),
        _( "Add Pin" ), _( "Add a pin" ),
        pin_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeSymbolText( "libedit.InteractiveDrawing.placeSymbolText",
        AS_GLOBAL, 0,
        _( "Add Text" ), _( "Add a text item" ),
        text_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawSymbolRectangle( "libedit.InteractiveDrawing.drawSymbolRectangle",
        AS_GLOBAL, 0,
        _( "Add Rectangle" ), _( "Add a rectangle" ),
        add_rectangle_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawSymbolCircle( "libedit.InteractiveDrawing.drawSymbolCircle",
        AS_GLOBAL, 0,
        _( "Add Circle" ), _( "Add a circle" ),
        add_circle_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawSymbolArc( "libedit.InteractiveDrawing.drawSymbolArc",
        AS_GLOBAL, 0,
        _( "Add Arc" ), _( "Add an arc" ),
        add_circle_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawSymbolLines( "libedit.InteractiveDrawing.drawSymbolLines",
        AS_GLOBAL, 0,
        _( "Add Lines" ), _( "Add connected graphic lines" ),
        add_circle_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeSymbolAnchor( "libedit.InteractiveDrawing.placeSymbolAnchor",
        AS_GLOBAL, 0,
        _( "Move Symbol Anchor" ), _( "Specify a new location for the symbol anchor" ),
        anchor_xpm, AF_ACTIVATE );


LIB_DRAWING_TOOLS::LIB_DRAWING_TOOLS() :
    TOOL_INTERACTIVE( "libedit.InteractiveDrawing" ),
    m_selectionTool( nullptr ),
    m_view( nullptr ),
    m_controls( nullptr ),
    m_frame( nullptr ),
    m_menu( *this )
{
}


LIB_DRAWING_TOOLS::~LIB_DRAWING_TOOLS()
{
}


bool LIB_DRAWING_TOOLS::Init()
{
    m_frame = getEditFrame<LIB_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    auto activeTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto& ctxMenu = m_menu.GetMenu();

    //
    // Build the drawing tool menu
    //
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeTool, 1 );

    ctxMenu.AddSeparator( activeTool, 1000 );
    m_menu.AddStandardSubMenus( m_frame );

    return true;
}


void LIB_DRAWING_TOOLS::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    m_controls = getViewControls();
    m_frame = getEditFrame<LIB_EDIT_FRAME>();
}


int LIB_DRAWING_TOOLS::PlacePin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_LIBEDIT_PIN_BUTT, wxCURSOR_PENCIL, _( "Add pin" ) );
    return doTwoClickPlace( LIB_PIN_T );
}


int LIB_DRAWING_TOOLS::PlaceText( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_LIBEDIT_BODY_TEXT_BUTT, wxCURSOR_PENCIL, _( "Add text" ) );
    return doTwoClickPlace( LIB_TEXT_T );
}


int LIB_DRAWING_TOOLS::doTwoClickPlace( KICAD_T aType )
{
    VECTOR2I  cursorPos = m_controls->GetCursorPosition();
    EDA_ITEM* item = nullptr;

    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    m_controls->ShowCursor( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( item )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete item;
                item = nullptr;

                if( !evt->IsActivate() )
                    continue;
            }

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            LIB_PART* part = m_frame->GetCurPart();

            if( !part )
                continue;

            // First click creates...
            if( !item )
            {
                m_frame->GetCanvas()->SetIgnoreMouseEvents( true );

                switch( aType )
                {
                case LIB_PIN_T:
                {
                    LIB_PIN_TOOL* pinTool = m_toolMgr->GetTool<LIB_PIN_TOOL>();

                    item = pinTool->CreatePin( wxPoint( cursorPos.x, -cursorPos.y), part );
                    break;
                }
                case LIB_TEXT_T:
                {
                    LIB_TEXT* text = new LIB_TEXT( part );
                    text->SetPosition( wxPoint( cursorPos.x, -cursorPos.y) );
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
                    wxFAIL_MSG( "doTwoClickPlace(): unknown type" );
                }

                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();

                if( item )
                {
                    item->SetFlags( IS_NEW | IS_MOVED );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                    m_selectionTool->AddItemToSel( item );
                }

                m_controls->SetCursorPosition( cursorPos, false );
            }

            // ... and second click places:
            else
            {
                m_frame->SaveCopyInUndoList( part );

                part->AddDrawItem( (LIB_ITEM*) item );
                item->ClearFlags( item->GetEditFlags() );

                m_frame->SetDrawItem( nullptr );
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
        else if( TOOL_EVT_UTILS::IsSelectionEvent( evt.get() ) )
        {
            // This happens if our text was replaced out from under us by ConvertTextType()
            SELECTION& selection = m_selectionTool->GetSelection();

            if( selection.GetSize() == 1 )
            {
                item = (LIB_ITEM*) selection.Front();
                m_view->ClearPreview();
                m_view->AddToPreview( item->Clone() );
            }
            else
                item = nullptr;
        }
        else if( item && ( evt->IsAction( &SCH_ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            static_cast<LIB_ITEM*>( item )->SetPosition( wxPoint( cursorPos.x, -cursorPos.y) );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        m_controls->SetAutoPan( !!item );
        m_controls->CaptureCursor( !!item );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int LIB_DRAWING_TOOLS::PlaceAnchor( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_LIBEDIT_ANCHOR_ITEM_BUTT, wxCURSOR_PENCIL, _( "Move symbol anchor" ) );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            LIB_PART* part = m_frame->GetCurPart();

            if( !part )
                continue;

            VECTOR2I cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );
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
            SELECTION emptySelection;

            m_menu.ShowContextMenu( emptySelection );
        }
    }

    m_frame->SetNoToolSelected();

    return 0;
}


void LIB_DRAWING_TOOLS::setTransitions()
{
    Go( &LIB_DRAWING_TOOLS::PlacePin,             SCH_ACTIONS::placeSymbolPin.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::PlaceText,            SCH_ACTIONS::placeSymbolText.MakeEvent() );
//    Go( &LIB_DRAWING_TOOLS::DrawRectangle,        SCH_ACTIONS::drawSymbolRectangle.MakeEvent() );
//    Go( &LIB_DRAWING_TOOLS::DrawCircle,           SCH_ACTIONS::drawSymbolCircle.MakeEvent() );
//    Go( &LIB_DRAWING_TOOLS::DrawArc,              SCH_ACTIONS::drawSymbolArc.MakeEvent() );
//    Go( &LIB_DRAWING_TOOLS::DrawLines,            SCH_ACTIONS::drawSymbolLines.MakeEvent() );
    Go( &LIB_DRAWING_TOOLS::PlaceAnchor,          SCH_ACTIONS::placeSymbolAnchor.MakeEvent() );
}
