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

#include <bitmaps.h>
#include <core/typeinfo.h>
#include <core/kicad_algo.h>
#include <ee_actions.h>
#include <ee_collectors.h>
#include <ee_selection_tool.h>
#include <eeschema_id.h> // For MAX_SELECT_ITEM_IDS
#include <symbol_edit_frame.h>
#include <lib_item.h>
#include <symbol_viewer_frame.h>
#include <math/util.h>
#include <geometry/shape_rect.h>
#include <menus_helpers.h>
#include <painter.h>
#include <preview_items/selection_area.h>
#include <sch_base_frame.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <sch_edit_frame.h>
#include <sch_item.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <trigo.h>
#include <view/view.h>
#include <view/view_controls.h>

SELECTION_CONDITION EE_CONDITIONS::SingleSymbol = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aSel.Front() );

        if( symbol )
            return !symbol->GetLibSymbolRef() || !symbol->GetLibSymbolRef()->IsPower();
    }

    return false;
};


SELECTION_CONDITION EE_CONDITIONS::SingleSymbolOrPower = []( const SELECTION& aSel )
{
    return aSel.GetSize() == 1 && aSel.Front()->Type() == SCH_SYMBOL_T;
};


SELECTION_CONDITION EE_CONDITIONS::SingleDeMorganSymbol = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aSel.Front() );

        if( symbol )
            return symbol->GetLibSymbolRef() && symbol->GetLibSymbolRef()->HasConversion();
    }

    return false;
};


SELECTION_CONDITION EE_CONDITIONS::SingleMultiUnitSymbol = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aSel.Front() );

        if( symbol )
            return symbol->GetLibSymbolRef() && symbol->GetLibSymbolRef()->GetUnitCount() >= 2;
    }

    return false;
};


#define HITTEST_THRESHOLD_PIXELS 5


EE_SELECTION_TOOL::EE_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveSelection" ),
        m_frame( nullptr ),
        m_nonModifiedCursor( KICURSOR::ARROW ),
        m_isSymbolEditor( false ),
        m_isSymbolViewer( false ),
        m_unit( 0 ),
        m_convert( 0 )
{
    m_selection.Clear();
}


EE_SELECTION_TOOL::~EE_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
}


using E_C = EE_CONDITIONS;


bool EE_SELECTION_TOOL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();

    SYMBOL_VIEWER_FRAME* symbolViewerFrame = dynamic_cast<SYMBOL_VIEWER_FRAME*>( m_frame );
    SYMBOL_EDIT_FRAME*   symbolEditorFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );

    if( symbolEditorFrame )
    {
        m_isSymbolEditor = true;
        m_unit = symbolEditorFrame->GetUnit();
        m_convert = symbolEditorFrame->GetConvert();
    }
    else
    {
        m_isSymbolViewer = symbolViewerFrame != nullptr;
    }

    static KICAD_T wireOrBusTypes[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };
    static KICAD_T connectedTypes[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T,
                                        SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, SCH_LABEL_T,
                                        SCH_SHEET_PIN_T, SCH_PIN_T, EOT };

    auto wireSelection =      E_C::MoreThan( 0 ) && E_C::OnlyType( SCH_LINE_LOCATE_WIRE_T );
    auto busSelection =       E_C::MoreThan( 0 ) && E_C::OnlyType( SCH_LINE_LOCATE_BUS_T );
    auto wireOrBusSelection = E_C::MoreThan( 0 ) && E_C::OnlyTypes( wireOrBusTypes );
    auto connectedSelection = E_C::MoreThan( 0 ) && E_C::OnlyTypes( connectedTypes );
    auto sheetSelection =     E_C::Count( 1 )    && E_C::OnlyType( SCH_SHEET_T );

    auto schEditSheetPageNumberCondition =
            [&] ( const SELECTION& aSel )
            {
                if( m_isSymbolEditor || m_isSymbolViewer )
                    return false;

                return E_C::LessThan( 2 )( aSel ) && E_C::OnlyType( SCH_SHEET_T )( aSel );
            };

    auto schEditCondition =
            [this] ( const SELECTION& aSel )
            {
                return !m_isSymbolEditor && !m_isSymbolViewer;
            };

    auto belowRootSheetCondition =
            [&]( const SELECTION& aSel )
            {
                SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

                return editFrame
                        && editFrame->GetCurrentSheet().Last() != &editFrame->Schematic().Root();
            };

    auto haveSymbolCondition =
            [&]( const SELECTION& sel )
            {
                return m_isSymbolEditor &&
                       static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();
            };

    auto& menu = m_menu.GetMenu();

    menu.AddItem( EE_ACTIONS::enterSheet,         sheetSelection && EE_CONDITIONS::Idle, 1 );
    menu.AddItem( EE_ACTIONS::explicitCrossProbe, sheetSelection && EE_CONDITIONS::Idle, 1 );
    menu.AddItem( EE_ACTIONS::leaveSheet,         belowRootSheetCondition, 1 );

    menu.AddSeparator( 100 );
    menu.AddItem( EE_ACTIONS::drawWire,           schEditCondition && EE_CONDITIONS::Empty, 100 );
    menu.AddItem( EE_ACTIONS::drawBus,            schEditCondition && EE_CONDITIONS::Empty, 100 );

    menu.AddSeparator( 100 );
    menu.AddItem( EE_ACTIONS::finishWire,         SCH_LINE_WIRE_BUS_TOOL::IsDrawingWire, 100 );

    menu.AddSeparator( 100 );
    menu.AddItem( EE_ACTIONS::finishBus,          SCH_LINE_WIRE_BUS_TOOL::IsDrawingBus, 100 );

    menu.AddSeparator( 200 );
    menu.AddItem( EE_ACTIONS::selectConnection,   wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeJunction,      wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeLabel,         wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeGlobalLabel,   wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeHierLabel,     wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::breakWire,          wireSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::breakBus,           busSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::importSingleSheetPin, sheetSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::assignNetclass,     connectedSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::editPageNumber,     schEditSheetPageNumberCondition, 250 );

    menu.AddSeparator( 400 );
    menu.AddItem( EE_ACTIONS::symbolProperties,
                  haveSymbolCondition && EE_CONDITIONS::Empty, 400 );
    menu.AddItem( EE_ACTIONS::pinTable,
                  haveSymbolCondition && EE_CONDITIONS::Empty, 400 );

    menu.AddSeparator( 1000 );
    m_frame->AddStandardSubMenus( m_menu );

    m_disambiguateTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( EE_SELECTION_TOOL::onDisambiguationExpire ), nullptr, this );

    return true;
}


