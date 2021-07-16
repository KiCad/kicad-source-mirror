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

#include "sch_drawing_tools.h"
#include "ee_selection_tool.h"
#include "ee_grid_helper.h"
#include <ee_actions.h>
#include <sch_edit_frame.h>
#include <project.h>
#include <id.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <widgets/infobar.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <sch_symbol.h>
#include <sch_no_connect.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_bitmap.h>
#include <schematic.h>
#include <symbol_library.h>
#include <eeschema_settings.h>
#include <dialogs/dialog_edit_label.h>
#include <dialogs/dialog_line_wire_bus_properties.h>
#include <dialogs/dialog_junction_props.h>
#include <dialogs/dialog_sheet_pin_properties.h>
#include <kicad_string.h>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>


SCH_DRAWING_TOOLS::SCH_DRAWING_TOOLS() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawing" ),
        m_lastSheetPinType( PINSHEETLABEL_SHAPE::PS_INPUT ),
        m_lastGlobalLabelShape( PINSHEETLABEL_SHAPE::PS_INPUT ),
        m_lastTextOrientation( LABEL_SPIN_STYLE::RIGHT ),
        m_lastTextBold( false ),
        m_lastTextItalic( false ),
        m_inPlaceSymbol( false ),
        m_inPlaceImage( false ),
        m_inSingleClickPlace( false ),
        m_inTwoClickPlace( false ),
        m_inDrawSheet( false )
{
}


bool SCH_DRAWING_TOOLS::Init()
{
    EE_TOOL_BASE::Init();

    auto belowRootSheetCondition =
            [&]( const SELECTION& aSel )
            {
                return m_frame->GetCurrentSheet().Last() != &m_frame->Schematic().Root();
            };

    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();
    ctxMenu.AddItem( EE_ACTIONS::leaveSheet, belowRootSheetCondition, 2 );

    return true;
}


EDA_RECT SCH_DRAWING_TOOLS::GetCanvasFreeAreaPixels()
{
    // calculate the area of the canvas in pixels that create no autopan when
    // is inside this area the mouse cursor
    wxSize canvas_size = m_frame->GetCanvas()->GetSize();
    EDA_RECT canvas_area( wxPoint( 0, 0 ), canvas_size );
    const KIGFX::VC_SETTINGS& v_settings = getViewControls()->GetSettings();

    if( v_settings.m_autoPanEnabled )
        canvas_area.Inflate( - v_settings.m_autoPanMargin );

    // Gives a margin of 2 pixels
    canvas_area.Inflate( -2 );

    return canvas_area;
}


