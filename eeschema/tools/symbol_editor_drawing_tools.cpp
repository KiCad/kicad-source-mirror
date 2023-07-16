/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <optional>
#include <symbol_edit_frame.h>
#include <sch_commit.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_pin_tool.h>
#include <tools/ee_grid_helper.h>
#include <lib_text.h>
#include <dialogs/dialog_lib_text_properties.h>
#include <lib_shape.h>
#include <lib_textbox.h>
#include <pgm_base.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include "dialog_lib_textbox_properties.h"

static void* g_lastPinWeakPtr;


SYMBOL_EDITOR_DRAWING_TOOLS::SYMBOL_EDITOR_DRAWING_TOOLS() :
        EE_TOOL_BASE<SYMBOL_EDIT_FRAME>( "eeschema.SymbolDrawing" ),
        m_lastTextBold( false ),
        m_lastTextItalic( false ),
        m_lastTextAngle( ANGLE_HORIZONTAL ),
        m_lastTextJust( GR_TEXT_H_ALIGN_LEFT ),
        m_lastFillStyle( FILL_T::NO_FILL ),
        m_lastFillColor( COLOR4D::UNSPECIFIED ),
        m_lastStroke( 0, PLOT_DASH_TYPE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_drawSpecificConvert( true ),
        m_drawSpecificUnit( false ),
        m_inDrawShape( false ),
        m_inTwoClickPlace( false )
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
    KICAD_T type = aEvent.Parameter<KICAD_T>();
    auto*   settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    auto*   pinTool = type == LIB_PIN_T ? m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>() : nullptr;

    if( m_inTwoClickPlace )
        return 0;

    REENTRANCY_GUARD guard( &m_inTwoClickPlace );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;
    bool                  ignorePrimePosition = false;
    LIB_ITEM*             item   = nullptr;
    bool                  isText = aEvent.IsAction( &EE_ACTIONS::placeSymbolText );
    COMMON_SETTINGS*      common_settings = Pgm().GetCommonSettings();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    m_frame->PushTool( aEvent );

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

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection );
                m_view->ClearPreview();
                delete item;
                item = nullptr;
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
    {
        m_toolMgr->PrimeTool( aEvent.Position() );
    }
    else if( common_settings->m_Input.immediate_actions && !aEvent.IsReactivate() )
    {
        m_toolMgr->PrimeTool( { 0, 0 } );
        ignorePrimePosition = true;
    }

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition() );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = item && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() )
        {
            m_frame->GetInfoBar()->Dismiss();

            if( item )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() && !isSyntheticClick )
        {
            if( item && evt->IsMoveTool() )
            {
                // we're already moving our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( item )
            {
                m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel item creation." ) );
                evt->SetPassEvent( false );
                continue;
            }

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                break;
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) || isSyntheticClick )
        {
            LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

            if( !symbol )
                continue;

            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

                switch( type )
                {
                case LIB_PIN_T:
                {
                    item = pinTool->CreatePin( VECTOR2I( cursorPos.x, -cursorPos.y ), symbol );
                    g_lastPinWeakPtr = item;
                    break;
                }
                case LIB_TEXT_T:
                {
                    LIB_TEXT* text = new LIB_TEXT( symbol );

                    if( m_drawSpecificUnit )
                        text->SetUnit( m_frame->GetUnit() );

                    if( m_drawSpecificConvert )
                        text->SetConvert( m_frame->GetConvert() );

                    text->SetPosition( VECTOR2I( cursorPos.x, -cursorPos.y ) );
                    text->SetTextSize( VECTOR2I( schIUScale.MilsToIU( settings->m_Defaults.text_size ),
                                                 schIUScale.MilsToIU( settings->m_Defaults.text_size ) ) );
                    text->SetTextAngle( m_lastTextAngle );

                    DIALOG_LIB_TEXT_PROPERTIES dlg( m_frame, text );

                    if( dlg.ShowModal() != wxID_OK || NoPrintableChars( text->GetText() ) )
                        delete text;
                    else
                        item = text;

                    break;
                }
                default:
                    wxFAIL_MSG( "TwoClickPlace(): unknown type" );
                }

                // If we started with a hotkey which has a position then warp back to that.
                // Otherwise update to the current mouse position pinned inside the autoscroll
                // boundaries.
                if( evt->IsPrime() && !ignorePrimePosition )
                {
                    cursorPos = grid.Align( evt->Position() );
                    getViewControls()->WarpMouseCursor( cursorPos, true );
                }
                else
                {
                    getViewControls()->PinCursorInsideNonAutoscrollArea( true );
                    cursorPos = getViewControls()->GetMousePosition();
                }

                if( item )
                {
                    item->SetPosition( VECTOR2I( cursorPos.x, -cursorPos.y ) );

                    item->SetFlags( IS_NEW | IS_MOVING );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                    m_selectionTool->AddItemToSel( item );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }

                controls->SetCursorPosition( cursorPos, false );
            }
            // ... and second click places:
            else
            {
                SCH_COMMIT commit( m_toolMgr );
                commit.Modify( symbol, m_frame->GetScreen() );

                switch( item->Type() )
                {
                case LIB_PIN_T:
                    pinTool->PlacePin( (LIB_PIN*) item );
                    item->ClearEditFlags();
                    commit.Push( _( "Add Pin" ) );
                    break;

                case LIB_TEXT_T:
                    symbol->AddDrawItem( (LIB_TEXT*) item );
                    item->ClearEditFlags();
                    commit.Push( _( "Add Text" ) );
                    break;

                default:
                    wxFAIL_MSG( "TwoClickPlace(): unknown type" );
                }

                item = nullptr;
                m_view->ClearPreview();
                m_frame->RebuildView();
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
            item->SetPosition( VECTOR2I( cursorPos.x, -cursorPos.y ) );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is an item to be placed
        controls->SetAutoPan( item != nullptr );
        controls->CaptureCursor( item != nullptr );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    controls->ForceCursorPosition( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    SHAPE_T requestedShape = aEvent.Parameter<SHAPE_T>();

    return doDrawShape( aEvent, requestedShape );
}


int SYMBOL_EDITOR_DRAWING_TOOLS::DrawSymbolTextBox( const TOOL_EVENT& aEvent )
{
    return doDrawShape( aEvent, std::nullopt /* Draw text box */ );
}


int SYMBOL_EDITOR_DRAWING_TOOLS::doDrawShape( const TOOL_EVENT& aEvent, std::optional<SHAPE_T> aDrawingShape )
{
    bool    isTextBox = !aDrawingShape.has_value();
    SHAPE_T toolType  = aDrawingShape.value_or( SHAPE_T::SEGMENT );

    SETTINGS_MANAGER&       settingsMgr = Pgm().GetSettingsManager();
    SYMBOL_EDITOR_SETTINGS* settings = settingsMgr.GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    SHAPE_T                 shapeType = toolType == SHAPE_T::SEGMENT ? SHAPE_T::POLY : toolType;
    LIB_SYMBOL*             symbol = m_frame->GetCurSymbol();
    LIB_SHAPE*              item = nullptr;
    wxString                description;

    if( m_inDrawShape )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawShape );

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection );
                m_view->ClearPreview();
                delete item;
                item = nullptr;
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = item && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() )
        {
            if( item )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() && !isSyntheticClick )
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) && !item )
        {
            // Update in case the symbol was changed while the tool was running
            symbol = m_frame->GetCurSymbol();

            if( !symbol )
                continue;

            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

            int lineWidth = schIUScale.MilsToIU( settings->m_Defaults.line_width );

            if( isTextBox )
            {
                LIB_TEXTBOX* textbox = new LIB_TEXTBOX( symbol, lineWidth, m_lastFillStyle );

                textbox->SetBold( m_lastTextBold );
                textbox->SetItalic( m_lastTextItalic );
                textbox->SetTextSize( VECTOR2I( schIUScale.MilsToIU( settings->m_Defaults.text_size ),
                                                schIUScale.MilsToIU( settings->m_Defaults.text_size ) ) );
                textbox->SetTextAngle( m_lastTextAngle );
                textbox->SetHorizJustify( m_lastTextJust );

                item = textbox;
                description = _( "Add Text Box" );
            }
            else
            {
                item = new LIB_SHAPE( symbol, shapeType, lineWidth, m_lastFillStyle );
                description = wxString::Format( _( "Add %s" ), item->EDA_SHAPE::GetFriendlyName() );
            }

            item->SetStroke( m_lastStroke );
            item->SetFillColor( m_lastFillColor );

            item->SetFlags( IS_NEW );
            item->BeginEdit( VECTOR2I( cursorPos.x, -cursorPos.y ) );

            if( m_drawSpecificUnit )
                item->SetUnit( m_frame->GetUnit() );

            if( m_drawSpecificConvert )
                item->SetConvert( m_frame->GetConvert() );

            m_selectionTool->AddItemToSel( item );
        }
        else if( item && ( evt->IsClick( BUT_LEFT )
                        || evt->IsDblClick( BUT_LEFT )
                        || isSyntheticClick
                        || evt->IsAction( &EE_ACTIONS::finishDrawing ) ) )
        {
            if( symbol != m_frame->GetCurSymbol() )
            {
                symbol = m_frame->GetCurSymbol();
                item->SetParent( symbol );
            }

            if( evt->IsDblClick( BUT_LEFT ) || evt->IsAction( &EE_ACTIONS::finishDrawing )
                    || !item->ContinueEdit( VECTOR2I( cursorPos.x, -cursorPos.y ) ) )
            {
                if( toolType == SHAPE_T::POLY )
                {
                    item->CalcEdit( item->GetPosition() );  // Close shape
                    item->EndEdit( true );
                }
                else
                {
                    item->EndEdit();
                }

                item->ClearEditFlags();

                if( isTextBox )
                {
                    LIB_TEXTBOX*                  textbox = static_cast<LIB_TEXTBOX*>( item );
                    DIALOG_LIB_TEXTBOX_PROPERTIES dlg( m_frame, static_cast<LIB_TEXTBOX*>( item ) );

                    if( dlg.ShowQuasiModal() != wxID_OK )
                    {
                        cleanup();
                        continue;
                    }

                    m_lastTextBold = textbox->IsBold();
                    m_lastTextItalic = textbox->IsItalic();
                    m_lastTextAngle = textbox->GetTextAngle();
                    m_lastTextJust = textbox->GetHorizJustify();
                }

                m_lastStroke = item->GetStroke();
                m_lastFillStyle = item->GetFillMode();
                m_lastFillColor = item->GetFillColor();

                m_view->ClearPreview();

                SCH_COMMIT commit( m_toolMgr );
                commit.Modify( symbol, m_frame->GetScreen() );

                symbol->AddDrawItem( item );
                item = nullptr;

                commit.Push( description );
                m_frame->RebuildView();
                m_toolMgr->PostAction( ACTIONS::activatePointEditor );
            }
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->CalcEdit( VECTOR2I( cursorPos.x, -cursorPos.y ) );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }
        else if( evt->IsDblClick( BUT_LEFT ) && !item )
        {
            m_toolMgr->RunAction( EE_ACTIONS::properties );
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
    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        if( evt->IsCancelInteractive() )
        {
            m_frame->PopTool( aEvent );
            break;
        }
        else if( evt->IsActivate() )
        {
            m_frame->PopTool( aEvent );
            break;
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

            if( !symbol )
                continue;

            VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );
            VECTOR2I offset( -cursorPos.x, cursorPos.y );

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
    std::vector<LIB_PIN*> pins = symbol->GetAllLibPins();

    for( LIB_PIN* test : pins )
    {
        if( (void*) test == g_lastPinWeakPtr )
        {
            sourcePin = test;
            break;
        }
    }

    if( sourcePin )
    {
        LIB_PIN* pin = pinTool->RepeatPin( sourcePin );
        g_lastPinWeakPtr = pin;

        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        if( pin )
            m_toolMgr->RunAction<EDA_ITEM*>( EE_ACTIONS::addItemToSel, pin );
    }

    return 0;
}


void SYMBOL_EDITOR_DRAWING_TOOLS::setTransitions()
{
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace,     EE_ACTIONS::placeSymbolPin.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace,     EE_ACTIONS::placeSymbolText.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         EE_ACTIONS::drawRectangle.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         EE_ACTIONS::drawCircle.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         EE_ACTIONS::drawArc.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         EE_ACTIONS::drawSymbolLines.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         EE_ACTIONS::drawSymbolPolygon.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawSymbolTextBox, EE_ACTIONS::drawSymbolTextBox.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::PlaceAnchor,       EE_ACTIONS::placeSymbolAnchor.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::RepeatDrawItem,    EE_ACTIONS::repeatDrawItem.MakeEvent() );
}