void EE_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();

    if( aReason == TOOL_BASE::MODEL_RELOAD )
    {
        // Remove pointers to the selected items from containers without changing their
        // properties (as they are already deleted while a new sheet is loaded)
        m_selection.Clear();
        getView()->GetPainter()->GetSettings()->SetHighlight( false );

        SYMBOL_EDIT_FRAME*   symbolEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        SYMBOL_VIEWER_FRAME* symbolViewerFrame = dynamic_cast<SYMBOL_VIEWER_FRAME*>( m_frame );

        if( symbolEditFrame )
        {
            m_isSymbolEditor = true;
            m_unit = symbolEditFrame->GetUnit();
            m_convert = symbolEditFrame->GetConvert();
        }
        else
            m_isSymbolViewer = symbolViewerFrame != nullptr;
    }
    else
        // Restore previous properties of selected items and remove them from containers
        ClearSelection();

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( &m_selection );
    getView()->Add( &m_selection );
}


int EE_SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );

    if( conditionalMenu )
        conditionalMenu->Evaluate( m_selection );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


const KICAD_T movableSchematicItems[] =
{
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_LINE_T,
    SCH_BITMAP_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    EOT
};


const KICAD_T movableSymbolItems[] =
{
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,
    LIB_FIELD_T,
    EOT
};


const KICAD_T movableSymbolAliasItems[] =
{
    LIB_FIELD_T,
    EOT
};