int SCH_DRAWING_TOOLS::PlaceSymbol( const TOOL_EVENT& aEvent )
{
    SCH_SYMBOL*                 symbol = aEvent.Parameter<SCH_SYMBOL*>();
    SCHLIB_FILTER               filter;
    std::vector<PICKED_SYMBOL>* historyList = nullptr;

    if( m_inPlaceSymbol )
        return 0;
    else
        m_inPlaceSymbol = true;

    if( aEvent.IsAction( &EE_ACTIONS::placeSymbol ) )
    {
        historyList = &m_symbolHistoryList;
    }
    else if (aEvent.IsAction( &EE_ACTIONS::placePower ) )
    {
        historyList = &m_powerHistoryList;
        filter.FilterPowerSymbols( true );
    }
    else
    {
        wxFAIL_MSG( "PlaceSymbol(): unexpected request" );
    }

    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto addSymbol =
            [&]( SCH_SYMBOL* aSymbol )
            {
                m_frame->SaveCopyForRepeatItem( aSymbol );

                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_selectionTool->AddItemToSel( aSymbol );

                aSymbol->SetParent( m_frame->GetScreen() );
                aSymbol->SetFlags( IS_NEW | IS_MOVING );
                m_frame->AddItemToScreenAndUndoList( m_frame->GetScreen(), aSymbol, false );

                // Set IS_MOVING again, as AddItemToScreenAndUndoList() will have cleared it.
                aSymbol->SetFlags( IS_MOVING );
                m_toolMgr->RunAction( ACTIONS::refreshPreview );
            };

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( symbol ? KICURSOR::MOVING
                                                               : KICURSOR::COMPONENT );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_frame->RollbackSchematicFromUndo();
                symbol = nullptr;
            };

    // Prime the pump
    if( symbol )
    {
        addSymbol( symbol );
        getViewControls()->WarpCursor( getViewControls()->GetMousePosition( false ) );
    }
    else if( !aEvent.IsReactivate() )
    {
        m_toolMgr->RunAction( EE_ACTIONS::cursorClick );
    }

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        if( evt->IsCancelInteractive() )
        {
            if( symbol )
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
            if( symbol && evt->IsMoveTool() )
            {
                // we're already moving our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( symbol )
            {
                m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel symbol creation." ) );
                evt->SetPassEvent( false );
                continue;
            }

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
            if( !symbol )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                // Store the mouse position: if it is outside the canvas,
                // (happens when clicking on a toolbar tool) one cannot
                // use the last stored cursor position to place the new symbol
                // (Current mouse pos after closing the dialog will be used)
                KIGFX::VIEW_CONTROLS* controls = getViewControls();
                VECTOR2D initialMousePos = controls->GetMousePosition(false);
                // Build the rectangle area acceptable to move the cursor without
                // having an auto-pan
                EDA_RECT canvas_area = GetCanvasFreeAreaPixels();

                // Pick the footprint to be placed
                bool footprintPreviews = m_frame->eeconfig()->m_Appearance.footprint_preview;
                PICKED_SYMBOL sel = m_frame->PickSymbolFromLibTree( &filter, *historyList, true,
                                                                    1, 1, footprintPreviews );
                // Restore cursor position after closing the dialog,
                // but only if it has meaning (i.e inside the canvas)
                VECTOR2D newMousePos = controls->GetMousePosition(false);

                if( canvas_area.Contains( wxPoint( initialMousePos ) ) )
                    controls->WarpCursor( controls->GetCursorPosition(), true );
                else if( !canvas_area.Contains( wxPoint( newMousePos ) ) )
                    // The mouse is outside the canvas area, after closing the dialog,
                    // thus can creating autopan issues. Warp the mouse to the canvas center
                    controls->WarpCursor( canvas_area.Centre(), false );

                LIB_SYMBOL* libSymbol = sel.LibId.IsValid() ?
                                        m_frame->GetLibSymbol( sel.LibId ) : nullptr;

                if( !libSymbol )
                    continue;

                wxPoint pos( cursorPos );
                symbol = new SCH_SYMBOL( *libSymbol, &m_frame->GetCurrentSheet(), sel, pos );
                addSymbol( symbol );

                // Update cursor now that we have a symbol
                setCursor();
            }
            else
            {
                if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                    symbol->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

                symbol->ClearEditFlags();

                m_toolMgr->RunAction( EE_ACTIONS::addNeededJunctions, true,
                                      &m_selectionTool->GetSelection() );

                m_view->Update( symbol );
                m_frame->GetScreen()->Update( symbol );
                m_frame->OnModify();

                SCH_SYMBOL* nextSymbol = nullptr;

                if( m_frame->eeconfig()->m_SymChooserPanel.place_all_units
                        || m_frame->eeconfig()->m_SymChooserPanel.keep_symbol )
                {
                    int new_unit = symbol->GetUnit();

                    if( m_frame->eeconfig()->m_SymChooserPanel.place_all_units
                        && symbol->GetUnit() < symbol->GetUnitCount() )
                    {
                        new_unit++;
                    }
                    else
                    {
                        new_unit = 1;
                    }

                    // We are either stepping to the next unit or next symbol
                    if( m_frame->eeconfig()->m_SymChooserPanel.keep_symbol || new_unit > 1 )
                    {
                        nextSymbol = static_cast<SCH_SYMBOL*>( symbol->Duplicate() );
                        nextSymbol->SetUnit( new_unit );
                        nextSymbol->SetUnitSelection( new_unit );

                        addSymbol( nextSymbol );
                    }
                }

                symbol = nextSymbol;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !symbol )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_SYM_MAX )
            {
                int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( symbol )
                {
                    m_frame->SelectUnit( symbol, unit );
                    m_toolMgr->RunAction( ACTIONS::refreshPreview );
                }
            }
        }
        else if( symbol && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            symbol->SetPosition( (wxPoint)cursorPos );
            m_view->Update( symbol );
        }
        else if( symbol && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a symbol to be placed
        getViewControls()->SetAutoPan( symbol != nullptr );
        getViewControls()->CaptureCursor( symbol != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_inPlaceSymbol = false;
    return 0;
}


int SCH_DRAWING_TOOLS::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP* image = aEvent.Parameter<SCH_BITMAP*>();
    bool        immediateMode = image;
    VECTOR2I    cursorPos = getViewControls()->GetCursorPosition();

    if( m_inPlaceImage )
        return 0;
    else
        m_inPlaceImage = true;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    // Add all the drawable symbols to preview
    if( image )
    {
        image->SetPosition( (wxPoint)cursorPos );
        m_view->ClearPreview();
        m_view->AddToPreview( image->Clone() );
    }

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto setCursor =
            [&]()
            {
                if( image )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete image;
                image = nullptr;
            };

    // Prime the pump
    if( image )
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    else if( !aEvent.IsReactivate() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        if( evt->IsCancelInteractive() )
        {
            if( image )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }

            if( immediateMode )
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( image && evt->IsMoveTool() )
            {
                // we're already moving our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( image )
            {
                m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel image creation." ) );
                evt->SetPassEvent( false );
                continue;
            }

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
            if( !image )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                // Store the mouse position: if it is outside the canvas,
                // (happens when clicking on a toolbar tool) one cannot
                // use the last stored cursor position to place the new symbol
                // (Current mouse pos after closing the dialog will be used)
                KIGFX::VIEW_CONTROLS* controls = getViewControls();
                VECTOR2D initialMousePos = controls->GetMousePosition(false);
                // Build the rectangle area acceptable to move the cursor without
                // having an auto-pan
                EDA_RECT canvas_area = GetCanvasFreeAreaPixels();

                wxFileDialog dlg( m_frame, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                                  _( "Image Files" ) + wxS( " " ) + wxImage::GetImageExtWildcard(),
                                  wxFD_OPEN );

                if( dlg.ShowModal() != wxID_OK )
                    continue;

                // Restore cursor position after closing the dialog,
                // but only if it has meaning (i.e inside the canvas)
                VECTOR2D newMousePos = controls->GetMousePosition( false );

                if( canvas_area.Contains( wxPoint( initialMousePos ) ) )
                    controls->WarpCursor( controls->GetCursorPosition(), true );
                else if( !canvas_area.Contains( wxPoint( newMousePos ) ) )
                    // The mouse is outside the canvas area, after closing the dialog,
                    // thus can creating autopan issues. Warp the mouse to the canvas center
                    controls->WarpCursor( canvas_area.Centre(), false );

                cursorPos = controls->GetMousePosition( true );

                wxString fullFilename = dlg.GetPath();

                if( wxFileExists( fullFilename ) )
                    image = new SCH_BITMAP( (wxPoint)cursorPos );

                if( !image || !image->ReadImageFile( fullFilename ) )
                {
                    wxMessageBox( _( "Could not load image from '%s'." ), fullFilename );
                    delete image;
                    image = nullptr;
                    continue;
                }

                image->SetFlags( IS_NEW | IS_MOVING );

                m_frame->SaveCopyForRepeatItem( image );

                m_view->ClearPreview();
                m_view->AddToPreview( image->Clone() );
                m_view->RecacheAllItems();  // Bitmaps are cached in Opengl

                m_selectionTool->AddItemToSel( image );

                getViewControls()->SetCursorPosition( cursorPos, false );
                setCursor();
            }
            else
            {
                m_frame->AddItemToScreenAndUndoList( m_frame->GetScreen(), image, false );
                image = nullptr;
                m_toolMgr->RunAction( ACTIONS::activatePointEditor );

                m_view->ClearPreview();

                if( immediateMode )
                {
                    m_frame->PopTool( tool );
                    break;
                }
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !image )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( image && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            image->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( image->Clone() );
            m_view->RecacheAllItems();  // Bitmaps are cached in Opengl
        }
        else if( image && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is an image to be placed
        getViewControls()->SetAutoPan( image != nullptr );
        getViewControls()->CaptureCursor( image != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_inPlaceImage = false;
    return 0;
}


int SCH_DRAWING_TOOLS::SingleClickPlace( const TOOL_EVENT& aEvent )
{
    wxPoint               cursorPos;
    KICAD_T               type = aEvent.Parameter<KICAD_T>();
    EE_GRID_HELPER        grid( m_toolMgr );
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    SCH_ITEM*             previewItem;
    bool                  loggedInfoBarError = false;

    if( m_inSingleClickPlace )
        return 0;

    if( type == SCH_JUNCTION_T && aEvent.HasPosition() )
    {
        EE_SELECTION& selection = m_selectionTool->GetSelection();
        SCH_LINE*     wire = dynamic_cast<SCH_LINE*>( selection.Front() );

        if( wire )
        {
            SEG seg( wire->GetStartPoint(), wire->GetEndPoint() );
            VECTOR2I nearest = seg.NearestPoint( getViewControls()->GetCursorPosition() );
            getViewControls()->SetCrossHairCursorPosition( nearest, false );
            getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );
        }
    }

    switch( type )
    {
    case SCH_NO_CONNECT_T:
        previewItem = new SCH_NO_CONNECT( cursorPos );
        previewItem->SetParent( m_frame->GetScreen() );
        break;

    case SCH_JUNCTION_T:
        previewItem = new SCH_JUNCTION( cursorPos );
        previewItem->SetParent( m_frame->GetScreen() );
        break;

    case SCH_BUS_WIRE_ENTRY_T:
        previewItem = new SCH_BUS_WIRE_ENTRY( cursorPos );
        previewItem->SetParent( m_frame->GetScreen() );
        break;

    case SCH_SHEET_PIN_T:
    {
        EE_SELECTION& selection = m_selectionTool->GetSelection();
        SCH_SHEET*    sheet = dynamic_cast<SCH_SHEET*>( selection.Front() );

        if( !sheet )
            return 0;

        SCH_HIERLABEL* label = importHierLabel( sheet );

        if( !label )
        {
            m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
            m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
            m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
            m_statusPopup->PopupFor( 2000 );
            return 0;
        }

        previewItem = createSheetPin( sheet, label );
    }
        break;

    default:
        wxASSERT_MSG( false, "Unknown item type in SCH_DRAWING_TOOLS::SingleClickPlace" );
        return 0;
    }

    m_inSingleClickPlace = true;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    cursorPos = static_cast<wxPoint>( aEvent.HasPosition() ?
                                      aEvent.Position() :
                                      controls->GetMousePosition() );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    m_view->ClearPreview();
    m_view->AddToPreview( previewItem->Clone() );

    // Prime the pump
    if( aEvent.HasPosition() && type != SCH_SHEET_PIN_T )
        m_toolMgr->RunAction( ACTIONS::cursorClick );
    else
        m_toolMgr->RunAction( ACTIONS::refreshPreview );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PLACE );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = evt->IsPrime() ? (wxPoint) evt->Position()
                                   : (wxPoint) controls->GetMousePosition();

        cursorPos = wxPoint( grid.BestSnapAnchor( cursorPos, LAYER_CONNECTABLE, nullptr ) );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() )
        {
            m_frame->PopTool( tool );
            break;
        }
        else if( evt->IsActivate() )
        {
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
            if( !m_frame->GetScreen()->GetItem( cursorPos, 0, type ) )
            {
                if( type == SCH_JUNCTION_T )
                {
                    if( !m_frame->GetScreen()->IsJunctionNeeded( cursorPos ) )
                    {
                        m_frame->ShowInfoBarError( _( "Junction location contains no joinable "
                                                      "wires and/or pins." ) );
                        loggedInfoBarError = true;
                        continue;
                    }
                    else if( loggedInfoBarError )
                    {
                        m_frame->GetInfoBar()->Dismiss();
                    }
                }

                SCH_ITEM* newItem = static_cast<SCH_ITEM*>( previewItem->Clone() );
                newItem->SetPosition( cursorPos );
                newItem->SetFlags( IS_NEW );

                m_frame->AddItemToScreenAndUndoList( m_frame->GetScreen(), newItem, false );

                if( type == SCH_JUNCTION_T )
                    m_frame->TestDanglingEnds();
                else
                    m_frame->SchematicCleanUp();

                m_frame->OnModify();
            }

            if( evt->IsDblClick( BUT_LEFT ) || type == SCH_SHEET_PIN_T )  // Finish tool.
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() )
        {
            previewItem->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( previewItem->Clone() );
        }
        else if( evt->Category() == TC_COMMAND )
        {
            if( ( type == SCH_BUS_WIRE_ENTRY_T )
                    && (   evt->IsAction( &EE_ACTIONS::rotateCW )
                        || evt->IsAction( &EE_ACTIONS::rotateCCW )
                        || evt->IsAction( &EE_ACTIONS::mirrorV )
                        || evt->IsAction( &EE_ACTIONS::mirrorH ) ) )
            {
                SCH_BUS_ENTRY_BASE* busItem = static_cast<SCH_BUS_ENTRY_BASE*>( previewItem );

                // The bus entries only rotate in one direction
                if( evt->IsAction( &EE_ACTIONS::rotateCW )
                        || evt->IsAction( &EE_ACTIONS::rotateCCW ) )
                {
                    busItem->Rotate( busItem->GetPosition() );
                }
                else if( evt->IsAction( &EE_ACTIONS::mirrorV ) )
                {
                    busItem->MirrorVertically( busItem->GetPosition().x );
                }
                else if( evt->IsAction( &EE_ACTIONS::mirrorH ) )
                {
                    busItem->MirrorHorizontally( busItem->GetPosition().y );
                }

                m_view->ClearPreview();
                m_view->AddToPreview( previewItem->Clone() );
            }
            else if( evt->IsAction( &EE_ACTIONS::properties ) )
            {
                switch( type )
                {
                case SCH_BUS_WIRE_ENTRY_T:
                {
                    std::deque<SCH_ITEM*> strokeItems;
                    strokeItems.push_back( previewItem );

                    DIALOG_LINE_WIRE_BUS_PROPERTIES dlg( m_frame, strokeItems );

                    if( dlg.ShowModal() == wxID_OK )
                    {
                        m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
                        m_frame->OnModify();
                    }
                }
                    break;

                case SCH_JUNCTION_T:
                {
                    std::deque<SCH_JUNCTION*> junctions;
                    junctions.push_back( static_cast<SCH_JUNCTION*>( previewItem ) );

                    DIALOG_JUNCTION_PROPS dlg( m_frame, junctions );

                    if( dlg.ShowModal() == wxID_OK )
                    {
                        m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
                        m_frame->OnModify();
                    }
                }
                    break;
                default:
                    // Do nothing
                    break;
                }

                m_view->ClearPreview();
                m_view->AddToPreview( previewItem->Clone() );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    delete previewItem;
    m_view->ClearPreview();

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls->ForceCursorPosition( false );
    m_inSingleClickPlace = false;
    return 0;
}


SCH_TEXT* SCH_DRAWING_TOOLS::createNewText( const VECTOR2I& aPosition, int aType )
{
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& settings = schematic->Settings();
    SCH_TEXT*           textItem = nullptr;

    switch( aType )
    {
    case LAYER_NOTES:
        textItem = new SCH_TEXT( (wxPoint) aPosition );
        break;

    case LAYER_LOCLABEL:
        textItem = new SCH_LABEL( (wxPoint) aPosition );
        break;

    case LAYER_HIERLABEL:
        textItem = new SCH_HIERLABEL( (wxPoint) aPosition );
        textItem->SetShape( m_lastGlobalLabelShape );
        break;

    case LAYER_GLOBLABEL:
        textItem = new SCH_GLOBALLABEL( (wxPoint) aPosition );
        textItem->SetShape( m_lastGlobalLabelShape );

        if( settings.m_IntersheetRefsShow )
            static_cast<SCH_GLOBALLABEL*>( textItem )->GetIntersheetRefs()->SetVisible( true );
        break;

    default:
        wxFAIL_MSG( "SCH_EDIT_FRAME::CreateNewText() unknown layer type" );
        return nullptr;
    }

    textItem->SetParent( schematic );
    textItem->SetBold( m_lastTextBold );
    textItem->SetItalic( m_lastTextItalic );
    textItem->SetLabelSpinStyle( m_lastTextOrientation );
    textItem->SetTextSize( wxSize( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
    textItem->SetFlags( IS_NEW | IS_MOVING );

    DIALOG_LABEL_EDITOR dlg( m_frame, textItem );

    // Must be quasi modal for syntax help
    if( dlg.ShowQuasiModal() != wxID_OK || NoPrintableChars( textItem->GetText() ) )
    {
        delete textItem;
        return nullptr;
    }

    m_lastTextBold = textItem->IsBold();
    m_lastTextItalic = textItem->IsItalic();
    m_lastTextOrientation = textItem->GetLabelSpinStyle();

    if( textItem->Type() == SCH_GLOBAL_LABEL_T || textItem->Type() == SCH_HIER_LABEL_T )
        m_lastGlobalLabelShape = textItem->GetShape();

    return textItem;
}


SCH_HIERLABEL* SCH_DRAWING_TOOLS::importHierLabel( SCH_SHEET* aSheet )
{
    if( !aSheet->GetScreen() )
        return nullptr;

    for( EDA_ITEM* item : aSheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

        /* A global label has been found: check if there a corresponding sheet label. */
        if( !aSheet->HasPin( label->GetText() ) )
            return label;
    }

    return nullptr;
}


SCH_SHEET_PIN* SCH_DRAWING_TOOLS::createSheetPin( SCH_SHEET* aSheet, SCH_HIERLABEL* aLabel )
{
    SCHEMATIC_SETTINGS& settings = aSheet->Schematic()->Settings();
    wxString            text;
    SCH_SHEET_PIN*      sheetPin;

    if( aLabel )
    {
        text = aLabel->GetText();
        m_lastSheetPinType = aLabel->GetShape();
    }

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), text );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetTextSize( wxSize( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
    sheetPin->SetShape( m_lastSheetPinType );

    if( !aLabel )
    {
        DIALOG_SHEET_PIN_PROPERTIES dlg( m_frame, sheetPin );

        if( dlg.ShowModal() != wxID_OK || NoPrintableChars( sheetPin->GetText() )  )
        {
            delete sheetPin;
            return nullptr;
        }
    }

    m_lastSheetPinType = sheetPin->GetShape();

    sheetPin->SetPosition( (wxPoint) getViewControls()->GetCursorPosition() );

    return sheetPin;
}


int SCH_DRAWING_TOOLS::TwoClickPlace( const TOOL_EVENT& aEvent )
{
    SCH_ITEM*             item = nullptr;
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );

    if( m_inTwoClickPlace )
        return 0;
    else
        m_inTwoClickPlace = true;

    bool isText        = aEvent.IsAction( &EE_ACTIONS::placeSchematicText );
    bool isGlobalLabel = aEvent.IsAction( &EE_ACTIONS::placeGlobalLabel );
    bool isHierLabel   = aEvent.IsAction( &EE_ACTIONS::placeHierLabel );
    bool isNetLabel    = aEvent.IsAction( &EE_ACTIONS::placeLabel );
    bool isSheetPin    = aEvent.IsAction( &EE_ACTIONS::importSheetPin );
    int  snapLayer     = isText ? LAYER_GRAPHICS : LAYER_CONNECTABLE;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    controls->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto setCursor =
            [&]()
            {
                if( item )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PLACE );
                else if( isText )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::TEXT );
                else if( isGlobalLabel )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LABEL_GLOBAL );
                else if( isNetLabel )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LABEL_NET );
                else if( isHierLabel )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LABEL_HIER );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto updatePreview =
            [&]()
            {
                m_view->ClearPreview();
                m_view->AddToPreview( item->Clone() );
                item->RunOnChildren( [&]( SCH_ITEM* aChild )
                                     {
                                         m_view->AddToPreview( aChild->Clone() );
                                     } );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete item;
                item = nullptr;
            };

    // Prime the pump
    // If the tool isn't being re-activated
    if( aEvent.HasPosition() || ( !aEvent.IsReactivate()
            && ( isText || isGlobalLabel || isHierLabel || isNetLabel ) ) )
    {
        m_toolMgr->RunAction( ACTIONS::cursorClick );
    }

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        VECTOR2I cursorPos = evt->IsPrime() ? evt->Position() : controls->GetMousePosition();
        cursorPos = grid.BestSnapAnchor( cursorPos, snapLayer, item );
        controls->ForceCursorPosition( true, cursorPos );

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
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                if( isText )
                {
                    item = createNewText( cursorPos, LAYER_NOTES );
                }
                else if( isGlobalLabel )
                {
                    item = createNewText( cursorPos, LAYER_GLOBLABEL );
                }
                else if( isHierLabel )
                {
                    item = createNewText( cursorPos, LAYER_HIERLABEL );
                }
                else if( isNetLabel )
                {
                    item = createNewText( cursorPos, LAYER_LOCLABEL );
                }
                else if( isSheetPin )
                {
                    EDA_ITEM*      i;
                    SCH_HIERLABEL* label = nullptr;
                    SCH_SHEET*     sheet = nullptr;

                    if( m_selectionTool->SelectPoint( cursorPos, EE_COLLECTOR::SheetsOnly, &i ) )
                        sheet = dynamic_cast<SCH_SHEET*>( i );

                    m_selectionTool->ClearSelection();

                    if( !sheet )
                    {
                        m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
                        m_statusPopup->SetText( _( "Click over a sheet." ) );
                        m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                        m_statusPopup->PopupFor( 2000 );
                        item = nullptr;
                    }
                    else
                    {
                        label = importHierLabel( sheet );

                        if( !label )
                        {
                            m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
                            m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
                            m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                            m_statusPopup->PopupFor( 2000 );
                            item = nullptr;
                        }
                        else
                        {
                            item = createSheetPin( sheet, label );
                        }
                    }
                }

                // Restore cursor after dialog
                controls->WarpCursor( controls->GetCursorPosition(), true );

                if( item )
                {
                    item->SetFlags( IS_NEW | IS_MOVING );
                    item->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );
                    updatePreview();

                    m_selectionTool->AddItemToSel( item );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }

                controls->SetCursorPosition( cursorPos, false );
            }
            else            // ... and second click places:
            {
                item->ClearFlags( IS_MOVING );
                m_frame->AddItemToScreenAndUndoList( m_frame->GetScreen(), (SCH_ITEM*) item,
                                                     false );
                item = nullptr;

                m_view->ClearPreview();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && evt->IsSelectionEvent() )
        {
            // This happens if our text was replaced out from under us by ConvertTextType()
            EE_SELECTION& selection = m_selectionTool->GetSelection();

            if( selection.GetSize() == 1 )
            {
                item = (SCH_ITEM*) selection.Front();
                updatePreview();
            }
            else
            {
                item = nullptr;
            }
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->SetPosition( (wxPoint) cursorPos );
            item->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );
            updatePreview();
        }
        else if( item && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
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
    m_inTwoClickPlace = false;
    return 0;
}


