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

#include <advanced_config.h>
#include <core/typeinfo.h>
#include <core/kicad_algo.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_compound.h>
#include <sch_actions.h>
#include <sch_collectors.h>
#include <sch_selection_tool.h>
#include <sch_base_frame.h>
#include <eeschema_id.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <math/util.h>
#include <deque>
#include <unordered_set>
#include <geometry/geometry_utils.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_line_chain.h>
#include <sch_painter.h>
#include <preview_items/selection_area.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <connection_graph.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_pin.h>
#include <sch_group.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_sheet_pin.h>
#include <sch_table.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_move_tool.h>
#include <tools/sch_point_editor.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_tool_utils.h>
#include <trigo.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <wx/log.h>

#include "symb_transforms_utils.h"


SELECTION_CONDITION SCH_CONDITIONS::SingleSymbol = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aSel.Front() );

        if( symbol )
            return !symbol->GetLibSymbolRef() || !symbol->GetLibSymbolRef()->IsPower();
    }

    return false;
};


SELECTION_CONDITION SCH_CONDITIONS::SingleSymbolOrPower = []( const SELECTION& aSel )
{
    return aSel.GetSize() == 1 && aSel.Front()->Type() == SCH_SYMBOL_T;
};


SELECTION_CONDITION SCH_CONDITIONS::SingleMultiBodyStyleSymbol = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aSel.Front() );

        if( symbol )
            return symbol->GetLibSymbolRef() && symbol->GetLibSymbolRef()->IsMultiBodyStyle();
    }

    return false;
};


SELECTION_CONDITION SCH_CONDITIONS::SingleMultiUnitSymbol = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( aSel.Front() );

        if( symbol )
            return symbol->GetLibSymbolRef() && symbol->GetLibSymbolRef()->GetUnitCount() >= 2;
    }

    return false;
};


SELECTION_CONDITION SCH_CONDITIONS::SingleMultiFunctionPin = []( const SELECTION& aSel )
{
    if( aSel.GetSize() == 1 )
    {
        SCH_PIN* pin = dynamic_cast<SCH_PIN*>( aSel.Front() );

        if( pin && pin->GetLibPin() )
            return !pin->GetLibPin()->GetAlternates().empty();
    }

    return false;
};


SELECTION_CONDITION SCH_CONDITIONS::SingleNonExcludedMarker = []( const SELECTION& aSel )
{
    if( aSel.CountType( SCH_MARKER_T ) != 1 )
        return false;

    return !static_cast<SCH_MARKER*>( aSel.Front() )->IsExcluded();
};


SELECTION_CONDITION SCH_CONDITIONS::MultipleSymbolsOrPower = []( const SELECTION& aSel )
{
    return aSel.GetSize() > 1 && aSel.OnlyContains( { SCH_SYMBOL_T } );
};


SELECTION_CONDITION SCH_CONDITIONS::AllPins = []( const SELECTION& aSel )
{
    return aSel.GetSize() >= 1 && aSel.OnlyContains( { SCH_PIN_T } );
};


SELECTION_CONDITION SCH_CONDITIONS::AllPinsOrSheetPins = []( const SELECTION& aSel )
{
    return aSel.GetSize() >= 1 && aSel.OnlyContains( { SCH_PIN_T, SCH_SHEET_PIN_T } );
};


static void passEvent( TOOL_EVENT* const aEvent, const TOOL_ACTION* const aAllowedActions[] )
{
    for( int i = 0; aAllowedActions[i]; ++i )
    {
        if( aEvent->IsAction( aAllowedActions[i] ) )
        {
            aEvent->SetPassEvent();
            break;
        }
    }
}


#define HITTEST_THRESHOLD_PIXELS 5


SCH_SELECTION_TOOL::SCH_SELECTION_TOOL() :
        SELECTION_TOOL( "common.InteractiveSelection" ),
        m_frame( nullptr ),
        m_nonModifiedCursor( KICURSOR::ARROW ),
        m_isSymbolEditor( false ),
        m_isSymbolViewer( false ),
        m_unit( 0 ),
        m_bodyStyle( 0 ),
        m_enteredGroup( nullptr ),
        m_selectionMode( SELECTION_MODE::INSIDE_RECTANGLE ),
        m_previous_first_cell( nullptr )
{
    m_filter.SetDefaults();
    m_selection.Clear();
}


SCH_SELECTION_TOOL::~SCH_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
    getView()->Remove( &m_enteredGroupOverlay );
}


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

static std::vector<KICAD_T> connectedLineTypes =
{
    SCH_ITEM_LOCATE_WIRE_T,
    SCH_ITEM_LOCATE_BUS_T
};

static std::vector<KICAD_T> expandConnectionGraphTypes =
{
    SCH_SYMBOL_T,
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
    SCH_JUNCTION_T,
    SCH_ITEM_LOCATE_GRAPHIC_LINE_T,
    SCH_SHAPE_T
};

static std::vector<KICAD_T> crossProbingTypes =
{
    SCH_SYMBOL_T,
    SCH_PIN_T,
    SCH_SHEET_T
};

static std::vector<KICAD_T> lineTypes = { SCH_LINE_T };
static std::vector<KICAD_T> sheetTypes = { SCH_SHEET_T };
static std::vector<KICAD_T> tableCellTypes = { SCH_TABLECELL_T };

bool SCH_SELECTION_TOOL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();

    SYMBOL_VIEWER_FRAME* symbolViewerFrame = dynamic_cast<SYMBOL_VIEWER_FRAME*>( m_frame );
    SYMBOL_EDIT_FRAME*   symbolEditorFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );

    if( symbolEditorFrame )
    {
        m_isSymbolEditor = true;
        m_unit = symbolEditorFrame->GetUnit();
        m_bodyStyle = symbolEditorFrame->GetBodyStyle();
    }
    else
    {
        m_isSymbolViewer = symbolViewerFrame != nullptr;
    }

    // clang-format off
    auto linesSelection =        SCH_CONDITIONS::MoreThan( 0 ) && SCH_CONDITIONS::OnlyTypes( lineTypes );
    auto wireOrBusSelection =    SCH_CONDITIONS::Count( 1 )    && SCH_CONDITIONS::OnlyTypes( connectedLineTypes );
    auto connectedSelection =    SCH_CONDITIONS::MoreThan( 0 ) && SCH_CONDITIONS::OnlyTypes( connectedTypes );
    auto expandableSelection =
                                 SCH_CONDITIONS::MoreThan( 0 ) && SCH_CONDITIONS::OnlyTypes( expandConnectionGraphTypes );
    auto sheetSelection =        SCH_CONDITIONS::Count( 1 )    && SCH_CONDITIONS::OnlyTypes( sheetTypes );
    auto crossProbingSelection = SCH_CONDITIONS::MoreThan( 0 ) && SCH_CONDITIONS::HasTypes( crossProbingTypes );
    auto tableCellSelection =    SCH_CONDITIONS::MoreThan( 0 ) && SCH_CONDITIONS::OnlyTypes( tableCellTypes );
    auto multiplePinsSelection = SCH_CONDITIONS::MoreThan( 1 ) && SCH_CONDITIONS::OnlyTypes( { SCH_PIN_T } );
    // clang-format on

    auto schEditSheetPageNumberCondition =
            [this] ( const SELECTION& aSel )
            {
                if( m_isSymbolEditor || m_isSymbolViewer )
                    return false;

                return SCH_CONDITIONS::LessThan( 2 )( aSel )
                        && SCH_CONDITIONS::OnlyTypes( sheetTypes )( aSel );
            };

    auto schEditCondition =
            [this] ( const SELECTION& aSel )
            {
                return !m_isSymbolEditor && !m_isSymbolViewer;
            };

    auto belowRootSheetCondition =
            [this]( const SELECTION& aSel )
            {
                SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

                return editFrame
                        && editFrame->GetCurrentSheet().Last() != &editFrame->Schematic().Root();
            };

    auto haveHighlight =
            [this]( const SELECTION& sel )
            {
                SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

                return editFrame && !editFrame->GetHighlightedConnection().IsEmpty();
            };

    auto haveSymbol =
            [this]( const SELECTION& sel )
            {
                return m_isSymbolEditor &&
                       static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();
            };

    auto groupEnterCondition =
            SELECTION_CONDITIONS::Count( 1 ) && SELECTION_CONDITIONS::HasType( SCH_GROUP_T );

    auto inGroupCondition =
            [this] ( const SELECTION& )
            {
                return m_enteredGroup != nullptr;
            };

    auto multipleUnitsSelection = []( const SELECTION& aSel )
        {
            return !GetSameSymbolMultiUnitSelection( aSel ).empty();
        };

    auto allowPinSwaps =
        [this]( const SELECTION& )
        {
            return m_frame->eeconfig() &&
                   m_frame->eeconfig()->m_Input.allow_unconstrained_pin_swaps;
        };


    auto& menu = m_menu->GetMenu();

    // clang-format off
    menu.AddItem( ACTIONS::groupEnter,                groupEnterCondition, 1 );
    menu.AddItem( ACTIONS::groupLeave,                inGroupCondition,    1 );
    menu.AddItem( SCH_ACTIONS::placeLinkedDesignBlock, groupEnterCondition, 1 );
    menu.AddItem( SCH_ACTIONS::saveToLinkedDesignBlock, groupEnterCondition, 1 );
    menu.AddItem( SCH_ACTIONS::clearHighlight,        haveHighlight && SCH_CONDITIONS::Idle, 1 );
    menu.AddSeparator(                                haveHighlight && SCH_CONDITIONS::Idle, 1 );

    menu.AddItem( SCH_ACTIONS::selectConnection,      expandableSelection && SCH_CONDITIONS::Idle, 2 );
    menu.AddItem( ACTIONS::selectColumns,             tableCellSelection && SCH_CONDITIONS::Idle, 2 );
    menu.AddItem( ACTIONS::selectRows,                tableCellSelection && SCH_CONDITIONS::Idle, 2 );
    menu.AddItem( ACTIONS::selectTable,               tableCellSelection && SCH_CONDITIONS::Idle, 2 );

    menu.AddSeparator( 100 );
    menu.AddItem( SCH_ACTIONS::drawWire,              schEditCondition && SCH_CONDITIONS::Empty, 100 );
    menu.AddItem( SCH_ACTIONS::drawBus,               schEditCondition && SCH_CONDITIONS::Empty, 100 );

    menu.AddSeparator( 100 );
    menu.AddItem( ACTIONS::finishInteractive,         SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus, 100 );

    menu.AddItem( SCH_ACTIONS::enterSheet,            sheetSelection && SCH_CONDITIONS::Idle, 150 );
    menu.AddItem( SCH_ACTIONS::selectOnPCB,           crossProbingSelection && schEditCondition && SCH_CONDITIONS::Idle, 150 );
    menu.AddItem( SCH_ACTIONS::leaveSheet,            belowRootSheetCondition, 150 );

    menu.AddSeparator( 200 );
    menu.AddItem( SCH_ACTIONS::placeJunction,         wireOrBusSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::placeLabel,            wireOrBusSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::placeClassLabel,       wireOrBusSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::placeGlobalLabel,      wireOrBusSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::placeHierLabel,        wireOrBusSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::breakWire,             linesSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::slice,                 linesSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::placeSheetPin,         sheetSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::autoplaceAllSheetPins, sheetSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::syncSheetPins,         sheetSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::swapPinLabels,         multiplePinsSelection && schEditCondition && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::swapUnitLabels,        multipleUnitsSelection && schEditCondition && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::swapPins,              multiplePinsSelection && schEditCondition && SCH_CONDITIONS::Idle && allowPinSwaps, 250 );
    menu.AddItem( SCH_ACTIONS::assignNetclass,        connectedSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::findNetInInspector,    connectedSelection && SCH_CONDITIONS::Idle, 250 );
    menu.AddItem( SCH_ACTIONS::editPageNumber,        schEditSheetPageNumberCondition, 250 );

    menu.AddSeparator( 400 );
    menu.AddItem( SCH_ACTIONS::symbolProperties,      haveSymbol && SCH_CONDITIONS::Empty, 400 );
    menu.AddItem( SCH_ACTIONS::pinTable,              haveSymbol && SCH_CONDITIONS::Empty, 400 );

    menu.AddSeparator( 1000 );
    m_frame->AddStandardSubMenus( *m_menu.get() );
    // clang-format on

    m_disambiguateTimer.SetOwner( this );
    Connect( m_disambiguateTimer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( SCH_SELECTION_TOOL::onDisambiguationExpire ), nullptr, this );

    return true;
}


