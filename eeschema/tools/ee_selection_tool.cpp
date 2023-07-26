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

#include <core/typeinfo.h>
#include <core/kicad_algo.h>
#include <geometry/shape_compound.h>
#include <ee_actions.h>
#include <ee_collectors.h>
#include <ee_selection_tool.h>
#include <eeschema_id.h>
#include <symbol_edit_frame.h>
#include <lib_item.h>
#include <symbol_viewer_frame.h>
#include <math/util.h>
#include <geometry/shape_rect.h>
#include <menus_helpers.h>
#include <sch_painter.h>
#include <preview_items/selection_area.h>
#include <sch_base_frame.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <sch_edit_frame.h>
#include <sch_item.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <lib_shape.h>
#include <schematic.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>
#include <tools/ee_point_editor.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_editor_control.h>
#include <trigo.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <wx/log.h>


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


SELECTION_CONDITION EE_CONDITIONS::SingleNonExcludedMarker = []( const SELECTION& aSel )
{
    if( aSel.CountType( SCH_MARKER_T ) != 1 )
        return false;

    return !static_cast<SCH_MARKER*>( aSel.Front() )->IsExcluded();
};


SELECTION_CONDITION EE_CONDITIONS::MultipleSymbolsOrPower = []( const SELECTION& aSel )
{
    return aSel.GetSize() > 1 && aSel.OnlyContains( { SCH_SYMBOL_T } );
};


#define HITTEST_THRESHOLD_PIXELS 5


EE_SELECTION_TOOL::EE_SELECTION_TOOL() :
        SELECTION_TOOL( "eeschema.InteractiveSelection" ),
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


static std::vector<KICAD_T> connectedTypes =
{
    SCH_SYMBOL_LOCATE_POWER_T,
    SCH_PIN_T,
    SCH_ITEM_LOCATE_WIRE_T,
    SCH_ITEM_LOCATE_BUS_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_SHEET_PIN_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_JUNCTION_T
};


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

    auto linesSelection =        E_C::MoreThan( 0 ) && E_C::OnlyTypes( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T,
                                                                         SCH_ITEM_LOCATE_GRAPHIC_LINE_T } );
    auto wireOrBusSelection =    E_C::Count( 1 )    && E_C::OnlyTypes( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } );
    auto connectedSelection =    E_C::Count( 1 )    && E_C::OnlyTypes( connectedTypes );
    auto sheetSelection =        E_C::Count( 1 )    && E_C::OnlyTypes( { SCH_SHEET_T } );
    auto crossProbingSelection = E_C::MoreThan( 0 ) && E_C::HasTypes( { SCH_SYMBOL_T, SCH_PIN_T, SCH_SHEET_T } );

    auto schEditSheetPageNumberCondition =
            [&] ( const SELECTION& aSel )
            {
                if( m_isSymbolEditor || m_isSymbolViewer )
                    return false;

                return E_C::LessThan( 2 )( aSel ) && E_C::OnlyTypes( { SCH_SHEET_T } )( aSel );
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

    auto haveHighlight =
            [&]( const SELECTION& sel )
            {
                SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

                return editFrame && !editFrame->GetHighlightedConnection().IsEmpty();
            };

    auto haveSymbol =
            [&]( const SELECTION& sel )
            {
                return m_isSymbolEditor &&
                       static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();
            };

    auto symbolDisplayNameIsEditable =
            [&]( const SELECTION& sel )
            {
                if ( !m_isSymbolEditor )
                    return false;

                SYMBOL_EDIT_FRAME* symbEditorFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );

                return symbEditorFrame
                        && symbEditorFrame->GetCurSymbol()
                        && symbEditorFrame->GetCurSymbol()->IsMulti()
                        && symbEditorFrame->IsSymbolEditable()
                        && !symbEditorFrame->IsSymbolAlias();
            };

    auto& menu = m_menu.GetMenu();

    menu.AddItem( EE_ACTIONS::clearHighlight,     haveHighlight && EE_CONDITIONS::Idle, 1 );
    menu.AddSeparator(                            haveHighlight && EE_CONDITIONS::Idle, 1 );

    menu.AddItem( EE_ACTIONS::enterSheet,         sheetSelection && EE_CONDITIONS::Idle, 2 );
    menu.AddItem( EE_ACTIONS::selectOnPCB,        crossProbingSelection && EE_CONDITIONS::Idle, 2 );
    menu.AddItem( EE_ACTIONS::leaveSheet,         belowRootSheetCondition, 2 );

    menu.AddSeparator( 100 );
    menu.AddItem( EE_ACTIONS::drawWire,           schEditCondition && EE_CONDITIONS::Empty, 100 );
    menu.AddItem( EE_ACTIONS::drawBus,            schEditCondition && EE_CONDITIONS::Empty, 100 );

    menu.AddSeparator( 100 );
    menu.AddItem( EE_ACTIONS::finishWire,         SCH_LINE_WIRE_BUS_TOOL::IsDrawingWire, 100 );

    menu.AddSeparator( 100 );
    menu.AddItem( EE_ACTIONS::finishBus,          SCH_LINE_WIRE_BUS_TOOL::IsDrawingBus, 100 );

    menu.AddSeparator( 200 );
    menu.AddItem( EE_ACTIONS::selectConnection,   connectedSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeJunction,      wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeLabel,         wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeClassLabel,    wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeGlobalLabel,   wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::placeHierLabel,     wireOrBusSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::breakWire,          linesSelection     && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::slice,              linesSelection     && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::importSheetPin,     sheetSelection     && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::assignNetclass,     connectedSelection && EE_CONDITIONS::Idle, 250 );
    menu.AddItem( EE_ACTIONS::editPageNumber,     schEditSheetPageNumberCondition, 250 );

    menu.AddSeparator( 400 );
    menu.AddItem( EE_ACTIONS::symbolProperties,   haveSymbol && EE_CONDITIONS::Empty, 400 );
    menu.AddItem( EE_ACTIONS::pinTable,           haveSymbol && EE_CONDITIONS::Empty, 400 );
    menu.AddItem( EE_ACTIONS::setUnitDisplayName,
                  haveSymbol && symbolDisplayNameIsEditable && EE_CONDITIONS::Empty, 400 );

    menu.AddSeparator( 1000 );
    m_frame->AddStandardSubMenus( m_menu );

    m_disambiguateTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( EE_SELECTION_TOOL::onDisambiguationExpire ), nullptr, this );

    return true;
}