int EE_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    KIID lastRolloverItem = niluuid;

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        bool displayWireCursor = false;
        bool displayBusCursor = false;
        bool displayLineCursor = false;
        KIID rolloverItem = lastRolloverItem;

        // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
        setModifiersState( evt->Modifier( MD_SHIFT ), evt->Modifier( MD_CTRL ),
                           evt->Modifier( MD_ALT ) );

        bool modifier_enabled = m_subtractive || m_additive || m_exclusive_or;

        MOUSE_DRAG_ACTION drag_action = m_frame->GetDragAction();
        EE_GRID_HELPER grid( m_toolMgr );

        if( evt->IsMouseDown( BUT_LEFT ) )
        {
            // Avoid triggering when running under other tools
            if( m_frame->ToolStackIsEmpty() )
            {
                m_originalCursor = m_toolMgr->GetMousePosition();
                m_disambiguateTimer.StartOnce( 500 );
            }
        }
        // Single click? Select single object
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // If the timer has stopped, then we have already run the disambiguate routine
            // and we don't want to register an extra click here
            if( !m_disambiguateTimer.IsRunning() )
            {
                evt->SetPassEvent();
                continue;
            }

            m_disambiguateTimer.Stop();


            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            EE_COLLECTOR collector;
            bool continueSelect = true;

            // Collect items at the clicked location (doesn't select them yet)
            if( CollectHits( collector, evt->Position()) )
            {
                narrowSelection( collector, evt->Position(), false );

                if( collector.GetCount() == 1 && !m_isSymbolEditor && !modifier_enabled )
                {
                    // Check if we want to auto start wires
                    VECTOR2I snappedCursorPos = grid.BestSnapAnchor( evt->Position(),
                                                                     LAYER_CONNECTABLE, nullptr );

                    if( m_frame->eeconfig()->m_Drawing.auto_start_wires
                            && collector[0]->IsPointClickableAnchor( (wxPoint) snappedCursorPos ) )
                    {
                        OPT_TOOL_EVENT newEvt;
                        SCH_CONNECTION* connection = collector[0]->Connection();

                        if( ( connection && ( connection->IsNet() || connection->IsUnconnected() ) )
                            || collector[0]->Type() == SCH_SYMBOL_T )
                        {
                            newEvt = EE_ACTIONS::drawWire.MakeEvent();
                        }
                        else if( connection && connection->IsBus() )
                        {
                            newEvt = EE_ACTIONS::drawBus.MakeEvent();
                        }
                        else if( collector[0]->Type() == SCH_LINE_T
                                 && static_cast<SCH_LINE*>( collector[0] )->IsGraphicLine() )
                        {
                            newEvt = EE_ACTIONS::drawLines.MakeEvent();
                        }

                        auto* params = newEvt->Parameter<DRAW_SEGMENT_EVENT_PARAMS*>();
                        auto* newParams = new DRAW_SEGMENT_EVENT_PARAMS();

                        *newParams= *params;
                        newParams->quitOnDraw = true;
                        newEvt->SetParameter( newParams );

                        getViewControls()->ForceCursorPosition( true, snappedCursorPos );
                        newEvt->SetMousePosition( snappedCursorPos );
                        newEvt->SetHasPosition( true );
                        newEvt->SetForceImmediate( true );
                        m_toolMgr->ProcessEvent( *newEvt );

                        continueSelect = false;
                    }
                    else if( collector[0]->IsHypertext() )
                    {
                        collector[0]->DoHypertextMenu( m_frame );
                        continueSelect = false;
                    }
                }
            }

            if( continueSelect )
            {
                // If we didn't click on an anchor, we perform a normal select, pass in the
                // items we previously collected
                selectPoint( collector, nullptr, nullptr, m_additive, m_subtractive,
                             m_exclusive_or );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_disambiguateTimer.Stop();

            // right click? if there is any object - show the context menu
            bool selectionCancelled = false;

            if( m_selection.Empty() )
            {
                ClearSelection();
                SelectPoint( evt->Position(), EE_COLLECTOR::AllItems, nullptr,
                             &selectionCancelled );
                m_selection.SetIsHover( true );
            }
            // If the cursor has moved off the bounding box of the selection by more than
            // a grid square, check to see if there is another item available for selection
            // under the cursor.  If there is, the user likely meant to get the context menu
            // for that item.  If there is no new item, then keep the original selection and
            // show the context menu for it.
            else if( !m_selection.GetBoundingBox().Inflate(
                        grid.GetGrid().x, grid.GetGrid().y ).Contains(
                                (wxPoint) evt->Position() ) )
            {
                EE_SELECTION saved_selection = m_selection;

                for( const auto& item : saved_selection )
                    RemoveItemFromSel( item, true );

                SelectPoint( evt->Position(), EE_COLLECTOR::AllItems, nullptr,
                             &selectionCancelled );

                if( m_selection.Empty() )
                {
                    m_selection.SetIsHover( false );

                    for( const auto& item : saved_selection )
                        AddItemToSel( item,  true);
                }
                else
                {
                    m_selection.SetIsHover( true );
                }
            }

            if( !selectionCancelled )
                m_menu.ShowContextMenu( m_selection );
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            m_disambiguateTimer.Stop();

            // double click? Display the properties window
            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            if( m_selection.Empty() )
                SelectPoint( evt->Position() );

            EDA_ITEM* item = m_selection.Front();

            if( item && item->Type() == SCH_SHEET_T )
                m_toolMgr->RunAction( EE_ACTIONS::enterSheet );
            else
                m_toolMgr->RunAction( EE_ACTIONS::properties );
        }
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            m_disambiguateTimer.Stop();

            // Middle double click?  Do zoom to fit or zoom to objects
            if( evt->Modifier( MD_CTRL ) ) // Is CTRL key down?
                m_toolMgr->RunAction( ACTIONS::zoomFitObjects, true );
            else
                m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
        }
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            m_disambiguateTimer.Stop();

            // Is another tool already moving a new object?  Don't allow a drag start
            if( !m_selection.Empty() && m_selection[0]->HasFlag( IS_NEW | IS_MOVING ) )
            {
                evt->SetPassEvent();
                continue;
            }

            // drag with LMB? Select multiple objects (or at least draw a selection box) or
            // drag them
            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            if( modifier_enabled || drag_action == MOUSE_DRAG_ACTION::SELECT )
            {
                selectMultiple();
            }
            else if( m_selection.Empty() && drag_action != MOUSE_DRAG_ACTION::DRAG_ANY )
            {
                selectMultiple();
            }
            else
            {
                if( m_isSymbolEditor )
                {
                    if( static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->IsSymbolAlias() )
                        m_selection = RequestSelection( movableSymbolAliasItems );
                    else
                        m_selection = RequestSelection( movableSymbolItems );
                }
                else
                {
                    m_selection = RequestSelection( movableSchematicItems );
                }

                // Check if dragging has started within any of selected items bounding box
                if( selectionContains( evt->Position() ) )
                {
                    // Yes -> run the move tool and wait till it finishes
                    if( m_isSymbolEditor )
                        m_toolMgr->InvokeTool( "eeschema.SymbolMoveTool" );
                    else
                        m_toolMgr->InvokeTool( "eeschema.InteractiveMove" );
                }
                else
                {
                    // No -> drag a selection box
                    selectMultiple();
                }
            }
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            m_disambiguateTimer.Stop();

            // context sub-menu selection?  Handle unit selection or bus unfolding
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_SYM_MAX )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selection.Front() );
                int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( symbol )
                    static_cast<SCH_EDIT_FRAME*>( m_frame )->SelectUnit( symbol, unit );
            }
            else if( evt->GetCommandId().get() >= ID_POPUP_SCH_UNFOLD_BUS
                     && evt->GetCommandId().get() <= ID_POPUP_SCH_UNFOLD_BUS_END )
            {
                wxString* net = new wxString( *evt->Parameter<wxString*>() );
                m_toolMgr->RunAction( EE_ACTIONS::unfoldBus, true, net );
            }

        }
        else if( evt->IsCancelInteractive() )
        {
            m_disambiguateTimer.Stop();

            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            ClearSelection();
        }
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            ClearSelection();
        }
        else if( evt->IsMotion() && !m_isSymbolEditor && m_frame->ToolStackIsEmpty() )
        {
            rolloverItem = niluuid;
            EE_COLLECTOR collector;

            // We are checking if we should display a pencil when hovering over anchors
            // for "auto starting" wires when clicked
            if( CollectHits( collector, evt->Position() ) )
            {
                narrowSelection( collector, evt->Position(), false );

                if( collector.GetCount() == 1 && !modifier_enabled )
                {
                    VECTOR2I snappedCursorPos = grid.BestSnapAnchor( evt->Position(),
                                                                     LAYER_CONNECTABLE, nullptr );

                    if( m_frame->eeconfig()->m_Drawing.auto_start_wires
                            && collector[0]->IsPointClickableAnchor( (wxPoint) snappedCursorPos ) )
                    {
                        SCH_CONNECTION* connection = collector[0]->Connection();

                        if( ( connection && ( connection->IsNet() || connection->IsUnconnected() ) )
                            || collector[0]->Type() == SCH_SYMBOL_T )
                        {
                            displayWireCursor = true;
                        }
                        else if( connection && connection->IsBus() )
                        {
                            displayBusCursor = true;
                        }
                        else if( collector[0]->Type() == SCH_LINE_T
                                 && static_cast<SCH_LINE*>( collector[0] )->IsGraphicLine() )
                        {
                            displayLineCursor = true;
                        }

                        getViewControls()->ForceCursorPosition( true, snappedCursorPos );
                    }
                    else if( collector[0]->IsHypertext()
                                && !collector[0]->IsSelected()
                                && !m_additive && !m_subtractive && !m_exclusive_or )
                    {
                        rolloverItem = collector[0]->m_Uuid;
                    }
                }
            }
            else
            {
                getViewControls()->ForceCursorPosition( false );
            }
        }
        else
        {
            evt->SetPassEvent();
        }

        if( rolloverItem != lastRolloverItem )
        {
            EDA_ITEM* item = m_frame->GetItem( lastRolloverItem );

            if( item  )
            {
                item->ClearFlags( IS_ROLLOVER );
                lastRolloverItem = niluuid;

                if( item->Type() == SCH_FIELD_T )
                    m_frame->GetCanvas()->GetView()->Update( item->GetParent() );
                else
                    m_frame->GetCanvas()->GetView()->Update( item );
            }

            item = m_frame->GetItem( rolloverItem );

            if( item )
            {
                item->SetFlags( IS_ROLLOVER );
                lastRolloverItem = rolloverItem;

                if( item->Type() == SCH_FIELD_T )
                    m_frame->GetCanvas()->GetView()->Update( item->GetParent() );
                else
                    m_frame->GetCanvas()->GetView()->Update( item );
            }
        }

        if( m_frame->ToolStackIsEmpty() )
        {
            if( displayWireCursor )
            {
                m_nonModifiedCursor = KICURSOR::LINE_WIRE_ADD;
            }
            else if( displayBusCursor )
            {
                m_nonModifiedCursor = KICURSOR::LINE_BUS;
            }
            else if( displayLineCursor )
            {
                m_nonModifiedCursor = KICURSOR::LINE_GRAPHIC;
            }
            else if( rolloverItem != niluuid )
            {
                m_nonModifiedCursor = KICURSOR::HAND;
            }
            else if( !m_selection.Empty()
                        && drag_action == MOUSE_DRAG_ACTION::DRAG_SELECTED
                        && evt->HasPosition()
                        && selectionContains( evt->Position() ) ) //move/drag option prediction
            {
                m_nonModifiedCursor = KICURSOR::MOVING;
            }
            else
            {
                m_nonModifiedCursor = KICURSOR::ARROW;
            }
        }
    }

    m_disambiguateTimer.Stop();

    // Shutting down; clear the selection
    m_selection.Clear();

    return 0;
}