void SCH_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();

    if( aReason != TOOL_BASE::REDRAW )
    {
        if( m_enteredGroup )
            ExitGroup();

        // Remove pointers to the selected items from containers without changing their
        // properties (as they are already deleted while a new sheet is loaded)
        m_selection.Clear();
    }

    if( aReason == RESET_REASON::SHUTDOWN )
        return;

    if( aReason == TOOL_BASE::MODEL_RELOAD || aReason == TOOL_BASE::SUPERMODEL_RELOAD )
    {
        getView()->GetPainter()->GetSettings()->SetHighlight( false );

        SYMBOL_EDIT_FRAME*   symbolEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        SYMBOL_VIEWER_FRAME* symbolViewerFrame = dynamic_cast<SYMBOL_VIEWER_FRAME*>( m_frame );

        if( symbolEditFrame )
        {
            m_isSymbolEditor = true;
            m_unit = symbolEditFrame->GetUnit();
            m_bodyStyle = symbolEditFrame->GetBodyStyle();
        }
        else
        {
            m_isSymbolViewer = symbolViewerFrame != nullptr;
        }
    }

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( &m_selection );
    getView()->Add( &m_selection );

    getView()->Remove( &m_enteredGroupOverlay );
    getView()->Add( &m_enteredGroupOverlay );
}