void EE_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();

    if( aReason != TOOL_BASE::REDRAW )
    {
        // Remove pointers to the selected items from containers without changing their
        // properties (as they are already deleted while a new sheet is loaded)
        m_selection.Clear();
    }

    if( aReason == TOOL_BASE::MODEL_RELOAD || aReason == TOOL_BASE::SUPERMODEL_RELOAD )
    {
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
        {
            m_isSymbolViewer = symbolViewerFrame != nullptr;
        }
    }

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( &m_selection );
    getView()->Add( &m_selection );
}


int EE_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    KIID lastRolloverItem = niluuid;
    EE_GRID_HELPER grid( m_toolMgr );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        bool selCancelled = false;
        bool displayWireCursor = false;
        bool displayBusCursor = false;
        bool displayLineCursor = false;
        KIID rolloverItem = lastRolloverItem;

        // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
        setModifiersState( evt->Modifier( MD_SHIFT ), evt->Modifier( MD_CTRL ),
                           evt->Modifier( MD_ALT ) );

        MOUSE_DRAG_ACTION drag_action = m_frame->GetDragAction();

        if( evt->IsMouseDown( BUT_LEFT ) )
        {
            if( !m_frame->ToolStackIsEmpty() )
            {
                // Avoid triggering when running under other tools
            }
            else if( m_toolMgr->GetTool<EE_POINT_EDITOR>()
                        && m_toolMgr->GetTool<EE_POINT_EDITOR>()->HasPoint() )
            {
                // Distinguish point editor from selection modification by checking modifiers
                if( hasModifier() )
                {
                    m_originalCursor = m_toolMgr->GetMousePosition();
                    m_disambiguateTimer.StartOnce( 500 );
                }
            }
            else
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

            // Collect items at the clicked location (doesn't select them yet)
            EE_COLLECTOR collector;
            CollectHits( collector, evt->Position() );
            narrowSelection( collector, evt->Position(), false );

            if( collector.GetCount() == 1 && !m_isSymbolEditor && !hasModifier() )
            {
                OPT_TOOL_EVENT autostart = autostartEvent( evt, grid, collector[0] );

                if( autostart )
                {
                    DRAW_SEGMENT_EVENT_PARAMS* params = new DRAW_SEGMENT_EVENT_PARAMS();

                    params->layer = autostart->Parameter<const DRAW_SEGMENT_EVENT_PARAMS*>()->layer;
                    params->quitOnDraw = true;
                    params->sourceSegment = dynamic_cast<SCH_LINE*>( collector[0] );

                    autostart->SetParameter<const DRAW_SEGMENT_EVENT_PARAMS*>( params );
                    m_toolMgr->ProcessEvent( *autostart );

                    selCancelled = true;
                }
                else if( collector[0]->IsHypertext() )
                {
                    collector[ 0 ]->DoHypertextAction( m_frame );
                    selCancelled = true;
                }
                else if( collector[0]->IsBrightened() )
                {
                    if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                    {
                        NET_NAVIGATOR_ITEM_DATA itemData( schframe->GetCurrentSheet(),
                                                          collector[0] );

                        schframe->SelectNetNavigatorItem( &itemData );
                    }
                }
            }

            if( !selCancelled )
            {
                selectPoint( collector, evt->Position(), nullptr, nullptr, m_additive,
                             m_subtractive, m_exclusive_or );
                m_selection.SetIsHover( false );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_disambiguateTimer.Stop();

            // right click? if there is any object - show the context menu
            if( m_selection.Empty() )
            {
                ClearSelection();
                SelectPoint( evt->Position(), { SCH_LOCATE_ANY_T }, nullptr, &selCancelled );
                m_selection.SetIsHover( true );
            }
            // If the cursor has moved off the bounding box of the selection by more than
            // a grid square, check to see if there is another item available for selection
            // under the cursor.  If there is, the user likely meant to get the context menu
            // for that item.  If there is no new item, then keep the original selection and
            // show the context menu for it.
            else if( !m_selection.GetBoundingBox().Inflate( grid.GetGrid().x, grid.GetGrid().y )
                        .Contains( evt->Position() ) )
            {
                EE_COLLECTOR collector;

                if( CollectHits( collector, evt->Position(), { SCH_LOCATE_ANY_T } ) )
                {
                    ClearSelection();

                    SelectPoint( evt->Position(), { SCH_LOCATE_ANY_T }, nullptr, &selCancelled );
                    m_selection.SetIsHover( true );
                }
            }

            if( !selCancelled )
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
                m_toolMgr->PostAction( EE_ACTIONS::enterSheet );
            else
                m_toolMgr->PostAction( EE_ACTIONS::properties );
        }
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            m_disambiguateTimer.Stop();

            // Middle double click?  Do zoom to fit or zoom to objects
            if( evt->Modifier( MD_CTRL ) ) // Is CTRL key down?
                m_toolMgr->RunAction( ACTIONS::zoomFitObjects );
            else
                m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
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

            if( hasModifier() || drag_action == MOUSE_DRAG_ACTION::SELECT )
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
                    {
                        m_selection = RequestSelection( { LIB_FIELD_T } );
                    }
                    else
                    {
                        m_selection = RequestSelection( { LIB_SHAPE_T,
                                                          LIB_TEXT_T,
                                                          LIB_TEXTBOX_T,
                                                          LIB_PIN_T,
                                                          LIB_FIELD_T } );
                    }
                }
                else
                {
                    m_selection = RequestSelection( EE_COLLECTOR::MovableItems );
                }

                // Check if dragging has started within any of selected items bounding box
                if( selectionContains( evt->DragOrigin() ) )
                {
                    // drag_is_move option exists only in schematic editor, not in symbol editor
                    // (m_frame->eeconfig() returns nullptr in Symbol Editor)
                    if( m_isSymbolEditor || m_frame->eeconfig()->m_Input.drag_is_move )
                        m_toolMgr->RunSynchronousAction( EE_ACTIONS::move, nullptr );
                    else
                        m_toolMgr->RunSynchronousAction( EE_ACTIONS::drag, nullptr );
                }
                else
                {
                    // No -> drag a selection box
                    selectMultiple();
                }
            }
        }
        else if( evt->IsMouseDown( BUT_AUX1 ) )
        {
            m_toolMgr->RunAction( EE_ACTIONS::navigateBack );
        }
        else if( evt->IsMouseDown( BUT_AUX2 ) )
        {
            m_toolMgr->RunAction( EE_ACTIONS::navigateForward );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            m_disambiguateTimer.Stop();

            // context sub-menu selection?  Handle unit selection or bus unfolding
            if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_UNIT_SYM_MAX )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selection.Front() );
                int unit = *evt->GetCommandId() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( symbol )
                    static_cast<SCH_EDIT_FRAME*>( m_frame )->SelectUnit( symbol, unit );
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_UNFOLD_BUS
                     && *evt->GetCommandId() <= ID_POPUP_SCH_UNFOLD_BUS_END )
            {
                wxString* net = new wxString( *evt->Parameter<wxString*>() );
                m_toolMgr->RunAction<wxString*>( EE_ACTIONS::unfoldBus, net );
            }
        }
        else if( evt->IsCancelInteractive() )
        {
            m_disambiguateTimer.Stop();

            // We didn't set these, but we have reports that they leak out of some other tools,
            // so we clear them here.
            getViewControls()->SetAutoPan( false );
            getViewControls()->CaptureCursor( false );

            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            if( !GetSelection().Empty() )
            {
                ClearSelection();
            }
            else if( evt->FirstResponder() == this && evt->GetCommandId() == (int) WXK_ESCAPE )
            {
                SCH_EDITOR_CONTROL* editor = m_toolMgr->GetTool<SCH_EDITOR_CONTROL>();

                if( editor && m_frame->eeconfig()->m_Input.esc_clears_net_highlight )
                    editor->ClearHighlight( *evt );
            }
        }
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->FocusOnItem( nullptr );

            ClearSelection();
        }
        else if( evt->IsMotion() && !m_isSymbolEditor && evt->FirstResponder() == this )
        {
            // Update cursor and rollover item
            rolloverItem = niluuid;
            EE_COLLECTOR collector;

            getViewControls()->ForceCursorPosition( false );

            if( CollectHits( collector, evt->Position() ) )
            {
                narrowSelection( collector, evt->Position(), false );

                if( collector.GetCount() == 1 && !hasModifier() )
                {
                    OPT_TOOL_EVENT autostartEvt = autostartEvent( evt, grid, collector[0] );

                    if( autostartEvt )
                    {
                        if( autostartEvt->Matches( EE_ACTIONS::drawBus.MakeEvent() ) )
                            displayBusCursor = true;
                        else if( autostartEvt->Matches( EE_ACTIONS::drawWire.MakeEvent() ) )
                            displayWireCursor = true;
                        else if( autostartEvt->Matches( EE_ACTIONS::drawLines.MakeEvent() ) )
                            displayLineCursor = true;
                    }
                    else if( collector[0]->IsHypertext() && !collector[0]->IsSelected() )
                    {
                        rolloverItem = collector[0]->m_Uuid;
                    }
                }
            }
        }
        else
        {
            evt->SetPassEvent();
        }

        if( rolloverItem != lastRolloverItem )
        {
            if( EDA_ITEM* item = m_frame->GetItem( lastRolloverItem ) )
            {
                item->ClearFlags( IS_ROLLOVER );
                lastRolloverItem = niluuid;

                if( item->Type() == SCH_FIELD_T )
                    m_frame->GetCanvas()->GetView()->Update( item->GetParent() );
                else
                    m_frame->GetCanvas()->GetView()->Update( item );
            }
        }

        if( rolloverItem != niluuid )
        {
            EDA_ITEM* item = m_frame->GetItem( rolloverItem );

            if( item && !( item->GetFlags() & IS_ROLLOVER ) )
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


OPT_TOOL_EVENT EE_SELECTION_TOOL::autostartEvent( TOOL_EVENT* aEvent, EE_GRID_HELPER& aGrid,
                                                  SCH_ITEM* aItem )
{
    VECTOR2I pos = aGrid.BestSnapAnchor( aEvent->Position(), aGrid.GetItemGrid( aItem ) );

    if( m_frame->eeconfig()->m_Drawing.auto_start_wires
            && !m_toolMgr->GetTool<EE_POINT_EDITOR>()->HasPoint()
            && aItem->IsPointClickableAnchor( pos ) )
    {
        OPT_TOOL_EVENT newEvt = EE_ACTIONS::drawWire.MakeEvent();

        if( aItem->Type() == SCH_BUS_BUS_ENTRY_T )
        {
            newEvt = EE_ACTIONS::drawBus.MakeEvent();
        }
        else if( aItem->Type() == SCH_BUS_WIRE_ENTRY_T )
        {
            SCH_BUS_WIRE_ENTRY* busEntry = static_cast<SCH_BUS_WIRE_ENTRY*>( aItem );

            if( !busEntry->m_connected_bus_item )
                newEvt = EE_ACTIONS::drawBus.MakeEvent();
        }
        else if( aItem->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( aItem );

            if( line->IsBus() )
                newEvt = EE_ACTIONS::drawBus.MakeEvent();
            else if( line->IsGraphicLine() )
                newEvt = EE_ACTIONS::drawLines.MakeEvent();
        }
        else if( aItem->Type() == SCH_LABEL_T || aItem->Type() == SCH_HIER_LABEL_T
                 || aItem->Type() == SCH_SHEET_PIN_T )
        {
            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aItem );
            SCH_CONNECTION  possibleConnection( label->Schematic()->ConnectionGraph() );
            possibleConnection.ConfigureFromLabel( label->GetText() );

            if( possibleConnection.IsBus() )
                newEvt = EE_ACTIONS::drawBus.MakeEvent();
        }

        newEvt->SetMousePosition( pos );
        newEvt->SetHasPosition( true );
        newEvt->SetForceImmediate( true );

        getViewControls()->ForceCursorPosition( true, pos );

        return newEvt;
    }

    return OPT_TOOL_EVENT();
}