int EE_SELECTION_TOOL::disambiguateCursor( const TOOL_EVENT& aEvent )
{
    VECTOR2I pos = m_toolMgr->GetMousePosition();

    if( pos != m_originalCursor )
        return 0;

    m_skip_heuristics = true;
    SelectPoint( pos, EE_COLLECTOR::AllItems, nullptr, &m_canceledMenu, false, m_additive,
            m_subtractive, m_exclusive_or );
    m_skip_heuristics = false;

    return 0;
}


void EE_SELECTION_TOOL::onDisambiguationExpire( wxTimerEvent& aEvent )
{
    m_toolMgr->ProcessEvent( EVENTS::DisambiguatePoint );
}


void EE_SELECTION_TOOL::OnIdle( wxIdleEvent& aEvent )
{
    if( m_frame->ToolStackIsEmpty() && !m_multiple )
    {
        wxMouseState keyboardState = wxGetMouseState();

        setModifiersState( keyboardState.ShiftDown(), keyboardState.ControlDown(),
                           keyboardState.AltDown() );

        if( m_additive )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ADD );
        else if( m_subtractive )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SUBTRACT );
        else if( m_exclusive_or )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::XOR );
        else
            m_frame->GetCanvas()->SetCurrentCursor( m_nonModifiedCursor );
    }
}


EE_SELECTION& EE_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


bool EE_SELECTION_TOOL::CollectHits( EE_COLLECTOR& aCollector, const VECTOR2I& aWhere,
                                     const KICAD_T* aFilterList )
{
    int pixelThreshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
    int gridThreshold = KiROUND( getView()->GetGAL()->GetGridSize().EuclideanNorm() );
    aCollector.m_Threshold = std::max( pixelThreshold, gridThreshold );

    if( m_isSymbolEditor )
    {
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return false;

        aCollector.Collect( symbol->GetDrawItems(), aFilterList, (wxPoint) aWhere, m_unit,
                            m_convert );
    }
    else
    {
        aCollector.Collect( m_frame->GetScreen(), aFilterList, (wxPoint) aWhere, m_unit,
                            m_convert );
    }

    return aCollector.GetCount() > 0;
}


void EE_SELECTION_TOOL::narrowSelection( EE_COLLECTOR& collector, const VECTOR2I& aWhere,
                                         bool aCheckLocked )
{
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !Selectable( collector[i], &aWhere ) )
        {
            collector.Remove( i );
            continue;
        }

        if( aCheckLocked && collector[i]->IsLocked() )
        {
            collector.Remove( i );
            continue;
        }

        // SelectPoint, unlike other selection routines, can select line ends
        if( collector[i]->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = (SCH_LINE*) collector[i];
            line->ClearFlags( STARTPOINT | ENDPOINT );

            if( HitTestPoints( line->GetStartPoint(), (wxPoint) aWhere, collector.m_Threshold ) )
                line->SetFlags( STARTPOINT );
            else if( HitTestPoints( line->GetEndPoint(), (wxPoint) aWhere, collector.m_Threshold ) )
                line->SetFlags( ENDPOINT );
            else
                line->SetFlags( STARTPOINT | ENDPOINT );
        }
    }

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
    {
        GuessSelectionCandidates( collector, aWhere );
    }
}


bool EE_SELECTION_TOOL::selectPoint( EE_COLLECTOR& aCollector, EDA_ITEM** aItem,
                                     bool* aSelectionCancelledFlag, bool aAdd, bool aSubtract,
                                     bool aExclusiveOr )
{
    m_selection.ClearReferencePoint();

    // If still more than one item we're going to have to ask the user.
    if( aCollector.GetCount() > 1 )
    {
        // Try to call selectionMenu via RunAction() to avoid event-loop contention
        // But it we cannot handle the event, then we don't have an active tool loop, so
        // handle it directly.
        if( !m_toolMgr->RunAction( EE_ACTIONS::selectionMenu, true, &aCollector ) )
        {
            if( !doSelectionMenu( &aCollector ) )
                aCollector.m_MenuCancelled = true;
        }

        if( aCollector.m_MenuCancelled )
        {
            if( aSelectionCancelledFlag )
                *aSelectionCancelledFlag = true;

            return false;
        }
    }

    if( !aAdd && !aSubtract && !aExclusiveOr )
        ClearSelection();

    bool anyAdded      = false;
    bool anySubtracted = false;

    if( aCollector.GetCount() > 0 )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( aSubtract || ( aExclusiveOr && aCollector[i]->IsSelected() ) )
            {
                unselect( aCollector[i] );
                anySubtracted = true;
            }
            else
            {
                select( aCollector[i] );
                anyAdded = true;
            }
        }
    }

    if( anyAdded )
    {
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

        if( aItem && aCollector.GetCount() == 1 )
            *aItem = aCollector[0];

        return true;
    }
    else if( anySubtracted )
    {
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
        return true;
    }

    return false;
}