int SCH_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    KIID lastRolloverItem = niluuid;
    EE_GRID_HELPER grid( m_toolMgr );

    auto pinOrientation =
        []( EDA_ITEM* aItem )
        {
            SCH_PIN* pin = dynamic_cast<SCH_PIN*>( aItem );

            if( pin )
            {
                const SCH_SYMBOL* parent = dynamic_cast<const SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !parent )
                    return pin->GetOrientation();
                else
                {
                    SCH_PIN dummy( *pin );
                    RotateAndMirrorPin( dummy, parent->GetOrientation() );
                    return dummy.GetOrientation();
                }
            }

            SCH_SHEET_PIN* sheetPin = dynamic_cast<SCH_SHEET_PIN*>( aItem );

            if( sheetPin )
            {
                switch( sheetPin->GetSide() )
                {
                default:
                case SHEET_SIDE::LEFT:   return PIN_ORIENTATION::PIN_RIGHT;
                case SHEET_SIDE::RIGHT:  return PIN_ORIENTATION::PIN_LEFT;
                case SHEET_SIDE::TOP:    return PIN_ORIENTATION::PIN_DOWN;
                case SHEET_SIDE::BOTTOM: return PIN_ORIENTATION::PIN_UP;
                }
            }

            return PIN_ORIENTATION::PIN_LEFT;
        };

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
            else if( m_toolMgr->GetTool<SCH_POINT_EDITOR>()
                        && m_toolMgr->GetTool<SCH_POINT_EDITOR>()->HasPoint() )
            {
                // Distinguish point editor from selection modification by checking modifiers
                if( hasModifier() )
                {
                    m_originalCursor = m_toolMgr->GetMousePosition();
                    m_disambiguateTimer.StartOnce( ADVANCED_CFG::GetCfg().m_DisambiguationMenuDelay );
                }
            }
            else
            {
                m_originalCursor = m_toolMgr->GetMousePosition();
                m_disambiguateTimer.StartOnce( ADVANCED_CFG::GetCfg().m_DisambiguationMenuDelay );
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
                schframe->ClearFocus();

            // Collect items at the clicked location (doesn't select them yet)
            SCH_COLLECTOR                collector;
            SCH_SELECTION_FILTER_OPTIONS rejected;

            CollectHits( collector, evt->Position() );
            size_t preFilterCount = collector.GetCount();
            rejected.SetAll( false );
            narrowSelection( collector, evt->Position(), false, false, &rejected );

            if( m_selection.GetSize() != 0 && dynamic_cast<SCH_TABLECELL*>( m_selection.GetItem( 0 ) ) && m_additive
                && collector.GetCount() == 1 && dynamic_cast<SCH_TABLECELL*>( collector[0] ) )
            {
                SCH_TABLECELL* firstCell = static_cast<SCH_TABLECELL*>( m_selection.GetItem( 0 ) );
                SCH_TABLECELL* clickedCell = static_cast<SCH_TABLECELL*>( collector[0] );
                bool allCellsFromSameTable = true;

                if( m_previous_first_cell == nullptr || m_selection.GetSize() == 1)
                {
                    m_previous_first_cell = firstCell;
                }

                for( EDA_ITEM* selection : m_selection )
                {
                    if( !static_cast<SCH_TABLECELL*>( selection )
                        || selection->GetParent() != clickedCell->GetParent() )
                    {
                        allCellsFromSameTable = false;
                    }
                }

                if( m_previous_first_cell && clickedCell && allCellsFromSameTable )
                {
                    for( auto selection : m_selection )
                    {
                        selection->ClearSelected();
                    }
                    m_selection.Clear();
                    SCH_TABLE* parentTable = dynamic_cast<SCH_TABLE*>( m_previous_first_cell->GetParent() );

                    VECTOR2D start = m_previous_first_cell->GetCenter();
                    VECTOR2D end = clickedCell->GetCenter();

                    if( parentTable )
                    {
                        InitializeSelectionState( parentTable );

                        VECTOR2D topLeft( std::min( start.x, end.x ), std::min( start.y, end.y ) );
                        VECTOR2D bottomRight( std::max( start.x, end.x ), std::max( start.y, end.y ) );

                        SelectCellsBetween( topLeft, bottomRight - topLeft, parentTable );
                    }
                }
            }
            else if( collector.GetCount() == 1 && !m_isSymbolEditor && !hasModifier() )
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
                if( collector.GetCount() == 0 && preFilterCount > 0 )
                {
                    if( SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( m_frame ) )
                        frame->HighlightSelectionFilter( rejected );
                }

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
                SCH_COLLECTOR collector;

                if( CollectHits( collector, evt->Position(), { SCH_LOCATE_ANY_T } ) )
                {
                    ClearSelection();

                    SelectPoint( evt->Position(), { SCH_LOCATE_ANY_T }, nullptr, &selCancelled );
                    m_selection.SetIsHover( true );
                }
            }

            if( !selCancelled )
                m_menu->ShowContextMenu( m_selection );
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            m_disambiguateTimer.Stop();

            // double click? Display the properties window
            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->ClearFocus();

            if( m_selection.Empty() )
                SelectPoint( evt->Position() );

            EDA_ITEM* item = m_selection.Front();

            if( item && item->Type() == SCH_SHEET_T )
                m_toolMgr->PostAction( SCH_ACTIONS::enterSheet );
            else if( m_selection.GetSize() == 1 && m_selection[0]->Type() == SCH_GROUP_T )
                EnterGroup();
            else
                m_toolMgr->PostAction( SCH_ACTIONS::properties );
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
                schframe->ClearFocus();

            SCH_COLLECTOR collector;

            if( m_selection.GetSize() == 1 && dynamic_cast<SCH_TABLE*>( m_selection.GetItem( 0 ) ) )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::move );
            }
            // Allow drag selecting table cells, except when they're inside a group that we haven't entered
            else if( CollectHits( collector, evt->DragOrigin(), { SCH_TABLECELL_T } )
                     && ( collector[0]->GetParent()->GetParentGroup() == nullptr
                          || collector[0]->GetParent()->GetParentGroup() == m_enteredGroup ) )
            {
                selectTableCells( static_cast<SCH_TABLE*>( collector[0]->GetParent() ) );
            }
            else if( hasModifier() || drag_action == MOUSE_DRAG_ACTION::SELECT )
            {
                if( m_selectionMode == SELECTION_MODE::INSIDE_LASSO
                        || m_selectionMode == SELECTION_MODE::TOUCHING_LASSO )
                    selectLasso();
                else
                    selectMultiple();
            }
            else if( m_selection.Empty() && drag_action != MOUSE_DRAG_ACTION::DRAG_ANY )
            {
                if( m_selectionMode == SELECTION_MODE::INSIDE_LASSO
                        || m_selectionMode == SELECTION_MODE::TOUCHING_LASSO )
                    selectLasso();
                else
                    selectMultiple();
            }
            else
            {
                if( m_isSymbolEditor )
                {
                    if( static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->IsSymbolAlias() )
                    {
                        m_selection = RequestSelection( { SCH_FIELD_T } );
                    }
                    else
                    {
                        m_selection = RequestSelection( { SCH_SHAPE_T,
                                                          SCH_TEXT_T,
                                                          SCH_TEXTBOX_T,
                                                          SCH_PIN_T,
                                                          SCH_FIELD_T } );
                    }
                }
                else
                {
                    m_selection = RequestSelection( SCH_COLLECTOR::MovableItems );
                }

                // Check if dragging has started within any of selected items bounding box
                if( evt->HasPosition() && selectionContains( evt->DragOrigin() ) )
                {
                    // drag_is_move option exists only in schematic editor, not in symbol editor
                    // (m_frame->eeconfig() returns nullptr in Symbol Editor)
                    if( m_isSymbolEditor || m_frame->eeconfig()->m_Input.drag_is_move )
                        m_toolMgr->RunAction( SCH_ACTIONS::move );
                    else
                        m_toolMgr->RunAction( SCH_ACTIONS::drag );
                }
                else
                {
                    // No -> drag a selection box
                    if( m_selectionMode == SELECTION_MODE::INSIDE_LASSO
                            || m_selectionMode == SELECTION_MODE::TOUCHING_LASSO )
                        selectLasso();
                    else
                        selectMultiple();
                }
            }
        }
        else if( evt->IsMouseDown( BUT_AUX1 ) )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::navigateBack );
        }
        else if( evt->IsMouseDown( BUT_AUX2 ) )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::navigateForward );
        }
        else if( evt->Action() == TA_MOUSE_WHEEL )
        {
            int field = -1;

            if( evt->Modifier() == ( MD_SHIFT | MD_ALT ) )
                field = 0;
            else if( evt->Modifier() == ( MD_CTRL | MD_ALT ) )
                field = 1;
            // any more?

            if( field >= 0 )
            {
                const int          delta = evt->Parameter<int>();
                ACTIONS::INCREMENT incParams{ delta > 0 ? 1 : -1, field };

                m_toolMgr->RunAction( ACTIONS::increment, incParams );
            }
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            m_disambiguateTimer.Stop();

            // context sub-menu selection?  Handle unit selection or bus unfolding
            if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_UNIT
                && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_UNIT_END )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selection.Front() );
                int unit = *evt->GetCommandId() - ID_POPUP_SCH_SELECT_UNIT;

                if( symbol )
                    static_cast<SCH_EDIT_FRAME*>( m_frame )->SelectUnit( symbol, unit );
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_PLACE_UNIT
                     && *evt->GetCommandId() <= ID_POPUP_SCH_PLACE_UNIT_END )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selection.Front() );
                int unit = *evt->GetCommandId() - ID_POPUP_SCH_PLACE_UNIT;

                if( symbol )
                    m_toolMgr->RunAction( SCH_ACTIONS::placeNextSymbolUnit,
                                           SCH_ACTIONS::PLACE_SYMBOL_UNIT_PARAMS{ symbol, unit } );
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_SELECT_BODY_STYLE
                     && *evt->GetCommandId() <= ID_POPUP_SCH_SELECT_BODY_STYLE_END )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_selection.Front() );
                int bodyStyle = ( *evt->GetCommandId() - ID_POPUP_SCH_SELECT_BODY_STYLE ) + 1;

                if( symbol && symbol->GetBodyStyle() != bodyStyle )
                    static_cast<SCH_EDIT_FRAME*>( m_frame )->SelectBodyStyle( symbol, bodyStyle );
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_ALT_PIN_FUNCTION
                     && *evt->GetCommandId() <= ID_POPUP_SCH_ALT_PIN_FUNCTION_END )
            {
                SCH_PIN* pin = dynamic_cast<SCH_PIN*>( m_selection.Front() );
                wxString alt = *evt->Parameter<wxString*>();

                if( pin )
                    static_cast<SCH_EDIT_FRAME*>( m_frame )->SetAltPinFunction( pin, alt );
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_PIN_TRICKS_START
                     && *evt->GetCommandId() <= ID_POPUP_SCH_PIN_TRICKS_END
                     && !m_isSymbolEditor && !m_isSymbolViewer )
            {
                SCH_EDIT_FRAME* sch_frame = static_cast<SCH_EDIT_FRAME*>( m_frame );

                // Keep track of new items so we make them the new selection at the end
                EDA_ITEMS  newItems;
                SCH_COMMIT commit( sch_frame );

                if( *evt->GetCommandId() == ID_POPUP_SCH_PIN_TRICKS_NO_CONNECT )
                {
                    for( EDA_ITEM* item : m_selection )
                    {
                        if( item->Type() != SCH_PIN_T && item->Type() != SCH_SHEET_PIN_T )
                            continue;

                        SCH_NO_CONNECT* nc = new SCH_NO_CONNECT( item->GetPosition() );
                        commit.Add( nc, sch_frame->GetScreen() );
                        newItems.push_back( nc );
                    }

                    if( !commit.Empty() )
                    {
                        commit.Push( wxS( "No Connect Pins" ) );
                        ClearSelection();
                    }
                }
                else if( *evt->GetCommandId() == ID_POPUP_SCH_PIN_TRICKS_WIRE )
                {
                    VECTOR2I wireGrid = grid.GetGridSize( GRID_HELPER_GRIDS::GRID_WIRES );

                    for( EDA_ITEM* item : m_selection )
                    {
                        if( item->Type() != SCH_PIN_T && item->Type() != SCH_SHEET_PIN_T )
                            continue;

                        SCH_LINE* wire = new SCH_LINE( item->GetPosition(), LAYER_WIRE );

                        // Add some length to the wire as nothing in our code base handles
                        // 0 length wires very well, least of all the ortho drag algorithm
                        VECTOR2I stub;

                        switch( pinOrientation( item ) )
                        {
                        default:
                        case PIN_ORIENTATION::PIN_RIGHT:
                            stub = VECTOR2I( -1 * wireGrid.x, 0 );
                            break;
                        case PIN_ORIENTATION::PIN_LEFT:
                            stub = VECTOR2I( 1 * wireGrid.x, 0 );
                            break;
                        case PIN_ORIENTATION::PIN_UP:
                            stub = VECTOR2I( 0, 1 * wireGrid.y );
                            break;
                        case PIN_ORIENTATION::PIN_DOWN:
                            stub = VECTOR2I( 0, -1 * wireGrid.y );
                            break;
                        }

                        wire->SetEndPoint( item->GetPosition() + stub );

                        m_frame->AddToScreen( wire, sch_frame->GetScreen() );
                        commit.Added( wire, sch_frame->GetScreen() );
                        newItems.push_back( wire );
                    }

                    if( !commit.Empty() )
                    {
                        ClearSelection();
                        AddItemsToSel( &newItems );

                        // Select only the ends so we can immediately start dragging them
                        for( EDA_ITEM* item : newItems )
                            static_cast<SCH_LINE*>( item )->SetFlags( ENDPOINT );

                        KIGFX::VIEW_CONTROLS* vc = getViewControls();

                        // Put the mouse on the nearest point of the first wire
                        SCH_LINE* first = static_cast<SCH_LINE*>( newItems[0] );
                        vc->SetCrossHairCursorPosition( first->GetEndPoint(), false );
                        vc->WarpMouseCursor( vc->GetCursorPosition(), true );

                        // Start the drag tool, canceling will remove the wires
                        if( m_toolMgr->RunSynchronousAction( SCH_ACTIONS::drag, &commit ) )
                            commit.Push( wxS( "Wire Pins" ) );
                        else
                            commit.Revert();
                    }
                }
                else
                {
                    // For every pin in the selection, add a label according to menu item
                    // selected by the user
                    for( EDA_ITEM* item : m_selection )
                    {
                        SCH_PIN*        pin = dynamic_cast<SCH_PIN*>( item );
                        SCH_SHEET_PIN*  sheetPin = dynamic_cast<SCH_SHEET_PIN*>( item );
                        SCH_LABEL_BASE* label = nullptr;
                        SCH_SHEET_PATH& sheetPath = sch_frame->GetCurrentSheet();

                        wxString labelText;

                        if( pin )
                        {
                            labelText = pin->GetShownName();

                            if( labelText.IsEmpty() )
                            {
                                labelText.Printf( "%s_%s",
                                                  pin->GetParentSymbol()->GetRef( &sheetPath ),
                                                  pin->GetNumber() );
                            }
                        }
                        else if( sheetPin )
                        {
                            labelText = sheetPin->GetShownText( &sheetPath, false );
                        }
                        else
                        {
                            continue;
                        }

                        switch( *evt->GetCommandId() )
                        {
                        case ID_POPUP_SCH_PIN_TRICKS_NET_LABEL:
                            label = new SCH_LABEL( item->GetPosition(), labelText );
                            break;
                        case ID_POPUP_SCH_PIN_TRICKS_HIER_LABEL:
                            label = new SCH_HIERLABEL( item->GetPosition(), labelText );
                            break;
                        case ID_POPUP_SCH_PIN_TRICKS_GLOBAL_LABEL:
                            label = new SCH_GLOBALLABEL( item->GetPosition(), labelText );
                            break;
                        default:
                            continue;
                        }

                        switch( pinOrientation( item ) )
                        {
                        default:
                        case PIN_ORIENTATION::PIN_RIGHT:
                            label->SetSpinStyle( SPIN_STYLE::SPIN::LEFT );
                            break;
                        case PIN_ORIENTATION::PIN_LEFT:
                            label->SetSpinStyle( SPIN_STYLE::SPIN::RIGHT );
                            break;
                        case PIN_ORIENTATION::PIN_UP:
                            label->SetSpinStyle( SPIN_STYLE::SPIN::BOTTOM );
                            break;
                        case PIN_ORIENTATION::PIN_DOWN:
                            label->SetSpinStyle( SPIN_STYLE::SPIN::UP );
                            break;
                        }

                        ELECTRICAL_PINTYPE pinType = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;

                        if( pin )
                        {
                            pinType = pin->GetType();
                        }
                        else if( sheetPin )
                        {
                            switch( sheetPin->GetLabelShape() )
                            {
                            case LABEL_INPUT:    pinType = ELECTRICAL_PINTYPE::PT_INPUT;    break;
                            case LABEL_OUTPUT:   pinType = ELECTRICAL_PINTYPE::PT_OUTPUT;   break;
                            case LABEL_BIDI:     pinType = ELECTRICAL_PINTYPE::PT_BIDI;     break;
                            case LABEL_TRISTATE: pinType = ELECTRICAL_PINTYPE::PT_TRISTATE; break;
                            case LABEL_PASSIVE:  pinType = ELECTRICAL_PINTYPE::PT_PASSIVE;  break;
                            }
                        }

                        switch( pinType )
                        {
                        case ELECTRICAL_PINTYPE::PT_BIDI:
                            label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
                            break;
                        case ELECTRICAL_PINTYPE::PT_INPUT:
                            label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
                            break;
                        case ELECTRICAL_PINTYPE::PT_OUTPUT:
                            label->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
                            break;
                        case ELECTRICAL_PINTYPE::PT_TRISTATE:
                            label->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE );
                            break;
                        case ELECTRICAL_PINTYPE::PT_UNSPECIFIED:
                            label->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );
                            break;
                        default:
                            label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
                        }

                        commit.Add( label, sch_frame->GetScreen() );
                        newItems.push_back( label );
                    }

                    if( !commit.Empty() )
                    {
                        commit.Push( wxS( "Label Pins" ) );

                        // Many users will want to drag these items to wire off of the pins, so
                        // pre-select them.
                        ClearSelection();
                        AddItemsToSel( &newItems );
                    }
                }
            }
            else if( *evt->GetCommandId() >= ID_POPUP_SCH_UNFOLD_BUS
                     && *evt->GetCommandId() <= ID_POPUP_SCH_UNFOLD_BUS_END )
            {
                wxString* net = new wxString( *evt->Parameter<wxString*>() );
                m_toolMgr->RunAction<wxString*>( SCH_ACTIONS::unfoldBus, net );
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
                schframe->ClearFocus();

            if( !GetSelection().Empty() )
            {
                ClearSelection();
            }
            else if( evt->FirstResponder() == this && evt->GetCommandId() == (int) WXK_ESCAPE )
            {
                if( m_enteredGroup )
                {
                    ExitGroup();
                }
                else
                {
                    SCH_EDITOR_CONTROL* editor = m_toolMgr->GetTool<SCH_EDITOR_CONTROL>();

                    if( editor && m_frame->eeconfig()->m_Input.esc_clears_net_highlight )
                        editor->ClearHighlight( *evt );
                }
            }
        }
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            if( SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
                schframe->ClearFocus();
        }
        else if( evt->IsMotion() && !m_isSymbolEditor && evt->FirstResponder() == this )
        {
            // Update cursor and rollover item
            rolloverItem = niluuid;
            SCH_COLLECTOR collector;

            getViewControls()->ForceCursorPosition( false );

            if( CollectHits( collector, evt->Position() ) )
            {
                narrowSelection( collector, evt->Position(), false, false, nullptr );

                if( collector.GetCount() == 1 && !hasModifier() )
                {
                    OPT_TOOL_EVENT autostartEvt = autostartEvent( evt, grid, collector[0] );

                    if( autostartEvt )
                    {
                        if( autostartEvt->Matches( SCH_ACTIONS::drawBus.MakeEvent() ) )
                            displayBusCursor = true;
                        else if( autostartEvt->Matches( SCH_ACTIONS::drawWire.MakeEvent() ) )
                            displayWireCursor = true;
                        else if( autostartEvt->Matches( SCH_ACTIONS::drawLines.MakeEvent() ) )
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

        if( lastRolloverItem != niluuid && lastRolloverItem != rolloverItem )
        {
            EDA_ITEM* item = m_frame->ResolveItem( lastRolloverItem );

            if( item->IsRollover() )
            {
                item->SetIsRollover( false );

                if( item->Type() == SCH_FIELD_T || item->Type() == SCH_TABLECELL_T )
                    m_frame->GetCanvas()->GetView()->Update( item->GetParent() );
                else
                    m_frame->GetCanvas()->GetView()->Update( item );
            }
        }

        if( rolloverItem != niluuid )
        {
            EDA_ITEM* item = m_frame->ResolveItem( rolloverItem );

            if( !item->IsRollover() )
            {
                item->SetIsRollover( true );

                if( item->Type() == SCH_FIELD_T || item->Type() == SCH_TABLECELL_T )
                    m_frame->GetCanvas()->GetView()->Update( item->GetParent() );
                else
                    m_frame->GetCanvas()->GetView()->Update( item );
            }
        }

        lastRolloverItem = rolloverItem;

        if( m_frame->ToolStackIsEmpty() )
        {
            if( displayWireCursor )
            {
                m_nonModifiedCursor = KICURSOR::LINE_WIRE;
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
                        && selectionContains( evt->Position() ) //move/drag option prediction
            )
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


void SCH_SELECTION_TOOL::EnterGroup()
{
    wxCHECK_RET( m_selection.GetSize() == 1 && m_selection[0]->Type() == SCH_GROUP_T,
                 wxT( "EnterGroup called when selection is not a single group" ) );
    SCH_GROUP* aGroup = static_cast<SCH_GROUP*>( m_selection[0] );

    if( m_enteredGroup != nullptr )
        ExitGroup();

    ClearSelection();
    m_enteredGroup = aGroup;
    m_enteredGroup->SetFlags( ENTERED );
    m_enteredGroup->RunOnChildren(
            [&]( SCH_ITEM* aChild )
            {
                if( aChild->Type() == SCH_LINE_T )
                    aChild->SetFlags( STARTPOINT | ENDPOINT );

                select( aChild );
            },
            RECURSE_MODE::NO_RECURSE );

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    getView()->Hide( m_enteredGroup, true );
    m_enteredGroupOverlay.Add( m_enteredGroup );
    getView()->Update( &m_enteredGroupOverlay );
}


void SCH_SELECTION_TOOL::ExitGroup( bool aSelectGroup )
{
    // Only continue if there is a group entered
    if( m_enteredGroup == nullptr )
        return;

    m_enteredGroup->ClearFlags( ENTERED );
    getView()->Hide( m_enteredGroup, false );
    ClearSelection();

    if( aSelectGroup )
    {
        select( m_enteredGroup );
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    m_enteredGroupOverlay.Clear();
    m_enteredGroup = nullptr;
    getView()->Update( &m_enteredGroupOverlay );
}


OPT_TOOL_EVENT SCH_SELECTION_TOOL::autostartEvent( TOOL_EVENT* aEvent, EE_GRID_HELPER& aGrid,
                                                   SCH_ITEM* aItem )
{
    VECTOR2I pos = aGrid.BestSnapAnchor( aEvent->Position(), aGrid.GetItemGrid( aItem ) );

    if( m_frame->eeconfig()->m_Drawing.auto_start_wires
            && !m_toolMgr->GetTool<SCH_POINT_EDITOR>()->HasPoint()
            && aItem->IsPointClickableAnchor( pos ) )
    {
        OPT_TOOL_EVENT newEvt = SCH_ACTIONS::drawWire.MakeEvent();

        if( aItem->Type() == SCH_BUS_BUS_ENTRY_T )
        {
            newEvt = SCH_ACTIONS::drawBus.MakeEvent();
        }
        else if( aItem->Type() == SCH_BUS_WIRE_ENTRY_T )
        {
            SCH_BUS_WIRE_ENTRY* busEntry = static_cast<SCH_BUS_WIRE_ENTRY*>( aItem );

            if( !busEntry->m_connected_bus_item )
                newEvt = SCH_ACTIONS::drawBus.MakeEvent();
        }
        else if( aItem->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( aItem );

            if( line->IsBus() )
                newEvt = SCH_ACTIONS::drawBus.MakeEvent();
            else if( line->IsGraphicLine() )
                newEvt = SCH_ACTIONS::drawLines.MakeEvent();
        }
        else if( aItem->Type() == SCH_LABEL_T || aItem->Type() == SCH_HIER_LABEL_T
                 || aItem->Type() == SCH_SHEET_PIN_T || aItem->Type() == SCH_GLOBAL_LABEL_T )
        {
            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( aItem );
            SCH_CONNECTION  possibleConnection( label->Schematic()->ConnectionGraph() );
            possibleConnection.ConfigureFromLabel( label->GetShownText( false ) );

            if( possibleConnection.IsBus() )
                newEvt = SCH_ACTIONS::drawBus.MakeEvent();
        }
        else if( aItem->Type() == SCH_SYMBOL_T )
        {
            const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( aItem );
            const SCH_PIN*    pin = symbol->GetPin( pos );

            if( !pin || !pin->IsPointClickableAnchor( pos ) )
                return OPT_TOOL_EVENT();

            if( !pin->IsVisible()
                && !( m_frame->eeconfig()->m_Appearance.show_hidden_pins
                      || m_frame->GetRenderSettings()->m_ShowHiddenPins ) )
            {
                return OPT_TOOL_EVENT();
            }
        }

        newEvt->SetMousePosition( pos );
        newEvt->SetHasPosition( true );
        newEvt->SetForceImmediate( true );

        getViewControls()->ForceCursorPosition( true, pos );

        return newEvt;
    }

    return OPT_TOOL_EVENT();
}


int SCH_SELECTION_TOOL::disambiguateCursor( const TOOL_EVENT& aEvent )
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


void SCH_SELECTION_TOOL::OnIdle( wxIdleEvent& aEvent )
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


SCH_SELECTION& SCH_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


bool SCH_SELECTION_TOOL::CollectHits( SCH_COLLECTOR& aCollector, const VECTOR2I& aWhere,
                                      const std::vector<KICAD_T>& aScanTypes )
{
    int pixelThreshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
    int gridThreshold = KiROUND( getView()->GetGAL()->GetGridSize().EuclideanNorm() / 2.0 );
    aCollector.m_Threshold = std::max( pixelThreshold, gridThreshold );
    aCollector.m_ShowPinElectricalTypes = m_frame->GetRenderSettings()->m_ShowPinsElectricalType;

    if( m_isSymbolEditor )
    {
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return false;

        aCollector.Collect( symbol->GetDrawItems(), aScanTypes, aWhere, m_unit, m_bodyStyle );
    }
    else
    {
        aCollector.Collect( m_frame->GetScreen(), aScanTypes, aWhere, m_unit, m_bodyStyle );

        // If pins are disabled in the filter, they will be removed later.  Let's add the parent
        // so that people can use pins to select symbols in this case.
        if( !m_filter.pins )
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


void SCH_SELECTION_TOOL::narrowSelection( SCH_COLLECTOR& collector, const VECTOR2I& aWhere,
                                          bool aCheckLocked, bool aSelectedOnly,
                                          SCH_SELECTION_FILTER_OPTIONS* aRejected )
{
    SYMBOL_EDIT_FRAME* symbolEditorFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );

    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( symbolEditorFrame )
        {
            // Do not select invisible items if they are not displayed
            EDA_ITEM* item = collector[i];

            if( item->Type() == SCH_FIELD_T )
            {
                if( !static_cast<SCH_FIELD*>( item )->IsVisible()
                    && !symbolEditorFrame->GetShowInvisibleFields() )
                {
                    collector.Remove( i );
                    continue;
                }
            }
            else if( item->Type() == SCH_PIN_T )
            {
                if( !static_cast<SCH_PIN*>( item )->IsVisible()
                    && !symbolEditorFrame->GetShowInvisiblePins() )
                {
                    collector.Remove( i );
                    continue;
                }
            }
        }

        if( !Selectable( collector[i], &aWhere ) )
        {
            collector.Remove( i );
            continue;
        }

        if( aCheckLocked && collector[i]->IsLocked() )
        {
            if( aRejected )
                aRejected->lockedItems = true;
            collector.Remove( i );
            continue;
        }

        if( !itemPassesFilter( collector[i], aRejected ) )
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

    filterCollectorForHierarchy( collector, false );

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
        GuessSelectionCandidates( collector, aWhere );
}


bool SCH_SELECTION_TOOL::selectPoint( SCH_COLLECTOR& aCollector, const VECTOR2I& aWhere,
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
        if( !m_toolMgr->RunAction<COLLECTOR*>( ACTIONS::selectionMenu, &aCollector ) )
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

    // It is possible for slop in the selection model to cause us to be outside the group,
    // but also selecting an item within the group, so only exit if the selection doesn't
    // have an item belonging to the group
    if( m_enteredGroup && !m_enteredGroup->GetBoundingBox().Contains( aWhere ) )
    {
        bool foundEnteredGroup = false;
        for( EDA_ITEM* item : aCollector )
        {
            if( item->GetParentGroup() == m_enteredGroup )
            {
                foundEnteredGroup = true;
                break;
            }
        }

        if( !foundEnteredGroup )
            ExitGroup();
    }

    filterCollectorForHierarchy( aCollector, true );

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

                if( line->GetStartPoint().Distance( aWhere ) <= aCollector.m_Threshold )
                    flags = STARTPOINT;
                else if( line->GetEndPoint().Distance( aWhere ) <= aCollector.m_Threshold )
                    flags = ENDPOINT;
                else
                    flags = STARTPOINT | ENDPOINT;
            }

            if( aSubtract
                || ( aExclusiveOr && aCollector[i]->IsSelected()
                     && ( !isLine || ( isLine && aCollector[i]->HasFlag( flags ) ) ) ) )
            {
                aCollector[i]->ClearFlags( flags );

                // Need to update end shadows after ctrl-click unselecting one of two selected
                // endpoints.
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

    m_frame->GetCanvas()->ForceRefresh();
    return false;
}


bool SCH_SELECTION_TOOL::SelectPoint( const VECTOR2I& aWhere,
                                      const std::vector<KICAD_T>& aScanTypes,
                                      EDA_ITEM** aItem, bool* aSelectionCancelledFlag,
                                      bool aCheckLocked, bool aAdd, bool aSubtract,
                                      bool aExclusiveOr )
{
    SCH_COLLECTOR collector;

    if( !CollectHits( collector, aWhere, aScanTypes ) )
        return false;

    size_t preFilterCount = collector.GetCount();
    SCH_SELECTION_FILTER_OPTIONS rejected;
    rejected.SetAll( false );
    narrowSelection( collector, aWhere, aCheckLocked, aSubtract, &rejected );

    if( collector.GetCount() == 0 && preFilterCount > 0 )
    {
        if( SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( m_frame ) )
            frame->HighlightSelectionFilter( rejected );

        return false;
    }

    return selectPoint( collector, aWhere, aItem, aSelectionCancelledFlag, aAdd, aSubtract,
                        aExclusiveOr );
}


int SCH_SELECTION_TOOL::SelectAll( const TOOL_EVENT& aEvent )
{
    SCH_COLLECTOR collection;
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    std::vector<EDA_ITEM*> sheetPins;

    // Filter the view items based on the selection box
    BOX2I selectionBox;

    selectionBox.SetMaximum();
    view->Query( selectionBox,
            [&]( KIGFX::VIEW_ITEM* viewItem ) -> bool
            {
                SCH_ITEM* item = static_cast<SCH_ITEM*>( viewItem );

                if( !item )
                    return true;

                collection.Append( item );
                return true;
            } );

    filterCollectorForHierarchy( collection, true );

    // Sheet pins aren't in the view; add them by hand
    for( EDA_ITEM* item : collection )
    {
        SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item );

        if( sheet )
        {
            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                sheetPins.emplace_back( pin );
        }
    }

    for( EDA_ITEM* pin : sheetPins )
        collection.Append( pin );

    for( EDA_ITEM* item : collection )
    {
        if( Selectable( item ) && itemPassesFilter( item, nullptr ) )
        {
            if( item->Type() == SCH_LINE_T )
                item->SetFlags( STARTPOINT | ENDPOINT );

            select( item );
        }
    }

    m_multiple = false;

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    m_frame->GetCanvas()->ForceRefresh();
    return 0;
}

int SCH_SELECTION_TOOL::UnselectAll( const TOOL_EVENT& aEvent )
{
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    // hold all visible items
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    // Filter the view items based on the selection box
    BOX2I selectionBox;

    selectionBox.SetMaximum();
    view->Query( selectionBox, selectedItems );         // Get the list of selected items

    for( KIGFX::VIEW::LAYER_ITEM_PAIR& pair : selectedItems )
    {
        SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( pair.first );

        if( sheet )
        {
            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                EDA_ITEM* item = dynamic_cast<EDA_ITEM*>( pin );

                if( item && Selectable( item ) )
                    unselect( item );
            }
        }

        if( EDA_ITEM* item = dynamic_cast<EDA_ITEM*>( pair.first ) )
        {
            if( Selectable( item ) )
                unselect( item );
        }
    }

    m_multiple = false;

    m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    m_frame->GetCanvas()->ForceRefresh();
    return 0;
}


void SCH_SELECTION_TOOL::GuessSelectionCandidates( SCH_COLLECTOR& collector, const VECTOR2I& aPos )
{
    // Prefer exact hits to sloppy ones
    std::set<EDA_ITEM*> exactHits;

    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        EDA_ITEM*   item = collector[ i ];
        SCH_LINE*   line = dynamic_cast<SCH_LINE*>( item );
        SCH_SHAPE*  shape = dynamic_cast<SCH_SHAPE*>( item );
        SCH_TABLE*  table = dynamic_cast<SCH_TABLE*>( item );

        // Lines are hard to hit.  Give them a bit more slop to still be considered "exact".
        if( line || ( shape && shape->GetShape() == SHAPE_T::POLY )
                 || ( shape && shape->GetShape() == SHAPE_T::ARC ) )
        {
            int pixelThreshold = KiROUND( getView()->ToWorld( 6 ) );

            if( item->HitTest( aPos, pixelThreshold ) )
                exactHits.insert( item );
        }
        else if( table )
        {
            // Consider table cells exact, but not the table itself
        }
        else
        {

            if( m_frame->GetRenderSettings()->m_ShowPinsElectricalType )
                item->SetFlags( SHOW_ELEC_TYPE );

            if( item->HitTest( aPos, 0 ) )
                exactHits.insert( item );

            item->ClearFlags( SHOW_ELEC_TYPE );
        }
    }

    if( exactHits.size() > 0 && exactHits.size() < (unsigned) collector.GetCount() )
    {
        for( int i = collector.GetCount() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = collector[ i ];

            if( !exactHits.contains( item ) )
                collector.Transfer( item );
        }
    }

    // Find the closest item.  (Note that at this point all hits are either exact or non-exact.)
    SEG       poss( aPos, aPos );
    EDA_ITEM* closest = nullptr;
    int       closestDist = INT_MAX / 4;

    for( EDA_ITEM* item : collector )
    {
        BOX2I bbox = item->GetBoundingBox();
        int   dist = INT_MAX / 4;

        // A dominating item is one that would unfairly win distance tests
        // and mask out other items. For example, a filled rectangle "wins"
        // with a zero distance over anything inside it.
        bool dominating = false;

        if( exactHits.contains( item ) )
        {
            if( item->Type() == SCH_PIN_T || item->Type() == SCH_JUNCTION_T )
            {
                closest = item;
                break;
            }

            SCH_LINE*   line = dynamic_cast<SCH_LINE*>( item );
            SCH_FIELD*  field = dynamic_cast<SCH_FIELD*>( item );
            EDA_TEXT*   text = dynamic_cast<EDA_TEXT*>( item );
            EDA_SHAPE*  shape = dynamic_cast<EDA_SHAPE*>( item );
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

            if( line )
            {
                dist = line->GetSeg().Distance( aPos );
            }
            else if( field )
            {
                BOX2I     box = field->GetBoundingBox();
                EDA_ANGLE orient = field->GetTextAngle();

                if( field->GetParent() && field->GetParent()->Type() == SCH_SYMBOL_T )
                {
                    if( static_cast<SCH_SYMBOL*>( field->GetParent() )->GetTransform().y1 )
                    {
                        if( orient.IsHorizontal() )
                            orient = ANGLE_VERTICAL;
                        else
                            orient = ANGLE_HORIZONTAL;
                    }
                }

                field->GetEffectiveTextShape( false, box, orient )->Collide( poss, INT_MAX / 4, &dist );
            }
            else if( text )
            {
                text->GetEffectiveTextShape( false )->Collide( poss, INT_MAX / 4, &dist );
            }
            else if( shape )
            {
                auto shapes = std::make_shared<SHAPE_COMPOUND>( shape->MakeEffectiveShapesForHitTesting() );

                shapes->Collide( poss, INT_MAX / 4, &dist );

                // Filled shapes win hit tests anywhere inside them
                dominating = shape->IsFilledForHitTesting();
            }
            else if( symbol )
            {
                bbox = symbol->GetBodyBoundingBox();

                SHAPE_RECT rect( bbox.GetPosition(), bbox.GetWidth(), bbox.GetHeight() );

                if( bbox.Contains( aPos ) )
                    dist = bbox.GetCenter().Distance( aPos );
                else
                    rect.Collide( poss, closestDist, &dist );
            }
            else
            {
                dist = bbox.GetCenter().Distance( aPos );
            }
        }
        else
        {
            SHAPE_RECT rect( bbox.GetPosition(), bbox.GetWidth(), bbox.GetHeight() );
            rect.Collide( poss, collector.m_Threshold, &dist );
        }

        // Don't promote dominating items to be the closest item
        // (they'll always win) - they'll still be available for selection, but they
        // won't boot out worthy competitors.
        if ( !dominating )
        {
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


SCH_SELECTION& SCH_SELECTION_TOOL::RequestSelection( const std::vector<KICAD_T>& aScanTypes,
                                                     bool aPromoteCellSelections,
                                                     bool aPromoteGroups )
{
    bool anyUnselected = false;
    bool anySelected = false;

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

        if( !isMoving )
            updateReferencePoint();
    }

    if( aPromoteGroups )
    {
        for( int i = (int) m_selection.GetSize() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = (EDA_ITEM*) m_selection.GetItem( i );

            std::set<EDA_ITEM*> selectedChildren;

            if( item->Type() == SCH_GROUP_T )
            {
                static_cast<SCH_ITEM*>(item)->RunOnChildren( [&]( SCH_ITEM* aChild )
                        {
                            if( aChild->IsType( aScanTypes ) )
                            selectedChildren.insert( aChild );
                        },
                        RECURSE_MODE::RECURSE );
                unselect( item );
                anyUnselected = true;
            }

            for( EDA_ITEM* child : selectedChildren )
            {
                if( !child->IsSelected() )
                {
                    if( child->Type() == SCH_LINE_T )
                        static_cast<SCH_LINE*>( child )->SetFlags( STARTPOINT | ENDPOINT );

                    select( child );
                    anySelected = true;
                }
            }
        }
    }

    if( aPromoteCellSelections )
    {
        std::set<EDA_ITEM*> parents;

        for( int i = (int) m_selection.GetSize() - 1; i >= 0; --i )
        {
            EDA_ITEM* item = (EDA_ITEM*) m_selection.GetItem( i );

            if( item->Type() == SCH_TABLECELL_T )
            {
                parents.insert( item->GetParent() );
                unselect( item );
                anyUnselected = true;
            }
        }

        for( EDA_ITEM* parent : parents )
        {
            if( !parent->IsSelected() )
            {
                select( parent );
                anySelected = true;
            }
        }
    }

    if( anyUnselected )
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

    if( anySelected )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return m_selection;
}


void SCH_SELECTION_TOOL::filterCollectedItems( SCH_COLLECTOR& aCollector, bool aMultiSelect )
{
    if( aCollector.GetCount() == 0 )
        return;

    std::set<EDA_ITEM*> rejected;

    for( EDA_ITEM* item : aCollector )
    {
        if( !itemPassesFilter( item, nullptr ) )
            rejected.insert( item );
    }

    for( EDA_ITEM* item : rejected )
        aCollector.Remove( item );
}


bool SCH_SELECTION_TOOL::itemPassesFilter( EDA_ITEM* aItem, SCH_SELECTION_FILTER_OPTIONS* aRejected )
{
    if( !aItem )
        return false;

    // Locking is not yet exposed uniformly in the schematic
#if 0
    if( SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( aItem ) )
    {
        if( schItem->IsLocked() && !m_filter.lockedItems )
            return false;
    }
#endif

    switch( aItem->Type() )
    {
    case SCH_SYMBOL_T:
    case SCH_SHEET_T:
        if( !m_filter.symbols )
        {
            if( aRejected )
                aRejected->symbols = true;
            return false;
        }

        break;

    case SCH_PIN_T:
    case SCH_SHEET_PIN_T:
        if( !m_filter.pins )
        {
            if( aRejected )
                aRejected->pins = true;
            return false;
        }

        break;

    case SCH_JUNCTION_T:
        if( !m_filter.wires )
        {
            if( aRejected )
                aRejected->wires = true;
            return false;
        }

        break;

    case SCH_LINE_T:
    {
        switch( static_cast<SCH_LINE*>( aItem )->GetLayer() )
        {
        case LAYER_WIRE:
        case LAYER_BUS:
            if( !m_filter.wires )
            {
                if( aRejected )
                    aRejected->wires = true;
                return false;
            }

            break;

        default:
            if( !m_filter.graphics )
            {
                if( aRejected )
                    aRejected->graphics = true;
                return false;
            }
        }

       break;
    }

    case SCH_SHAPE_T:
        if( !m_filter.graphics )
        {
            if( aRejected )
                aRejected->graphics = true;
            return false;
        }

        break;

    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
    case SCH_TABLE_T:
    case SCH_TABLECELL_T:
    case SCH_FIELD_T:
        if( !m_filter.text )
        {
            if( aRejected )
                aRejected->text = true;
            return false;
        }

        break;

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        if( !m_filter.labels )
        {
            if( aRejected )
                aRejected->labels = true;
            return false;
        }

        break;

    case SCH_BITMAP_T:
        if( !m_filter.images )
        {
            if( aRejected )
                aRejected->images = true;
            return false;
        }

        break;

    case SCH_RULE_AREA_T:
        if( !m_filter.ruleAreas )
        {
            if( aRejected )
                aRejected->ruleAreas = true;
            return false;
        }

        break;

    default:
        if( !m_filter.otherItems )
        {
            if( aRejected )
                aRejected->otherItems = true;
            return false;
        }

        break;
    }

    return true;
}


void SCH_SELECTION_TOOL::updateReferencePoint()
{
    VECTOR2I refP( 0, 0 );

    if( m_selection.Size() > 0 )
        refP = static_cast<SCH_ITEM*>( m_selection.GetTopLeftItem() )->GetPosition();

    m_selection.SetReferencePoint( refP );
}


int SCH_SELECTION_TOOL::SetSelectPoly( const TOOL_EVENT& aEvent )
{
    m_selectionMode = SELECTION_MODE::INSIDE_LASSO;
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SELECT_LASSO );
    m_toolMgr->PostAction( ACTIONS::selectionTool );
    return 0;
}


int SCH_SELECTION_TOOL::SetSelectRect( const TOOL_EVENT& aEvent )
{
    m_selectionMode = SELECTION_MODE::INSIDE_RECTANGLE;
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_toolMgr->PostAction( ACTIONS::selectionTool );
    return 0;
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


bool SCH_SELECTION_TOOL::selectMultiple()
{
    // Block selection not allowed in symbol viewer frame: no actual code to handle
    // a selection, so return to avoid to draw a selection rectangle, and to avoid crashes.
    if( m_frame->IsType( FRAME_T::FRAME_SCH_VIEWER ) )
        return false;

    bool cancelled = false;     // Was the tool canceled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    while( TOOL_EVENT* evt = Wait() )
    {
        /* Selection mode depends on direction of drag-selection:
         * Left > Right : Select objects that are fully enclosed by selection
         * Right > Left : Select objects that are crossed by selection
         */
        bool isGreedy = area.GetEnd().x < area.GetOrigin().x;

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
            area.SetMode( isGreedy ? SELECTION_MODE::TOUCHING_RECTANGLE
                                   : SELECTION_MODE::INSIDE_RECTANGLE );

            view->SetVisible( &area, true );
            view->Update( &area );
            getViewControls()->SetAutoPan( true );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( false );
            view->SetVisible( &area, false );
            SelectMultiple( area, m_drag_subtractive, false );
            evt->SetPassEvent( false );
            break;
        }

        passEvent( evt, allowedActions );
    }

    getViewControls()->SetAutoPan( false );

    // Stop drawing the selection box
    view->Remove( &area );
    m_multiple = false;         // Multiple selection mode is inactive

    if( !cancelled )
        m_selection.ClearReferencePoint();

    return cancelled;
}


bool SCH_SELECTION_TOOL::selectLasso()
{
    bool cancelled = false;
    m_multiple = true;
    KIGFX::PREVIEW::SELECTION_AREA area;
    getView()->Add( &area );
    getView()->SetVisible( &area, true );
    getViewControls()->SetAutoPan( true );

    SHAPE_LINE_CHAIN points;
    points.SetClosed( true );

    SELECTION_MODE selectionMode = SELECTION_MODE::TOUCHING_LASSO;
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SELECT_LASSO );

    while( TOOL_EVENT* evt = Wait() )
    {
        double shapeArea = area.GetPoly().Area( false );
        bool   isClockwise = shapeArea > 0 ? true : false;

        if( getView()->IsMirroredX() && shapeArea != 0 )
            isClockwise = !isClockwise;

        if( isClockwise )
        {
            selectionMode = SELECTION_MODE::INSIDE_LASSO;
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SELECT_WINDOW );
        }
        else
        {
            selectionMode = SELECTION_MODE::TOUCHING_LASSO;
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SELECT_LASSO );
        }

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }
        else if(   evt->IsDrag( BUT_LEFT )
                || evt->IsClick( BUT_LEFT )
                || evt->IsAction( &ACTIONS::cursorClick ) )
        {
            points.Append( evt->Position() );
        }
        else if(   evt->IsDblClick( BUT_LEFT )
                || evt->IsAction( &ACTIONS::cursorDblClick )
                || evt->IsAction( &ACTIONS::finishInteractive ) )
        {
            area.GetPoly().GenerateBBoxCache();
            SelectMultiple( area, m_drag_subtractive, false );
            break;
        }
        else if(   evt->IsAction( &ACTIONS::doDelete )
                || evt->IsAction( &ACTIONS::undo ) )
        {
            if( points.GetPointCount() > 0 )
            {
                getViewControls()->SetCursorPosition( points.CLastPoint() );
                points.Remove( points.GetPointCount() - 1 );
            }
        }
        else
        {
            passEvent( evt, allowedActions );
        }

        if( points.PointCount() > 0 )
        {
            if( !m_drag_additive && !m_drag_subtractive )
            {
                if( m_selection.GetSize() > 0 )
                {
                    ClearSelection( true );
                    m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
                }
            }
        }

        area.SetPoly( points );
        area.GetPoly().Append( m_toolMgr->GetMousePosition() );
        area.SetAdditive( m_drag_additive );
        area.SetSubtractive( m_drag_subtractive );
        area.SetExclusiveOr( false );
        area.SetMode( selectionMode );
        getView()->Update( &area );
    }

    getViewControls()->SetAutoPan( false );
    getView()->SetVisible( &area, false );
    getView()->Remove( &area );
    m_multiple = false;

    if( !cancelled )
        m_selection.ClearReferencePoint();

    return cancelled;
}


void SCH_SELECTION_TOOL::SelectMultiple( KIGFX::PREVIEW::SELECTION_AREA& aArea, bool aSubtractive,
                                         bool aExclusiveOr )
{
    KIGFX::VIEW* view = getView();

    SELECTION_MODE selectionMode = aArea.GetMode();
    bool containedMode = ( selectionMode == SELECTION_MODE::INSIDE_RECTANGLE
                           || selectionMode == SELECTION_MODE::INSIDE_LASSO );
    bool boxMode = ( selectionMode == SELECTION_MODE::INSIDE_RECTANGLE
                     || selectionMode == SELECTION_MODE::TOUCHING_RECTANGLE );

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> candidates;
    BOX2I selectionRect = aArea.ViewBBox();
    view->Query( selectionRect, candidates );

    std::set<SCH_ITEM*> uniqueCandidates;

    for( const auto& [viewItem, layer] : candidates )
    {
        if( viewItem->IsSCH_ITEM() )
            uniqueCandidates.insert( static_cast<SCH_ITEM*>( viewItem ) );
    }

    for( KIGFX::VIEW_ITEM* item : uniqueCandidates )
    {
        if( SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item ) )
        {
            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                if( boxMode ? selectionRect.Intersects( pin->GetBoundingBox() )
                            : KIGEOM::BoxHitTest( aArea.GetPoly(), pin->GetBoundingBox(), true ) )
                    uniqueCandidates.insert( pin );
            }
        }
        else if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
        {
            for( SCH_PIN* pin : symbol->GetPins() )
            {
                if( boxMode ? selectionRect.Intersects( pin->GetBoundingBox() )
                            : KIGEOM::BoxHitTest( aArea.GetPoly(), pin->GetBoundingBox(), true ) )
                    uniqueCandidates.insert( pin );
            }

            for( SCH_FIELD& field : symbol->GetFields() )
            {
                if( field.IsVisible()
                        && ( boxMode ? selectionRect.Intersects( field.GetBoundingBox() )
                                     : KIGEOM::BoxHitTest( aArea.GetPoly(), field.GetBoundingBox(), true ) ) )
                {
                    uniqueCandidates.insert( &field );
                }
            }
        }
    }

    SCH_COLLECTOR       collector;
    SCH_COLLECTOR       pinsCollector;
    std::set<EDA_ITEM*> group_items;

    for( EDA_ITEM* item : m_frame->GetScreen()->Items().OfType( SCH_GROUP_T ) )
    {
        SCH_GROUP* group = static_cast<SCH_GROUP*>( item );

        if( m_enteredGroup == group )
            continue;

        std::unordered_set<EDA_ITEM*>& newset = group->GetItems();

        auto boxContained =
                [&]( const BOX2I& aBox )
                {
                    return boxMode ? selectionRect.Contains( aBox )
                                   : KIGEOM::BoxHitTest( aArea.GetPoly(), aBox, true );
                };

        if( containedMode && boxContained( group->GetBoundingBox() ) && newset.size() )
        {
            for( EDA_ITEM* group_item : newset )
            {
                if( !group_item->IsSCH_ITEM() )
                    continue;

                if( Selectable( static_cast<SCH_ITEM*>( group_item ) ) )
                    collector.Append( *newset.begin() );
            }
        }

        for( EDA_ITEM* group_item : newset )
            group_items.emplace( group_item );
    }

    auto hitTest =
            [&]( SCH_ITEM* aItem )
            {
                return boxMode ? aItem->HitTest( selectionRect, containedMode )
                               : aItem->HitTest( aArea.GetPoly(), containedMode );
            };

    for( SCH_ITEM* item : uniqueCandidates )
    {
        if( Selectable( item ) && ( hitTest( item ) || item->Type() == SCH_LINE_T )
            && ( !containedMode || !group_items.count( item ) ) )
        {
            if( item->Type() == SCH_PIN_T && !m_isSymbolEditor )
                pinsCollector.Append( item );
            else
                collector.Append( item );
        }
    }

    filterCollectedItems( collector, true );
    filterCollectorForHierarchy( collector, true );

    if( collector.GetCount() == 0 )
    {
        collector = pinsCollector;
        filterCollectedItems( collector, true );
        filterCollectorForHierarchy( collector, true );
    }

    std::sort( collector.begin(), collector.end(),
               []( EDA_ITEM* a, EDA_ITEM* b )
               {
                   VECTOR2I aPos = a->GetPosition();
                   VECTOR2I bPos = b->GetPosition();

                   if( aPos.y == bPos.y )
                       return aPos.x < bPos.x;

                   return aPos.y < bPos.y;
               } );

    bool anyAdded = false;
    bool anySubtracted = false;

    auto selectItem =
            [&]( EDA_ITEM* aItem, EDA_ITEM_FLAGS flags )
            {
                if( aSubtractive || ( aExclusiveOr && aItem->IsSelected() ) )
                {
                    if( aExclusiveOr )
                        aItem->XorFlags( flags );
                    else
                        aItem->ClearFlags( flags );

                    if( !aItem->HasFlag( STARTPOINT ) && !aItem->HasFlag( ENDPOINT ) )
                    {
                        unselect( aItem );
                        anySubtracted = true;
                    }

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

    std::vector<EDA_ITEM*> flaggedItems;

    auto shapeContains =
            [&]( const VECTOR2I& aPoint )
            {
                return boxMode ? selectionRect.Contains( aPoint )
                                : aArea.GetPoly().PointInside( aPoint );
            };

    for( EDA_ITEM* item : collector )
    {
        EDA_ITEM_FLAGS flags = 0;

        item->SetFlags( SELECTION_CANDIDATE );
        flaggedItems.push_back( item );

        if( m_frame->GetRenderSettings()->m_ShowPinsElectricalType )
            item->SetFlags( SHOW_ELEC_TYPE );

        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );
            bool hits = false;

            if( boxMode )
                hits = line->HitTest( selectionRect, false );
            else
                hits = line->HitTest( aArea.GetPoly(), false );

            if( ( !containedMode && hits )
                || ( shapeContains( line->GetEndPoint() ) && shapeContains( line->GetStartPoint() ) ) )
            {
                flags |= STARTPOINT | ENDPOINT;
            }
            else if( containedMode )
            {
                if( shapeContains( line->GetStartPoint() ) && line->IsStartDangling() )
                    flags |= STARTPOINT;

                if( shapeContains( line->GetEndPoint() ) && line->IsEndDangling() )
                    flags |= ENDPOINT;
            }

            if( flags & ( STARTPOINT | ENDPOINT ) )
                selectItem( item, flags );
        }
        else
            selectItem( item, flags );

        item->ClearFlags( SHOW_ELEC_TYPE );
    }

    for( EDA_ITEM* item : pinsCollector )
    {
        if( m_frame->GetRenderSettings()->m_ShowPinsElectricalType )
            item->SetFlags( SHOW_ELEC_TYPE );

        if( Selectable( item ) && itemPassesFilter( item, nullptr )
            && !item->GetParent()->HasFlag( SELECTION_CANDIDATE ) && hitTest( static_cast<SCH_ITEM*>( item ) ) )
        {
            selectItem( item, 0 );
        }

        item->ClearFlags( SHOW_ELEC_TYPE );
    }

    for( EDA_ITEM* item : flaggedItems )
        item->ClearFlags( SELECTION_CANDIDATE );

    m_selection.SetIsHover( false );

    if( anyAdded )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    else if( anySubtracted )
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
}