int EE_SELECTION_TOOL::disambiguateCursor( const TOOL_EVENT& aEvent )
{
    wxMouseState keyboardState = wxGetMouseState();

    setModifiersState( keyboardState.ShiftDown(), keyboardState.ControlDown(),
                       keyboardState.AltDown() );

    m_skip_heuristics = true;
    SelectPoint( m_originalCursor, { SCH_LOCATE_ANY_T }, nullptr, &m_canceledMenu, false,
                 m_additive, m_subtractive, m_exclusive_or );
    m_skip_heuristics = false;

    return 0;
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
                                     const std::vector<KICAD_T>& aScanTypes )
{
    int pixelThreshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
    int gridThreshold = KiROUND( getView()->GetGAL()->GetGridSize().EuclideanNorm() / 2 );
    aCollector.m_Threshold = std::max( pixelThreshold, gridThreshold );
    aCollector.m_ShowPinElectricalTypes = m_frame->GetRenderSettings()->m_ShowPinsElectricalType;

    if( m_isSymbolEditor )
    {
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return false;

        aCollector.Collect( symbol->GetDrawItems(), aScanTypes, aWhere, m_unit, m_convert );
    }
    else
    {
        aCollector.Collect( m_frame->GetScreen(), aScanTypes, aWhere, m_unit, m_convert );

        if( m_frame->eeconfig()->m_Selection.select_pin_selects_symbol )
        {
            int originalCount = aCollector.GetCount();

            for( int ii = 0; ii < originalCount; ++ii )
            {
                if( aCollector[ii]->Type() == SCH_PIN_T )
                {
                    SCH_PIN* pin = static_cast<SCH_PIN*>( aCollector[ii] );

                    if( !aCollector.HasItem( pin->GetParentSymbol() ) )
                        aCollector.Append( pin->GetParentSymbol() );
                }
            }
        }
    }

    return aCollector.GetCount() > 0;
}


