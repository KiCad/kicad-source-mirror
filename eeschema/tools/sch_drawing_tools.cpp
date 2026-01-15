/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
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

#include "sch_sheet_path.h"
#include <memory>
#include <set>

#include <kiplatform/ui.h>
#include <optional>
#include <project_sch.h>
#include <tools/sch_drawing_tools.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_selection_tool.h>
#include <tools/ee_grid_helper.h>
#include <tools/rule_area_create_helper.h>
#include <gal/graphics_abstraction_layer.h>
#include <sch_actions.h>
#include <sch_tool_utils.h>
#include <sch_edit_frame.h>
#include <pgm_base.h>
#include <design_block.h>
#include <widgets/sch_design_block_pane.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <sch_symbol.h>
#include <sch_no_connect.h>
#include <sch_group.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_label.h>
#include <sch_bitmap.h>
#include <schematic.h>
#include <sch_commit.h>
#include <scoped_set_reset.h>
#include <libraries/legacy_symbol_library.h>
#include <eeschema_settings.h>
#include <dialogs/dialog_label_properties.h>
#include <dialogs/dialog_text_properties.h>
#include <dialogs/dialog_wire_bus_properties.h>
#include <dialogs/dialog_junction_props.h>
#include <dialogs/dialog_table_properties.h>
#include <import_gfx/dialog_import_gfx_sch.h>
#include <sync_sheet_pin/sheet_synchronization_agent.h>
#include <string_utils.h>
//#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