void SCH_SELECTION_TOOL::filterCollectorForHierarchy( SCH_COLLECTOR& aCollector,
                                                      bool aMultiselect ) const
{
    std::unordered_set<EDA_ITEM*> toAdd;

    // Set SELECTION_CANDIDATE on all parents which are included in the GENERAL_COLLECTOR.  This
    // algorithm is O(3n), whereas checking for the parent inclusion could potentially be O(n^2).
    for( int j = 0; j < aCollector.GetCount(); j++ )
    {
        if( aCollector[j]->GetParent() )
            aCollector[j]->GetParent()->ClearFlags( SELECTION_CANDIDATE );

        if( aCollector[j]->GetParentSymbol() )
            aCollector[j]->GetParentSymbol()->ClearFlags( SELECTION_CANDIDATE );
    }

    if( aMultiselect )
    {
        for( int j = 0; j < aCollector.GetCount(); j++ )
            aCollector[j]->SetFlags( SELECTION_CANDIDATE );
    }

    for( int j = 0; j < aCollector.GetCount(); )
    {
        SCH_ITEM* item = aCollector[j];
        SYMBOL*   sym = item->GetParentSymbol();
        SCH_ITEM* start = item;

        if( !m_isSymbolEditor && sym )
            start = sym;

        // If a group is entered, disallow selections of objects outside the group.
        if( m_enteredGroup && !SCH_GROUP::WithinScope( item, m_enteredGroup, m_isSymbolEditor ) )
        {
            aCollector.Remove( item );
            continue;
        }

        // If any element is a member of a group, replace those elements with the top containing
        // group.
        if( EDA_GROUP* top = SCH_GROUP::TopLevelGroup( start, m_enteredGroup, m_isSymbolEditor ) )
        {
            if( top->AsEdaItem() != item )
            {
                toAdd.insert( top->AsEdaItem() );
                top->AsEdaItem()->SetFlags( SELECTION_CANDIDATE );

                aCollector.Remove( item );
                continue;
            }
        }

        // Symbols are a bit easier as they can't be nested.
        if( sym && ( sym->GetFlags() & SELECTION_CANDIDATE ) )
        {
            // Remove children of selected items
            aCollector.Remove( item );
            continue;
        }

        ++j;
    }

    for( EDA_ITEM* item : toAdd )
    {
        if( !aCollector.HasItem( item ) )
            aCollector.Append( item );
    }
}