void EE_SELECTION_TOOL::narrowSelection( EE_COLLECTOR& collector, const VECTOR2I& aWhere,
                                         bool aCheckLocked, bool aSelectedOnly )
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

        if( aSelectedOnly && !collector[i]->IsSelected() )
        {
            collector.Remove( i );
            continue;
        }
    }

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
        GuessSelectionCandidates( collector, aWhere );
}


bool EE_SELECTION_TOOL::selectPoint( EE_COLLECTOR& aCollector, const VECTOR2I& aWhere,
                                     EDA_ITEM** aItem, bool* aSelectionCancelledFlag, bool aAdd,
                                     bool aSubtract, bool aExclusiveOr )
{
    m_selection.ClearReferencePoint();

    // If still more than one item we're going to have to ask the user.
    if( aCollector.GetCount() > 1 )
    {
        // Try to call selectionMenu via RunAction() to avoid event-loop contention
        // But it we cannot handle the event, then we don't have an active tool loop, so
        // handle it directly.
        if( !m_toolMgr->RunAction<COLLECTOR*>( EE_ACTIONS::selectionMenu, &aCollector ) )
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

    int  addedCount = 0;
    bool anySubtracted = false;

    if( aCollector.GetCount() > 0 )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            EDA_ITEM_FLAGS flags = 0;
            bool           isLine = aCollector[i]->Type() == SCH_LINE_T;

            // Handle line ends specially
            if( isLine )
            {
                SCH_LINE* line = (SCH_LINE*) aCollector[i];

                if( HitTestPoints( line->GetStartPoint(), aWhere, aCollector.m_Threshold ) )
                    flags = STARTPOINT;
                else if( HitTestPoints( line->GetEndPoint(), aWhere, aCollector.m_Threshold ) )
                    flags = ENDPOINT;
                else
                    flags = STARTPOINT | ENDPOINT;
            }

            if( aSubtract
                || ( aExclusiveOr && aCollector[i]->IsSelected()
                     && ( !isLine || ( isLine && aCollector[i]->HasFlag( flags ) ) ) ) )
            {
                aCollector[i]->ClearFlags( flags );

                // Need to update end shadows after ctrl-click unselecting one of two selected endpoints
                if( isLine )
                    getView()->Update( aCollector[i] );

                if( !aCollector[i]->HasFlag( STARTPOINT ) && !aCollector[i]->HasFlag( ENDPOINT ) )
                {
                    unselect( aCollector[i] );
                    anySubtracted = true;
                }
            }
            else
            {
                aCollector[i]->SetFlags( flags );
                select( aCollector[i] );
                addedCount++;
            }
        }
    }

    if( addedCount == 1 )
    {
        m_toolMgr->ProcessEvent( EVENTS::PointSelectedEvent );

        if( aItem && aCollector.GetCount() == 1 )
            *aItem = aCollector[0];

        return true;
    }
    else if( addedCount > 1 )
    {
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
        return true;
    }
    else if( anySubtracted )
    {
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
        return true;
    }

    return false;
}


