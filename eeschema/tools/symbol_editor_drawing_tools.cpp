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

#include <sch_actions.h>
#include <optional>
#include <symbol_edit_frame.h>
#include <sch_commit.h>
#include <gal/graphics_abstraction_layer.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_pin_tool.h>
#include <tools/ee_grid_helper.h>
#include <dialogs/dialog_text_properties.h>
#include <sch_shape.h>
#include <sch_textbox.h>
#include <pgm_base.h>
#include <view/view_controls.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <wx/msgdlg.h>
#include <import_gfx/dialog_import_gfx_sch.h>


KIID SYMBOL_EDITOR_DRAWING_TOOLS::g_lastPin;


SYMBOL_EDITOR_DRAWING_TOOLS::SYMBOL_EDITOR_DRAWING_TOOLS() :
        SCH_TOOL_BASE<SYMBOL_EDIT_FRAME>( "eeschema.SymbolDrawing" ),
        m_lastTextBold( false ),
        m_lastTextItalic( false ),
        m_lastTextAngle( ANGLE_HORIZONTAL ),
        m_lastTextJust( GR_TEXT_H_ALIGN_LEFT ),
        m_lastFillStyle( FILL_T::NO_FILL ),
        m_lastFillColor( COLOR4D::UNSPECIFIED ),
        m_lastStroke( 0, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_drawSpecificBodyStyle( true ),
        m_drawSpecificUnit( false ),
        m_inDrawShape( false ),
        m_inPlaceAnchor( false ),
        m_inTwoClickPlace( false )
{
}


bool SYMBOL_EDITOR_DRAWING_TOOLS::Init()
{
    SCH_TOOL_BASE::Init();

    auto isDrawingCondition =
            [] ( const SELECTION& aSel )
            {
                SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aSel.Front() );
                return item && item->IsNew();
            };

    m_menu->GetMenu().AddItem( ACTIONS::finishInteractive, isDrawingCondition, 2 );

    return true;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace( const TOOL_EVENT& aEvent )
{
    KICAD_T                 type = aEvent.Parameter<KICAD_T>();
    SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );
    SYMBOL_EDITOR_PIN_TOOL* pinTool = type == SCH_PIN_T ? m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>()
                                                        : nullptr;

    if( m_inTwoClickPlace )
        return 0;

    REENTRANCY_GUARD guard( &m_inTwoClickPlace );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;
    bool                  ignorePrimePosition = false;
    SCH_ITEM*             item   = nullptr;
    bool                  isText = aEvent.IsAction( &SCH_ACTIONS::placeSymbolText );
    COMMON_SETTINGS*      common_settings = Pgm().GetCommonSettings();

    m_toolMgr->RunAction( ACTIONS::selectionClear );

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
                m_toolMgr->RunAction( ACTIONS::selectionClear );
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

    SCH_COMMIT commit( m_toolMgr );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), grid.GetItemGrid( item ) );
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
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                switch( type )
                {
                case SCH_PIN_T:
                    item = pinTool->CreatePin( cursorPos, symbol );

                    if( item )
                        g_lastPin = item->m_Uuid;

                    break;

                case SCH_TEXT_T:
                {
                    SCH_TEXT* text = new SCH_TEXT( cursorPos, wxEmptyString, LAYER_DEVICE );

                    text->SetParent( symbol );

                    if( m_drawSpecificUnit )
                        text->SetUnit( m_frame->GetUnit() );

                    if( m_drawSpecificBodyStyle )
                        text->SetBodyStyle( m_frame->GetBodyStyle() );

                    if( cfg )
                    {
                        text->SetTextSize( VECTOR2I( schIUScale.MilsToIU( cfg->m_Defaults.text_size ),
                                                     schIUScale.MilsToIU( cfg->m_Defaults.text_size ) ) );
                    }

                    text->SetTextAngle( m_lastTextAngle );

                    DIALOG_TEXT_PROPERTIES dlg( m_frame, text );

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
                    cursorPos = grid.Align( evt->Position(), grid.GetItemGrid( item ) );
                    getViewControls()->WarpMouseCursor( cursorPos, true );
                }
                else
                {
                    getViewControls()->PinCursorInsideNonAutoscrollArea( true );
                    cursorPos = grid.Align( getViewControls()->GetMousePosition(), grid.GetItemGrid( item ) );
                }

                if( item )
                {
                    item->SetPosition( VECTOR2I( cursorPos.x, -cursorPos.y ) );

                    item->SetFlags( IS_NEW | IS_MOVING );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item, false );
                    m_selectionTool->AddItemToSel( item );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }

                if( m_frame->GetMoveWarpsCursor() )
                    controls->SetCursorPosition( cursorPos, false );

                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            }
            // ... and second click places:
            else
            {
                commit.Modify( symbol, m_frame->GetScreen() );

                switch( item->Type() )
                {
                case SCH_PIN_T:
                    pinTool->PlacePin( &commit, static_cast<SCH_PIN*>( item ) );
                    item->ClearEditFlags();
                    commit.Push( _( "Place Pin" ) );
                    break;

                case SCH_TEXT_T:
                    symbol->AddDrawItem( static_cast<SCH_TEXT*>( item ) );
                    item->ClearEditFlags();
                    commit.Push( _( "Draw Text" ) );
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

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::increment ) )
        {
            if( evt->HasParameter() )
                m_toolMgr->RunSynchronousAction( ACTIONS::increment, &commit, evt->Parameter<ACTIONS::INCREMENT>() );
            else
                m_toolMgr->RunSynchronousAction( ACTIONS::increment, &commit, ACTIONS::INCREMENT { 1, 0 } );
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->SetPosition( VECTOR2I( cursorPos.x, cursorPos.y ) );
            m_view->ClearPreview();
            m_view->AddToPreview( item, false );
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

    KIGFX::VIEW_CONTROLS*   controls = getViewControls();
    SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );
    EE_GRID_HELPER          grid( m_toolMgr );
    VECTOR2I                cursorPos;
    SHAPE_T                 shapeType = toolType == SHAPE_T::SEGMENT ? SHAPE_T::POLY : toolType;
    LIB_SYMBOL*             symbol = m_frame->GetCurSymbol();
    SCH_SHAPE*              item = nullptr;
    wxString                description;

    if( m_inDrawShape )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawShape );

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
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
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), grid.GetItemGrid( item ) );
        controls->ForceCursorPosition( true, cursorPos );

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

            m_toolMgr->RunAction( ACTIONS::selectionClear );

            int lineWidth = schIUScale.MilsToIU( cfg ? cfg->m_Defaults.line_width : DEFAULT_LINE_WIDTH_MILS );

            if( isTextBox )
            {
                SCH_TEXTBOX* textbox = new SCH_TEXTBOX( LAYER_DEVICE, lineWidth, m_lastFillStyle );

                textbox->SetParent( symbol );

                if( cfg )
                {
                    textbox->SetTextSize( VECTOR2I( schIUScale.MilsToIU( cfg->m_Defaults.text_size ),
                                                    schIUScale.MilsToIU( cfg->m_Defaults.text_size ) ) );
                }

                // Must be after SetTextSize()
                textbox->SetBold( m_lastTextBold );
                textbox->SetItalic( m_lastTextItalic );

                textbox->SetTextAngle( m_lastTextAngle );
                textbox->SetHorizJustify( m_lastTextJust );

                item = textbox;
                description = _( "Add Text Box" );
            }
            else
            {
                item = new SCH_SHAPE( shapeType, LAYER_DEVICE, lineWidth, m_lastFillStyle );
                item->SetParent( symbol );
                description = wxString::Format( _( "Add %s" ), item->GetFriendlyName() );
            }

            item->SetStroke( m_lastStroke );
            item->SetFillColor( m_lastFillColor );

            item->SetFlags( IS_NEW );
            item->BeginEdit( cursorPos );

            if( m_drawSpecificUnit )
                item->SetUnit( m_frame->GetUnit() );

            if( m_drawSpecificBodyStyle )
                item->SetBodyStyle( m_frame->GetBodyStyle() );

            m_selectionTool->AddItemToSel( item );
        }
        else if( item && ( evt->IsClick( BUT_LEFT )
                        || evt->IsDblClick( BUT_LEFT )
                        || isSyntheticClick
                        || evt->IsAction( &ACTIONS::finishInteractive ) ) )
        {
            if( symbol != m_frame->GetCurSymbol() )
            {
                symbol = m_frame->GetCurSymbol();
                item->SetParent( symbol );
            }

            if( evt->IsDblClick( BUT_LEFT ) || evt->IsAction( &ACTIONS::finishInteractive )
                || !item->ContinueEdit( VECTOR2I( cursorPos.x, cursorPos.y ) ) )
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
                    SCH_TEXTBOX*           textbox = static_cast<SCH_TEXTBOX*>( item );
                    DIALOG_TEXT_PROPERTIES dlg( m_frame, static_cast<SCH_TEXTBOX*>( item ) );

                    // QuasiModal required for syntax help and Scintilla auto-complete
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
            item->CalcEdit( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }
        else if( evt->IsDblClick( BUT_LEFT ) && !item )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::properties );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a shape being drawn
        controls->SetAutoPan( item != nullptr );
        controls->CaptureCursor( item != nullptr );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::PlaceAnchor( const TOOL_EVENT& aEvent )
{
    if( m_inPlaceAnchor )
        return 0;

    REENTRANCY_GUARD guard( &m_inPlaceAnchor );

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

            symbol->Move( -cursorPos );

            // Refresh the view without changing the viewport
            m_view->SetCenter( m_view->GetCenter() + cursorPos );
            m_view->RecacheAllItems();
            m_frame->OnModify();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::ImportGraphics( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    // Note: PlaceImportedGraphics() will convert PCB_SHAPE_T and PCB_TEXT_T to footprint
    // items if needed
    DIALOG_IMPORT_GFX_SCH dlg( m_frame );
    int                   dlgResult = dlg.ShowModal();

    std::list<std::unique_ptr<EDA_ITEM>>& list = dlg.GetImportedItems();

    if( dlgResult != wxID_OK )
        return 0;

    // Ensure the list is not empty:
    if( list.empty() )
    {
        wxMessageBox( _( "No graphic items found in file." ) );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::cancelInteractive );

    KIGFX::VIEW_CONTROLS*  controls = getViewControls();
    std::vector<SCH_ITEM*> newItems;      // all new items, including group
    std::vector<SCH_ITEM*> selectedItems; // the group, or newItems if no group
    SCH_SELECTION          preview;
    SCH_COMMIT             commit( m_toolMgr );

    for( std::unique_ptr<EDA_ITEM>& ptr : list )
    {
        SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( ptr.get() );
        wxCHECK2( item, continue );

        newItems.push_back( item );
        selectedItems.push_back( item );
        preview.Add( item );

        ptr.release();
    }

    if( !dlg.IsPlacementInteractive() )
    {
        commit.Modify( symbol, m_frame->GetScreen() );

        // Place the imported drawings
        for( SCH_ITEM* item : newItems )
        {
            symbol->AddDrawItem( item );
            item->ClearEditFlags();
        }

        commit.Push( _( "Import Graphic" ) );
        m_frame->RebuildView();

        return 0;
    }

    m_view->Add( &preview );

    // Clear the current selection then select the drawings so that edit tools work on them
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    EDA_ITEMS selItems( selectedItems.begin(), selectedItems.end() );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &selItems );

    m_frame->PushTool( aEvent );

    auto setCursor = [&]()
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
    };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    //SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DXF );
    EE_GRID_HELPER grid( m_toolMgr );

    // Now move the new items to the current cursor position:
    VECTOR2I cursorPos = controls->GetCursorPosition( !aEvent.DisableGridSnapping() );
    VECTOR2I delta = cursorPos;
    VECTOR2I currentOffset;

    for( SCH_ITEM* item : selectedItems )
        item->Move( delta );

    currentOffset += delta;

    m_view->Update( &preview );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            for( SCH_ITEM* item : newItems )
                delete item;

            break;
        }
        else if( evt->IsMotion() )
        {
            delta = cursorPos - currentOffset;

            for( SCH_ITEM* item : selectedItems )
                item->Move( delta );

            currentOffset += delta;

            m_view->Update( &preview );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            commit.Modify( symbol, m_frame->GetScreen() );

            // Place the imported drawings
            for( SCH_ITEM* item : newItems )
            {
                symbol->AddDrawItem( item );
                item->ClearEditFlags();
            }

            commit.Push( _( "Import Graphic" ) );
            break; // This is a one-shot command, not a tool
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    preview.Clear();
    m_view->Remove( &preview );

    m_frame->RebuildView();

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls->ForceCursorPosition( false );

    m_frame->PopTool( aEvent );

    return 0;
}