int SCH_SELECTION_TOOL::updateSelection( const TOOL_EVENT& aEvent )
{
    getView()->Update( &m_selection );
    getView()->Update( &m_enteredGroupOverlay );

    return 0;
}


void SCH_SELECTION_TOOL::InitializeSelectionState( SCH_TABLE* aTable )
{
    for( SCH_TABLECELL* cell : aTable->GetCells() )
    {
        if( cell->IsSelected() )
            cell->SetFlags( SELECTION_CANDIDATE );
        else
            cell->ClearFlags( SELECTION_CANDIDATE );
    }
}

void SCH_SELECTION_TOOL::SelectCellsBetween( const VECTOR2D& start, const VECTOR2D& end, SCH_TABLE* aTable )
{
    BOX2I selectionRect( start, end );
    selectionRect.Normalize();

    auto wasSelected = []( EDA_ITEM* aItem )
    {
        return ( aItem->GetFlags() & SELECTION_CANDIDATE ) > 0;
    };

    for( SCH_TABLECELL* cell : aTable->GetCells() )
    {
        bool doSelect = false;

        if( cell->HitTest( selectionRect, false ) )
        {
            if( m_subtractive )
                doSelect = false;
            else if( m_exclusive_or )
                doSelect = !wasSelected( cell );
            else
                doSelect = true;
        }
        else if( wasSelected( cell ) )
        {
            doSelect = m_additive || m_subtractive || m_exclusive_or;
        }

        if( doSelect && !cell->IsSelected() )
            select( cell );
        else if( !doSelect && cell->IsSelected() )
            unselect( cell );
    }
}

