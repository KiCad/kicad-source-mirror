/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_edit_frame.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_pin_tool.h>
#include <bitmaps.h>
#include <lib_text.h>
#include <dialogs/dialog_lib_edit_text.h>
#include <lib_arc.h>
#include <lib_circle.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <pgm_base.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <settings/settings_manager.h>
#include <kicad_string.h>

static void* g_lastPinWeakPtr;


SYMBOL_EDITOR_DRAWING_TOOLS::SYMBOL_EDITOR_DRAWING_TOOLS() :
        EE_TOOL_BASE<SYMBOL_EDIT_FRAME>( "eeschema.SymbolDrawing" ),
        m_lastTextAngle( 0.0 ),
        m_lastFillStyle( FILL_TYPE::NO_FILL ),
        m_drawSpecificConvert( true ),
        m_drawSpecificUnit( false )
{
}


bool SYMBOL_EDITOR_DRAWING_TOOLS::Init()
{
    EE_TOOL_BASE::Init();

    auto isDrawingCondition =
            [] ( const SELECTION& aSel )
            {
                LIB_ITEM* item = (LIB_ITEM*) aSel.Front();
                return item && item->IsNew();
            };

    m_menu.GetMenu().AddItem( EE_ACTIONS::finishDrawing, isDrawingCondition, 2 );

    return true;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace( const TOOL_EVENT& aEvent )
{
    KICAD_T   type = aEvent.Parameter<KICAD_T>();
    auto*     settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    auto*     pinTool = type == LIB_PIN_T ? m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>() : nullptr;
    VECTOR2I  cursorPos;
    EDA_ITEM* item   = nullptr;
    bool      isText = aEvent.IsAction( &EE_ACTIONS::placeSymbolText );

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    if( aEvent.HasPosition() || ( isText && !aEvent.IsReactivate() ) )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    auto setCursor =
            [&]()
            {
                if( item )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PLACE );
                else if( isText )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::TEXT );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        auto cleanup =
                [&] ()
                {
                    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                    m_view->ClearPreview();
                    delete item;
                    item = nullptr;
                };

        if( evt->IsCancelInteractive() )
        {
            if( item )
            {
                cleanup();
            }
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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

            if( !symbol )
                continue;

            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                switch( type )
                {
                case LIB_PIN_T:
                {
                    item = pinTool->CreatePin( wxPoint( cursorPos.x, -cursorPos.y ), symbol );
                    g_lastPinWeakPtr = item;
                    break;
                }
                case LIB_TEXT_T:
                {
                    LIB_TEXT* text = new LIB_TEXT( symbol );

                    text->SetPosition( wxPoint( cursorPos.x, -cursorPos.y ) );
                    text->SetTextSize( wxSize( Mils2iu( settings->m_Defaults.text_size ),
                                               Mils2iu( settings->m_Defaults.text_size ) ) );
                    text->SetTextAngle( m_lastTextAngle );

                    DIALOG_LIB_EDIT_TEXT dlg( m_frame, text );

                    if( dlg.ShowModal() != wxID_OK || NoPrintableChars( text->GetText() ) )
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
                    item->SetFlags( IS_NEW | IS_MOVING );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                    m_selectionTool->AddItemToSel( item );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }

                getViewControls()->SetCursorPosition( cursorPos, false );
            }
            // ... and second click places:
            else
            {
                m_frame->SaveCopyInUndoList( symbol );

                switch( item->Type() )
                {
                case LIB_PIN_T:
                    pinTool->PlacePin( (LIB_PIN*) item );
                    break;
                case LIB_TEXT_T:
                    symbol->AddDrawItem( (LIB_TEXT*) item );
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
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is an item to be placed
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    SYMBOL_EDITOR_SETTINGS* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    KICAD_T                 type = aEvent.Parameter<KICAD_T>();

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
    LIB_ITEM* item = nullptr;

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        auto cleanup =
                [&] ()
                {
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
            if( !symbol )
                continue;

            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            switch( type )
            {
            case LIB_ARC_T:       item = new LIB_ARC( symbol );       break;
            case LIB_CIRCLE_T:    item = new LIB_CIRCLE( symbol );    break;
            case LIB_POLYLINE_T:  item = new LIB_POLYLINE( symbol );  break;
            case LIB_RECTANGLE_T: item = new LIB_RECTANGLE( symbol ); break;
            default: break;     // keep compiler quiet
            }

            wxCHECK( item, 0 );

            item->SetWidth( Mils2iu( settings->m_Defaults.line_width ) );
            item->SetFillMode( m_lastFillStyle );
            item->SetFlags( IS_NEW );
            item->BeginEdit( wxPoint( cursorPos.x, -cursorPos.y ) );

            if( m_drawSpecificUnit )
                item->SetUnit( m_frame->GetUnit() );

            if( m_drawSpecificConvert )
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

                m_frame->SaveCopyInUndoList( symbol );
                symbol->AddDrawItem( item );
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
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::PlaceAnchor( const TOOL_EVENT& aEvent )
{
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

            if( !symbol )
                continue;

            VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );
            wxPoint  offset( -cursorPos.x, cursorPos.y );

            symbol->SetOffset( offset );

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
        {
            evt->SetPassEvent();
        }
    }

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    SYMBOL_EDITOR_PIN_TOOL* pinTool = m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>();
    LIB_SYMBOL*   symbol = m_frame->GetCurSymbol();
    LIB_PIN*      sourcePin = nullptr;

    if( !symbol )
        return 0;

    // See if we have a pin matching our weak ptr
    for( LIB_PIN* test = symbol->GetNextPin();  test;  test = symbol->GetNextPin( test ) )
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


void SYMBOL_EDITOR_DRAWING_TOOLS::setTransitions()
{
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace,  EE_ACTIONS::placeSymbolPin.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace,  EE_ACTIONS::placeSymbolText.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,      EE_ACTIONS::drawSymbolRectangle.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,      EE_ACTIONS::drawSymbolCircle.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,      EE_ACTIONS::drawSymbolArc.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,      EE_ACTIONS::drawSymbolLines.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::PlaceAnchor,    EE_ACTIONS::placeSymbolAnchor.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::RepeatDrawItem, EE_ACTIONS::repeatDrawItem.MakeEvent() );
}