bool EE_SELECTION_TOOL::SelectPoint( const VECTOR2I& aWhere, const KICAD_T* aFilterList,
                                     EDA_ITEM** aItem, bool* aSelectionCancelledFlag,
                                     bool aCheckLocked, bool aAdd, bool aSubtract,
                                     bool aExclusiveOr )
{
    EE_COLLECTOR collector;

    if( !CollectHits( collector, aWhere, aFilterList ) )
        return false;

    narrowSelection( collector, aWhere, aCheckLocked );

    return selectPoint( collector, aItem, aSelectionCancelledFlag, aAdd, aSubtract, aExclusiveOr );
}


int EE_SELECTION_TOOL::SelectAll( const TOOL_EVENT& aEvent )
{
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    // hold all visible items
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> sheetPins;

    // Filter the view items based on the selection box
    BOX2I selectionBox;

    selectionBox.SetMaximum();
    view->Query( selectionBox, selectedItems );         // Get the list of selected items

    // Sheet pins aren't in the view; add them by hand
    for( KIGFX::VIEW::LAYER_ITEM_PAIR& pair : selectedItems )
    {
        SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( pair.first );

        if( sheet )
        {
            int layer = pair.second;

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                sheetPins.emplace_back( KIGFX::VIEW::LAYER_ITEM_PAIR( pin, layer ) );
        }
    }

    selectedItems.insert( selectedItems.end(), sheetPins.begin(), sheetPins.end() );

    for( const std::pair<KIGFX::VIEW_ITEM*, int>& item_pair : selectedItems )
    {
        if( EDA_ITEM* item = dynamic_cast<EDA_ITEM*>( item_pair.first ) )
        {
            if( Selectable( item ) )
                select( item );
        }
    }

    m_multiple = false;

    return 0;
}


void EE_SELECTION_TOOL::GuessSelectionCandidates( EE_COLLECTOR& collector, const VECTOR2I& aPos )
{
    // Prefer exact hits to sloppy ones
    std::set<EDA_ITEM*> exactHits;

    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        EDA_ITEM* item = collector[ i ];
        SCH_LINE* line = dynamic_cast<SCH_LINE*>( item );

        // Lines are hard to hit.  Give them a bit more slop to still be considered "exact".

        if( line )
        {
            if( line->HitTest( (wxPoint) aPos, Mils2iu( DANGLING_SYMBOL_SIZE ) ) )
                exactHits.insert( line );
        }
        else
        {
            if( item->HitTest( (wxPoint) aPos, 0 ) )
                exactHits.insert( item );
        }
    }

    if( exactHits.size() > 0 && exactHits.size() < (unsigned) collector.GetCount() )
    {
        for( int i = collector.GetCount() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = collector[ i ];

            if( !exactHits.count( item ) )
                collector.Transfer( item );
        }
    }

    // Find the closest item.  (Note that at this point all hits are either exact or non-exact.)
    EDA_ITEM* closest = nullptr;
    int       closestDist = INT_MAX;

    for( EDA_ITEM* item : collector )
    {
        EDA_RECT bbox = item->GetBoundingBox();
        int      dist;

        if( exactHits.count( item ) )
        {
            wxPoint   pos( aPos );
            SCH_LINE* line = dynamic_cast<SCH_LINE*>( item );

            if( line )
                dist = DistanceLinePoint( line->GetStartPoint(), line->GetEndPoint(), pos );
            else
                dist = EuclideanNorm( bbox.GetCenter() - pos ) * 2;
        }
        else
        {
            SHAPE_RECT rect( bbox.GetPosition(), bbox.GetWidth(), bbox.GetHeight() );
            rect.Collide( SEG( aPos, aPos ), collector.m_Threshold, &dist );
        }

        if( item->IsType( EE_COLLECTOR::FieldOwners ) )
            dist += INT_MAX / 4;

        if( dist < closestDist )
        {
            closestDist = dist;
            closest = item;
        }
    }

    // Construct a tight box (1/2 height and width) around the center of the closest item.
    // All items which exist at least partly outside this box have sufficient other areas
    // for selection and can be dropped.
    if( closest ) // Don't try and get a tight bbox if nothing is near the mouse pointer
    {
        EDA_RECT tightBox = closest->GetBoundingBox();
        tightBox.Inflate( -tightBox.GetWidth() / 4, -tightBox.GetHeight() / 4 );

        for( int i = collector.GetCount() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = collector[i];

            if( item == closest )
                continue;

            if( !item->HitTest( tightBox, true ) )
                collector.Transfer( item );
        }
    }
}


EE_SELECTION& EE_SELECTION_TOOL::RequestSelection( const KICAD_T aFilterList[] )
{
    if( m_selection.Empty() )
    {
        VECTOR2D cursorPos = getViewControls()->GetCursorPosition( true );

        ClearSelection();
        SelectPoint( cursorPos, aFilterList );
        m_selection.SetIsHover( true );
        m_selection.ClearReferencePoint();
    }
    else        // Trim an existing selection by aFilterList
    {
        for( int i = (int) m_selection.GetSize() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = (EDA_ITEM*) m_selection.GetItem( i );

            if( !item->IsType( aFilterList ) )
            {
                unselect( item );
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
            }
        }
    }

    updateReferencePoint();

    return m_selection;
}


void EE_SELECTION_TOOL::updateReferencePoint()
{
    VECTOR2I refP( 0, 0 );

    if( m_selection.Size() > 0 )
    {
        if( m_isSymbolEditor )
            refP = static_cast<LIB_ITEM*>( m_selection.GetTopLeftItem() )->GetPosition();
        else
            refP = static_cast<SCH_ITEM*>( m_selection.GetTopLeftItem() )->GetPosition();
    }

    m_selection.SetReferencePoint( refP );
}