bool SCH_SELECTION_TOOL::selectTableCells( SCH_TABLE* aTable )
{
    bool cancelled = false;
    m_multiple = true;

    InitializeSelectionState( aTable );

    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( true );
            SelectCellsBetween( evt->DragOrigin(), evt->Position() - evt->DragOrigin(), aTable );
        }
        else if( evt->IsMouseUp( BUT_LEFT ) )
        {
            m_selection.SetIsHover( false );

            bool anyAdded = false;
            bool anySubtracted = false;

            for( SCH_TABLECELL* cell : aTable->GetCells() )
            {
                if( cell->IsSelected() && ( cell->GetFlags() & SELECTION_CANDIDATE ) <= 0 )
                    anyAdded = true;
                else if( ( cell->GetFlags() & SELECTION_CANDIDATE ) > 0 && !cell->IsSelected() )
                    anySubtracted = true;
            }

            if( anyAdded )
                m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
            if( anySubtracted )
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

            break;
        }
        else
        {
            for( int i = 0; allowedActions[i]; ++i )
            {
                if( evt->IsAction( allowedActions[i] ) )
                {
                    evt->SetPassEvent();
                    break;
                }
            }
        }
    }

    getViewControls()->SetAutoPan( false );

    m_multiple = false;

    if( !cancelled )
        m_selection.ClearReferencePoint();

    return cancelled;
}