bool EE_SELECTION_TOOL::SelectPoint( const VECTOR2I& aWhere,
                                     const std::vector<KICAD_T>& aScanTypes,
                                     EDA_ITEM** aItem, bool* aSelectionCancelledFlag,
                                     bool aCheckLocked, bool aAdd, bool aSubtract,
                                     bool aExclusiveOr )
{
    EE_COLLECTOR collector;

    if( !CollectHits( collector, aWhere, aScanTypes ) )
        return false;

    narrowSelection( collector, aWhere, aCheckLocked, aSubtract );

    return selectPoint( collector, aWhere, aItem, aSelectionCancelledFlag, aAdd, aSubtract,
                        aExclusiveOr );
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
            {
                if( item->Type() == SCH_LINE_T )
                    item->SetFlags( STARTPOINT | ENDPOINT );

                select( item );
            }
        }
    }

    m_multiple = false;

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void EE_SELECTION_TOOL::GuessSelectionCandidates( EE_COLLECTOR& collector, const VECTOR2I& aPos )
{
    // Prefer exact hits to sloppy ones
    std::set<EDA_ITEM*> exactHits;

    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        EDA_ITEM*   item = collector[ i ];
        SCH_LINE*   line = dynamic_cast<SCH_LINE*>( item );
        LIB_SHAPE*  shape = dynamic_cast<LIB_SHAPE*>( item );
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

        // Lines are hard to hit.  Give them a bit more slop to still be considered "exact".

        if( line || ( shape && shape->GetShape() == SHAPE_T::POLY )
                 || ( shape && shape->GetShape() == SHAPE_T::ARC ) )
        {
            int pixelThreshold = KiROUND( getView()->ToWorld( 6 ) );

            if( item->HitTest( aPos, pixelThreshold ) )
                exactHits.insert( item );
        }
        else if( symbol && m_frame->eeconfig()->m_Selection.select_pin_selects_symbol )
        {
            if( symbol->GetBodyAndPinsBoundingBox().Contains( aPos ) )
                exactHits.insert( item );
        }
        else
        {
            if( item->HitTest( aPos, 0 ) )
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
    VECTOR2I  pos( aPos );
    SEG       poss( m_isSymbolEditor ? mapCoords( pos ) : pos,
                    m_isSymbolEditor ? mapCoords( pos ) : pos );
    EDA_ITEM* closest = nullptr;
    int       closestDist = INT_MAX / 2;

    for( EDA_ITEM* item : collector )
    {
        BOX2I bbox = item->GetBoundingBox();
        int   dist = INT_MAX / 2;

        if( exactHits.count( item ) )
        {
            if( item->Type() == SCH_PIN_T || item->Type() == SCH_JUNCTION_T )
            {
                closest = item;
                break;
            }

            SCH_LINE*   line = dynamic_cast<SCH_LINE*>( item );
            EDA_TEXT*   text = dynamic_cast<EDA_TEXT*>( item );
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

            if( line )
            {
                dist = KiROUND( DistanceLinePoint( line->GetStartPoint(), line->GetEndPoint(), pos ) );
            }
            else if( text )
            {
                if( SCH_FIELD* field = dynamic_cast<SCH_FIELD*>( text ) )
                {
                    if( field->GetParent() && field->GetParent()->Type() == SCH_SYMBOL_T )
                    {
                        symbol = static_cast<SCH_SYMBOL*>( field->GetParent() );

                        VECTOR2I relPos = pos - symbol->GetPosition();
                        relPos = symbol->GetTransform().InverseTransform().TransformCoordinate( relPos );
                        pos = relPos + symbol->GetPosition();

                        poss = SEG( pos, pos );
                    }
                }

                text->GetEffectiveTextShape( false )->Collide( poss, INT_MAX / 2, &dist );
            }
            else if( symbol )
            {
                try
                {
                    bbox = symbol->GetBodyBoundingBox();
                }
                catch( const boost::bad_pointer& exc )
                {
                    // This may be overkill and could be an assertion but we are more likely to
                    // find any boost pointer container errors this way.
                    wxLogError( wxT( "Boost bad pointer exception '%s' occurred." ), exc.what() );
                }

                SHAPE_RECT rect( bbox.GetPosition(), bbox.GetWidth(), bbox.GetHeight() );

                if( bbox.Contains( pos ) )
                    dist = KiROUND( EuclideanNorm( bbox.GetCenter() - pos ) );
                else
                    rect.Collide( poss, closestDist, &dist );
            }
            else
            {
                dist = KiROUND( EuclideanNorm( bbox.GetCenter() - pos ) );
            }
        }
        else
        {
            SHAPE_RECT rect( bbox.GetPosition(), bbox.GetWidth(), bbox.GetHeight() );
            rect.Collide( poss, collector.m_Threshold, &dist );
        }

        if( dist == closestDist )
        {
            if( item->GetParent() == closest )
                closest = item;
        }
        else if( dist < closestDist )
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
        BOX2I tightBox = closest->GetBoundingBox();
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


EE_SELECTION& EE_SELECTION_TOOL::RequestSelection( const std::vector<KICAD_T>& aScanTypes )
{
    if( m_selection.Empty() )
    {
        VECTOR2D cursorPos = getViewControls()->GetCursorPosition( true );

        ClearSelection();
        SelectPoint( cursorPos, aScanTypes );
        m_selection.SetIsHover( true );
        m_selection.ClearReferencePoint();
    }
    else        // Trim an existing selection by aFilterList
    {
        bool isMoving = false;
        bool anyUnselected = false;

        for( int i = (int) m_selection.GetSize() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = (EDA_ITEM*) m_selection.GetItem( i );
            isMoving |= static_cast<SCH_ITEM*>( item )->IsMoving();

            if( !item->IsType( aScanTypes ) )
            {
                unselect( item );
                anyUnselected = true;
            }
        }

        if( anyUnselected )
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

        if( !isMoving )
            updateReferencePoint();
    }

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


// Some navigation actions are allowed in selectMultiple
const TOOL_ACTION* allowedActions[] = { &ACTIONS::panUp,          &ACTIONS::panDown,
                                        &ACTIONS::panLeft,        &ACTIONS::panRight,
                                        &ACTIONS::cursorUp,       &ACTIONS::cursorDown,
                                        &ACTIONS::cursorLeft,     &ACTIONS::cursorRight,
                                        &ACTIONS::cursorUpFast,   &ACTIONS::cursorDownFast,
                                        &ACTIONS::cursorLeftFast, &ACTIONS::cursorRightFast,
                                        &ACTIONS::zoomIn,         &ACTIONS::zoomOut,
                                        &ACTIONS::zoomInCenter,   &ACTIONS::zoomOutCenter,
                                        &ACTIONS::zoomCenter,     &ACTIONS::zoomFitScreen,
                                        &ACTIONS::zoomFitObjects, nullptr };


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
        int height = area.GetEnd().y - area.GetOrigin().y;

        /* Selection mode depends on direction of drag-selection:
         * Left > Right : Select objects that are fully enclosed by selection
         * Right > Left : Select objects that are crossed by selection
         */
        bool isGreedy = width < 0;

        if( view->IsMirroredX() )
            isGreedy = !isGreedy;

        m_frame->GetCanvas()->SetCurrentCursor( isGreedy ? KICURSOR::SELECT_LASSO
                                                         : KICURSOR::SELECT_WINDOW );

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

            // Fetch items from the RTree that are in our area of interest
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> nearbyViewItems;
            view->Query( area.ViewBBox(), nearbyViewItems );

            // Build lists of nearby items and their children
            std::unordered_set<EDA_ITEM*> nearbyItems;
            std::vector<EDA_ITEM*>        nearbyChildren;
            std::vector<EDA_ITEM*>        flaggedItems;

            for( KIGFX::VIEW::LAYER_ITEM_PAIR& pair : nearbyViewItems )
            {
                if( EDA_ITEM* item = dynamic_cast<EDA_ITEM*>( pair.first ) )
                {
                    if( nearbyItems.insert( item ).second )
                    {
                        item->ClearFlags( CANDIDATE );

                        if( SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item ) )
                        {
                            sch_item->RunOnChildren(
                                    [&]( SCH_ITEM* aChild )
                                    {
                                        // Filter pins by unit
                                        if( SCH_PIN* pin = dynamic_cast<SCH_PIN*>( aChild ) )
                                        {
                                            int unit = pin->GetLibPin()->GetUnit();

                                            if( unit && unit != pin->GetParentSymbol()->GetUnit() )
                                                return;
                                        }

                                        nearbyChildren.push_back( aChild );
                                    } );
                        }
                    }
                }
            }

            BOX2I selectionRect( area.GetOrigin(), VECTOR2I( width, height ) );
            selectionRect.Normalize();

            bool anyAdded = false;
            bool anySubtracted = false;

            auto selectItem =
                    [&]( EDA_ITEM* aItem, EDA_ITEM_FLAGS flags )
                    {
                        if( m_subtractive || ( m_exclusive_or && aItem->IsSelected() ) )
                        {
                            if ( m_exclusive_or )
                                aItem->XorFlags( flags );
                            else
                                aItem->ClearFlags( flags );

                            if( !aItem->HasFlag( STARTPOINT ) && !aItem->HasFlag( ENDPOINT ) )
                            {
                                unselect( aItem );
                                anySubtracted = true;
                            }

                            // We changed one line endpoint on a selected line,
                            // update the view at least.
                            if( flags && !anySubtracted )
                                getView()->Update( aItem );
                        }
                        else
                        {
                            aItem->SetFlags( flags );
                            select( aItem );
                            anyAdded = true;
                        }
                    };

            for( EDA_ITEM* item : nearbyItems )
            {
                bool           selected = false;
                EDA_ITEM_FLAGS flags    = 0;

                if( m_frame->GetRenderSettings()->m_ShowPinsElectricalType )
                    item->SetFlags( SHOW_ELEC_TYPE );

                if( Selectable( item ) )
                {
                    if( item->Type() == SCH_LINE_T )
                    {
                        SCH_LINE* line = static_cast<SCH_LINE*>( item );

                        if( ( isGreedy && line->HitTest( selectionRect, false ) )
                            || ( selectionRect.Contains( line->GetEndPoint() )
                                 && selectionRect.Contains( line->GetStartPoint() ) ) )
                        {
                            selected = true;
                            flags |= STARTPOINT | ENDPOINT;
                        }
                        else if( !isGreedy )
                        {
                            if( selectionRect.Contains( line->GetStartPoint() )
                                && line->IsStartDangling() )
                            {
                                selected = true;
                                flags |= STARTPOINT;
                            }

                            if( selectionRect.Contains( line->GetEndPoint() )
                                && line->IsEndDangling() )
                            {
                                selected = true;
                                flags |= ENDPOINT;
                            }
                        }
                    }
                    else
                    {
                        selected = item->HitTest( selectionRect, !isGreedy );
                    }
                }

                if( selected )
                {
                    item->SetFlags( CANDIDATE );
                    flaggedItems.push_back( item );
                    selectItem( item, flags );
                }

                item->ClearFlags( SHOW_ELEC_TYPE );
            }

            for( EDA_ITEM* item : nearbyChildren )
            {
                if( m_frame->GetRenderSettings()->m_ShowPinsElectricalType )
                    item->SetFlags( SHOW_ELEC_TYPE );

                if( Selectable( item )
                        && !item->GetParent()->HasFlag( CANDIDATE )
                        && item->HitTest( selectionRect, !isGreedy ) )
                {
                    selectItem( item, 0 );
                }

                item->ClearFlags( SHOW_ELEC_TYPE );
            }

            for( EDA_ITEM* item : flaggedItems )
                item->ClearFlags( CANDIDATE );

            m_selection.SetIsHover( false );

            // Inform other potentially interested tools
            if( anyAdded )
                m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

            if( anySubtracted )
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

            break;  // Stop waiting for events
        }

        // Allow some actions for navigation
        for( int i = 0; allowedActions[i]; ++i )
        {
            if( evt->IsAction( allowedActions[i] ) )
            {
                evt->SetPassEvent();
                break;
            }
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


EDA_ITEM* EE_SELECTION_TOOL::GetNode( const VECTOR2I& aPosition )
{
    EE_COLLECTOR collector;

    //TODO(snh): Reimplement after exposing KNN interface
    int pixelThreshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
    int gridThreshold = KiROUND( getView()->GetGAL()->GetGridSize().EuclideanNorm() );
    int thresholdMax = std::max( pixelThreshold, gridThreshold );

    for( int threshold : { 0, thresholdMax/4, thresholdMax/2, thresholdMax } )
    {
        collector.m_Threshold = threshold;
        collector.Collect( m_frame->GetScreen(), connectedTypes, aPosition );

        if( collector.GetCount() > 0 )
            break;
    }

    return collector.GetCount() ? collector[ 0 ] : nullptr;
}


int EE_SELECTION_TOOL::SelectNode( const TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( false );

    SelectPoint( cursorPos, connectedTypes );
    return 0;
}


int EE_SELECTION_TOOL::SelectConnection( const TOOL_EVENT& aEvent )
{
    RequestSelection( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } );

    if( m_selection.Empty() )
        return 0;

    SCH_LINE* line = (SCH_LINE*) m_selection.Front();
    unsigned  done = false;

    m_frame->GetScreen()->ClearDrawingState();
    std::set<SCH_ITEM*> conns = m_frame->GetScreen()->MarkConnections( line, false );

    for( SCH_ITEM* item : conns )
    {
        if( item->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } )
                && !item->IsSelected() )
        {
            done = true;
        }

        select( item );
    }

    if( !done )
    {
        conns = m_frame->GetScreen()->MarkConnections( line, true );

        for( SCH_ITEM* item : conns )
            select( item );
    }

    if( m_selection.GetSize() > 1 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int EE_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    ClearSelection();
    return 0;
}


void EE_SELECTION_TOOL::ZoomFitCrossProbeBBox( const BOX2I& aBBox )
{
    if( aBBox.GetWidth() == 0 )
        return;

    BOX2I bbox = aBBox;
    bbox.Normalize();

    VECTOR2I bbSize = bbox.Inflate( bbox.GetWidth() * 0.2f ).GetSize();
    VECTOR2D screenSize = getView()->GetViewport().GetSize();

    // This code tries to come up with a zoom factor that doesn't simply zoom in
    // to the cross probed symbol, but instead shows a reasonable amount of the
    // circuit around it to provide context.  This reduces or eliminates the need
    // to manually change the zoom because it's too close.

    // Using the default text height as a constant to compare against, use the
    // height of the bounding box of visible items for a footprint to figure out
    // if this is a big symbol (like a processor) or a small symbol (like a resistor).
    // This ratio is not useful by itself as a scaling factor.  It must be "bent" to
    // provide good scaling at varying symbol sizes.  Bigger symbols need less
    // scaling than small ones.
    double currTextHeight = schIUScale.MilsToIU( DEFAULT_TEXT_SIZE );

    double compRatio = bbSize.y / currTextHeight; // Ratio of symbol to text height
    double compRatioBent = 1.0;

    // LUT to scale zoom ratio to provide reasonable schematic context.  Must work
    // with symbols of varying sizes (e.g. 0402 package and 200 pin BGA).
    // "first" is used as the input and "second" as the output
    //
    // "first" = compRatio (symbol height / default text height)
    // "second" = Amount to scale ratio by
    std::vector<std::pair<double, double>> lut{ { 1.25, 16 }, // 32
                                                { 2.5, 12 },  //24
                                                { 5, 8 },     // 16
                                                { 6, 6 },     //
                                                { 10, 4 },    //8
                                                { 20, 2 },    //4
                                                { 40, 1.5 },  // 2
                                                { 100, 1 } };

    std::vector<std::pair<double, double>>::iterator it;

    // Large symbol default is last LUT entry (1:1).
    compRatioBent = lut.back().second;

    // Use LUT to do linear interpolation of "compRatio" within "first", then
    // use that result to linearly interpolate "second" which gives the scaling
    // factor needed.
    if( compRatio >= lut.front().first )
    {
        for( it = lut.begin(); it < lut.end() - 1; it++ )
        {
            if( it->first <= compRatio && next( it )->first >= compRatio )
            {
                double diffx = compRatio - it->first;
                double diffn = next( it )->first - it->first;

                compRatioBent = it->second + ( next( it )->second - it->second ) * diffx / diffn;
                break; // We have our interpolated value
            }
        }
    }
    else
    {
        compRatioBent = lut.front().second; // Small symbol default is first entry
    }

    // This is similar to the original KiCad code that scaled the zoom to make sure
    // symbols were visible on screen.  It's simply a ratio of screen size to
    // symbol size, and its job is to zoom in to make the component fullscreen.
    // Earlier in the code the symbol BBox is given a 20% margin to add some
    // breathing room. We compare the height of this enlarged symbol bbox to the
    // default text height.  If a symbol will end up with the sides clipped, we
    // adjust later to make sure it fits on screen.
    screenSize.x = std::max( 10.0, screenSize.x );
    screenSize.y = std::max( 10.0, screenSize.y );
    double ratio = std::max( -1.0, fabs( bbSize.y / screenSize.y ) );

    // Original KiCad code for how much to scale the zoom
    double kicadRatio =
            std::max( fabs( bbSize.x / screenSize.x ), fabs( bbSize.y / screenSize.y ) );

    // If the width of the part we're probing is bigger than what the screen width
    // will be after the zoom, then punt and use the KiCad zoom algorithm since it
    // guarantees the part's width will be encompassed within the screen.
    if( bbSize.x > screenSize.x * ratio * compRatioBent )
    {
        // Use standard KiCad zoom for parts too wide to fit on screen/
        ratio = kicadRatio;
        compRatioBent = 1.0; // Reset so we don't modify the "KiCad" ratio
        wxLogTrace( "CROSS_PROBE_SCALE",
                    "Part TOO WIDE for screen.  Using normal KiCad zoom ratio: %1.5f", ratio );
    }

    // Now that "compRatioBent" holds our final scaling factor we apply it to the
    // original fullscreen zoom ratio to arrive at the final ratio itself.
    ratio *= compRatioBent;

    bool alwaysZoom = false; // DEBUG - allows us to minimize zooming or not

    // Try not to zoom on every cross-probe; it gets very noisy
    if( ( ratio < 0.5 || ratio > 1.0 ) || alwaysZoom )
        getView()->SetScale( getView()->GetScale() / ratio );
}


void EE_SELECTION_TOOL::SyncSelection( const std::optional<SCH_SHEET_PATH>& targetSheetPath,
                                       SCH_ITEM* focusItem, const std::vector<SCH_ITEM*>& items )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return;

    if( targetSheetPath && targetSheetPath != editFrame->Schematic().CurrentSheet() )
    {
        editFrame->Schematic().SetCurrentSheet( *targetSheetPath );
        editFrame->DisplayCurrentSheet();
    }

    ClearSelection( items.size() > 0 ? true /*quiet mode*/ : false );

    // Perform individual selection of each item before processing the event.
    for( SCH_ITEM* item : items )
        select( item );

    BOX2I bbox = m_selection.GetBoundingBox();

    if( bbox.GetWidth() != 0 && bbox.GetHeight() != 0 )
    {
        if( m_frame->eeconfig()->m_CrossProbing.center_on_items )
        {
            if( m_frame->eeconfig()->m_CrossProbing.zoom_to_fit )
                ZoomFitCrossProbeBBox( bbox );

            editFrame->FocusOnItem( focusItem );

            if( !focusItem )
                editFrame->FocusOnLocation( bbox.Centre() );
        }
    }

    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
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
                select( &item );
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

                if( pin->IsPointClickableAnchor( grid.BestSnapAnchor(
                            *aPos, grid.GetItemGrid( static_cast<const SCH_ITEM*>( aItem ) ) ) ) )
                    return true;
            }

            return false;
        }

        break;
    }

    case SCH_DIRECTIVE_LABEL_T:
        if( !m_frame->eeconfig()->m_Appearance.show_directive_labels )
            return false;

        break;

    case LIB_SYMBOL_T:    // In symbol_editor we do not want to select the symbol itself.
        return false;

    case LIB_FIELD_T:     // LIB_FIELD object can always be edited.
        break;

    case LIB_SHAPE_T:
    case LIB_TEXT_T:
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