bool EE_SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool canceled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    while( TOOL_EVENT* evt = Wait() )
    {
        int width = area.GetEnd().x - area.GetOrigin().x;

        /* Selection mode depends on direction of drag-selection:
         * Left > Right : Select objects that are fully enclosed by selection
         * Right > Left : Select objects that are crossed by selection
         */
        bool windowSelection = width >= 0;

        if( view->IsMirroredX() )
            windowSelection = !windowSelection;

        m_frame->GetCanvas()->SetCurrentCursor( windowSelection ? KICURSOR::SELECT_WINDOW
                                                                : KICURSOR::SELECT_LASSO );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_drag_additive && !m_drag_subtractive )
                ClearSelection();

            // Start drawing a selection box
            area.SetOrigin( evt->DragOrigin() );
            area.SetEnd( evt->Position() );
            area.SetAdditive( m_drag_additive );
            area.SetSubtractive( m_drag_subtractive );
            area.SetExclusiveOr( false );

            view->SetVisible( &area, true );
            view->Update( &area );
            getViewControls()->SetAutoPan( true );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( false );

            // End drawing the selection box
            view->SetVisible( &area, false );

            // Mark items within the selection box as selected
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> children;

            // Filter the view items based on the selection box
            BOX2I selectionBox = area.ViewBBox();
            view->Query( selectionBox, selectedItems );         // Get the list of selected items

            // Some children aren't in the view; add them by hand.
            // DO NOT add them directly to selectedItems.  If we add enough to cause the vector
            // to grow it will re-allocate and invalidate the top-level for-loop iterator.
            for( KIGFX::VIEW::LAYER_ITEM_PAIR& pair : selectedItems )
            {
                SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( pair.first );

                if( sheet )
                {
                    int layer = pair.second;

                    for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                        children.emplace_back( KIGFX::VIEW::LAYER_ITEM_PAIR( pin, layer ) );
                }

                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pair.first );

                if( symbol )
                {
                    int layer = pair.second;

                    for( SCH_FIELD& field : symbol->GetFields() )
                        children.emplace_back( KIGFX::VIEW::LAYER_ITEM_PAIR( &field, layer ) );
                }

                SCH_GLOBALLABEL* gLabel = dynamic_cast<SCH_GLOBALLABEL*>( pair.first );

                if( gLabel )
                {
                    int        layer = pair.second;
                    SCH_FIELD* intersheetRef = gLabel->GetIntersheetRefs();

                    children.emplace_back( KIGFX::VIEW::LAYER_ITEM_PAIR( intersheetRef, layer ) );
                }
            }

            selectedItems.insert( selectedItems.end(), children.begin(), children.end() );

            int height = area.GetEnd().y - area.GetOrigin().y;

            bool anyAdded = false;
            bool anySubtracted = false;

            // Construct an EDA_RECT to determine EDA_ITEM selection
            EDA_RECT selectionRect( (wxPoint) area.GetOrigin(), wxSize( width, height ) );

            selectionRect.Normalize();

            for( KIGFX::VIEW::LAYER_ITEM_PAIR& pair : selectedItems )
            {
                EDA_ITEM* item = dynamic_cast<EDA_ITEM*>( pair.first );

                if( item && Selectable( item ) && item->HitTest( selectionRect, windowSelection ) )
                {
                    if( m_subtractive || ( m_exclusive_or && item->IsSelected() ) )
                    {
                        unselect( item );
                        anySubtracted = true;
                    }
                    else
                    {
                        select( item );
                        item->SetFlags( STARTPOINT | ENDPOINT );
                        anyAdded = true;
                    }
                }
            }

            m_selection.SetIsHover( false );

            // Inform other potentially interested tools
            if( anyAdded )
                m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

            if( anySubtracted )
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

            break;  // Stop waiting for events
        }
    }

    getViewControls()->SetAutoPan( false );

    // Stop drawing the selection box
    view->Remove( &area );
    m_multiple = false;         // Multiple selection mode is inactive

    if( !cancelled )
        m_selection.ClearReferencePoint();

    return cancelled;
}


static KICAD_T nodeTypes[] =
{
    SCH_SYMBOL_LOCATE_POWER_T,
    SCH_PIN_T,
    SCH_LINE_LOCATE_WIRE_T,
    SCH_LINE_LOCATE_BUS_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_SHEET_PIN_T,
    SCH_JUNCTION_T,
    EOT
};


EDA_ITEM* EE_SELECTION_TOOL::GetNode( VECTOR2I aPosition )
{
    EE_COLLECTOR collector;

    //TODO(snh): Reimplement after exposing KNN interface
    int pixelThreshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
    int gridThreshold = KiROUND( getView()->GetGAL()->GetGridSize().EuclideanNorm() );
    int thresholdMax = std::max( pixelThreshold, gridThreshold );

    for( int threshold : { 0, thresholdMax/4, thresholdMax/2, thresholdMax } )
    {
        collector.m_Threshold = threshold;
        collector.Collect( m_frame->GetScreen(), nodeTypes, (wxPoint) aPosition );

        if( collector.GetCount() > 0 )
            break;
    }

    return collector.GetCount() ? collector[ 0 ] : nullptr;
}


int EE_SELECTION_TOOL::SelectNode( const TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( false );

    SelectPoint( cursorPos, nodeTypes  );

    return 0;
}