EDA_ITEM* SCH_SELECTION_TOOL::GetNode( const VECTOR2I& aPosition )
{
    SCH_COLLECTOR collector;

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


int SCH_SELECTION_TOOL::SelectNode( const TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( false );

    SelectPoint( cursorPos, connectedTypes );
    return 0;
}


std::set<SCH_ITEM*> SCH_SELECTION_TOOL::expandConnectionWithGraph( const SCH_SELECTION& aItems )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( m_isSymbolEditor || m_isSymbolViewer || !editFrame )
        return {};

    CONNECTION_GRAPH* graph = editFrame->Schematic().ConnectionGraph();

    if( !graph )
        return {};

    SCH_SHEET_PATH&        currentSheet = editFrame->GetCurrentSheet();
    std::vector<SCH_ITEM*> startItems;
    std::set<SCH_ITEM*>    added;

    // Build the list of starting items for the connection graph traversal
    for( auto item : aItems )
    {
        if( !item->IsSCH_ITEM() )
            continue;

        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        // If we're a symbol, start from all its pins
        if( schItem->Type() == SCH_SYMBOL_T )
        {
            for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( schItem )->GetPins( &currentSheet ) )
            {
                if( pin )
                    startItems.push_back( pin );
            }
        }
        else if( schItem->IsConnectable() )
        {
            startItems.push_back( schItem );
        }
    }

    if( startItems.empty() )
        return {};

    std::deque<SCH_ITEM*>         queue;
    std::unordered_set<SCH_ITEM*> visited;

    auto enqueue = [&]( SCH_ITEM* aItem )
    {
        if( !aItem )
            return;

        if( visited.insert( aItem ).second )
            queue.push_back( aItem );
    };

    for( SCH_ITEM* item : startItems )
        enqueue( item );

    while( !queue.empty() )
    {
        SCH_ITEM* item = queue.front();
        queue.pop_front();

        if( SCH_PIN* pin = dynamic_cast<SCH_PIN*>( item ) )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParent() );

            if( symbol && Selectable( symbol ) && itemPassesFilter( symbol, nullptr ) && !symbol->IsSelected() )
                added.insert( symbol );
        }

        const SCH_ITEM_VEC& neighbors = item->ConnectedItems( currentSheet );

        for( SCH_ITEM* neighbor : neighbors )
        {
            if( !neighbor )
                continue;

            if( neighbor->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( neighbor );

                if( Selectable( symbol ) && itemPassesFilter( symbol, nullptr ) && !symbol->IsSelected() )
                    added.insert( symbol );

                continue;
            }

            enqueue( neighbor );
        }

        if( !Selectable( item ) || !itemPassesFilter( item, nullptr ) )
            continue;

        added.insert( item );
    }

    return added;
}


std::set<SCH_ITEM*> SCH_SELECTION_TOOL::expandConnectionGraphically( const SCH_SELECTION& aItems )
{
    std::set<SCH_ITEM*> added;

    for( auto item : aItems )
    {
        if( !item->IsSCH_ITEM() )
            continue;

        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        std::set<SCH_ITEM*> conns = m_frame->GetScreen()->MarkConnections( schItem, schItem->IsConnectable() );

        // Make sure we don't add things the user has disabled in the selection filter
        for( SCH_ITEM* connItem : conns )
        {
            if( !Selectable( connItem ) || !itemPassesFilter( connItem, nullptr ) )
                continue;

            added.insert( connItem );
        }
    }

    return added;
}


int SCH_SELECTION_TOOL::SelectConnection( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION originalSelection = RequestSelection( expandConnectionGraphTypes );

    if( m_selection.Empty() )
        return 0;

    SCH_SELECTION connectableSelection;
    SCH_SELECTION graphicalSelection;

    // We need to filter the selection into connectable items (wires, pins, symbols)
    // and non-connectable items (shapes, unconnectable lines) for processing
    // with the graph or by the graphical are-endpoints-touching method.
    for( EDA_ITEM* selItem : originalSelection.GetItems() )
    {
        if( !selItem->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( selItem );

        if( item->Type() == SCH_LINE_T && !item->IsConnectable() )
            graphicalSelection.Add( item );
        else if( item->Type() == SCH_SHAPE_T )
            graphicalSelection.Add( item );
        else
            connectableSelection.Add( item );
    }

    ClearSelection( true );

    std::set<SCH_ITEM*> graphAdded;
    std::set<SCH_ITEM*> graphicalAdded;

    if( !connectableSelection.Empty() )
        graphAdded = expandConnectionWithGraph( connectableSelection );

    if( !graphicalSelection.Empty() )
        graphicalAdded = expandConnectionGraphically( graphicalSelection );

    // For whatever reason, the connection graph isn't working (e.g. in symbol editor )
    // so fall back to graphical expansion for those items if nothing was added.
    if( graphAdded.empty() && !connectableSelection.Empty() )
    {
        SCH_SELECTION combinedSelection = connectableSelection;

        for( EDA_ITEM* selItem : graphicalSelection.GetItems() )
            combinedSelection.Add( selItem );

        graphicalSelection = combinedSelection;
    }

    graphicalAdded = expandConnectionGraphically( graphicalSelection );

    // Add everything to the selection, including the original selection
    for( auto item : graphAdded )
        AddItemToSel( item, true );

    for( auto item : graphicalAdded )
        AddItemToSel( item, true );

    for( auto item : originalSelection )
        AddItemToSel( item, true );

    m_selection.SetIsHover( originalSelection.IsHover() );

    if( originalSelection.HasReferencePoint() )
        m_selection.SetReferencePoint( originalSelection.GetReferencePoint() );
    else
        m_selection.ClearReferencePoint();

    getView()->Update( &m_selection );

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int SCH_SELECTION_TOOL::SelectColumns( const TOOL_EVENT& aEvent )
{
    std::set<std::pair<SCH_TABLE*, int>> columns;
    bool                                 added = false;

    for( EDA_ITEM* item : m_selection )
    {
        if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( item ) )
        {
            SCH_TABLE* table = static_cast<SCH_TABLE*>( cell->GetParent() );
            columns.insert( std::make_pair( table, cell->GetColumn() ) );
        }
    }

    for( auto& [ table, col ] : columns )
    {
        for( int row = 0; row < table->GetRowCount(); ++row )
        {
            SCH_TABLECELL* cell = table->GetCell( row, col );

            if( !cell->IsSelected() )
            {
                select( table->GetCell( row, col ) );
                added = true;
            }
        }
    }

    if( added )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int SCH_SELECTION_TOOL::SelectRows( const TOOL_EVENT& aEvent )
{
    std::set<std::pair<SCH_TABLE*, int>> rows;
    bool                                 added = false;

    for( EDA_ITEM* item : m_selection )
    {
        if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( item ) )
        {
            SCH_TABLE* table = static_cast<SCH_TABLE*>( cell->GetParent() );
            rows.insert( std::make_pair( table, cell->GetRow() ) );
        }
    }

    for( auto& [ table, row ] : rows )
    {
        for( int col = 0; col < table->GetColCount(); ++col )
        {
            SCH_TABLECELL* cell = table->GetCell( row, col );

            if( !cell->IsSelected() )
            {
                select( table->GetCell( row, col ) );
                added = true;
            }
        }
    }

    if( added )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int SCH_SELECTION_TOOL::SelectTable( const TOOL_EVENT& aEvent )
{
    std::set<SCH_TABLE*> tables;
    bool                 added = false;

    for( EDA_ITEM* item : m_selection )
    {
        if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( item ) )
            tables.insert( static_cast<SCH_TABLE*>( cell->GetParent() ) );
    }

    ClearSelection();

    for( SCH_TABLE* table : tables )
    {
        if( !table->IsSelected() )
        {
            select( table );
            added = true;
        }
    }

    if( added )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int SCH_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    ClearSelection();
    return 0;
}


void SCH_SELECTION_TOOL::ZoomFitCrossProbeBBox( const BOX2I& aBBox )
{
    if( aBBox.GetWidth() == 0 )
        return;

    BOX2I bbox = aBBox;
    bbox.Normalize();

    VECTOR2I bbSize = bbox.Inflate( KiROUND( bbox.GetWidth() * 0.2f ) ).GetSize();
    VECTOR2D screenSize = getView()->GetViewport().GetSize();

    // This code tries to come up with a zoom factor that doesn't simply zoom in to the cross
    // probed symbol, but instead shows a reasonable amount of the circuit around it to provide
    // context.  This reduces the need to manually change the zoom because it's too close.

    // Using the default text height as a constant to compare against, use the height of the
    // bounding box of visible items for a footprint to figure out if this is a big symbol (like
    // a processor) or a small symbol (like a resistor).  This ratio is not useful by itself as a
    // scaling factor.  It must be "bent" to provide good scaling at varying symbol sizes.  Bigger
    // symbols need less scaling than small ones.
    double currTextHeight = schIUScale.MilsToIU( DEFAULT_TEXT_SIZE );

    double compRatio = bbSize.y / currTextHeight; // Ratio of symbol to text height
    double compRatioBent = 1.0;

    // LUT to scale zoom ratio to provide reasonable schematic context.  Must work with symbols
    // of varying sizes (e.g. 0402 package and 200 pin BGA).
    // Each entry represents a compRatio (symbol height / default text height) and an amount to
    // scale by.
    std::vector<std::pair<double, double>> lut{ { 1.25,  16 },
                                                {  2.5,  12 },
                                                {    5,   8 },
                                                {    6,   6 },
                                                {   10,   4 },
                                                {   20,   2 },
                                                {   40, 1.5 },
                                                {  100,   1 } };

    std::vector<std::pair<double, double>>::iterator it;

    // Large symbol default is last LUT entry (1:1).
    compRatioBent = lut.back().second;

    // Use LUT to do linear interpolation of "compRatio" within "first", then use that result to
    // linearly interpolate "second" which gives the scaling factor needed.
    if( compRatio >= lut.front().first )
    {
        for( it = lut.begin(); it < lut.end() - 1; ++it )
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

    // This is similar to the original KiCad code that scaled the zoom to make sure symbols were
    // visible on screen.  It's simply a ratio of screen size to symbol size, and its job is to
    // zoom in to make the component fullscreen.  Earlier in the code the symbol BBox is given a
    // 20% margin to add some breathing room.  We compare the height of this enlarged symbol bbox
    // to the default text height.  If a symbol will end up with the sides clipped, we adjust
    // later to make sure it fits on screen.
    screenSize.x = std::max( 10.0, screenSize.x );
    screenSize.y = std::max( 10.0, screenSize.y );
    double ratio = std::max( -1.0, fabs( bbSize.y / screenSize.y ) );

    // Original KiCad code for how much to scale the zoom
    double kicadRatio = std::max( fabs( bbSize.x / screenSize.x ),
                                  fabs( bbSize.y / screenSize.y ) );

    // If the width of the part we're probing is bigger than what the screen width will be after
    // the zoom, then punt and use the KiCad zoom algorithm since it guarantees the part's width
    // will be encompassed within the screen.
    if( bbSize.x > screenSize.x * ratio * compRatioBent )
    {
        // Use standard KiCad zoom for parts too wide to fit on screen/
        ratio = kicadRatio;
        compRatioBent = 1.0; // Reset so we don't modify the "KiCad" ratio
        wxLogTrace( "CROSS_PROBE_SCALE",
                    "Part TOO WIDE for screen.  Using normal KiCad zoom ratio: %1.5f", ratio );
    }

    // Now that "compRatioBent" holds our final scaling factor we apply it to the original
    // fullscreen zoom ratio to arrive at the final ratio itself.
    ratio *= compRatioBent;

    bool alwaysZoom = false; // DEBUG - allows us to minimize zooming or not

    // Try not to zoom on every cross-probe; it gets very noisy
    if( ( ratio < 0.5 || ratio > 1.0 ) || alwaysZoom )
        getView()->SetScale( getView()->GetScale() / ratio );
}


void SCH_SELECTION_TOOL::SyncSelection( const std::optional<SCH_SHEET_PATH>& targetSheetPath,
                                        SCH_ITEM* focusItem, const std::vector<SCH_ITEM*>& items )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return;

    if( targetSheetPath && targetSheetPath != editFrame->Schematic().CurrentSheet() )
    {
        SCH_SHEET_PATH path = targetSheetPath.value();
        m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &path );
    }

    ClearSelection( items.size() > 0 ? true /*quiet mode*/ : false );

    // Perform individual selection of each item before processing the event.
    for( SCH_ITEM* item : items )
    {
        SCH_ITEM* parent = dynamic_cast<SCH_ITEM*>( item->GetParent() );

        // Make sure we only select items on the current screen
        if( m_frame->GetScreen()->CheckIfOnDrawList( item )
            || ( parent && m_frame->GetScreen()->CheckIfOnDrawList( parent ) ) )
        {
            select( item );
        }
    }

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


void SCH_SELECTION_TOOL::RebuildSelection()
{
    m_selection.Clear();

    bool enteredGroupFound = false;

    if( m_isSymbolEditor )
    {
        LIB_SYMBOL* start = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        for( SCH_ITEM& item : start->GetDrawItems() )
        {
            if( item.IsSelected() )
                select( &item );

            if( item.Type() == SCH_GROUP_T )
            {
                if( &item == m_enteredGroup )
                {
                    item.SetFlags( ENTERED );
                    enteredGroupFound = true;
                }
                else
                {
                    item.ClearFlags( ENTERED );
                }
            }
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
                        },
                        RECURSE_MODE::NO_RECURSE );
            }

            if( item->Type() == SCH_GROUP_T )
            {
                if( item == m_enteredGroup )
                {
                    item->SetFlags( ENTERED );
                    enteredGroupFound = true;
                }
                else
                {
                    item->ClearFlags( ENTERED );
                }
            }
        }
    }

    updateReferencePoint();

    if( !enteredGroupFound )
    {
        m_enteredGroupOverlay.Clear();
        m_enteredGroup = nullptr;
    }

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
}