int SCH_DRAWING_TOOLS::DrawSheet( const TOOL_EVENT& aEvent )
{
    SCH_SHEET* sheet = nullptr;

    if( m_inDrawSheet )
        return 0;
    else
        m_inDrawSheet = true;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete sheet;
                sheet = nullptr;
            };

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        if( evt->IsCancelInteractive() )
        {
            if( sheet )
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
            if( sheet && evt->IsMoveTool() )
            {
                // we're already drawing our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( sheet )
            {
                m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel sheet creation." ) );
                evt->SetPassEvent( false );
                continue;
            }

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
        else if( !sheet && ( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) ) )
        {
            EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();

            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            sheet = new SCH_SHEET( m_frame->GetCurrentSheet().Last(),
                                   static_cast<wxPoint>( cursorPos ) );
            sheet->SetFlags( IS_NEW | IS_RESIZING );
            sheet->SetScreen( nullptr );
            sheet->SetBorderWidth( cfg->m_Drawing.default_line_thickness );
            sheet->SetBorderColor( cfg->m_Drawing.default_sheet_border_color );
            sheet->SetBackgroundColor( cfg->m_Drawing.default_sheet_background_color );
            sheet->GetFields()[ SHEETNAME ].SetText( "Untitled Sheet" );
            sheet->GetFields()[ SHEETFILENAME ].SetText( "untitled." + KiCadSchematicFileExtension );
            sizeSheet( sheet, cursorPos );

            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
        }
        else if( sheet && ( evt->IsClick( BUT_LEFT )
                         || evt->IsDblClick( BUT_LEFT )
                         || evt->IsAction( &EE_ACTIONS::finishSheet ) ) )
        {
            m_view->ClearPreview();
            getViewControls()->SetAutoPan( false );
            getViewControls()->CaptureCursor( false );

            // Find the list of paths in the hierarchy that refer to the destination sheet where
            // the new sheet will be drawn
            SCH_SCREEN*    currentScreen = m_frame->GetCurrentSheet().LastScreen();
            SCH_SHEET_LIST hierarchy = m_frame->Schematic().GetSheets();
            SCH_SHEET_LIST instances = hierarchy.FindAllSheetsForScreen( currentScreen );
            instances.SortByPageNumbers();

            int pageNum = static_cast<int>( hierarchy.size() ) + 1;

            // Set a page number for all the instances of the new sheet in the hierarchy
            for( SCH_SHEET_PATH& instance : instances )
            {
                SCH_SHEET_PATH sheetPath = instance;
                sheetPath.push_back( sheet );
                sheet->AddInstance( sheetPath.Path() );
                sheet->SetPageNumber( sheetPath, wxString::Format( "%d", pageNum++ ) );
            }

            if( m_frame->EditSheetProperties( static_cast<SCH_SHEET*>( sheet ),
                                              &m_frame->GetCurrentSheet(), nullptr ) )
            {
                sheet->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

                m_frame->AddItemToScreenAndUndoList( m_frame->GetScreen(), sheet, false );
                m_frame->UpdateHierarchyNavigator();
                m_selectionTool->AddItemToSel( sheet );
            }
            else
            {
                delete sheet;
            }

            sheet = nullptr;
        }
        else if( sheet && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            sizeSheet( sheet, cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !sheet )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a sheet to be placed
        getViewControls()->SetAutoPan( sheet != nullptr );
        getViewControls()->CaptureCursor( sheet != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_inDrawSheet = false;
    return 0;
}


void SCH_DRAWING_TOOLS::sizeSheet( SCH_SHEET* aSheet, VECTOR2I aPos )
{
    wxPoint pos = aSheet->GetPosition();
    wxPoint size = (wxPoint) aPos - pos;

    size.x = std::max( size.x, MIN_SHEET_WIDTH );
    size.y = std::max( size.y, MIN_SHEET_HEIGHT );

    wxPoint grid = m_frame->GetNearestGridPosition( pos + size );
    aSheet->Resize( wxSize( grid.x - pos.x, grid.y - pos.y ) );
}


void SCH_DRAWING_TOOLS::setTransitions()
{
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,         EE_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,         EE_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeHierLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,           EE_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::importSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::importSingleSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceImage,          EE_ACTIONS::placeImage.MakeEvent() );
}