SCH_DRAWING_TOOLS::SCH_DRAWING_TOOLS() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawing" ),
        m_lastSheetPinType( LABEL_FLAG_SHAPE::L_INPUT ),
        m_lastGlobalLabelShape( LABEL_FLAG_SHAPE::L_INPUT ),
        m_lastNetClassFlagShape( LABEL_FLAG_SHAPE::F_ROUND ),
        m_lastTextOrientation( SPIN_STYLE::RIGHT ),
        m_lastTextBold( false ),
        m_lastTextItalic( false ),
        m_lastTextAngle( ANGLE_0 ),
        m_lastTextboxAngle( ANGLE_0 ),
        m_lastTextHJustify( GR_TEXT_H_ALIGN_CENTER ),
        m_lastTextVJustify( GR_TEXT_V_ALIGN_CENTER ),
        m_lastTextboxHJustify( GR_TEXT_H_ALIGN_LEFT ),
        m_lastTextboxVJustify( GR_TEXT_V_ALIGN_TOP ),
        m_lastFillStyle( FILL_T::NO_FILL ),
        m_lastTextboxFillStyle( FILL_T::NO_FILL ),
        m_lastFillColor( COLOR4D::UNSPECIFIED ),
        m_lastTextboxFillColor( COLOR4D::UNSPECIFIED ),
        m_lastStroke( 0, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_lastTextboxStroke( 0, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_mruPath( wxEmptyString ),
        m_lastAutoLabelRotateOnPlacement( false ),
        m_drawingRuleArea( false ),
        m_inDrawingTool( false )
{
}


bool SCH_DRAWING_TOOLS::Init()
{
    SCH_TOOL_BASE::Init();

    auto belowRootSheetCondition =
            [this]( const SELECTION& aSel )
            {
                return m_frame->GetCurrentSheet().Last() != &m_frame->Schematic().Root();
            };

    auto inDrawingRuleArea =
            [this]( const SELECTION& aSel )
            {
                return m_drawingRuleArea;
            };

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();
    ctxMenu.AddItem( SCH_ACTIONS::leaveSheet,      belowRootSheetCondition, 150 );
    ctxMenu.AddItem( SCH_ACTIONS::closeOutline,    inDrawingRuleArea,       200 );
    ctxMenu.AddItem( SCH_ACTIONS::deleteLastPoint, inDrawingRuleArea,       200 );

    return true;
}


int SCH_DRAWING_TOOLS::PlaceSymbol( const TOOL_EVENT& aEvent )
{
    const SCH_ACTIONS::PLACE_SYMBOL_PARAMS& toolParams = aEvent.Parameter<SCH_ACTIONS::PLACE_SYMBOL_PARAMS>();

    SCH_SYMBOL* symbol = toolParams.m_Symbol;

    // If we get a parameterised symbol, we probably just want to place that and get out of the placmeent tool,
    // rather than popping up the chooser afterwards
    bool placeOneOnly = symbol != nullptr;

    SYMBOL_LIBRARY_FILTER       filter;
    std::vector<PICKED_SYMBOL>* historyList = nullptr;
    bool                        ignorePrimePosition = false;
    COMMON_SETTINGS*            common_settings = Pgm().GetCommonSettings();
    SCHEMATIC_SETTINGS&         schSettings = m_frame->Schematic().Settings();
    SCH_SCREEN*                 screen = m_frame->GetScreen();
    bool                        keepSymbol = false;
    bool                        placeAllUnits = false;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

    // First we need to get all instances of this sheet so we can annotate whatever symbols we place on all copies
    SCH_SHEET_LIST hierarchy = m_frame->Schematic().Hierarchy();
    SCH_SHEET_LIST newInstances = hierarchy.FindAllSheetsForScreen( m_frame->GetCurrentSheet().LastScreen() );
    newInstances.SortByPageNumbers();

    // Get a list of all references in the schematic to avoid duplicates wherever they're placed
    SCH_REFERENCE_LIST existingRefs;
    hierarchy.GetSymbols( existingRefs );
    existingRefs.SortByReferenceOnly();

    if( aEvent.IsAction( &SCH_ACTIONS::placeSymbol ) )
    {
        historyList = &m_symbolHistoryList;
    }
    else if (aEvent.IsAction( &SCH_ACTIONS::placePower ) )
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
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_selectionTool->AddItemToSel( aSymbol );

                aSymbol->SetFlags( IS_NEW | IS_MOVING );

                m_view->ClearPreview();
                m_view->AddToPreview( aSymbol, false );   // Add, but not give ownership

                // Set IS_MOVING again, as AddItemToCommitAndScreen() will have cleared it.
                aSymbol->SetFlags( IS_MOVING );
                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            };

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( symbol ? KICURSOR::MOVING : KICURSOR::COMPONENT );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
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
                    SCH_REFERENCE newReference( symbol, instance );
                    SCH_REFERENCE_LIST refs;
                    refs.AddItem( newReference );
                    refs.SetRefDesTracker( schSettings.m_refDesTracker );

                    if( cfg->m_AnnotatePanel.automatic || newReference.AlwaysAnnotate() )
                    {
                        refs.ReannotateByOptions( (ANNOTATE_ORDER_T) schSettings.m_AnnotateSortOrder,
                                                  (ANNOTATE_ALGO_T) schSettings.m_AnnotateMethod,
                                                  schSettings.m_AnnotateStartNum, existingRefs, false,
                                                  &hierarchy );

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

        if( toolParams.m_Reannotate )
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

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_CONNECTABLE );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = symbol && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( symbol && evt->IsAction( &ACTIONS::undo ) ) )
        {
            m_frame->GetInfoBar()->Dismiss();

            if( symbol )
            {
                cleanup();

                if( keepSymbol )
                {
                    // Re-enter symbol chooser
                    m_toolMgr->PostAction( ACTIONS::cursorClick );
                }
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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || isSyntheticClick
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
            if( !symbol )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                SYMBOL_LIBRARY_ADAPTER* libs = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );
                LEGACY_SYMBOL_LIB*       cache = PROJECT_SCH::LegacySchLibs( &m_frame->Prj() )->GetCacheLibrary();

                std::set<UTF8>             unique_libid;
                std::vector<PICKED_SYMBOL> alreadyPlaced;

                for( SCH_SHEET_PATH& sheet : hierarchy )
                {
                    for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                    {
                        SCH_SYMBOL* s = static_cast<SCH_SYMBOL*>( item );

                        if( !unique_libid.insert( s->GetLibId().Format() ).second )
                            continue;

                        LIB_SYMBOL* libSymbol = SchGetLibSymbol( s->GetLibId(), libs, cache );

                        if( libSymbol )
                        {
                            PICKED_SYMBOL pickedSymbol;
                            pickedSymbol.LibId = libSymbol->GetLibId();
                            alreadyPlaced.push_back( pickedSymbol );
                        }
                    }
                }

                // Pick the symbol to be placed
                bool footprintPreviews = m_frame->eeconfig()->m_Appearance.footprint_preview;
                PICKED_SYMBOL sel = m_frame->PickSymbolFromLibrary( &filter, *historyList, alreadyPlaced,
                                                                    footprintPreviews );

                keepSymbol = sel.KeepSymbol;
                placeAllUnits = sel.PlaceAllUnits;

                LIB_SYMBOL* libSymbol = sel.LibId.IsValid() ? m_frame->GetLibSymbol( sel.LibId ) : nullptr;

                if( !libSymbol )
                    continue;

                // If we started with a hotkey which has a position then warp back to that.
                // Otherwise update to the current mouse position pinned inside the autoscroll
                // boundaries.
                if( evt->IsPrime() && !ignorePrimePosition )
                {
                    cursorPos = grid.Align( evt->Position(), GRID_HELPER_GRIDS::GRID_CONNECTABLE );
                    getViewControls()->WarpMouseCursor( cursorPos, true );
                }
                else
                {
                    getViewControls()->PinCursorInsideNonAutoscrollArea( true );
                    cursorPos = grid.Align( getViewControls()->GetMousePosition(),
                                            GRID_HELPER_GRIDS::GRID_CONNECTABLE );
                }

                EESCHEMA_SETTINGS*    cfg = m_frame->eeconfig();

                if( !libSymbol->IsLocalPower() && cfg->m_Drawing.new_power_symbols == POWER_SYMBOLS::LOCAL )
                {
                    libSymbol->SetLocalPower();
                    wxString keywords = libSymbol->GetKeyWords();

                    // Adjust the KiCad library default fields to match the new power symbol type
                    if( keywords.Contains( wxT( "global power" ) ) )
                    {
                        keywords.Replace( wxT( "global power" ), wxT( "local power" ) );
                        libSymbol->SetKeyWords( keywords );
                    }

                    wxString desc = libSymbol->GetDescription();

                    if( desc.Contains( wxT( "global label" ) ) )
                    {
                        desc.Replace( wxT( "global label" ), wxT( "local label" ) );
                        libSymbol->SetDescription( desc );
                    }
                }
                else if( !libSymbol->IsGlobalPower()
                         && cfg->m_Drawing.new_power_symbols == POWER_SYMBOLS::GLOBAL )
                {
                    // We do not currently have local power symbols in the KiCad library, so
                    // don't update any fields
                    libSymbol->SetGlobalPower();
                }

                symbol = new SCH_SYMBOL( *libSymbol, &m_frame->GetCurrentSheet(), sel, cursorPos,
                                         &m_frame->Schematic() );
                addSymbol( symbol );
                annotate();

                // Update the list of references for the next symbol placement.
                SCH_REFERENCE placedSymbolReference( symbol, m_frame->GetCurrentSheet() );
                existingRefs.AddItem( placedSymbolReference );
                existingRefs.SortByReferenceOnly();

                if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                {
                    // Not placed yet, so pass a nullptr screen reference
                    symbol->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
                }

                // Update cursor now that we have a symbol
                setCursor();
            }
            else
            {
                m_view->ClearPreview();
                m_frame->AddToScreen( symbol, screen );

                if( m_frame->eeconfig()->m_AutoplaceFields.enable )
                    symbol->AutoplaceFields( screen, AUTOPLACE_AUTO );

                m_frame->SaveCopyForRepeatItem( symbol );

                SCH_COMMIT commit( m_toolMgr );
                commit.Added( symbol, screen );

                SCH_LINE_WIRE_BUS_TOOL* lwbTool = m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
                lwbTool->TrimOverLappingWires( &commit, &m_selectionTool->GetSelection() );
                lwbTool->AddJunctionsIfNeeded( &commit, &m_selectionTool->GetSelection() );

                commit.Push( _( "Place Symbol" ) );

                if( placeOneOnly )
                {
                    m_frame->PopTool( aEvent );
                    break;
                }

                SCH_SYMBOL* nextSymbol = nullptr;

                if( keepSymbol || placeAllUnits )
                {
                    SCH_REFERENCE currentReference( symbol, m_frame->GetCurrentSheet() );
                    SCHEMATIC& schematic = m_frame->Schematic();

                    if( placeAllUnits )
                    {
                        while( currentReference.GetUnit() <= symbol->GetUnitCount()
                               && schematic.Contains( currentReference ) )
                        {
                            currentReference.SetUnit( currentReference.GetUnit() + 1 );
                        }

                        if( currentReference.GetUnit() > symbol->GetUnitCount() )
                        {
                            currentReference.SetUnit( 1 );
                        }
                    }

                    // We are either stepping to the next unit or next symbol
                    if( keepSymbol || currentReference.GetUnit() > 1 )
                    {
                        nextSymbol = static_cast<SCH_SYMBOL*>( symbol->Duplicate( IGNORE_PARENT_GROUP ) );
                        nextSymbol->SetUnit( currentReference.GetUnit() );
                        nextSymbol->SetUnitSelection( currentReference.GetUnit() );

                        addSymbol( nextSymbol );
                        symbol = nextSymbol;

                        if( currentReference.GetUnit() == 1 )
                            annotate();

                        // Update the list of references for the next symbol placement.
                        SCH_REFERENCE placedSymbolReference( symbol, m_frame->GetCurrentSheet() );
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

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_UNIT
                && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_UNIT_END )
            {
                int unit = *evt->GetCommandId() - ID_POPUP_SCH_SELECT_UNIT;

                if( symbol )
                {
                    m_frame->SelectUnit( symbol, unit );
                    m_toolMgr->PostAction( ACTIONS::refreshPreview );
                }
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_BODY_STYLE
                     && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_BODY_STYLE_END )
            {
                int bodyStyle = ( *evt->GetCommandId() - ID_POPUP_SCH_SELECT_BODY_STYLE ) + 1;

                if( symbol && symbol->GetBodyStyle() != bodyStyle )
                {
                    m_frame->SelectBodyStyle( symbol, bodyStyle );
                    m_toolMgr->PostAction( ACTIONS::refreshPreview );
                }
            }
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                 || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                 || evt->IsAction( &ACTIONS::paste ) )
        {
            if( symbol )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
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
        else if( symbol && (   evt->IsAction( &ACTIONS::redo )
                            || evt->IsAction( &SCH_ACTIONS::editWithLibEdit )
                            || evt->IsAction( &SCH_ACTIONS::changeSymbol ) ) )
        {
            wxBell();
        }
        else if( symbol && (   evt->IsAction( &SCH_ACTIONS::properties )
                            || evt->IsAction( &SCH_ACTIONS::editReference )
                            || evt->IsAction( &SCH_ACTIONS::editValue )
                            || evt->IsAction( &SCH_ACTIONS::editFootprint )
                            || evt->IsAction( &SCH_ACTIONS::autoplaceFields )
                            || evt->IsAction( &SCH_ACTIONS::cycleBodyStyle )
                            || evt->IsAction( &SCH_ACTIONS::setExcludeFromBOM )
                            || evt->IsAction( &SCH_ACTIONS::setExcludeFromBoard )
                            || evt->IsAction( &SCH_ACTIONS::setExcludeFromSimulation )
                            || evt->IsAction( &SCH_ACTIONS::setDNP )
                            || evt->IsAction( &SCH_ACTIONS::rotateCW )
                            || evt->IsAction( &SCH_ACTIONS::rotateCCW )
                            || evt->IsAction( &SCH_ACTIONS::mirrorV )
                            || evt->IsAction( &SCH_ACTIONS::mirrorH ) ) )
        {
            m_toolMgr->PostAction( ACTIONS::refreshPreview );
            evt->SetPassEvent();
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


int SCH_DRAWING_TOOLS::PlaceNextSymbolUnit( const TOOL_EVENT& aEvent )
{
    const SCH_ACTIONS::PLACE_SYMBOL_UNIT_PARAMS& params =
            aEvent.Parameter<SCH_ACTIONS::PLACE_SYMBOL_UNIT_PARAMS>();
    SCH_SYMBOL* symbol = params.m_Symbol;
    int requestedUnit = params.m_Unit;

    // TODO: get from selection
    if( !symbol )
    {
        static const std::vector<KICAD_T> symbolTypes = { SCH_SYMBOL_T };
        SCH_SELECTION& selection = m_selectionTool->RequestSelection( symbolTypes );

        if( selection.Size() != 1 )
        {
            m_frame->ShowInfoBarMsg( _( "Select a single symbol to place the next unit." ) );
            return 0;
        }

        wxCHECK( selection.Front()->Type() == SCH_SYMBOL_T, 0 );
        symbol = static_cast<SCH_SYMBOL*>( selection.Front() );
    }

    if( !symbol )
        return 0;

    if( !symbol->IsMultiUnit() )
    {
        m_frame->ShowInfoBarMsg( _( "This symbol has only one unit." ) );
        return 0;
    }

    const std::set<int> missingUnits = GetUnplacedUnitsForSymbol( *symbol );

    if( missingUnits.empty() )
    {
        m_frame->ShowInfoBarMsg( _( "All units of this symbol are already placed." ) );
        return 0;
    }

    int nextMissing;

    if( requestedUnit > 0 )
    {
        if( missingUnits.count( requestedUnit ) == 0 )
        {
            m_frame->ShowInfoBarMsg( _( "Requested unit already placed." ) );
            return 0;
        }

        nextMissing = requestedUnit;
    }
    else
    {
        // Find the lowest unit number that is missing
        nextMissing = *std::min_element( missingUnits.begin(), missingUnits.end() );
    }

    std::unique_ptr<SCH_SYMBOL> newSymbol = std::make_unique<SCH_SYMBOL>( *symbol );
    const SCH_SHEET_PATH&       sheetPath = m_frame->GetCurrentSheet();

    // Use SetUnitSelection(int) to update ALL instance references at once.
    // This is important for shared sheets where the same screen is used by multiple
    // sheet instances - we want the new symbol unit to appear correctly on all instances.
    newSymbol->SetUnitSelection( nextMissing );
    newSymbol->SetUnit( nextMissing );
    newSymbol->SetRefProp( symbol->GetRef( &sheetPath, false ) );

    // Post the new symbol - don't reannotate it - we set the reference ourselves
    m_toolMgr->PostAction( SCH_ACTIONS::placeSymbol,
                           SCH_ACTIONS::PLACE_SYMBOL_PARAMS{ newSymbol.release(), false } );
    return 0;
}


int SCH_DRAWING_TOOLS::ImportSheet( const TOOL_EVENT& aEvent )
{
    COMMON_SETTINGS*            common_settings = Pgm().GetCommonSettings();
    EESCHEMA_SETTINGS*          cfg = m_frame->eeconfig();
    SCHEMATIC_SETTINGS&         schSettings = m_frame->Schematic().Settings();
    SCH_SCREEN*                 screen = m_frame->GetScreen();
    SCH_SHEET_PATH&             sheetPath = m_frame->GetCurrentSheet();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

    // Guard to reset forced cursor positioning on exit, regardless of error path
    struct RESET_FORCED_CURSOR_GUARD
    {
        KIGFX::VIEW_CONTROLS* m_controls;

        ~RESET_FORCED_CURSOR_GUARD() { m_controls->ForceCursorPosition( false ); }
    };

    RESET_FORCED_CURSOR_GUARD forcedCursorGuard{ controls };

    if( !cfg || !common_settings )
        return 0;

    if( m_inDrawingTool )
        return 0;

    bool placingDesignBlock = aEvent.IsAction( &SCH_ACTIONS::placeDesignBlock );

    std::unique_ptr<DESIGN_BLOCK> designBlock;
    wxString                      sheetFileName = wxEmptyString;
    int                           suffix = 1;

    if( placingDesignBlock )
    {
        SCH_DESIGN_BLOCK_PANE* designBlockPane = m_frame->GetDesignBlockPane();

        if( designBlockPane->GetSelectedLibId().IsValid() )
        {
            designBlock.reset( designBlockPane->GetDesignBlock( designBlockPane->GetSelectedLibId(),
                                                                true, true ) );

            if( !designBlock )
            {
                wxString msg;
                msg.Printf( _( "Could not find design block %s." ),
                            designBlockPane->GetSelectedLibId().GetUniStringLibId() );
                m_frame->ShowInfoBarError( msg, true );
                return 0;
            }

            sheetFileName = designBlock->GetSchematicFile();

            if( sheetFileName.IsEmpty() || !wxFileExists( sheetFileName ) )
            {
                m_frame->ShowInfoBarError( _( "Design block has no schematic to place." ), true );
                return 0;
            }
        }
    }
    else
    {
        wxString* importSourceFile = aEvent.Parameter<wxString*>();

        if( importSourceFile != nullptr )
            sheetFileName = *importSourceFile;
    }

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( designBlock ? KICURSOR::MOVING
                                                                    : KICURSOR::COMPONENT );
            };

    auto placeSheetContents =
            [&]()
            {
                SCH_COMMIT          commit( m_toolMgr );
                SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

                EDA_ITEMS newItems;
                bool      keepAnnotations = cfg->m_DesignBlockChooserPanel.keep_annotations;
                bool      placeAsGroup = cfg->m_DesignBlockChooserPanel.place_as_group;
                bool      repeatPlacement = cfg->m_DesignBlockChooserPanel.repeated_placement;

                selectionTool->ClearSelection();

                // Mark all existing items on the screen so we don't select them after appending
                for( EDA_ITEM* item : screen->Items() )
                    item->SetFlags( SKIP_STRUCT );

                if( !m_frame->LoadSheetFromFile( sheetPath.Last(), &sheetPath, sheetFileName, true,
                                                 placingDesignBlock ) )
                {
                    return false;
                }

                m_frame->SetSheetNumberAndCount();

                m_frame->SyncView();
                m_frame->OnModify();
                m_frame->HardRedraw(); // Full reinit of the current screen and the display.

                SCH_GROUP* group = nullptr;

                if( placeAsGroup )
                {
                    group = new SCH_GROUP( screen );

                    if( designBlock )
                    {
                        group->SetName( designBlock->GetLibId().GetLibItemName() );
                        group->SetDesignBlockLibId( designBlock->GetLibId() );
                    }
                    else
                    {
                        group->SetName( wxFileName( sheetFileName ).GetName() );
                    }

                    if( repeatPlacement )
                        group->SetName( group->GetName() + wxString::Format( "%d", suffix++ ) );
                }

                // Select all new items
                for( EDA_ITEM* item : screen->Items() )
                {
                    if( !item->HasFlag( SKIP_STRUCT ) )
                    {
                        if( item->Type() == SCH_SYMBOL_T && !keepAnnotations )
                            static_cast<SCH_SYMBOL*>( item )->ClearAnnotation( &sheetPath, false );

                        if( item->Type() == SCH_LINE_T )
                            item->SetFlags( STARTPOINT | ENDPOINT );

                        if( !item->GetParentGroup() )
                        {
                            if( placeAsGroup )
                                group->AddItem( item );

                            newItems.emplace_back( item );
                        }

                        commit.Added( item, screen );
                    }
                    else
                    {
                        item->ClearFlags( SKIP_STRUCT );
                    }
                }

                if( placeAsGroup )
                {
                    commit.Add( group, screen );
                    selectionTool->AddItemToSel( group );
                }
                else
                {
                    selectionTool->AddItemsToSel( &newItems, true );
                }

                cursorPos = grid.Align( controls->GetMousePosition(),
                                        grid.GetSelectionGrid( selectionTool->GetSelection() ) );
                controls->ForceCursorPosition( true, cursorPos );

                // Move everything to our current mouse position now
                // that we have a selection to get a reference point
                VECTOR2I anchorPos = selectionTool->GetSelection().GetReferencePoint();
                VECTOR2I delta = cursorPos - anchorPos;

                // Will all be SCH_ITEMs as these were pulled from the screen->Items()
                for( EDA_ITEM* item : newItems )
                    static_cast<SCH_ITEM*>( item )->Move( delta );

                if( !keepAnnotations )
                {
                    if( cfg->m_AnnotatePanel.automatic )
                    {
                        NULL_REPORTER reporter;
                        m_frame->AnnotateSymbols( &commit, ANNOTATE_SELECTION,
                                                  (ANNOTATE_ORDER_T) schSettings.m_AnnotateSortOrder,
                                                  (ANNOTATE_ALGO_T) schSettings.m_AnnotateMethod, true /* recursive */,
                                                  schSettings.m_AnnotateStartNum, false, false, false, reporter );
                    }

                    // Annotation will clear selection, so we need to restore it
                    for( EDA_ITEM* item : newItems )
                    {
                        if( item->Type() == SCH_LINE_T )
                            item->SetFlags( STARTPOINT | ENDPOINT );
                    }

                    if( placeAsGroup )
                        selectionTool->AddItemToSel( group );
                    else
                        selectionTool->AddItemsToSel( &newItems, true );
                }

                // Start moving selection, cancel undoes the insertion
                bool placed = m_toolMgr->RunSynchronousAction( SCH_ACTIONS::move, &commit );

                // Update our cursor position to the new location in case we're placing repeated copies
                cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_CONNECTABLE );

                if( placed )
                {
                    commit.Push( placingDesignBlock ? _( "Add Design Block" )
                                                    : _( "Import Schematic Sheet Content" ) );
                }
                else
                {
                    commit.Revert();
                }

                selectionTool->RebuildSelection();
                m_frame->UpdateHierarchyNavigator();

                return placed;
            };

    // Whether we are placing the sheet as a sheet, or as its contents, we need to get a filename
    // if we weren't provided one
    if( sheetFileName.IsEmpty() )
    {
        wxString path;
        wxString file;

        if (!placingDesignBlock)
        {
            if( sheetFileName.IsEmpty() )
            {
                path = wxPathOnly( m_frame->Prj().GetProjectFullName() );
                file = wxEmptyString;
            }
            else
            {
                path = wxPathOnly( sheetFileName );
                file = wxFileName( sheetFileName ).GetFullName();
            }

            // Open file chooser dialog even if we have been provided a file so the user
            // can select the options they want
            wxFileDialog dlg( m_frame, _( "Choose Schematic" ), path, file,
                              FILEEXT::KiCadSchematicFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

            FILEDLG_IMPORT_SHEET_CONTENTS dlgHook( cfg );
            dlg.SetCustomizeHook( dlgHook );

            if( dlg.ShowModal() == wxID_CANCEL )
                return 0;

            sheetFileName = dlg.GetPath();

            m_frame->GetDesignBlockPane()->UpdateCheckboxes();
        }

        if( sheetFileName.IsEmpty() )
            return 0;
    }

    // If we're placing sheet contents, we don't even want to run our tool loop, just add the items
    // to the canvas and run the move tool
    if( !cfg->m_DesignBlockChooserPanel.place_as_sheet )
    {
        while( placeSheetContents() && cfg->m_DesignBlockChooserPanel.repeated_placement )
        {}

        m_toolMgr->RunAction( ACTIONS::selectionClear );
        m_view->ClearPreview();
        return 0;
    }

    // We're placing a sheet as a sheet, we need to run a small tool loop to get the starting
    // coordinate of the sheet drawing
    m_frame->PushTool( aEvent );

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    if( common_settings->m_Input.immediate_actions && !aEvent.IsReactivate() )
        m_toolMgr->PrimeTool( { 0, 0 } );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_CONNECTABLE );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = designBlock && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( designBlock && evt->IsAction( &ACTIONS::undo ) ) )
        {
            m_frame->GetInfoBar()->Dismiss();
            break;
        }
        else if( evt->IsActivate() && !isSyntheticClick )
        {
            m_frame->GetInfoBar()->Dismiss();
            break;
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || isSyntheticClick
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
            if( placingDesignBlock )
            {
                // drawSheet must delete designBlock
                m_toolMgr->PostAction( SCH_ACTIONS::drawSheetFromDesignBlock, designBlock.release() );
            }
            else
            {
                // drawSheet must delete sheetFileName
                m_toolMgr->PostAction( SCH_ACTIONS::drawSheetFromFile, new wxString( sheetFileName ) );
            }

            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !designBlock )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                 || evt->IsAction( &SCH_ACTIONS::repeatDrawItem ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_frame->PopTool( aEvent );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP*      image = aEvent.Parameter<SCH_BITMAP*>();
    bool             immediateMode = image != nullptr;
    bool             ignorePrimePosition = false;
    COMMON_SETTINGS* common_settings = Pgm().GetCommonSettings();

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    EE_GRID_HELPER        grid( m_toolMgr );
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2I              cursorPos;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

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
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
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
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
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

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = image && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( image && evt->IsAction( &ACTIONS::undo ) ) )
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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || isSyntheticClick
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
            if( !image )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                wxFileDialog dlg( m_frame, _( "Choose Image" ), m_mruPath, wxEmptyString,
                                  _( "Image Files" ) + wxS( " " ) + wxImage::GetImageExtWildcard(), wxFD_OPEN );

                bool cancelled = false;

                RunMainStack(
                        [&]()
                        {
                            cancelled = dlg.ShowModal() != wxID_OK;
                        } );

                if( cancelled )
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

                if( !image || !image->GetReferenceImage().ReadImageFile( fullFilename ) )
                {
                    wxMessageBox( wxString::Format( _( "Could not load image from '%s'." ), fullFilename ) );
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
                commit.Push( _( "Place Image" ) );

                image = nullptr;
                m_toolMgr->PostAction( ACTIONS::activatePointEditor );

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

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                 || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                 || evt->IsAction( &ACTIONS::paste ) )
        {
            if( image )
            {
                // This doesn't really make sense; we'll just end up dragging a stack of
                // objects so we ignore the duplicate and just carry on.
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
            break;
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
        else if( image && evt->IsAction( &ACTIONS::redo ) )
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


int SCH_DRAWING_TOOLS::ImportGraphics( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    // Note: PlaceImportedGraphics() will convert PCB_SHAPE_T and PCB_TEXT_T to footprint
    // items if needed
    DIALOG_IMPORT_GFX_SCH dlg( m_frame );

    // Set filename on drag-and-drop
    if( aEvent.HasParameter() )
        dlg.SetFilenameOverride( *aEvent.Parameter<wxString*>() );

    int dlgResult = dlg.ShowModal();

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
        wxCHECK2_MSG( item, continue, wxString::Format( "Bad item type: ", ptr->Type() ) );

        newItems.push_back( item );
        selectedItems.push_back( item );
        preview.Add( item );

        ptr.release();
    }

    if( !dlg.IsPlacementInteractive() )
    {
        // Place the imported drawings
        for( SCH_ITEM* item : newItems )
            commit.Add(item, m_frame->GetScreen());

        commit.Push( _( "Import Graphic" ) );
        return 0;
    }

    m_view->Add( &preview );

    // Clear the current selection then select the drawings so that edit tools work on them
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    EDA_ITEMS selItems( selectedItems.begin(), selectedItems.end() );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &selItems );

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
            // Place the imported drawings
            for( SCH_ITEM* item : newItems )
                commit.Add( item, m_frame->GetScreen() );

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

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls->ForceCursorPosition( false );

    m_frame->PopTool( aEvent );

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
    bool                  allowRepeat = false;  // Set to true to allow new item repetition

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    if( type == SCH_JUNCTION_T && aEvent.HasPosition() )
    {
        SCH_SELECTION& selection = m_selectionTool->GetSelection();
        SCH_LINE*      wire = dynamic_cast<SCH_LINE*>( selection.Front() );

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
        allowRepeat = true;
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
        allowRepeat = true;
        break;

    default:
        wxASSERT_MSG( false, "Unknown item type in SCH_DRAWING_TOOLS::SingleClickPlace" );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::selectionClear );

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
        m_toolMgr->PostAction( ACTIONS::refreshPreview );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = evt->IsPrime() ? evt->Position() : controls->GetMousePosition();
        cursorPos = grid.BestSnapAnchor( cursorPos, grid.GetItemGrid( previewItem ), nullptr );
        controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() )
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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
            if( !screen->GetItem( cursorPos, 0, type ) )
            {
                if( type == SCH_JUNCTION_T )
                {
                    if( !screen->IsExplicitJunctionAllowed( cursorPos ) )
                    {
                        m_frame->ShowInfoBarError( _( "Junction location contains no joinable wires and/or pins." ) );
                        loggedInfoBarError = true;
                        continue;
                    }
                    else if( loggedInfoBarError )
                    {
                        m_frame->GetInfoBar()->Dismiss();
                    }
                }

                if( type == SCH_JUNCTION_T )
                {
                    SCH_COMMIT commit( m_toolMgr );
                    SCH_LINE_WIRE_BUS_TOOL* lwbTool =
                            m_toolMgr->GetTool<SCH_LINE_WIRE_BUS_TOOL>();
                    lwbTool->AddJunction( &commit, screen, cursorPos );

                    m_frame->Schematic().CleanUp( &commit );

                    commit.Push( description );
                }
                else
                {
                    SCH_ITEM* newItem = static_cast<SCH_ITEM*>( previewItem->Clone() );
                    const_cast<KIID&>( newItem->m_Uuid ) = KIID();
                    newItem->SetPosition( cursorPos );
                    newItem->SetFlags( IS_NEW );
                    m_frame->AddToScreen( newItem, screen );

                    if( allowRepeat )
                        m_frame->SaveCopyForRepeatItem( newItem );

                    SCH_COMMIT commit( m_toolMgr );
                    commit.Added( newItem, screen );

                    m_frame->Schematic().CleanUp( &commit );

                    commit.Push( description );
                }
            }

            if( evt->IsDblClick( BUT_LEFT ) || type == SCH_SHEET_PIN_T )  // Finish tool.
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
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
            if( ( type == SCH_BUS_WIRE_ENTRY_T ) && (   evt->IsAction( &SCH_ACTIONS::rotateCW )
                                                     || evt->IsAction( &SCH_ACTIONS::rotateCCW )
                                                     || evt->IsAction( &SCH_ACTIONS::mirrorV )
                                                     || evt->IsAction( &SCH_ACTIONS::mirrorH ) ) )
            {
                SCH_BUS_ENTRY_BASE* busItem = static_cast<SCH_BUS_ENTRY_BASE*>( previewItem );

                if( evt->IsAction( &SCH_ACTIONS::rotateCW ) )
                {
                    busItem->Rotate( busItem->GetPosition(), false );
                }
                else if( evt->IsAction( &SCH_ACTIONS::rotateCCW ) )
                {
                    busItem->Rotate( busItem->GetPosition(), true );
                }
                else if( evt->IsAction( &SCH_ACTIONS::mirrorV ) )
                {
                    busItem->MirrorVertically( busItem->GetPosition().y );
                }
                else if( evt->IsAction( &SCH_ACTIONS::mirrorH ) )
                {
                    busItem->MirrorHorizontally( busItem->GetPosition().x );
                }

                m_view->ClearPreview();
                m_view->AddToPreview( previewItem->Clone() );
            }
            else if( evt->IsAction( &SCH_ACTIONS::properties ) )
            {
                switch( type )
                {
                case SCH_BUS_WIRE_ENTRY_T:
                {
                    std::deque<SCH_ITEM*> strokeItems;
                    strokeItems.push_back( previewItem );

                    DIALOG_WIRE_BUS_PROPERTIES dlg( m_frame, strokeItems );

                    RunMainStack(
                            [&]()
                            {
                                dlg.ShowModal();
                            } );

                    break;
                }

                case SCH_JUNCTION_T:
                {
                    std::deque<SCH_JUNCTION*> junctions;
                    junctions.push_back( static_cast<SCH_JUNCTION*>( previewItem ) );

                    DIALOG_JUNCTION_PROPS dlg( m_frame, junctions );

                    RunMainStack(
                            [&]()
                            {
                                dlg.ShowModal();
                            } );

                    break;
                }

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

    return 0;
}


SCH_LINE* SCH_DRAWING_TOOLS::findWire( const VECTOR2I& aPosition )
{
    for( SCH_ITEM* item : m_frame->GetScreen()->Items().Overlapping( SCH_LINE_T, aPosition ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->GetEditFlags() & STRUCT_DELETED )
            continue;

        if( line->IsWire() )
            return line;
    }

    return nullptr;
}


wxString SCH_DRAWING_TOOLS::findWireLabelDriverName( SCH_LINE* aWire )
{
    wxASSERT( aWire->IsWire() );

    SCH_SHEET_PATH sheetPath = m_frame->GetCurrentSheet();

    if( SCH_CONNECTION* wireConnection = aWire->Connection( &sheetPath ) )
    {
        SCH_ITEM* wireDriver = wireConnection->Driver();

        if( wireDriver && wireDriver->IsType( { SCH_LABEL_T, SCH_GLOBAL_LABEL_T } ) )
            return wireConnection->LocalName();
    }

    return wxEmptyString;
}


bool SCH_DRAWING_TOOLS::createNewLabel( const VECTOR2I& aPosition, int aType,
                                        std::list<std::unique_ptr<SCH_LABEL_BASE>>& aLabelList )
{
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& settings = schematic->Settings();
    SCH_LABEL_BASE*     labelItem = nullptr;
    SCH_GLOBALLABEL*    globalLabel = nullptr;
    wxString            netName;

    switch( aType )
    {
    case LAYER_LOCLABEL:
        labelItem = new SCH_LABEL( aPosition );

        if( SCH_LINE* wire = findWire( aPosition ) )
            netName = findWireLabelDriverName( wire );

        break;

    case LAYER_NETCLASS_REFS:
        labelItem = new SCH_DIRECTIVE_LABEL( aPosition );
        labelItem->SetShape( m_lastNetClassFlagShape );
        labelItem->GetFields().emplace_back( labelItem, FIELD_T::USER, wxT( "Netclass" ) );
        labelItem->GetFields().emplace_back( labelItem, FIELD_T::USER, wxT( "Component Class" ) );
        labelItem->GetFields().back().SetItalic( true );
        labelItem->GetFields().back().SetVisible( true );
        break;

    case LAYER_HIERLABEL:
        labelItem = new SCH_HIERLABEL( aPosition );
        labelItem->SetShape( m_lastGlobalLabelShape );
        labelItem->SetAutoRotateOnPlacement( m_lastAutoLabelRotateOnPlacement );
        break;

    case LAYER_GLOBLABEL:
        globalLabel = new SCH_GLOBALLABEL( aPosition );
        globalLabel->SetShape( m_lastGlobalLabelShape );
        globalLabel->GetField( FIELD_T::INTERSHEET_REFS )->SetVisible( settings.m_IntersheetRefsShow );
        globalLabel->SetAutoRotateOnPlacement( m_lastAutoLabelRotateOnPlacement );
        labelItem = globalLabel;

        if( SCH_LINE* wire = findWire( aPosition ) )
            netName = findWireLabelDriverName( wire );

        break;

    default:
        wxFAIL_MSG( "SCH_DRAWING_TOOLS::createNewLabel() unknown label type" );
        return false;
    }

    // The normal parent is the current screen for these labels, set by SCH_SCREEN::Append()
    // but it is also used during placement for SCH_HIERLABEL before beeing appended
    labelItem->SetParent( m_frame->GetScreen() );

    labelItem->SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );

    if( aType != LAYER_NETCLASS_REFS )
    {
        // Must be after SetTextSize()
        labelItem->SetBold( m_lastTextBold );
        labelItem->SetItalic( m_lastTextItalic );
    }

    labelItem->SetSpinStyle( m_lastTextOrientation );
    labelItem->SetFlags( IS_NEW | IS_MOVING );

    if( !netName.IsEmpty() )
    {
        // Auto-create from attached wire
        labelItem->SetText( netName );
    }
    else
    {
        DIALOG_LABEL_PROPERTIES dlg( m_frame, labelItem, true );

        dlg.SetLabelList( &aLabelList );

        // QuasiModal required for syntax help and Scintilla auto-complete
        if( dlg.ShowQuasiModal() != wxID_OK )
        {
            dlg.GetFieldsGridTable()->DetachFields();
            delete labelItem;
            return false;
        }
    }

    if( aType != LAYER_NETCLASS_REFS )
    {
        m_lastTextBold = labelItem->IsBold();
        m_lastTextItalic = labelItem->IsItalic();
    }

    m_lastTextOrientation = labelItem->GetSpinStyle();

    if( aType == LAYER_GLOBLABEL || aType == LAYER_HIERLABEL )
    {
        m_lastGlobalLabelShape = labelItem->GetShape();
        m_lastAutoLabelRotateOnPlacement = labelItem->AutoRotateOnPlacement();
    }
    else if( aType == LAYER_NETCLASS_REFS )
    {
        m_lastNetClassFlagShape = labelItem->GetShape();
    }

    if( aLabelList.empty() )
        aLabelList.push_back( std::unique_ptr<SCH_LABEL_BASE>( labelItem ) );
    else // DIALOG_LABEL_PROPERTIES already filled in aLabelList; labelItem is extraneous to needs
        delete labelItem;

    return true;
}


SCH_TEXT* SCH_DRAWING_TOOLS::createNewText( const VECTOR2I& aPosition )
{
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& settings = schematic->Settings();
    SCH_TEXT*           textItem = nullptr;

    textItem = new SCH_TEXT( aPosition );
    textItem->SetParent( schematic );
    textItem->SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
    // Must be after SetTextSize()
    textItem->SetBold( m_lastTextBold );
    textItem->SetItalic( m_lastTextItalic );
    textItem->SetHorizJustify( m_lastTextHJustify );
    textItem->SetVertJustify( m_lastTextVJustify );
    textItem->SetTextAngle( m_lastTextAngle );
    textItem->SetFlags( IS_NEW | IS_MOVING );

    DIALOG_TEXT_PROPERTIES dlg( m_frame, textItem );

    // QuasiModal required for syntax help and Scintilla auto-complete
    if( dlg.ShowQuasiModal() != wxID_OK )
    {
        delete textItem;
        return nullptr;
    }

    m_lastTextBold = textItem->IsBold();
    m_lastTextItalic = textItem->IsItalic();
    m_lastTextHJustify = textItem->GetHorizJustify();
    m_lastTextVJustify = textItem->GetVertJustify();
    m_lastTextAngle = textItem->GetTextAngle();
    return textItem;
}


SCH_SHEET_PIN* SCH_DRAWING_TOOLS::createNewSheetPin( SCH_SHEET* aSheet, const VECTOR2I& aPosition )
{
    SCHEMATIC_SETTINGS& settings = aSheet->Schematic()->Settings();
    SCH_SHEET_PIN*      pin = new SCH_SHEET_PIN( aSheet );

    pin->SetFlags( IS_NEW | IS_MOVING );
    pin->SetText( std::to_string( aSheet->GetPins().size() + 1 ) );
    pin->SetTextSize( VECTOR2I( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
    pin->SetPosition( aPosition );
    pin->ClearSelected();

    m_lastSheetPinType = pin->GetShape();

    return pin;
}


SCH_SHEET_PIN* SCH_DRAWING_TOOLS::createNewSheetPinFromLabel( SCH_SHEET*      aSheet,
                                                              const VECTOR2I& aPosition,
                                                              SCH_HIERLABEL*  aLabel )
{
    auto pin = createNewSheetPin( aSheet, aPosition );
    pin->SetText( aLabel->GetText() );
    pin->SetShape( aLabel->GetShape() );
    return pin;
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

    std::list<std::unique_ptr<SCH_LABEL_BASE>> itemsToPlace;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    bool isText        = aEvent.IsAction( &SCH_ACTIONS::placeSchematicText );
    bool isGlobalLabel = aEvent.IsAction( &SCH_ACTIONS::placeGlobalLabel );
    bool isHierLabel   = aEvent.IsAction( &SCH_ACTIONS::placeHierLabel );
    bool isClassLabel  = aEvent.IsAction( &SCH_ACTIONS::placeClassLabel );
    bool isNetLabel    = aEvent.IsAction( &SCH_ACTIONS::placeLabel );
    bool isSheetPin    = aEvent.IsAction( &SCH_ACTIONS::placeSheetPin );

    GRID_HELPER_GRIDS snapGrid = isText ? GRID_TEXT : GRID_CONNECTABLE;

    // If we have a selected sheet use it, otherwise try to get one under the cursor
    if( isSheetPin )
        sheet = dynamic_cast<SCH_SHEET*>( m_selectionTool->GetSelection().Front() );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

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
                m_view->AddToPreview( item, false );
                item->RunOnChildren( [&]( SCH_ITEM* aChild )
                                     {
                                         m_view->AddToPreview( aChild, false );
                                     },
                                     RECURSE_MODE::NO_RECURSE );
                m_frame->SetMsgPanel( item );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_view->ClearPreview();
                delete item;
                item = nullptr;

                while( !itemsToPlace.empty() )
                {
                    itemsToPlace.front().release();
                    itemsToPlace.pop_front();
                }
            };

    auto prepItemForPlacement =
            [&]( SCH_ITEM* aItem, const VECTOR2I& cursorPos )
            {
                item->SetPosition( cursorPos );

                item->SetFlags( IS_NEW | IS_MOVING );

                // Not placed yet, so pass a nullptr screen reference
                item->AutoplaceFields( nullptr, AUTOPLACE_AUTO );

                updatePreview();
                m_selectionTool->ClearSelection( true );
                m_selectionTool->AddItemToSel( item );
                m_toolMgr->PostAction( ACTIONS::refreshPreview );

                // update the cursor so it looks correct before another event
                setCursor();
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

    SCH_COMMIT commit( m_toolMgr );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        VECTOR2I cursorPos = controls->GetMousePosition();
        cursorPos = grid.BestSnapAnchor( cursorPos, snapGrid, item );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = item && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

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
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || isSyntheticClick
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
        {
        PLACE_NEXT:
            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                if( isText )
                {
                    item = createNewText( cursorPos );
                    description = _( "Add Text" );
                }
                else if( isHierLabel )
                {
                    if( m_dialogSyncSheetPin && m_dialogSyncSheetPin->GetPlacementTemplate() )
                    {
                        auto pin = static_cast<SCH_HIERLABEL*>( m_dialogSyncSheetPin->GetPlacementTemplate() );
                        SCH_HIERLABEL* label = new SCH_HIERLABEL( cursorPos );
                        SCHEMATIC*     schematic = getModel<SCHEMATIC>();
                        label->SetText( pin->GetText() );
                        label->SetShape( pin->GetShape() );
                        label->SetAutoRotateOnPlacement( m_lastAutoLabelRotateOnPlacement );
                        label->SetParent( schematic );
                        label->SetBold( m_lastTextBold );
                        label->SetItalic( m_lastTextItalic );
                        label->SetSpinStyle( m_lastTextOrientation );
                        label->SetTextSize( VECTOR2I( schematic->Settings().m_DefaultTextSize,
                                                      schematic->Settings().m_DefaultTextSize ) );
                        label->SetFlags( IS_NEW | IS_MOVING );
                        itemsToPlace.push_back( std::unique_ptr<SCH_LABEL_BASE>( label ) );
                    }
                    else
                    {
                        createNewLabel( cursorPos, LAYER_HIERLABEL, itemsToPlace );
                    }

                    description = _( "Add Hierarchical Label" );
                }
                else if( isNetLabel )
                {
                    createNewLabel( cursorPos, LAYER_LOCLABEL, itemsToPlace );
                    description = _( "Add Label" );
                }
                else if( isGlobalLabel )
                {
                    createNewLabel( cursorPos, LAYER_GLOBLABEL, itemsToPlace );
                    description = _( "Add Label" );
                }
                else if( isClassLabel )
                {
                    createNewLabel( cursorPos, LAYER_NETCLASS_REFS, itemsToPlace );
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
                        m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition()
                                             + wxPoint( 20, 20 ) );
                        m_statusPopup->PopupFor( 2000 );
                        item = nullptr;
                    }
                    else
                    {
                        // User is using the 'Sync Sheet Pins' tool
                        if( m_dialogSyncSheetPin && m_dialogSyncSheetPin->GetPlacementTemplate() )
                        {
                            item = createNewSheetPinFromLabel(
                                    sheet, cursorPos,
                                    static_cast<SCH_HIERLABEL*>( m_dialogSyncSheetPin->GetPlacementTemplate() ) );
                        }
                        else
                        {
                            // User is using the 'Place Sheet Pins' tool
                            SCH_HIERLABEL* label = importHierLabel( sheet );

                            if( !label )
                            {
                                m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( m_frame );
                                m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
                                m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
                                m_statusPopup->PopupFor( 2000 );
                                item = nullptr;

                                m_frame->PopTool( aEvent );
                                break;
                            }

                            item = createNewSheetPinFromLabel( sheet, cursorPos, label );
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
                    cursorPos = grid.BestSnapAnchor( cursorPos, snapGrid, item );
                }

                if( !itemsToPlace.empty() )
                {
                    item = itemsToPlace.front().release();
                    itemsToPlace.pop_front();
                }

                if( item )
                    prepItemForPlacement( item, cursorPos );

                if( m_frame->GetMoveWarpsCursor() )
                    controls->SetCursorPosition( cursorPos, false );

                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            }
            else            // ... and second click places:
            {
                item->ClearFlags( IS_MOVING );

                if( item->IsConnectable() )
                    m_frame->AutoRotateItem( m_frame->GetScreen(), item );

                if( isSheetPin && sheet )
                {
                    // Sheet pins are owned by their parent sheet.
                    commit.Modify( sheet, m_frame->GetScreen() );
                    sheet->AddPin( (SCH_SHEET_PIN*) item );
                }
                else
                {
                    m_frame->SaveCopyForRepeatItem( item );
                    m_frame->AddToScreen( item, m_frame->GetScreen() );
                    commit.Added( item, m_frame->GetScreen() );
                }

                item->AutoplaceFields( m_frame->GetScreen(), AUTOPLACE_AUTO );

                commit.Push( description );

                m_view->ClearPreview();

                if( m_dialogSyncSheetPin && m_dialogSyncSheetPin->GetPlacementTemplate() )
                {
                    m_dialogSyncSheetPin->EndPlaceItem( item );

                    if( m_dialogSyncSheetPin->CanPlaceMore() )
                    {
                        item = nullptr;
                        goto PLACE_NEXT;
                    }

                    m_frame->PopTool( aEvent );
                    m_toolMgr->RunAction( ACTIONS::selectionClear );
                    m_dialogSyncSheetPin->Show( true );
                    break;
                }

                item = nullptr;

                if( isSheetPin && sheet )
                {
                    SCH_HIERLABEL* label = importHierLabel( sheet );

                    if( !label )
                    {
                        m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( m_frame );
                        m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
                        m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
                        m_statusPopup->PopupFor( 2000 );

                        m_frame->PopTool( aEvent );
                        break;
                    }

                    item = createNewSheetPinFromLabel( sheet, cursorPos, label );
                }
                else if( !itemsToPlace.empty() )
                {
                    item = itemsToPlace.front().release();
                    itemsToPlace.pop_front();
                    prepItemForPlacement( item, cursorPos );
                }
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && evt->IsSelectionEvent() )
        {
            // This happens if our text was replaced out from under us by ConvertTextType()
            SCH_SELECTION& selection = m_selectionTool->GetSelection();

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
        else if( evt->IsAction( &ACTIONS::increment ) )
        {
            if( evt->HasParameter() )
                m_toolMgr->RunSynchronousAction( ACTIONS::increment, &commit, evt->Parameter<ACTIONS::INCREMENT>() );
            else
                m_toolMgr->RunSynchronousAction( ACTIONS::increment, &commit, ACTIONS::INCREMENT { 1, 0 } );
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                || evt->IsAction( &ACTIONS::paste ) )
        {
            if( item )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
            break;
        }
        else if( item && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->SetPosition( cursorPos );

            // Not placed yet, so pass a nullptr screen reference
            item->AutoplaceFields( nullptr, AUTOPLACE_AUTO );

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
        else if( item && (   evt->IsAction( &SCH_ACTIONS::toDLabel )
                          || evt->IsAction( &SCH_ACTIONS::toGLabel )
                          || evt->IsAction( &SCH_ACTIONS::toHLabel )
                          || evt->IsAction( &SCH_ACTIONS::toLabel )
                          || evt->IsAction( &SCH_ACTIONS::toText )
                          || evt->IsAction( &SCH_ACTIONS::toTextBox ) ) )
        {
            wxBell();
        }
        else if( item && (   evt->IsAction( &SCH_ACTIONS::properties )
                          || evt->IsAction( &SCH_ACTIONS::autoplaceFields )
                          || evt->IsAction( &SCH_ACTIONS::rotateCW )
                          || evt->IsAction( &SCH_ACTIONS::rotateCCW )
                          || evt->IsAction( &SCH_ACTIONS::mirrorV )
                          || evt->IsAction( &SCH_ACTIONS::mirrorH ) ) )
        {
            m_toolMgr->PostAction( ACTIONS::refreshPreview );
            evt->SetPassEvent();
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

    if( m_dialogSyncSheetPin && m_dialogSyncSheetPin->CanPlaceMore() )
    {
        m_dialogSyncSheetPin->EndPlacement();
        m_dialogSyncSheetPin->Show( true );
    }

    return 0;
}


int SCH_DRAWING_TOOLS::DrawShape( const TOOL_EVENT& aEvent )
{
    SCHEMATIC*          schematic = getModel<SCHEMATIC>();
    SCHEMATIC_SETTINGS& sch_settings = schematic->Settings();
    SCH_SHAPE*          item = nullptr;
    bool                isTextBox = aEvent.IsAction( &SCH_ACTIONS::drawTextBox );
    SHAPE_T             type = aEvent.Parameter<SHAPE_T>();
    wxString            description;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

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
    getViewControls()->ShowCursor( true );

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

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = item && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( item && evt->IsAction( &ACTIONS::undo ) ) )
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
        else if( !item && (   evt->IsClick( BUT_LEFT )
                           || evt->IsAction( &ACTIONS::cursorClick ) ) )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            if( isTextBox )
            {
                SCH_TEXTBOX* textbox = new SCH_TEXTBOX( LAYER_NOTES, 0, m_lastTextboxFillStyle );

                textbox->SetTextSize( VECTOR2I( sch_settings.m_DefaultTextSize,
                                                sch_settings.m_DefaultTextSize ) );

                // Must come after SetTextSize()
                textbox->SetBold( m_lastTextBold );
                textbox->SetItalic( m_lastTextItalic );

                textbox->SetTextAngle( m_lastTextboxAngle );
                textbox->SetHorizJustify( m_lastTextboxHJustify );
                textbox->SetVertJustify( m_lastTextboxVJustify );
                textbox->SetStroke( m_lastTextboxStroke );
                textbox->SetFillColor( m_lastTextboxFillColor );
                textbox->SetParent( schematic );

                item = textbox;
                description = _( "Add Text Box" );
            }
            else
            {
                item = new SCH_SHAPE( type, LAYER_NOTES, 0, m_lastFillStyle );

                item->SetStroke( m_lastStroke );
                item->SetFillColor( m_lastFillColor );
                item->SetParent( schematic );
                description = wxString::Format( _( "Add %s" ), item->GetFriendlyName() );
            }

            item->SetFlags( IS_NEW );
            item->BeginEdit( cursorPos );

            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }
        else if( item && (   evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                          || isSyntheticClick
                          || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick )
                          || evt->IsAction( &ACTIONS::finishInteractive ) ) )
        {
            bool finished = false;

            if( evt->IsDblClick( BUT_LEFT )
                    || evt->IsAction( &ACTIONS::cursorDblClick )
                    || evt->IsAction( &ACTIONS::finishInteractive ) )
            {
                finished = true;
            }
            else
            {
                finished = !item->ContinueEdit( cursorPos );
            }

            if( finished )
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
                    m_lastTextboxAngle = textbox->GetTextAngle();
                    m_lastTextboxHJustify = textbox->GetHorizJustify();
                    m_lastTextboxVJustify = textbox->GetVertJustify();
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
                m_toolMgr->PostAction( ACTIONS::activatePointEditor );
            }
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                || evt->IsAction( &ACTIONS::paste ) )
        {
            if( item )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
            break;
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
            m_toolMgr->RunAction( SCH_ACTIONS::properties );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && evt->IsAction( &ACTIONS::redo ) )
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


int SCH_DRAWING_TOOLS::DrawRuleArea( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD       guard( &m_inDrawingTool );
    SCOPED_SET_RESET<bool> scopedDrawMode( m_drawingRuleArea, true );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

    RULE_AREA_CREATE_HELPER ruleAreaTool( *getView(), m_frame, m_toolMgr );
    POLYGON_GEOM_MANAGER    polyGeomMgr( ruleAreaTool );
    bool                    started = false;

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
            [&]()
            {
                polyGeomMgr.Reset();
                started = false;
                getViewControls()->SetAutoPan( false );
                getViewControls()->CaptureCursor( false );
                m_toolMgr->RunAction( ACTIONS::selectionClear );
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    //m_controls->ForceCursorPosition( false );

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

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_CONNECTABLE );
        controls->ForceCursorPosition( true, cursorPos );

        polyGeomMgr.SetLeaderMode( m_frame->eeconfig()->m_Drawing.line_mode == LINE_MODE_FREE ? LEADER_MODE::DIRECT
                                                                                              : LEADER_MODE::DEG45 );

        if( evt->IsCancelInteractive() )
        {
            if( started )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );

                // We've handled the cancel event.  Don't cancel other tools
                evt->SetPassEvent( false );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( started )
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
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !started )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        // events that lock in nodes
        else if(   evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick )
                || evt->IsAction( &SCH_ACTIONS::closeOutline ) )
        {
            // Check if it is double click / closing line (so we have to finish the zone)
            const bool endPolygon = evt->IsDblClick( BUT_LEFT )
                                    || evt->IsAction( &ACTIONS::cursorDblClick )
                                    || evt->IsAction( &SCH_ACTIONS::closeOutline )
                                    || polyGeomMgr.NewPointClosesOutline( cursorPos );

            if( endPolygon )
            {
                polyGeomMgr.SetFinished();
                polyGeomMgr.Reset();

                started = false;
                getViewControls()->SetAutoPan( false );
                getViewControls()->CaptureCursor( false );
            }
            // adding a corner
            else if( polyGeomMgr.AddPoint( cursorPos ) )
            {
                if( !started )
                {
                    started = true;

                    getViewControls()->SetAutoPan( true );
                    getViewControls()->CaptureCursor( true );
                }
            }
        }
        else if( started && (   evt->IsAction( &SCH_ACTIONS::deleteLastPoint )
                             || evt->IsAction( &ACTIONS::doDelete )
                             || evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( std::optional<VECTOR2I> last = polyGeomMgr.DeleteLastCorner() )
            {
                cursorPos = last.value();
                getViewControls()->WarpMouseCursor( cursorPos, true );
                getViewControls()->ForceCursorPosition( true, cursorPos );
                polyGeomMgr.SetCursorPosition( cursorPos );
            }
            else
            {
                cleanup();
            }
        }
        else if( started && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            polyGeomMgr.SetCursorPosition( cursorPos );
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                || evt->IsAction( &ACTIONS::paste ) )
        {
            if( started )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
            break;
        }
        else
        {
            evt->SetPassEvent();
        }

    } // end while

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SCH_DRAWING_TOOLS::DrawTable( const TOOL_EVENT& aEvent )
{
    SCHEMATIC* schematic = getModel<SCHEMATIC>();
    SCH_TABLE* table = nullptr;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;

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
                delete table;
                table = nullptr;
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
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = table && evt->IsActivate() && evt->HasPosition() && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( table && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( table )
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
            if( table && evt->IsMoveTool() )
            {
                // we're already drawing our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( table )
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
        else if( !table && (   evt->IsClick( BUT_LEFT )
                            || evt->IsAction( &ACTIONS::cursorClick ) ) )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            table = new SCH_TABLE( 0 );
            table->SetColCount( 1 );

            SCH_TABLECELL* tableCell = new SCH_TABLECELL();
            int defaultTextSize = schematic->Settings().m_DefaultTextSize;

            tableCell->SetTextSize( VECTOR2I( defaultTextSize, defaultTextSize ) );
            table->AddCell( tableCell );

            table->SetParent( schematic );
            table->SetFlags( IS_NEW );
            table->SetPosition( cursorPos );

            m_view->ClearPreview();
            m_view->AddToPreview( table->Clone() );
        }
        else if( table && (   evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                           || isSyntheticClick
                           || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick )
                           || evt->IsAction( &SCH_ACTIONS::finishInteractive ) ) )
        {
            table->ClearEditFlags();
            table->SetFlags( IS_NEW );
            table->Normalize();

            DIALOG_TABLE_PROPERTIES dlg( m_frame, table );

            // QuasiModal required for Scintilla auto-complete
            if( dlg.ShowQuasiModal() == wxID_OK )
            {
                SCH_COMMIT commit( m_toolMgr );
                commit.Add( table, m_frame->GetScreen() );
                commit.Push( _( "Draw Table" ) );

                m_selectionTool->AddItemToSel( table );
                m_toolMgr->PostAction( ACTIONS::activatePointEditor );
            }
            else
            {
                delete table;
            }

            table = nullptr;
            m_view->ClearPreview();
        }
        else if( table && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            VECTOR2I gridSize = grid.GetGridSize( grid.GetItemGrid( table ) );
            int      fontSize = schematic->Settings().m_DefaultTextSize;
            VECTOR2I origin( table->GetPosition() );
            VECTOR2I requestedSize( cursorPos - origin );

            int colCount = std::max( 1, requestedSize.x / ( fontSize * 15 ) );
            int rowCount = std::max( 1, requestedSize.y / ( fontSize * 2  ) );

            VECTOR2I cellSize( std::max( gridSize.x * 5, requestedSize.x / colCount ),
                               std::max( gridSize.y * 2, requestedSize.y / rowCount ) );

            cellSize.x = KiROUND( (double) cellSize.x / gridSize.x ) * gridSize.x;
            cellSize.y = KiROUND( (double) cellSize.y / gridSize.y ) * gridSize.y;

            table->ClearCells();
            table->SetColCount( colCount );

            for( int col = 0; col < colCount; ++col )
                table->SetColWidth( col, cellSize.x );

            for( int row = 0; row < rowCount; ++row )
            {
                table->SetRowHeight( row, cellSize.y );

                for( int col = 0; col < colCount; ++col )
                {
                    SCH_TABLECELL* cell = new SCH_TABLECELL();
                    int defaultTextSize = schematic->Settings().m_DefaultTextSize;

                    cell->SetTextSize( VECTOR2I( defaultTextSize, defaultTextSize ) );
                    cell->SetPosition( origin + VECTOR2I( col * cellSize.x, row * cellSize.y ) );
                    cell->SetEnd( cell->GetPosition() + cellSize );
                    table->AddCell( cell );
                }
            }

            m_view->ClearPreview();
            m_view->AddToPreview( table->Clone() );
            m_frame->SetMsgPanel( table );
        }
        else if( evt->IsDblClick( BUT_LEFT ) && !table )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::properties );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !table )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                || evt->IsAction( &ACTIONS::paste ) )
        {
            if( table )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
            break;
        }
        else if( table && evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( table != nullptr );
        getViewControls()->CaptureCursor( table != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int SCH_DRAWING_TOOLS::DrawSheet( const TOOL_EVENT& aEvent )
{
    bool isDrawSheetCopy = aEvent.IsAction( &SCH_ACTIONS::drawSheetFromFile );
    bool isDrawSheetFromDesignBlock = aEvent.IsAction( &SCH_ACTIONS::drawSheetFromDesignBlock );

    std::unique_ptr<DESIGN_BLOCK> designBlock;

    SCH_SHEET* sheet = nullptr;
    wxString   filename;
    SCH_GROUP* sheetGroup = nullptr;

    if( isDrawSheetCopy )
    {
        wxString* ptr = aEvent.Parameter<wxString*>();
        wxCHECK( ptr, 0 );

        // We own the string if we're importing a sheet
        filename = *ptr;
        delete ptr;
    }
    else if( isDrawSheetFromDesignBlock )
    {
        designBlock.reset( aEvent.Parameter<DESIGN_BLOCK*>() );
        wxCHECK( designBlock, 0 );
        filename = designBlock->GetSchematicFile();
    }

    if( ( isDrawSheetCopy || isDrawSheetFromDesignBlock ) && !wxFileExists( filename ) )
    {
        wxMessageBox( wxString::Format( _( "File '%s' does not exist." ), filename ) );
        return 0;
    }

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    EESCHEMA_SETTINGS*    cfg = m_frame->eeconfig();
    SCHEMATIC_SETTINGS&   schSettings = m_frame->Schematic().Settings();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    VECTOR2I              cursorPos;
    bool                  startedWithDrag = false;   // Track if initial sheet placement started with a drag

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
                delete sheet;
                sheet = nullptr;
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() && !( isDrawSheetCopy || isDrawSheetFromDesignBlock ) )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        cursorPos = grid.Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
        controls->ForceCursorPosition( true, cursorPos );

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = sheet && evt->IsActivate() && evt->HasPosition()
                                && evt->Matches( aEvent );

        if( evt->IsCancelInteractive() || ( sheet && evt->IsAction( &ACTIONS::undo ) ) )
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
        else if( !sheet && (   evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                            || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick )
                            || evt->IsDrag( BUT_LEFT ) ) )
        {
            SCH_SELECTION& selection = m_selectionTool->GetSelection();

            if( selection.Size() == 1
                    && selection.Front()->Type() == SCH_SHEET_T
                    && selection.Front()->GetBoundingBox().Contains( cursorPos ) )
            {
                if( evt->IsClick( BUT_LEFT ) || evt->IsAction( &ACTIONS::cursorClick ) )
                {
                    // sheet already selected
                    continue;
                }
                else if( evt->IsDblClick( BUT_LEFT ) || evt->IsAction( &ACTIONS::cursorDblClick ) )
                {
                    m_toolMgr->PostAction( SCH_ACTIONS::enterSheet );
                    m_frame->PopTool( aEvent );
                    break;
                }
            }

            m_toolMgr->RunAction( ACTIONS::selectionClear );

            VECTOR2I sheetPos = evt->IsDrag( BUT_LEFT ) ?
                               grid.Align( evt->DragOrigin(), GRID_HELPER_GRIDS::GRID_GRAPHICS ) :
                               cursorPos;

            // Remember whether this sheet was initiated with a drag so we can treat mouse-up as
            // the terminating (second) click.
            startedWithDrag = evt->IsDrag( BUT_LEFT );

            sheet = new SCH_SHEET( m_frame->GetCurrentSheet().Last(), sheetPos );
            sheet->SetScreen( nullptr );

            wxString ext = wxString( "." ) + FILEEXT::KiCadSchematicFileExtension;

            if( isDrawSheetCopy )
            {
                wxFileName fn( filename );

                sheet->GetField( FIELD_T::SHEET_NAME )->SetText( fn.GetName() );
                sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fn.GetName() + ext );
            }
            else if( isDrawSheetFromDesignBlock )
            {
                wxFileName fn( filename );

                sheet->GetField( FIELD_T::SHEET_NAME )->SetText( designBlock->GetLibId().GetLibItemName() );
                sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fn.GetName() + ext );

                std::vector<SCH_FIELD>& sheetFields = sheet->GetFields();

                // Copy default fields into the sheet
                for( const auto& [fieldName, fieldValue] : designBlock->GetFields() )
                {
                    sheetFields.emplace_back( sheet, FIELD_T::USER, fieldName );
                    sheetFields.back().SetText( fieldValue );
                    sheetFields.back().SetVisible( false );
                }
            }
            else
            {
                sheet->GetField( FIELD_T::SHEET_NAME )->SetText( wxT( "Untitled Sheet" ) );
                sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( wxT( "untitled" ) + ext );
            }

            sheet->SetFlags( IS_NEW | IS_MOVING );
            sheet->SetBorderWidth( schIUScale.MilsToIU( cfg->m_Drawing.default_line_thickness ) );
            sheet->SetBorderColor( cfg->m_Drawing.default_sheet_border_color );
            sheet->SetBackgroundColor( cfg->m_Drawing.default_sheet_background_color );
            sizeSheet( sheet, cursorPos );

            SCH_SHEET_LIST hierarchy = m_frame->Schematic().Hierarchy();
            SCH_SHEET_PATH instance = m_frame->GetCurrentSheet();
            instance.push_back( sheet );
            wxString pageNumber;

            // Find the next available page number by checking all existing page numbers
            std::set<int> usedPageNumbers;

            for( const SCH_SHEET_PATH& path : hierarchy )
            {
                wxString existingPageNum = path.GetPageNumber();
                long pageNum = 0;

                if( existingPageNum.ToLong( &pageNum ) && pageNum > 0 )
                    usedPageNumbers.insert( static_cast<int>( pageNum ) );
            }

            // Find the first available number starting from 1
            int nextAvailable = 1;

            while( usedPageNumbers.count( nextAvailable ) > 0 )
                nextAvailable++;

            pageNumber.Printf( wxT( "%d" ), nextAvailable );
            instance.SetPageNumber( pageNumber );

            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
        }
        else if( sheet && (   evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT )
                           || isSyntheticClick
                           || evt->IsAction( &ACTIONS::cursorClick ) || evt->IsAction( &ACTIONS::cursorDblClick )
                           || evt->IsAction( &ACTIONS::finishInteractive )
                           || ( startedWithDrag && evt->IsMouseUp( BUT_LEFT ) ) ) )
        {
            getViewControls()->SetAutoPan( false );
            getViewControls()->CaptureCursor( false );

            if( m_frame->EditSheetProperties( static_cast<SCH_SHEET*>( sheet ), &m_frame->GetCurrentSheet(),
                                              nullptr, nullptr, nullptr, &filename ) )
            {
                m_view->ClearPreview();

                sheet->AutoplaceFields( m_frame->GetScreen(), AUTOPLACE_AUTO );

                // Use the commit we were provided or make our own
                SCH_COMMIT  tempCommit = SCH_COMMIT( m_toolMgr );
                SCH_COMMIT& c = evt->Commit() ? *( (SCH_COMMIT*) evt->Commit() ) : tempCommit;

                // We need to manually add the sheet to the screen otherwise annotation will not be able to find
                // the sheet and its symbols to annotate.
                m_frame->AddToScreen( sheet );
                c.Added( sheet, m_frame->GetScreen() );

                // Refresh the hierarchy so the new sheet and its symbols are found during annotation.
                // The cached hierarchy was built before this sheet was added.
                m_frame->Schematic().RefreshHierarchy();

                // This convoluted logic means we always annotate unless we are drawing a copy/design block
                // and the user has explicitly requested we keep the annotations via checkbox

                if( cfg->m_AnnotatePanel.automatic
                    && !( ( isDrawSheetCopy || isDrawSheetFromDesignBlock )
                          && cfg->m_DesignBlockChooserPanel.keep_annotations ) )
                {
                    // Annotation will remove this from selection, but we add it back later
                    m_selectionTool->AddItemToSel( sheet );

                    NULL_REPORTER reporter;
                    m_frame->AnnotateSymbols( &c,
                                              ANNOTATE_SELECTION,
                                              (ANNOTATE_ORDER_T) schSettings.m_AnnotateSortOrder,
                                              (ANNOTATE_ALGO_T) schSettings.m_AnnotateMethod,
                                              true,   /* recursive */
                                              schSettings.m_AnnotateStartNum,
                                              true,   /* reset */
                                              false,  /* regroup */
                                              false,  /* repair */
                                              reporter );
                }

                if( isDrawSheetFromDesignBlock && cfg->m_DesignBlockChooserPanel.place_as_group )
                {
                    sheetGroup = new SCH_GROUP( m_frame->GetScreen() );
                    sheetGroup->SetName( designBlock->GetLibId().GetLibItemName() );
                    sheetGroup->SetDesignBlockLibId( designBlock->GetLibId() );
                    c.Add( sheetGroup, m_frame->GetScreen() );
                    c.Modify( sheet, m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );
                    sheetGroup->AddItem( sheet );
                }

                c.Push( isDrawSheetCopy ? "Import Sheet Copy" : "Draw Sheet" );

                if( sheetGroup )
                    m_selectionTool->AddItemToSel( sheetGroup );
                else
                    m_selectionTool->AddItemToSel( sheet );

                if( ( isDrawSheetCopy || isDrawSheetFromDesignBlock )
                    && !cfg->m_DesignBlockChooserPanel.repeated_placement )
                {
                    m_frame->PopTool( aEvent );
                    break;
                }
            }
            else
            {
                m_view->ClearPreview();
                delete sheet;
            }

            sheet = nullptr;
        }
        else if( evt->IsAction( &ACTIONS::duplicate )
                || evt->IsAction( &SCH_ACTIONS::repeatDrawItem )
                || evt->IsAction( &ACTIONS::paste ) )
        {
            if( sheet )
            {
                wxBell();
                continue;
            }

            // Exit.  The duplicate/repeat/paste will run in its own loop.
            m_frame->PopTool( aEvent );
            evt->SetPassEvent();
            break;
        }
        else if( sheet && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion()
                           || evt->IsDrag( BUT_LEFT ) ) )
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

            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( sheet && evt->IsAction( &ACTIONS::redo ) )
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