int EE_SELECTION_TOOL::SelectConnection( const TOOL_EVENT& aEvent )
{
    static KICAD_T wiresAndBuses[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };

    RequestSelection( wiresAndBuses );

    if( m_selection.Empty() )
        return 0;

    SCH_LINE* line = (SCH_LINE*) m_selection.Front();
    EDA_ITEMS items;

    m_frame->GetScreen()->ClearDrawingState();
    std::set<SCH_ITEM*> conns = m_frame->GetScreen()->MarkConnections( line );

    for( SCH_ITEM* item : conns )
        select( item );

    if( m_selection.GetSize() > 1 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int EE_SELECTION_TOOL::AddItemToSel( const TOOL_EVENT& aEvent )
{
    AddItemToSel( aEvent.Parameter<EDA_ITEM*>() );
    m_selection.SetIsHover( false );
    return 0;
}


void EE_SELECTION_TOOL::AddItemToSel( EDA_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        select( aItem );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
}


int EE_SELECTION_TOOL::AddItemsToSel( const TOOL_EVENT& aEvent )
{
    AddItemsToSel( aEvent.Parameter<EDA_ITEMS*>(), false );
    m_selection.SetIsHover( false );
    return 0;
}


void EE_SELECTION_TOOL::AddItemsToSel( EDA_ITEMS* aList, bool aQuietMode )
{
    if( aList )
    {
        for( EDA_ITEM* item : *aList )
            select( item );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
}


int EE_SELECTION_TOOL::RemoveItemFromSel( const TOOL_EVENT& aEvent )
{
    RemoveItemFromSel( aEvent.Parameter<EDA_ITEM*>() );
    m_selection.SetIsHover( false );
    return 0;
}


void EE_SELECTION_TOOL::RemoveItemFromSel( EDA_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        unselect( aItem );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }
}


int EE_SELECTION_TOOL::RemoveItemsFromSel( const TOOL_EVENT& aEvent )
{
    RemoveItemsFromSel( aEvent.Parameter<EDA_ITEMS*>(), false );
    m_selection.SetIsHover( false );
    return 0;
}


void EE_SELECTION_TOOL::RemoveItemsFromSel( EDA_ITEMS* aList, bool aQuietMode )
{
    if( aList )
    {
        for( EDA_ITEM* item : *aList )
            unselect( item );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }
}


void EE_SELECTION_TOOL::RemoveItemsFromSel( std::vector<KIID>* aList, bool aQuietMode )
{
    EDA_ITEMS removeItems;

    for( EDA_ITEM* item : m_selection )
    {
        if( alg::contains( *aList, item->m_Uuid ) )
            removeItems.push_back( item );
    }

    RemoveItemsFromSel( &removeItems, aQuietMode );
}


void EE_SELECTION_TOOL::BrightenItem( EDA_ITEM* aItem )
{
    highlight( aItem, BRIGHTENED );
}


void EE_SELECTION_TOOL::UnbrightenItem( EDA_ITEM* aItem )
{
    unhighlight( aItem, BRIGHTENED );
}


int EE_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    ClearSelection();

    return 0;
}


void EE_SELECTION_TOOL::RebuildSelection()
{
    m_selection.Clear();

    if( m_isSymbolEditor )
    {
        LIB_SYMBOL* start = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        for( LIB_ITEM& item : start->GetDrawItems() )
        {
            if( item.IsSelected() )
                select( static_cast<EDA_ITEM*>( &item ) );
        }
    }
    else
    {
        for( SCH_ITEM* item : m_frame->GetScreen()->Items() )
        {
            // If the field and symbol are selected, only use the symbol
            if( item->IsSelected() )
            {
                select( item );
            }
            else
            {
                item->RunOnChildren(
                        [&]( SCH_ITEM* aChild )
                        {
                            if( aChild->IsSelected() )
                                select( aChild );
                        } );
            }
        }
    }

    updateReferencePoint();

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
}


int EE_SELECTION_TOOL::SelectionMenu( const TOOL_EVENT& aEvent )
{
    EE_COLLECTOR* collector = aEvent.Parameter<EE_COLLECTOR*>();

    if( !doSelectionMenu( collector ) )
        collector->m_MenuCancelled = true;

    return 0;
}


bool EE_SELECTION_TOOL::doSelectionMenu( EE_COLLECTOR* aCollector )
{
    EDA_ITEM*   current = nullptr;
    bool        selectAll = false;
    bool        expandSelection = false;

    do
    {
        /// The user has requested the full, non-limited list of selection items
        if( expandSelection )
            aCollector->Combine();

        expandSelection = false;

        int         limit = std::min( 9, aCollector->GetCount() );
        ACTION_MENU menu( true );

        for( int i = 0; i < limit; ++i )
        {
            wxString  text;
            EDA_ITEM* item = ( *aCollector )[i];
            text           = item->GetSelectMenuText( m_frame->GetUserUnits() );

            wxString menuText = wxString::Format( "&%d. %s\t%d", i + 1, text, i + 1 );
            menu.Add( menuText, i + 1, item->GetMenuImage() );
        }

        menu.AppendSeparator();
        menu.Add( _( "Select &All\tA" ), limit + 1, BITMAPS::INVALID_BITMAP );

        if( !expandSelection && aCollector->HasAdditionalItems() )
            menu.Add( _( "&Expand Selection\tE" ), limit + 2, BITMAPS::INVALID_BITMAP );

        if( aCollector->m_MenuTitle.Length() )
        {
            menu.SetTitle( aCollector->m_MenuTitle );
            menu.SetIcon( BITMAPS::info );
            menu.DisplayTitle( true );
        }
        else
        {
            menu.DisplayTitle( false );
        }

        SetContextMenu( &menu, CMENU_NOW );

        while( TOOL_EVENT* evt = Wait() )
        {
            if( evt->Action() == TA_CHOICE_MENU_UPDATE )
            {
                if( selectAll )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        unhighlight( ( *aCollector )[i], BRIGHTENED );
                }
                else if( current )
                {
                    unhighlight( current, BRIGHTENED );
                }

                int id = *evt->GetCommandId();

                // User has pointed an item, so show it in a different way
                if( id > 0 && id <= limit )
                {
                    current = ( *aCollector )[id - 1];
                    highlight( current, BRIGHTENED );
                }
                else
                {
                    current = nullptr;
                }

                // User has pointed on the "Select All" option
                if( id == limit + 1 )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        highlight( ( *aCollector )[i], BRIGHTENED );
                    selectAll = true;
                }
                else
                {
                    selectAll = false;
                }
            }
            else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
            {
                if( selectAll )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        unhighlight( ( *aCollector )[i], BRIGHTENED );
                }
                else if( current )
                    unhighlight( current, BRIGHTENED );

                OPT<int> id = evt->GetCommandId();

                // User has selected the "Select All" option
                if( id == limit + 1 )
                {
                    selectAll = true;
                    current   = nullptr;
                }
                else if( id == limit + 2 )
                {
                    selectAll       = false;
                    current         = nullptr;
                    expandSelection = true;
                }
                // User has selected an item, so this one will be returned
                else if( id && ( *id > 0 ) && ( *id <= limit ) )
                {
                    selectAll = false;
                    current   = ( *aCollector )[*id - 1];
                }
                // User has cancelled the menu (either by <esc> or clicking out of it)
                else
                {
                    selectAll = false;
                    current   = nullptr;
                }
            }
            else if( evt->Action() == TA_CHOICE_MENU_CLOSED )
            {
                break;
            }

            getView()->UpdateItems();
            m_frame->GetCanvas()->Refresh();
        }
    } while( expandSelection );

    if( selectAll )
        return true;
    else if( current )
    {
        unhighlight( current, BRIGHTENED );

        getView()->UpdateItems();
        m_frame->GetCanvas()->Refresh();

        aCollector->Empty();
        aCollector->Append( current );
        return true;
    }

    return false;
}