bool SCH_SELECTION_TOOL::Selectable( const EDA_ITEM* aItem, const VECTOR2I* aPos,
                                     bool checkVisibilityOnly ) const
{
    // NOTE: in the future this is where Eeschema layer/itemtype visibility will be handled

    SYMBOL_EDIT_FRAME* symEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );

    // Do not allow selection of anything except fields when the current symbol in the symbol
    // editor is a derived symbol.
    if( symEditFrame && symEditFrame->IsSymbolAlias() && aItem->Type() != SCH_FIELD_T )
        return false;

    switch( aItem->Type() )
    {
    case SCH_PIN_T:
    {
        const SCH_PIN* pin = static_cast<const SCH_PIN*>( aItem );

        if( symEditFrame )
        {
            if( pin->GetUnit() && pin->GetUnit() != symEditFrame->GetUnit() )
                return false;

            if( pin->GetBodyStyle() && pin->GetBodyStyle() != symEditFrame->GetBodyStyle() )
                return false;
        }

        if( !pin->IsVisible() && !m_frame->GetShowAllPins() )
            return false;

        if( !m_filter.pins )
        {
            // Pin anchors have to be allowed for auto-starting wires.
            if( aPos )
            {
                EE_GRID_HELPER    grid( m_toolMgr );
                GRID_HELPER_GRIDS pinGrid = grid.GetItemGrid( pin );

                if( pin->IsPointClickableAnchor( grid.BestSnapAnchor( *aPos, pinGrid ) ) )
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

    case SCH_FIELD_T:     // SCH_FIELD objects are not unit/body-style-specific.
    {
        const SCH_FIELD* field = static_cast<const SCH_FIELD*>( aItem );

        if( !field->IsVisible() && !( symEditFrame && symEditFrame->GetShowInvisibleFields() ) )
            return false;

        break;
    }

    case SCH_SHAPE_T:
    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
        if( symEditFrame )
        {
            const SCH_ITEM* sch_item = static_cast<const SCH_ITEM*>( aItem );

            if( sch_item->GetUnit() && sch_item->GetUnit() != symEditFrame->GetUnit() )
                return false;

            if( sch_item->GetBodyStyle() && sch_item->GetBodyStyle() != symEditFrame->GetBodyStyle() )
                return false;
        }

        break;

    case SCH_MARKER_T:  // Always selectable
        return true;

    case SCH_TABLECELL_T:
    {
        const SCH_TABLECELL* cell = static_cast<const SCH_TABLECELL*>( aItem );

        if( cell->GetColSpan() == 0 || cell->GetRowSpan() == 0 )
            return false;

        break;
    }

    case NOT_USED:      // Things like CONSTRUCTION_GEOM that aren't part of the model
        return false;

    default:            // Suppress warnings
        break;
    }

    return true;
}


void SCH_SELECTION_TOOL::ClearSelection( bool aQuietMode )
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


void SCH_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    // Don't group when we select new items, the schematic editor selects all new items for moving.
    // The PCB editor doesn't need this logic because it doesn't select new items for moving.
    if( m_enteredGroup && !aItem->IsNew()
        && !SCH_GROUP::WithinScope( static_cast<SCH_ITEM*>( aItem ), m_enteredGroup, m_isSymbolEditor ) )
    {
        ExitGroup();
    }

    highlight( aItem, SELECTED, &m_selection );
}


void SCH_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
}


void SCH_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
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
        // We don't want to select group children if the group itself is selected,
        // we can only select them when the group is entered
        if( sch_item->Type() != SCH_GROUP_T )
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
                    },
                    RECURSE_MODE::NO_RECURSE );
        }
    }

    if( aGroup && aMode != BRIGHTENED )
        getView()->Hide( aItem, true );

    if( aItem->GetParent() && aItem->GetParent()->Type() != SCHEMATIC_T )
        getView()->Update( aItem->GetParent(), KIGFX::REPAINT );

    getView()->Update( aItem, KIGFX::REPAINT );
}


void SCH_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
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

                    if( aGroup )
                        aGroup->Remove( aChild );
                },
                RECURSE_MODE::NO_RECURSE );
    }

    if( aItem->GetParent() && aItem->GetParent()->Type() != SCHEMATIC_T )
        getView()->Update( aItem->GetParent(), KIGFX::REPAINT );

    getView()->Update( aItem, KIGFX::REPAINT );
}


bool SCH_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    int            margin = KiROUND( getView()->ToWorld( GRIP_MARGIN ) );

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


int SCH_SELECTION_TOOL::SelectNext( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( !editFrame || !editFrame->GetNetNavigator() || m_selection.Size() == 0 )
        return 0;

    if( !m_selection.Front()->IsBrightened() )
        return 0;

    const SCH_ITEM* item = editFrame->SelectNextPrevNetNavigatorItem( true );

    if( item )
    {
        ClearSelection();
        select( const_cast<SCH_ITEM*>( item ) );
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int SCH_SELECTION_TOOL::SelectPrevious( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    if( !editFrame || !editFrame->GetNetNavigator() || m_selection.Size() == 0 )
        return 0;

    if( !m_selection.Front()->IsBrightened() )
        return 0;

    const SCH_ITEM* item = editFrame->SelectNextPrevNetNavigatorItem( false );

    if( item )
    {
        ClearSelection();
        select( const_cast<SCH_ITEM*>( item ) );
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


void SCH_SELECTION_TOOL::setTransitions()
{
    Go( &SCH_SELECTION_TOOL::UpdateMenu,          ACTIONS::updateMenu.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::Main,                ACTIONS::selectionActivate.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectNode,          SCH_ACTIONS::selectNode.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectConnection,    SCH_ACTIONS::selectConnection.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectColumns,       ACTIONS::selectColumns.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectRows,          ACTIONS::selectRows.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectTable,         ACTIONS::selectTable.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::ClearSelection,      ACTIONS::selectionClear.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::SetSelectPoly,       ACTIONS::selectSetLasso.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SetSelectRect,       ACTIONS::selectSetRect.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::AddItemToSel,        ACTIONS::selectItem.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::AddItemsToSel,       ACTIONS::selectItems.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::RemoveItemFromSel,   ACTIONS::unselectItem.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::RemoveItemsFromSel,  ACTIONS::unselectItems.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectionMenu,       ACTIONS::selectionMenu.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::SelectAll,           ACTIONS::selectAll.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::UnselectAll,         ACTIONS::unselectAll.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::SelectNext,          SCH_ACTIONS::nextNetItem.MakeEvent() );
    Go( &SCH_SELECTION_TOOL::SelectPrevious,      SCH_ACTIONS::previousNetItem.MakeEvent() );

    Go( &SCH_SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsModified );
    Go( &SCH_SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsMoved );

    Go( &SCH_SELECTION_TOOL::disambiguateCursor,  EVENTS::DisambiguatePoint );
}