int SCH_DRAWING_TOOLS::doSyncSheetsPins( std::list<SCH_SHEET_PATH> sheetPaths )
{
    if( !sheetPaths.size() )
        return 0;

    m_dialogSyncSheetPin = std::make_unique<DIALOG_SYNC_SHEET_PINS>(
            m_frame, std::move( sheetPaths ),
            std::make_shared<SHEET_SYNCHRONIZATION_AGENT>(
                    [&]( EDA_ITEM* aItem, SCH_SHEET_PATH aPath,
                         SHEET_SYNCHRONIZATION_AGENT::MODIFICATION const& aModify )
                    {
                        SCH_COMMIT commit( m_toolMgr );

                        if( auto pin = dynamic_cast<SCH_SHEET_PIN*>( aItem ) )
                        {
                            commit.Modify( pin->GetParent(), aPath.LastScreen() );
                            aModify();
                            commit.Push( _( "Modify sheet pin" ) );
                        }
                        else
                        {
                            commit.Modify( aItem, aPath.LastScreen() );
                            aModify();
                            commit.Push( _( "Modify schematic item" ) );
                        }

                        updateItem( aItem, true );
                        m_frame->OnModify();
                    },
                    [&]( EDA_ITEM* aItem, SCH_SHEET_PATH aPath )
                    {
                        m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &aPath );
                        SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                        selectionTool->UnbrightenItem( aItem );
                        selectionTool->AddItemToSel( aItem, true );
                        m_toolMgr->RunAction( ACTIONS::doDelete );
                    },
                    [&]( SCH_SHEET* aItem, SCH_SHEET_PATH aPath,
                         SHEET_SYNCHRONIZATION_AGENT::SHEET_SYNCHRONIZATION_PLACEMENT aOp,
                         std::set<EDA_ITEM*> aTemplates )
                    {
                        switch( aOp )
                        {
                        case SHEET_SYNCHRONIZATION_AGENT::PLACE_HIERLABEL:
                        {
                            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
                            m_dialogSyncSheetPin->Hide();
                            m_dialogSyncSheetPin->PreparePlacementTemplate(
                                    sheet, DIALOG_SYNC_SHEET_PINS::PlaceItemKind::HIERLABEL, aTemplates );
                            m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &aPath );
                            m_toolMgr->RunAction( SCH_ACTIONS::placeHierLabel );
                            break;
                        }
                        case SHEET_SYNCHRONIZATION_AGENT::PLACE_SHEET_PIN:
                        {
                            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
                            m_dialogSyncSheetPin->Hide();
                            m_dialogSyncSheetPin->PreparePlacementTemplate(
                                    sheet, DIALOG_SYNC_SHEET_PINS::PlaceItemKind::SHEET_PIN, aTemplates );
                            m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &aPath );
                            m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->SyncSelection( {}, nullptr, { sheet } );
                            m_toolMgr->RunAction( SCH_ACTIONS::placeSheetPin );
                            break;
                        }
                        }
                    },
                    m_toolMgr, m_frame ) );
    m_dialogSyncSheetPin->Show( true );
    return 0;
}