void EE_SELECTION_TOOL::ClearSelection( bool aQuietMode )
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( m_selection.Front(), SELECTED, &m_selection );

    getView()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    // Inform other potentially interested tools
    if( !aQuietMode )
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


void EE_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
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
                    {
                        aChild->SetSelected();
                        getView()->Hide( aChild, true );
                    }
                    else if( aMode == BRIGHTENED )
                    {
                        aChild->SetBrightened();
                    }
                } );
    }

    if( aGroup && aMode != BRIGHTENED )
        getView()->Hide( aItem, true );

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent(), KIGFX::REPAINT );
    else
        getView()->Update( aItem, KIGFX::REPAINT );
}


void EE_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    KICAD_T itemType = aItem->Type();

    if( aMode == SELECTED )
    {
        aItem->ClearSelected();
        // Lines need endpoints cleared here
        if( aItem->Type() == SCH_LINE_T )
            aItem->ClearFlags( STARTPOINT | ENDPOINT );

        if( aMode != BRIGHTENED )
            getView()->Hide( aItem, false );
    }
    else if( aMode == BRIGHTENED )
    {
        aItem->ClearBrightened();
    }

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
                    {
                        aChild->ClearSelected();
                        getView()->Hide( aChild, false );
                    }
                    else if( aMode == BRIGHTENED )
                    {
                        aChild->ClearBrightened();
                    }
                } );
    }

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent(), KIGFX::REPAINT );
    else
        getView()->Update( aItem, KIGFX::REPAINT );
}


bool EE_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    double         margin = getView()->ToWorld( GRIP_MARGIN );

    // Check if the point is located within any of the currently selected items bounding boxes
    for( EDA_ITEM* item : m_selection )
    {
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin ); // Give some margin for gripping an item

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