bool EE_SELECTION_TOOL::Selectable( const EDA_ITEM* aItem, const VECTOR2I* aPos,
                                    bool checkVisibilityOnly ) const
{
    // NOTE: in the future this is where Eeschema layer/itemtype visibility will be handled

    SYMBOL_EDIT_FRAME* symEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );

    // Do not allow selection of anything except fields when the current symbol in the symbol
    // editor is a derived symbol.
    if( symEditFrame && symEditFrame->IsSymbolAlias() && aItem->Type() != LIB_FIELD_T )
        return false;

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        const SCH_PIN* pin = static_cast<const SCH_PIN*>( aItem );

        if( !pin->IsVisible() && !m_frame->GetShowAllPins() )
            return false;

        if( m_frame->eeconfig()->m_Selection.select_pin_selects_symbol )
        {
            // Pin anchors have to be allowed for auto-starting wires.
            if( aPos )
            {
                EE_GRID_HELPER grid( m_toolMgr );
                VECTOR2I       cursorPos = grid.BestSnapAnchor( *aPos, LAYER_CONNECTABLE, nullptr );

                if( pin->IsPointClickableAnchor( (wxPoint) cursorPos ) )
                    return true;
            }

            return false;
        }
    }
        break;

    case LIB_SYMBOL_T:    // In symbol_editor we do not want to select the symbol itself.
        return false;

    case LIB_FIELD_T:     // LIB_FIELD object can always be edited.
        break;

    case LIB_ARC_T:
    case LIB_CIRCLE_T:
    case LIB_TEXT_T:
    case LIB_RECTANGLE_T:
    case LIB_POLYLINE_T:
    case LIB_BEZIER_T:
    case LIB_PIN_T:
        if( symEditFrame )
        {
            LIB_ITEM* lib_item = (LIB_ITEM*) aItem;

            if( lib_item->GetUnit() && lib_item->GetUnit() != symEditFrame->GetUnit() )
                return false;

            if( lib_item->GetConvert() && lib_item->GetConvert() != symEditFrame->GetConvert() )
                return false;
        }

        break;

    case SCH_MARKER_T:  // Always selectable
        return true;

    default:            // Suppress warnings
        break;
    }

    return true;
}


void EE_SELECTION_TOOL::ClearSelection()
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( (EDA_ITEM*) m_selection.Front(), SELECTED, &m_selection );

    getView()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
}


void EE_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    highlight( aItem, SELECTED, &m_selection );
}


void EE_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
}


void EE_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aMode, EE_SELECTION* aGroup )
{
    KICAD_T itemType = aItem->Type();

    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aGroup )
        aGroup->Add( aItem );

    // Highlight pins and fields.  (All the other symbol children are currently only
    // represented in the LIB_SYMBOL and will inherit the settings of the parent symbol.)
    if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( aItem ) )
    {
        sch_item->RunOnChildren(
            [&]( SCH_ITEM* aChild )
            {
                if( aMode == SELECTED )
                    aChild->SetSelected();
                else if( aMode == BRIGHTENED )
                    aChild->SetBrightened();
            } );
    }

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent() );
    else
        getView()->Update( aItem );
}


void EE_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aMode, EE_SELECTION* aGroup )
{
    KICAD_T itemType = aItem->Type();

    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aGroup )
        aGroup->Remove( aItem );

    // Unhighlight pins and fields.  (All the other symbol children are currently only
    // represented in the LIB_SYMBOL.)
    if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( aItem ) )
    {
        sch_item->RunOnChildren(
            [&]( SCH_ITEM* aChild )
            {
                if( aMode == SELECTED )
                    aChild->ClearSelected();
                else if( aMode == BRIGHTENED )
                    aChild->ClearBrightened();
            } );
    }

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent() );
    else
        getView()->Update( aItem );
}


bool EE_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    VECTOR2I margin = getView()->ToWorld( VECTOR2I( GRIP_MARGIN, GRIP_MARGIN ), false );

    // Check if the point is located within any of the currently selected items bounding boxes
    for( EDA_ITEM* item : m_selection )
    {
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin.x, margin.y );    // Give some margin for gripping an item

        if( itemBox.Contains( aPoint ) )
            return true;
    }

    return false;
}


void EE_SELECTION_TOOL::setTransitions()
{
    Go( &EE_SELECTION_TOOL::UpdateMenu,          ACTIONS::updateMenu.MakeEvent() );

    Go( &EE_SELECTION_TOOL::Main,                EE_ACTIONS::selectionActivate.MakeEvent() );
    Go( &EE_SELECTION_TOOL::SelectNode,          EE_ACTIONS::selectNode.MakeEvent() );
    Go( &EE_SELECTION_TOOL::SelectConnection,    EE_ACTIONS::selectConnection.MakeEvent() );
    Go( &EE_SELECTION_TOOL::ClearSelection,      EE_ACTIONS::clearSelection.MakeEvent() );

    Go( &EE_SELECTION_TOOL::AddItemToSel,        EE_ACTIONS::addItemToSel.MakeEvent() );
    Go( &EE_SELECTION_TOOL::AddItemsToSel,       EE_ACTIONS::addItemsToSel.MakeEvent() );
    Go( &EE_SELECTION_TOOL::RemoveItemFromSel,   EE_ACTIONS::removeItemFromSel.MakeEvent() );
    Go( &EE_SELECTION_TOOL::RemoveItemsFromSel,  EE_ACTIONS::removeItemsFromSel.MakeEvent() );
    Go( &EE_SELECTION_TOOL::SelectionMenu,       EE_ACTIONS::selectionMenu.MakeEvent() );

    Go( &EE_SELECTION_TOOL::SelectAll,           EE_ACTIONS::selectAll.MakeEvent() );

    Go( &EE_SELECTION_TOOL::disambiguateCursor,  EVENTS::DisambiguatePoint );
}