int SCH_DRAWING_TOOLS::SyncSheetsPins( const TOOL_EVENT& aEvent )
{
    SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( m_selectionTool->GetSelection().Front() );

    if( !sheet )
    {
        VECTOR2I cursorPos =  getViewControls()->GetMousePosition();

        if( EDA_ITEM* i = nullptr; static_cast<void>(m_selectionTool->SelectPoint( cursorPos, { SCH_SHEET_T }, &i ) ) , i != nullptr )
        {
            sheet = dynamic_cast<SCH_SHEET*>( i );
        }
    }

    if ( sheet )
    {
        SCH_SHEET_PATH current = m_frame->GetCurrentSheet();
        current.push_back( sheet );
        return doSyncSheetsPins( { current } );
    }

    return 0;
}


int SCH_DRAWING_TOOLS::AutoPlaceAllSheetPins( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( m_selectionTool->GetSelection().Front() );

    if( !sheet )
        return 0;

    std::vector<SCH_HIERLABEL*> labels = importHierLabels( sheet );

    if( labels.empty() )
    {
        m_frame->PushTool( aEvent );
        m_statusPopup = std::make_unique<STATUS_TEXT_POPUP>( m_frame );
        m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
        m_statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
        m_statusPopup->PopupFor( 2000 );
        m_frame->PopTool( aEvent );
        m_toolMgr->RunAction( ACTIONS::selectionClear );
        m_view->ClearPreview();
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    SCH_COMMIT commit( m_toolMgr );
    BOX2I      bbox = sheet->GetBoundingBox();
    VECTOR2I   cursorPos = bbox.GetPosition();
    SCH_ITEM*  lastPlacedLabel = nullptr;

    auto calculatePositionForLabel =
            [&]( const SCH_ITEM* lastLabel, const SCH_HIERLABEL* currentLabel ) -> VECTOR2I
            {
                if( !lastLabel )
                    return cursorPos;

                int lastX = lastLabel->GetPosition().x;
                int lastY = lastLabel->GetPosition().y;
                int lastWidth = lastLabel->GetBoundingBox().GetWidth();
                int lastHeight = lastLabel->GetBoundingBox().GetHeight();

                int currentWidth = currentLabel->GetBoundingBox().GetWidth();
                int currentHeight = currentLabel->GetBoundingBox().GetHeight();

                // If there is enough space, place the label to the right of the last placed label
                if( ( lastX + lastWidth + currentWidth ) <= ( bbox.GetPosition().x + bbox.GetSize().x ) )
                    return { lastX + lastWidth, lastY };

                // If not enough space to the right, move to the next row if vertical space allows
                if( ( lastY + lastHeight + currentHeight ) <= ( bbox.GetPosition().y + bbox.GetSize().y ) )
                    return { bbox.GetPosition().x, lastY + lastHeight };

                return cursorPos;
            };

    for( SCH_HIERLABEL* label : labels )
    {
        if( !lastPlacedLabel )
        {
            std::vector<SCH_SHEET_PIN*> existingPins = sheet->GetPins();

            if( !existingPins.empty() )
            {
                std::sort( existingPins.begin(), existingPins.end(),
                           []( const SCH_ITEM* a, const SCH_ITEM* b )
                           {
                               return ( a->GetPosition().x < b->GetPosition().x )
                                      || ( a->GetPosition().x == b->GetPosition().x
                                           && a->GetPosition().y < b->GetPosition().y );
                           } );

                lastPlacedLabel = existingPins.back();
            }
        }

        cursorPos = calculatePositionForLabel( lastPlacedLabel, label );
        SCH_ITEM* item = createNewSheetPinFromLabel( sheet, cursorPos, label );

        if( item )
        {
            item->SetFlags( IS_NEW | IS_MOVING );
            item->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
            item->ClearFlags( IS_MOVING );

            if( item->IsConnectable() )
                m_frame->AutoRotateItem( m_frame->GetScreen(), item );

            commit.Modify( sheet, m_frame->GetScreen() );

            sheet->AddPin( static_cast<SCH_SHEET_PIN*>( item ) );
            item->AutoplaceFields( m_frame->GetScreen(), AUTOPLACE_AUTO );

            commit.Push( _( "Add Sheet Pin" ) );

            lastPlacedLabel = item;
        }
    }

    return 0;
}


int SCH_DRAWING_TOOLS::SyncAllSheetsPins( const TOOL_EVENT& aEvent )
{
    static const std::function<void( std::list<SCH_SHEET_PATH>&, SCH_SCREEN*, std::set<SCH_SCREEN*>&,
                                     SCH_SHEET_PATH const& )> getSheetChildren =
            []( std::list<SCH_SHEET_PATH>& aPaths, SCH_SCREEN* aScene, std::set<SCH_SCREEN*>& aVisited,
                SCH_SHEET_PATH const& aCurPath )
            {
                if( ! aScene || aVisited.find(aScene) != aVisited.end() )
                    return ;

                std::vector<SCH_ITEM*> sheetChildren;
                aScene->GetSheets( &sheetChildren );
                aVisited.insert( aScene );

                for( SCH_ITEM* child : sheetChildren )
                {
                    SCH_SHEET_PATH cp = aCurPath;
                    SCH_SHEET* sheet = static_cast<SCH_SHEET*>( child );
                    cp.push_back( sheet );
                    aPaths.push_back( cp );
                    getSheetChildren( aPaths, sheet->GetScreen(), aVisited, cp );
                }
            };

    std::list<SCH_SHEET_PATH> sheetPaths;
    std::set<SCH_SCREEN*> visited;

    // Build sheet paths for each top-level sheet (don't include virtual root in paths)
    std::vector<SCH_SHEET*> topLevelSheets = m_frame->Schematic().GetTopLevelSheets();

    for( SCH_SHEET* topSheet : topLevelSheets )
    {
        if( topSheet && topSheet->GetScreen() )
        {
            SCH_SHEET_PATH current;
            current.push_back( topSheet );
            getSheetChildren( sheetPaths, topSheet->GetScreen(), visited, current );
        }
    }

    if( sheetPaths.size() == 0 )
    {
        m_frame->ShowInfoBarMsg( _( "No sub schematic found in the current project" ) );
        return 0;
    }


    return doSyncSheetsPins( std::move( sheetPaths ) );
}

SCH_HIERLABEL* SCH_DRAWING_TOOLS::importHierLabel( SCH_SHEET* aSheet )
{
    if( !aSheet->GetScreen() )
        return nullptr;

    std::vector<SCH_HIERLABEL*> labels;

    for( EDA_ITEM* item : aSheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );
        labels.push_back( label );
    }

    std::sort( labels.begin(), labels.end(),
               []( const SCH_HIERLABEL* label1, const SCH_HIERLABEL* label2 )
               {
                   return StrNumCmp( label1->GetText(), label2->GetText(), true ) < 0;
               } );

    for( SCH_HIERLABEL* label : labels )
    {
        if( !aSheet->HasPin( label->GetText() ) )
            return label;
    }

    return nullptr;
}


