/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>

#include <tools/sch_drawing_tools.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_grid_helper.h>
#include <ee_actions.h>
#include <sch_edit_frame.h>
#include <project.h>
#include <pgm_base.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <sch_symbol.h>
#include <sch_no_connect.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_bitmap.h>
#include <schematic.h>
#include <sch_commit.h>
#include <symbol_library_common.h>
#include <eeschema_settings.h>
#include <dialogs/dialog_label_properties.h>
#include <dialogs/dialog_text_properties.h>
#include <dialogs/dialog_wire_bus_properties.h>
#include <dialogs/dialog_junction_props.h>
#include <dialogs/dialog_sheet_pin_properties.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>

SCH_DRAWING_TOOLS::SCH_DRAWING_TOOLS() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawing" ),
        m_lastSheetPinType( LABEL_FLAG_SHAPE::L_INPUT ),
        m_lastGlobalLabelShape( LABEL_FLAG_SHAPE::L_INPUT ),
        m_lastNetClassFlagShape( LABEL_FLAG_SHAPE::F_ROUND ),
        m_lastTextOrientation( TEXT_SPIN_STYLE::RIGHT ),
        m_lastTextBold( false ),
        m_lastTextItalic( false ),
        m_lastTextAngle( ANGLE_0 ),
        m_lastTextJust( GR_TEXT_H_ALIGN_LEFT ),
        m_lastFillStyle( FILL_T::NO_FILL ),
        m_lastTextboxFillStyle( FILL_T::NO_FILL ),
        m_lastFillColor( COLOR4D::UNSPECIFIED ),
        m_lastTextboxFillColor( COLOR4D::UNSPECIFIED ),
        m_lastStroke( 0, PLOT_DASH_TYPE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_lastTextboxStroke( 0, PLOT_DASH_TYPE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_mruPath( wxEmptyString ),
        m_lastAutoLabelRotateOnPlacement( false ),
        m_inPlaceSymbol( false ),
        m_inDrawShape( false ),
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


int SCH_DRAWING_TOOLS::PlaceSymbol( const TOOL_EVENT& aEvent )
{
    SCH_SYMBOL*                 symbol = aEvent.Parameter<SCH_SYMBOL*>();
    SYMBOL_LIBRARY_FILTER       filter;
    std::vector<PICKED_SYMBOL>* historyList = nullptr;
    bool                        ignorePrimePosition = false;
    COMMON_SETTINGS*            common_settings = Pgm().GetCommonSettings();
    EE_GRID_HELPER              grid( m_toolMgr );
    SCH_SCREEN*                 screen = m_frame->GetScreen();

    if( m_inPlaceSymbol )
        return 0;

    REENTRANCY_GUARD guard( &m_inPlaceSymbol );

    // First we need to get all instances of this sheet so we can annotate
    // whatever symbols we place on all copies
    SCH_SHEET_LIST hierarchy = m_frame->Schematic().GetSheets();
    SCH_SHEET_LIST newInstances =
            hierarchy.FindAllSheetsForScreen( m_frame->GetCurrentSheet().LastScreen() );
    newInstances.SortByPageNumbers();

    // Get a list of all references in the schematic to avoid duplicates wherever
    // they're placed
    SCH_REFERENCE_LIST existingRefs;
    hierarchy.GetSymbols( existingRefs );
    existingRefs.SortByReferenceOnly();

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

    m_frame->PushTool( aEvent );

    auto addSymbol =
            [this]( SCH_SYMBOL* aSymbol )
            {
                m_frame->SaveCopyForRepeatItem( aSymbol );

                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_selectionTool->AddItemToSel( aSymbol );

                aSymbol->SetFlags( IS_NEW | IS_MOVING );

                m_view->ClearPreview();
                m_view->AddToPreview( aSymbol, false );   // Add, but not give ownership

                // Set IS_MOVING again, as AddItemToCommitAndScreen() will have cleared it.
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
            [&]()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete symbol;
                symbol = nullptr;

                existingRefs.Clear();
                hierarchy.GetSymbols( existingRefs );
                existingRefs.SortByReferenceOnly();
            };

    auto annotate =
            [&]()
            {
                EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();

                // Then we need to annotate all instances by sheet
                for( SCH_SHEET_PATH& instance : newInstances )
                {
                    SCH_REFERENCE newReference( symbol, symbol->GetLibSymbolRef().get(), instance );
                    SCH_REFERENCE_LIST refs;
                    refs.AddItem( newReference );

                    if( cfg->m_AnnotatePanel.automatic || newReference.AlwaysAnnotate() )
                    {
                        refs.ReannotateByOptions( (ANNOTATE_ORDER_T) cfg->m_AnnotatePanel.sort_order,
                                                  (ANNOTATE_ALGO_T) cfg->m_AnnotatePanel.method,
                                                  m_frame->Schematic().Settings().m_AnnotateStartNum,
                                                  existingRefs, false, &hierarchy );

                        refs.UpdateAnnotation();

                        // Update existing refs for next iteration
                        for( size_t i = 0; i < refs.GetCount(); i++ )
                            existingRefs.AddItem( refs[i] );
                    }
                }

                m_frame->GetCurrentSheet().UpdateAllScreenReferences();
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    // Prime the pump
    if( symbol )
    {
        addSymbol( symbol );
        annotate();
        getViewControls()->WarpMouseCursor( getViewControls()->GetMousePosition( false ) );
    }
    else if( aEvent.HasPosition() )
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

        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = symbol && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || evt->IsAction( &ACTIONS::undo ) )
        {
            m_frame->GetInfoBar()->Dismiss();

            if( symbol )
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) || isSyntheticClick )
        {
            if( !symbol )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                // Pick the symbol to be placed
                bool footprintPreviews = m_frame->eeconfig()->m_Appearance.footprint_preview;
                PICKED_SYMBOL sel = m_frame->PickSymbolFromLibTree( &filter, *historyList, true,
                                                                    1, 1, footprintPreviews );

                LIB_SYMBOL* libSymbol = sel.LibId.IsValid() ? m_frame->GetLibSymbol( sel.LibId )
                                                            : nullptr;

                if( !libSymbol )
                    continue;

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

                symbol = new SCH_SYMBOL( *libSymbol, &m_frame->GetCurrentSheet(), sel, cursorPos,
                                         &m_frame->Schematic() );
                addSymbol( symbol );
                annotate();

                // Update the list of references for the next symbol placement.
                SCH_REFERENCE placedSymbolReference( symbol, symbol->GetLibSymbolRef().get(),
                                                     m_frame->GetCurrentSheet() );
                existingRefs.AddItem( placedSymbolReference );
                existingRefs.SortByReferenceOnly();

                if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                    symbol->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

                // Update cursor now that we have a symbol
                setCursor();
            }
            else
            {
                m_view->ClearPreview();
                m_frame->AddToScreen( symbol, screen );

                if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                    symbol->AutoplaceFields( screen, false /* aManual */ );

                SCH_COMMIT commit( m_toolMgr );
                commit.Added( symbol, screen );

                SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
                lwbTool->TrimOverLappingWires( &commit, &m_selectionTool->GetSelection() );
                lwbTool->AddJunctionsIfNeeded( &commit, &m_selectionTool->GetSelection() );

                commit.Push( _( "Add Symbol" ) );

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

                        // Start new annotation sequence at first unit
                        if( new_unit == 1 )
                            nextSymbol->ClearAnnotation( nullptr, false );

                        addSymbol( nextSymbol );
                        symbol = nextSymbol;
                        annotate();

                        // Update the list of references for the next symbol placement.
                        SCH_REFERENCE placedSymbolReference( symbol,
                                                             symbol->GetLibSymbolRef().get(),
                                                             m_frame->GetCurrentSheet() );
                        existingRefs.AddItem( placedSymbolReference );
                        existingRefs.SortByReferenceOnly();
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
            if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_UNIT_SYM_MAX )
            {
                int unit = *evt->GetCommandId() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( symbol )
                {
                    m_frame->SelectUnit( symbol, unit );
                    m_toolMgr->RunAction( ACTIONS::refreshPreview );
                }
            }
        }
        else if( evt->IsAction( &ACTIONS::duplicate ) )
        {
            if( symbol )
            {
                // This doesn't really make sense; we'll just end up dragging a stack of
                // objects so we ignore the duplicate and just carry on.
                wxBell();
                continue;
            }

            // Exit.  The duplicate will run in its own loop.
            m_frame->PopTool( aEvent );
            break;
        }
        else if( symbol && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            symbol->SetPosition( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( symbol, false );   // Add, but not give ownership
            m_frame->SetMsgPanel( symbol );
        }
        else if( symbol && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
        }
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
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

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP*      image = aEvent.Parameter<SCH_BITMAP*>();
    bool             immediateMode = image != nullptr;
    EE_GRID_HELPER   grid( m_toolMgr );
    bool             ignorePrimePosition = false;
    COMMON_SETTINGS* common_settings = Pgm().GetCommonSettings();

    if( m_inPlaceImage )
        return 0;

    REENTRANCY_GUARD guard( &m_inPlaceImage );

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    // Add all the drawable symbols to preview
    if( image )
    {
        image->SetPosition( getViewControls()->GetCursorPosition() );
        m_view->ClearPreview();
        m_view->AddToPreview( image, false );   // Add, but not give ownership
    }

    m_frame->PushTool( aEvent );

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
                m_view->RecacheAllItems();
                delete image;
                image = nullptr;
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    // Prime the pump
    if( image )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else if( aEvent.HasPosition() )
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
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->DisableGridSnapping() );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = image && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || evt->IsAction( &ACTIONS::undo ) )
        {
            m_frame->GetInfoBar()->Dismiss();

            if( image )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }

            if( immediateMode )
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() && !isSyntheticClick )
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) || isSyntheticClick )
        {
            if( !image )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                wxFileDialog dlg( m_frame, _( "Choose Image" ), m_mruPath, wxEmptyString,
                                  _( "Image Files" ) + wxS( " " ) + wxImage::GetImageExtWildcard(),
                                  wxFD_OPEN );

                if( dlg.ShowModal() != wxID_OK )
                    continue;

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

                wxString fullFilename = dlg.GetPath();
                m_mruPath = wxPathOnly( fullFilename );

                if( wxFileExists( fullFilename ) )
                    image = new SCH_BITMAP( cursorPos );

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
                m_view->AddToPreview( image, false );   // Add, but not give ownership
                m_view->RecacheAllItems();              // Bitmaps are cached in Opengl

                m_selectionTool->AddItemToSel( image );

                getViewControls()->SetCursorPosition( cursorPos, false );
                setCursor();
            }
            else
            {
                SCH_COMMIT commit( m_toolMgr );
                commit.Add( image, m_frame->GetScreen() );
                commit.Push( _( "Add Image" ) );

                image = nullptr;
                m_toolMgr->RunAction( ACTIONS::activatePointEditor );

                m_view->ClearPreview();

                if( immediateMode )
                {
                    m_frame->PopTool( aEvent );
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
            image->SetPosition( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( image, false );   // Add, but not give ownership
            m_view->RecacheAllItems();              // Bitmaps are cached in Opengl
            m_frame->SetMsgPanel( image );
        }
        else if( image && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
        }
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
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

    return 0;
}


int SCH_DRAWING_TOOLS::SingleClickPlace( const TOOL_EVENT& aEvent )
{
    VECTOR2I              cursorPos;
    KICAD_T               type = aEvent.Parameter<KICAD_T>();
    EE_GRID_HELPER        grid( m_toolMgr );
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    SCH_ITEM*             previewItem;
    bool                  loggedInfoBarError = false;
    wxString              description;
    SCH_SCREEN*           screen = m_frame->GetScreen();

    if( m_inSingleClickPlace )
        return 0;

    REENTRANCY_GUARD guard( &m_inSingleClickPlace );

    if( type == SCH_JUNCTION_T && aEvent.HasPosition() )
    {
        EE_SELECTION& selection = m_selectionTool->GetSelection();
        SCH_LINE*     wire = dynamic_cast<SCH_LINE*>( selection.Front() );

        if( wire )
        {
            SEG seg( wire->GetStartPoint(), wire->GetEndPoint() );
            VECTOR2I nearest = seg.NearestPoint( getViewControls()->GetCursorPosition() );
            getViewControls()->SetCrossHairCursorPosition( nearest, false );
            getViewControls()->WarpMouseCursor( getViewControls()->GetCursorPosition(), true );
        }
    }

    switch( type )
    {
    case SCH_NO_CONNECT_T:
        previewItem = new SCH_NO_CONNECT( cursorPos );
        previewItem->SetParent( screen );
        description = _( "Add No Connect Flag" );
        break;

    case SCH_JUNCTION_T:
        previewItem = new SCH_JUNCTION( cursorPos );
        previewItem->SetParent( screen );
        description = _( "Add Junction" );
        break;

    case SCH_BUS_WIRE_ENTRY_T:
        previewItem = new SCH_BUS_WIRE_ENTRY( cursorPos );
        previewItem->SetParent( screen );
        description = _( "Add Wire to Bus Entry" );
        break;

    default:
        wxASSERT_MSG( false, "Unknown item type in SCH_DRAWING_TOOLS::SingleClickPlace" );
        return 0;
    }

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    cursorPos = aEvent.HasPosition() ? aEvent.Position() : controls->GetMousePosition();

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PLACE );
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    m_view->ClearPreview();
    m_view->AddToPreview( previewItem->Clone() );

    // Prime the pump
    if( aEvent.HasPosition() && type != SCH_SHEET_PIN_T )
        m_toolMgr->PrimeTool( aEvent.Position() );
    else
        m_toolMgr->RunAction( ACTIONS::refreshPreview );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = evt->IsPrime() ? evt->Position() : controls->GetMousePosition();
        cursorPos = grid.BestSnapAnchor( cursorPos, LAYER_CONNECTABLE, nullptr );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || evt->IsAction( &ACTIONS::undo ) )
        {
            m_frame->PopTool( aEvent );
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !screen->GetItem( cursorPos, 0, type ) )
            {
                if( type == SCH_JUNCTION_T )
                {
                    if( !screen->IsExplicitJunctionAllowed( cursorPos ) )
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
                const_cast<KIID&>( newItem->m_Uuid ) = KIID();
                newItem->SetPosition( cursorPos );
                newItem->SetFlags( IS_NEW );

                SCH_COMMIT commit( m_toolMgr );
                commit.Add( newItem, screen );

                m_frame->SchematicCleanUp( &commit );

                commit.Push( description );
            }

            if( evt->IsDblClick( BUT_LEFT ) || type == SCH_SHEET_PIN_T )  // Finish tool.
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() )
        {
            previewItem->SetPosition( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( previewItem->Clone() );
            m_frame->SetMsgPanel( previewItem );
        }
        else if( evt->Category() == TC_COMMAND )
        {
            if( ( type == SCH_BUS_WIRE_ENTRY_T ) && (   evt->IsAction( &EE_ACTIONS::rotateCW )
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
                    busItem->MirrorVertically( busItem->GetPosition().y );
                }
                else if( evt->IsAction( &EE_ACTIONS::mirrorH ) )
                {
                    busItem->MirrorHorizontally( busItem->GetPosition().x );
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

                    DIALOG_WIRE_BUS_PROPERTIES dlg( m_frame, strokeItems );
                }
                    break;

                case SCH_JUNCTION_T:
                {
                    std::deque<SCH_JUNCTION*> junctions;
                    junctions.push_back( static_cast<SCH_JUNCTION*>( previewItem ) );

                    DIALOG_JUNCTION_PROPS dlg( m_frame, junctions );
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
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
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

    return 0;
}


SCH_TEXT* SCH_DRAWING_TOOLS::createNewText( const VECTOR2I& aPosition, int aType )
{
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& settings = schematic->Settings();
    SCH_TEXT*           textItem = nullptr;
    SCH_LABEL_BASE*     labelItem = nullptr;

    switch( aType )
    {
    case LAYER_NOTES:
        textItem = new SCH_TEXT( aPosition );
        break;

    case LAYER_LOCLABEL:
        labelItem = new SCH_LABEL( aPosition );
        textItem = labelItem;
        break;

    case LAYER_NETCLASS_REFS:
        labelItem = new SCH_DIRECTIVE_LABEL( aPosition );
        labelItem->SetShape( m_lastNetClassFlagShape );
        labelItem->GetFields().emplace_back( SCH_FIELD( {0,0}, 0, labelItem, wxT( "Netclass" ) ) );
        labelItem->GetFields().back().SetItalic( true );
        labelItem->GetFields().back().SetVisible( true );
        textItem = labelItem;
        break;

    case LAYER_HIERLABEL:
        labelItem = new SCH_HIERLABEL( aPosition );
        labelItem->SetShape( m_lastGlobalLabelShape );
        labelItem->SetAutoRotateOnPlacement( m_lastAutoLabelRotateOnPlacement );
        textItem = labelItem;
        break;

    case LAYER_GLOBLABEL:
        labelItem = new SCH_GLOBALLABEL( aPosition );
        labelItem->SetShape( m_lastGlobalLabelShape );
        labelItem->GetFields()[0].SetVisible( settings.m_IntersheetRefsShow );
        labelItem->SetAutoRotateOnPlacement( m_lastAutoLabelRotateOnPlacement );
        textItem = labelItem;
        break;

    default:
        wxFAIL_MSG( "SCH_EDIT_FRAME::CreateNewText() unknown layer type" );
        return nullptr;
    }

    textItem->SetParent( schematic );

    if( aType != LAYER_NETCLASS_REFS )
    {
        textItem->SetBold( m_lastTextBold );
        textItem->SetItalic( m_lastTextItalic );
    }

    textItem->SetTextSpinStyle( m_lastTextOrientation );
    textItem->SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
    textItem->SetFlags( IS_NEW | IS_MOVING );

    if( !labelItem )
    {
        DIALOG_TEXT_PROPERTIES dlg( m_frame, textItem );

        // QuasiModal required for syntax help and Scintilla auto-complete
        if( dlg.ShowQuasiModal() != wxID_OK )
        {
            delete textItem;
            return nullptr;
        }
    }
    else
    {
        DIALOG_LABEL_PROPERTIES dlg( m_frame, static_cast<SCH_LABEL_BASE*>( textItem ) );

        // Must be quasi modal for syntax help
        if( dlg.ShowQuasiModal() != wxID_OK )
        {
            delete labelItem;
            return nullptr;
        }
    }

    wxString text = textItem->GetText();

    if( textItem->Type() != SCH_DIRECTIVE_LABEL_T && NoPrintableChars( text ) )
    {
        delete textItem;
        return nullptr;
    }

    if( aType != LAYER_NETCLASS_REFS )
    {
        m_lastTextBold = textItem->IsBold();
        m_lastTextItalic = textItem->IsItalic();
    }

    m_lastTextOrientation = textItem->GetTextSpinStyle();

    if( aType == LAYER_GLOBLABEL || aType == LAYER_HIERLABEL )
    {
        m_lastGlobalLabelShape = labelItem->GetShape();
        m_lastAutoLabelRotateOnPlacement = labelItem->AutoRotateOnPlacement();
    }
    else if( aType == LAYER_NETCLASS_REFS )
    {
        m_lastNetClassFlagShape = labelItem->GetShape();
    }

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


SCH_SHEET_PIN* SCH_DRAWING_TOOLS::createSheetPin( SCH_SHEET* aSheet, SCH_HIERLABEL* aLabel,
                                                  const VECTOR2I& aPosition )
{
    SCHEMATIC_SETTINGS& settings = aSheet->Schematic()->Settings();
    wxString            text;
    SCH_SHEET_PIN*      sheetPin;

    if( aLabel )
    {
        text = aLabel->GetText();
        m_lastSheetPinType = aLabel->GetShape();
    }

    sheetPin = new SCH_SHEET_PIN( aSheet, VECTOR2I( 0, 0 ), text );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
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

    sheetPin->SetPosition( aPosition );

    return sheetPin;
}


int SCH_DRAWING_TOOLS::TwoClickPlace( const TOOL_EVENT& aEvent )
{
    SCH_ITEM*             item = nullptr;
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    bool                  ignorePrimePosition = false;
    COMMON_SETTINGS*      common_settings = Pgm().GetCommonSettings();
    SCH_SHEET*            sheet = nullptr;
    wxString              description;

    if( m_inTwoClickPlace )
        return 0;

    REENTRANCY_GUARD guard( &m_inTwoClickPlace );

    bool isText        = aEvent.IsAction( &EE_ACTIONS::placeSchematicText );
    bool isGlobalLabel = aEvent.IsAction( &EE_ACTIONS::placeGlobalLabel );
    bool isHierLabel   = aEvent.IsAction( &EE_ACTIONS::placeHierLabel );
    bool isClassLabel  = aEvent.IsAction( &EE_ACTIONS::placeClassLabel );
    bool isNetLabel    = aEvent.IsAction( &EE_ACTIONS::placeLabel );
    bool isSheetPin    = aEvent.IsAction( &EE_ACTIONS::importSheetPin );
    int  snapLayer     = isText ? LAYER_GRAPHICS : LAYER_CONNECTABLE;

    // If we have a selected sheet use it, otherwise try to get one under the cursor
    if( isSheetPin )
        sheet = dynamic_cast<SCH_SHEET*>( m_selectionTool->GetSelection().Front() );

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    m_frame->PushTool( aEvent );

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
                else if( isClassLabel )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LABEL_NET );    // JEY TODO: netclass directive cursor
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
                m_frame->SetMsgPanel( item );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
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
    else if( common_settings->m_Input.immediate_actions && !aEvent.IsReactivate()
                && ( isText || isGlobalLabel || isHierLabel || isClassLabel || isNetLabel ) )
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

        VECTOR2I cursorPos = controls->GetMousePosition();
        cursorPos = grid.BestSnapAnchor( cursorPos, snapLayer, item );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = item && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        auto createNextSheetPin =
                [&]()
                {
                    if( !sheet )
                        return;

                    SCH_HIERLABEL* label = importHierLabel( sheet );

                    if( !label )
                    {
                        m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( m_frame );
                        m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
                        m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                        m_statusPopup->PopupFor( 2000 );
                        item = nullptr;
                    }
                    else
                    {
                        item = createSheetPin( sheet, label, cursorPos );

                        if( item->Type() == SCH_SHEET_PIN_T )
                        {
                            item->ClearSelected();
                        }
                    }
                };

        if( evt->IsCancelInteractive() || evt->IsAction( &ACTIONS::undo ) )
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
                // leave ourselves on the stack so we come back after the move
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
            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                if( isText )
                {
                    item = createNewText( cursorPos, LAYER_NOTES );
                    description = _( "Add Text" );
                }
                else if( isHierLabel )
                {
                    item = createNewText( cursorPos, LAYER_HIERLABEL );
                    description = _( "Add Hierarchical Label" );
                }
                else if( isNetLabel || isGlobalLabel )
                {
                    wxString netName;

                    for( SCH_ITEM* overlapItem :
                         m_frame->GetScreen()->Items().Overlapping( cursorPos ) )
                    {
                        if( overlapItem->GetEditFlags() & STRUCT_DELETED )
                            continue;

                        if( overlapItem->Type() == SCH_LINE_T )
                        {
                            SCH_LINE* line = static_cast<SCH_LINE*>( overlapItem );
                            if( line->IsWire() )
                            {
                                netName = line->GetNetname(m_frame->GetCurrentSheet());
                                break;
                            }
                        }
                    }

                    if( netName.IsEmpty() )
                    {
                        // no connected net label found -> open up the new label dialog
                        if( isGlobalLabel )
                            item = createNewText( cursorPos, LAYER_GLOBLABEL );
                        else
                            item = createNewText( cursorPos, LAYER_LOCLABEL );
                    }
                    else
                    {
                        // connected net label found -> create the label immediately
                        SCHEMATIC*          schematic = getModel<SCHEMATIC>();
                        SCHEMATIC_SETTINGS& sch_settings = schematic->Settings();
                        SCH_LABEL_BASE*     labelItem = nullptr;

                        if( isGlobalLabel )
                        {
                            labelItem = new SCH_GLOBALLABEL( cursorPos );
                            labelItem->SetShape( m_lastGlobalLabelShape );
                            // make intersheets reference visible based on settings
                            labelItem->GetFields()[0].SetVisible( sch_settings.m_IntersheetRefsShow );
                        }
                        else
                        {
                            labelItem = new SCH_LABEL( cursorPos );
                        }

                        labelItem->SetParent( getModel<SCHEMATIC>() );
                        labelItem->SetBold( m_lastTextBold );
                        labelItem->SetItalic( m_lastTextItalic );
                        labelItem->SetTextSpinStyle( m_lastTextOrientation );
                        labelItem->SetTextSize( VECTOR2I( sch_settings.m_DefaultTextSize,
                                                          sch_settings.m_DefaultTextSize ) );
                        labelItem->SetFlags( IS_NEW | IS_MOVING );
                        labelItem->SetText( netName );
                        item = labelItem;
                    }

                    description = _( "Add Label" );
                }
                else if( isClassLabel )
                {
                    item = createNewText( cursorPos, LAYER_NETCLASS_REFS );
                    description = _( "Add Label" );
                }
                else if( isSheetPin )
                {
                    EDA_ITEM* i = nullptr;

                    // If we didn't have a sheet selected, try to find one under the cursor
                    if( !sheet && m_selectionTool->SelectPoint( cursorPos, { SCH_SHEET_T }, &i ) )
                        sheet = dynamic_cast<SCH_SHEET*>( i );

                    if( !sheet )
                    {
                        m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( m_frame );
                        m_statusPopup->SetText( _( "Click over a sheet." ) );
                        m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                        m_statusPopup->PopupFor( 2000 );
                        item = nullptr;
                    }
                    else
                    {
                        createNextSheetPin();

                        if( !item )
                        {
                            m_frame->PopTool( aEvent );
                            break;
                        }
                    }

                    description = _( "Add Sheet Pin" );
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
                    cursorPos = grid.BestSnapAnchor( cursorPos, snapLayer, item );
                }

                if( item )
                {
                    item->SetPosition( cursorPos );

                    item->SetFlags( IS_NEW | IS_MOVING );
                    item->AutoplaceFields( nullptr, false /* aManual */ );
                    updatePreview();

                    if( item->Type() != SCH_SHEET_PIN_T )
                        m_selectionTool->AddItemToSel( item );

                    m_toolMgr->RunAction( ACTIONS::refreshPreview );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }

                controls->SetCursorPosition( cursorPos, false );
            }
            else            // ... and second click places:
            {
                item->ClearFlags( IS_MOVING );

                if( item->IsConnectable() )
                    m_frame->AutoRotateItem( m_frame->GetScreen(), item );

                m_frame->AddToScreen( item, m_frame->GetScreen() );
                item->AutoplaceFields( m_frame->GetScreen(), false /* aManual */ );

                SCH_COMMIT commit( m_toolMgr );
                commit.Added( item, m_frame->GetScreen() );
                commit.Push( description );

                item = nullptr;
                m_view->ClearPreview();

                // Exit the tool when this sheet runs out of pins for convenience
                if( isSheetPin )
                {
                    createNextSheetPin();

                    if( !item )
                    {
                        m_frame->PopTool( aEvent );
                        break;
                    }
                }
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
            item->SetPosition( cursorPos );
            item->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );
            updatePreview();
        }
        else if( item && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
        }
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
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


int SCH_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& sch_settings = schematic->Settings();
    SCH_SHAPE*          item = nullptr;
    bool                isTextBox = aEvent.IsAction( &EE_ACTIONS::drawTextBox );
    SHAPE_T             type = aEvent.Parameter<SHAPE_T>();
    wxString            description;

    if( m_inDrawShape )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawShape );

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    m_frame->PushTool( aEvent );

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

        if( evt->IsCancelInteractive() || evt->IsAction( &ACTIONS::undo ) )
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
            if( item && evt->IsMoveTool() )
            {
                // we're already drawing our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

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
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            if( isTextBox )
            {
                SCH_TEXTBOX* textbox = new SCH_TEXTBOX( 0, m_lastTextboxFillStyle );

                textbox->SetBold( m_lastTextBold );
                textbox->SetItalic( m_lastTextItalic );
                textbox->SetTextSize( VECTOR2I( sch_settings.m_DefaultTextSize,
                                                sch_settings.m_DefaultTextSize ) );
                textbox->SetTextAngle( m_lastTextAngle );
                textbox->SetHorizJustify( m_lastTextJust );
                textbox->SetStroke( m_lastTextboxStroke );
                textbox->SetFillColor( m_lastTextboxFillColor );
                textbox->SetParent( schematic );

                item = textbox;
                description = _( "Add Text Box" );
            }
            else
            {
                item = new SCH_SHAPE( type, 0, m_lastFillStyle );

                item->SetStroke( m_lastStroke );
                item->SetFillColor( m_lastFillColor );
                item->SetParent( schematic );
                description = wxString::Format( _( "Add %s" ), item->EDA_SHAPE::GetFriendlyName() );
            }

            item->SetFlags( IS_NEW );
            item->BeginEdit( cursorPos );

            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }
        else if( item && ( evt->IsClick( BUT_LEFT )
                        || evt->IsDblClick( BUT_LEFT )
                        || isSyntheticClick
                        || evt->IsAction( &EE_ACTIONS::finishDrawing ) ) )
        {
            if( evt->IsDblClick( BUT_LEFT )
                    || evt->IsAction( &EE_ACTIONS::finishDrawing )
                    || !item->ContinueEdit( cursorPos ) )
            {
                item->EndEdit();
                item->ClearEditFlags();
                item->SetFlags( IS_NEW );

                if( isTextBox )
                {
                    SCH_TEXTBOX*           textbox = static_cast<SCH_TEXTBOX*>( item );
                    DIALOG_TEXT_PROPERTIES dlg( m_frame, textbox );

                    getViewControls()->SetAutoPan( false );
                    getViewControls()->CaptureCursor( false );

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
                    m_lastTextboxStroke = textbox->GetStroke();
                    m_lastTextboxFillStyle = textbox->GetFillMode();
                    m_lastTextboxFillColor = textbox->GetFillColor();
                }
                else
                {
                    m_lastStroke = item->GetStroke();
                    m_lastFillStyle = item->GetFillMode();
                    m_lastFillColor = item->GetFillColor();
                }

                SCH_COMMIT commit( m_toolMgr );
                commit.Add( item, m_frame->GetScreen() );
                commit.Push( wxString::Format( _( "Draw %s" ), item->GetClass() ) );

                m_selectionTool->AddItemToSel( item );
                item = nullptr;

                m_view->ClearPreview();
                m_toolMgr->RunAction( ACTIONS::activatePointEditor );
            }
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->CalcEdit( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
            m_frame->SetMsgPanel( item );
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
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
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


int SCH_DRAWING_TOOLS::DrawSheet( const TOOL_EVENT& aEvent )
{
    SCH_SHEET* sheet = nullptr;

    if( m_inDrawSheet )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawSheet );

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    m_frame->PushTool( aEvent );

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
        bool isSyntheticClick = sheet && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || evt->IsAction( &ACTIONS::undo ) )
        {
            m_frame->GetInfoBar()->Dismiss();

            if( sheet )
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( !sheet && ( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) ) )
        {
            EE_SELECTION&      selection = m_selectionTool->GetSelection();
            EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();

            if( selection.Size() == 1
                    && selection.Front()->Type() == SCH_SHEET_T
                    && selection.Front()->GetBoundingBox().Contains( cursorPos ) )
            {
                if( evt->IsClick( BUT_LEFT ) )
                {
                    // sheet already selected
                    continue;
                }
                else if( evt->IsDblClick( BUT_LEFT ) )
                {
                    m_toolMgr->RunAction( EE_ACTIONS::enterSheet, false );
                    break;
                }
            }

            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            sheet = new SCH_SHEET( m_frame->GetCurrentSheet().Last(), cursorPos );
            sheet->SetFlags( IS_NEW | IS_MOVING );
            sheet->SetScreen( nullptr );
            sheet->SetBorderWidth( schIUScale.MilsToIU( cfg->m_Drawing.default_line_thickness ) );
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
                         || isSyntheticClick
                         || evt->IsAction( &EE_ACTIONS::finishSheet ) ) )
        {
            getViewControls()->SetAutoPan( false );
            getViewControls()->CaptureCursor( false );

            if( m_frame->EditSheetProperties( static_cast<SCH_SHEET*>( sheet ),
                                              &m_frame->GetCurrentSheet(), nullptr ) )
            {
                m_view->ClearPreview();

                sheet->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

                SCH_COMMIT commit( m_toolMgr );
                commit.Add( sheet, m_frame->GetScreen() );
                commit.Push( "Draw Sheet" );

                m_frame->UpdateHierarchyNavigator();
                m_selectionTool->AddItemToSel( sheet );
            }
            else
            {
                m_view->ClearPreview();
                delete sheet;
            }

            sheet = nullptr;
        }
        else if( sheet && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            sizeSheet( sheet, cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
            m_frame->SetMsgPanel( sheet );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !sheet )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
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

    return 0;
}


void SCH_DRAWING_TOOLS::sizeSheet( SCH_SHEET* aSheet, const VECTOR2I& aPos )
{
    VECTOR2I pos = aSheet->GetPosition();
    VECTOR2I size = aPos - pos;

    size.x = std::max( size.x, schIUScale.MilsToIU( MIN_SHEET_WIDTH ) );
    size.y = std::max( size.y, schIUScale.MilsToIU( MIN_SHEET_HEIGHT ) );

    VECTOR2I grid = m_frame->GetNearestGridPosition( pos + size );
    aSheet->Resize( VECTOR2I( grid.x - pos.x, grid.y - pos.y ) );
}


void SCH_DRAWING_TOOLS::setTransitions()
{
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,         EE_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,         EE_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeClassLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeHierLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,           EE_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::importSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,           EE_ACTIONS::drawRectangle.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,           EE_ACTIONS::drawCircle.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,           EE_ACTIONS::drawArc.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,           EE_ACTIONS::drawTextBox.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceImage,          EE_ACTIONS::placeImage.MakeEvent() );
}