int SYMBOL_EDITOR_DRAWING_TOOLS::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    SYMBOL_EDITOR_PIN_TOOL* pinTool = m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>();
    LIB_SYMBOL*   symbol = m_frame->GetCurSymbol();
    SCH_PIN*      sourcePin = nullptr;

    if( !symbol )
        return 0;

    for( SCH_PIN* test : symbol->GetPins() )
    {
        if( test->m_Uuid == g_lastPin )
        {
            sourcePin = test;
            break;
        }
    }

    if( sourcePin )
    {
        SCH_PIN* pin = pinTool->RepeatPin( sourcePin );

        if( pin )
            g_lastPin = pin->m_Uuid;

        m_toolMgr->RunAction( ACTIONS::selectionClear );

        if( pin )
            m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, pin );
    }

    return 0;
}


void SYMBOL_EDITOR_DRAWING_TOOLS::setTransitions()
{
    // clang-format off
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace,     SCH_ACTIONS::placeSymbolPin.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::TwoClickPlace,     SCH_ACTIONS::placeSymbolText.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         SCH_ACTIONS::drawRectangle.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         SCH_ACTIONS::drawCircle.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         SCH_ACTIONS::drawArc.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         SCH_ACTIONS::drawBezier.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         SCH_ACTIONS::drawSymbolLines.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawShape,         SCH_ACTIONS::drawSymbolPolygon.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::DrawSymbolTextBox, SCH_ACTIONS::drawSymbolTextBox.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::PlaceAnchor,       SCH_ACTIONS::placeSymbolAnchor.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::ImportGraphics,    SCH_ACTIONS::importGraphics.MakeEvent() );
    Go( &SYMBOL_EDITOR_DRAWING_TOOLS::RepeatDrawItem,    SCH_ACTIONS::repeatDrawItem.MakeEvent() );
    // clang-format on
}