std::vector<SCH_HIERLABEL*> SCH_DRAWING_TOOLS::importHierLabels( SCH_SHEET* aSheet )
{
    if( !aSheet->GetScreen() )
        return {};

    std::vector<SCH_HIERLABEL*> labels;

    for( EDA_ITEM* item : aSheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

        if( !aSheet->HasPin( label->GetText() ) )
            labels.push_back( label );
    }

    return labels;
}


void SCH_DRAWING_TOOLS::setTransitions()
{
    // clang-format off
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,           SCH_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,           SCH_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceNextSymbolUnit,   SCH_ACTIONS::placeNextSymbolUnit.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,      SCH_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,      SCH_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,      SCH_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,         SCH_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,         SCH_ACTIONS::placeClassLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,         SCH_ACTIONS::placeHierLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,         SCH_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,             SCH_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,             SCH_ACTIONS::drawSheetFromFile.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,             SCH_ACTIONS::drawSheetFromDesignBlock.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,         SCH_ACTIONS::placeSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::ImportSheet,           SCH_ACTIONS::placeDesignBlock.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::ImportSheet,           SCH_ACTIONS::importSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,         SCH_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,             SCH_ACTIONS::drawRectangle.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,             SCH_ACTIONS::drawCircle.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,             SCH_ACTIONS::drawArc.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,             SCH_ACTIONS::drawBezier.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawShape,             SCH_ACTIONS::drawTextBox.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawRuleArea,          SCH_ACTIONS::drawRuleArea.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawTable,             SCH_ACTIONS::drawTable.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceImage,            SCH_ACTIONS::placeImage.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::ImportGraphics,        SCH_ACTIONS::importGraphics.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::ImportGraphics,        SCH_ACTIONS::ddImportGraphics.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SyncSheetsPins,        SCH_ACTIONS::syncSheetPins.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SyncAllSheetsPins,     SCH_ACTIONS::syncAllSheetsPins.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AutoPlaceAllSheetPins, SCH_ACTIONS::autoplaceAllSheetPins.MakeEvent() );
    // clang-format on
}
