/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <sch_line_wire_bus_tool.h>

#include <boost/optional/optional.hpp>
#include <wx/debug.h>
#include <wx/gdicmn.h>
#include <wx/menu.h>
#include <wx/string.h>
#include <wx/stringimpl.h>
#include <wx/translation.h>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <bitmaps.h>
#include <advanced_config.h>
#include <base_screen.h>
#include <base_struct.h>
#include <core/typeinfo.h>
#include <eda_text.h>
#include <layers_id_colors_and_visibility.h>
#include <math/vector2d.h>
#include <tool/actions.h>
#include <tool/conditional_menu.h>
#include <tool/selection.h>
#include <tool/selection_conditions.h>
#include <tool/tool_action.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <trigo.h>
#include <undo_redo_container.h>
#include <view/view_controls.h>

#include <connection_graph.h>
#include <eeschema_id.h>
#include <general.h>
#include <sch_bus_entry.h>
#include <sch_connection.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_item.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_text.h>
#include <sch_view.h>

#include <ee_actions.h>
#include <ee_point_editor.h>
#include <ee_selection.h>
#include <ee_selection_tool.h>

class BUS_UNFOLD_MENU : public ACTION_MENU
{
public:
    BUS_UNFOLD_MENU() :
        ACTION_MENU( true ),
        m_showTitle( false )
    {
        SetIcon( add_line2bus_xpm );
        SetTitle( _( "Unfold from Bus" ) );
    }

    void SetShowTitle()
    {
        m_showTitle = true;
    }


protected:
    ACTION_MENU* create() const override
    {
        return new BUS_UNFOLD_MENU();
    }

private:
    void update() override
    {
        SCH_EDIT_FRAME*    frame = (SCH_EDIT_FRAME*) getToolManager()->GetEditFrame();
        EE_SELECTION_TOOL* selTool = getToolManager()->GetTool<EE_SELECTION_TOOL>();
        KICAD_T            busType[] = { SCH_LINE_LOCATE_BUS_T, EOT };
        EE_SELECTION&      selection = selTool->RequestSelection( busType );
        SCH_LINE*          bus = (SCH_LINE*) selection.Front();

        Clear();

        // TODO(JE) remove once real-time is enabled
        if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        {
            frame->RecalculateConnections( NO_CLEANUP );

            // Pick up the pointer again because it may have been changed by SchematicCleanUp
            selection = selTool->RequestSelection( busType );
            bus = (SCH_LINE*) selection.Front();
        }
        if( !bus )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "no bus selected" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        SCH_CONNECTION* connection = bus->Connection( *g_CurrentSheet );

        if( !connection ||  !connection->IsBus() || connection->Members().empty() )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "bus has no connections" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        int idx = 0;

        if( m_showTitle )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "Unfold from Bus" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
        }

        for( const auto& member : connection->Members() )
        {
            int id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
            wxString name = member->LocalName();

            if( member->Type() == CONNECTION_TYPE::BUS )
            {
                ACTION_MENU* submenu = new ACTION_MENU( true );
                AppendSubMenu( submenu, name );

                for( const auto& sub_member : member->Members() )
                {
                    id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
                    submenu->Append( id, sub_member->LocalName(), wxEmptyString );
                }
            }
            else
            {
                Append( id, name, wxEmptyString );
            }
        }
    }

    bool m_showTitle;
};


SCH_LINE_WIRE_BUS_TOOL::SCH_LINE_WIRE_BUS_TOOL() :
    EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawingLineWireBus" )
{
    m_busUnfold = {};
    m_wires.reserve( 16 );
}


SCH_LINE_WIRE_BUS_TOOL::~SCH_LINE_WIRE_BUS_TOOL()
{
}


bool SCH_LINE_WIRE_BUS_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto wireOrBusTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->IsCurrentTool( EE_ACTIONS::drawWire )
              || m_frame->IsCurrentTool( EE_ACTIONS::drawBus ) );
    };

    auto lineTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->IsCurrentTool( EE_ACTIONS::drawLines ) );
    };

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };

    auto busSelection = EE_CONDITIONS::MoreThan( 0 )
                            && EE_CONDITIONS::OnlyType( SCH_LINE_LOCATE_BUS_T );

    auto& ctxMenu = m_menu.GetMenu();

    // Build the tool menu
    //
    ctxMenu.AddItem( EE_ACTIONS::leaveSheet,         belowRootSheetCondition, 2 );

    ctxMenu.AddSeparator( 10 );
    ctxMenu.AddItem( EE_ACTIONS::drawWire,           wireOrBusTool && EE_CONDITIONS::Idle, 10 );
    ctxMenu.AddItem( EE_ACTIONS::drawBus,            wireOrBusTool && EE_CONDITIONS::Idle, 10 );
    ctxMenu.AddItem( EE_ACTIONS::drawLines,          lineTool && EE_CONDITIONS::Idle, 10 );
    ctxMenu.AddItem( EE_ACTIONS::finishWire,         IsDrawingWire, 10 );
    ctxMenu.AddItem( EE_ACTIONS::finishBus,          IsDrawingBus, 10 );
    ctxMenu.AddItem( EE_ACTIONS::finishLine,         IsDrawingLine, 10 );

    std::shared_ptr<BUS_UNFOLD_MENU> busUnfoldMenu = std::make_shared<BUS_UNFOLD_MENU>();
    busUnfoldMenu->SetTool( this );
    m_menu.AddSubMenu( busUnfoldMenu );
    ctxMenu.AddMenu( busUnfoldMenu.get(),            EE_CONDITIONS::Idle, 10 );

    ctxMenu.AddSeparator( 100 );
    ctxMenu.AddItem( EE_ACTIONS::placeJunction,      wireOrBusTool && EE_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::placeLabel,         wireOrBusTool && EE_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::placeGlobalLabel,   wireOrBusTool && EE_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::placeHierLabel,     wireOrBusTool && EE_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::breakWire,          wireOrBusTool && EE_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::breakBus,           wireOrBusTool && EE_CONDITIONS::Idle, 100 );

    ctxMenu.AddSeparator( 200 );
    ctxMenu.AddItem( EE_ACTIONS::selectNode,         wireOrBusTool && EE_CONDITIONS::Idle, 200 );
    ctxMenu.AddItem( EE_ACTIONS::selectConnection,   wireOrBusTool && EE_CONDITIONS::Idle, 200 );

    // Add bus unfolding to the selection tool
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    std::shared_ptr<BUS_UNFOLD_MENU> selBusUnfoldMenu = std::make_shared<BUS_UNFOLD_MENU>();
    selBusUnfoldMenu->SetTool( m_selectionTool );
    m_selectionTool->GetToolMenu().AddSubMenu( selBusUnfoldMenu );
    selToolMenu.AddMenu( selBusUnfoldMenu.get(),     busSelection && EE_CONDITIONS::Idle, 100 );

    return true;
}


bool SCH_LINE_WIRE_BUS_TOOL::IsDrawingLine( const SELECTION& aSelection )
{
    static KICAD_T graphicLineType[] = { SCH_LINE_LOCATE_GRAPHIC_LINE_T, EOT };
    return IsDrawingLineWireOrBus( aSelection ) && aSelection.Front()->IsType( graphicLineType );
}


bool SCH_LINE_WIRE_BUS_TOOL::IsDrawingWire( const SELECTION& aSelection )
{
    static KICAD_T wireType[] = { SCH_LINE_LOCATE_WIRE_T, EOT };
    return IsDrawingLineWireOrBus( aSelection ) && aSelection.Front()->IsType( wireType );
}


bool SCH_LINE_WIRE_BUS_TOOL::IsDrawingBus( const SELECTION& aSelection )
{
    static KICAD_T busType[] = { SCH_LINE_LOCATE_BUS_T, EOT };
    return IsDrawingLineWireOrBus( aSelection ) && aSelection.Front()->IsType( busType );
}


bool SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( const SELECTION& aSelection )
{
    // NOTE: for immediate hotkeys, it is NOT required that the line, wire or bus tool
    // be selected
    SCH_ITEM* item = (SCH_ITEM*) aSelection.Front();
    return item && item->IsNew() && item->Type() == SCH_LINE_T;
}


int SCH_LINE_WIRE_BUS_TOOL::DrawSegments( const TOOL_EVENT& aEvent )
{
    SCH_LAYER_ID layer = aEvent.Parameter<SCH_LAYER_ID>();

    if( aEvent.HasPosition() )
        getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );

    if( aEvent.HasPosition() )
    {
        VECTOR2D cursorPos = getViewControls()->GetCursorPosition( !aEvent.Modifier( MD_ALT ) );
        startSegments( layer, cursorPos );
    }

    return doDrawSegments( tool, layer );
}


int SCH_LINE_WIRE_BUS_TOOL::UnfoldBus( const TOOL_EVENT& aEvent )
{
    wxString* netPtr = aEvent.Parameter<wxString*>();
    wxString  net;
    SCH_LINE* segment = nullptr;

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    if( netPtr )
    {
        net = *netPtr;
        delete netPtr;
    }
    else
    {
        BUS_UNFOLD_MENU unfoldMenu;
        unfoldMenu.SetTool( this );
        unfoldMenu.SetShowTitle();

        SetContextMenu( &unfoldMenu, CMENU_NOW );

        while( TOOL_EVENT* evt = Wait() )
        {
            if( evt->Action() == TA_CHOICE_MENU_CHOICE )
            {
                OPT<int> id = evt->GetCommandId();

                if( id && ( *id > 0 ) )
                    net = *evt->Parameter<wxString*>();

                break;
            }
        }
    }

    // Break a wire for the given net out of the bus
    if( !net.IsEmpty() )
        segment = doUnfoldBus( net );

    // If we have an unfolded wire to draw, then draw it
    if( segment )
        return doDrawSegments( tool, LAYER_WIRE );
    else
    {
        m_frame->PopTool( tool );
        return 0;
    }
}


SCH_LINE* SCH_LINE_WIRE_BUS_TOOL::doUnfoldBus( const wxString& aNet )
{
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    wxPoint  pos = (wxPoint) getViewControls()->GetCursorPosition();

    m_busUnfold.entry = new SCH_BUS_WIRE_ENTRY( pos, '\\' );
    m_busUnfold.entry->SetParent( m_frame->GetScreen() );
    m_frame->AddToScreen( m_busUnfold.entry );

    m_busUnfold.label = new SCH_LABEL( m_busUnfold.entry->m_End(), aNet );
    m_busUnfold.label->SetTextSize( wxSize( GetDefaultTextSize(), GetDefaultTextSize() ) );
    m_busUnfold.label->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );
    m_busUnfold.label->SetParent( m_frame->GetScreen() );
    m_busUnfold.label->SetFlags( IS_NEW | IS_MOVED );

    m_busUnfold.in_progress = true;
    m_busUnfold.origin = pos;
    m_busUnfold.net_name = aNet;

    getViewControls()->SetCrossHairCursorPosition( m_busUnfold.entry->m_End(), false );

    return startSegments( LAYER_WIRE, m_busUnfold.entry->m_End() );
}


const SCH_SHEET_PIN* SCH_LINE_WIRE_BUS_TOOL::getSheetPin( const wxPoint& aPosition )
{
    SCH_SCREEN* screen = m_frame->GetScreen();

    for( auto item : screen->Items().Overlapping( SCH_SHEET_T, aPosition ) )
    {
        auto sheet = static_cast<SCH_SHEET*>( item );

        for( SCH_SHEET_PIN* pin : sheet->GetPins() )
        {
            if( pin->GetPosition() == aPosition )
                return pin;
        }
    }

    return nullptr;
}


void SCH_LINE_WIRE_BUS_TOOL::computeBreakPoint( const std::pair<SCH_LINE*, SCH_LINE*>& aSegments,
        wxPoint& aPosition )
{
    wxCHECK_RET( aSegments.first && aSegments.second,
            wxT( "Cannot compute break point of NULL line segment." ) );

    SCH_LINE* segment = aSegments.first;
    SCH_LINE* next_segment = aSegments.second;

    wxPoint midPoint;
    int iDx = segment->GetEndPoint().x - segment->GetStartPoint().x;
    int iDy = segment->GetEndPoint().y - segment->GetStartPoint().y;

    const SCH_SHEET_PIN* connectedPin = getSheetPin( segment->GetStartPoint() );
    auto force = connectedPin ? connectedPin->GetEdge() : SHEET_UNDEFINED_SIDE;

    if( force == SHEET_LEFT_SIDE || force == SHEET_RIGHT_SIDE )
    {
        if( aPosition.x == connectedPin->GetPosition().x )  // push outside sheet boundary
        {
            int direction = ( force == SHEET_LEFT_SIDE ) ? -1 : 1;
            aPosition.x += int( m_frame->GetScreen()->GetGridSize().x * direction );
        }

        midPoint.x = aPosition.x;
        midPoint.y = segment->GetStartPoint().y;     // force horizontal
    }
    else if( iDy != 0 )    // keep the first segment orientation (vertical)
    {
        midPoint.x = segment->GetStartPoint().x;
        midPoint.y = aPosition.y;
    }
    else if( iDx != 0 )    // keep the first segment orientation (horizontal)
    {
        midPoint.x = aPosition.x;
        midPoint.y = segment->GetStartPoint().y;
    }
    else
    {
        if( std::abs( aPosition.x - segment->GetStartPoint().x ) <
            std::abs( aPosition.y - segment->GetStartPoint().y ) )
        {
            midPoint.x = segment->GetStartPoint().x;
            midPoint.y = aPosition.y;
        }
        else
        {
            midPoint.x = aPosition.x;
            midPoint.y = segment->GetStartPoint().y;
        }
    }

    segment->SetEndPoint( midPoint );
    next_segment->SetStartPoint( midPoint );
    next_segment->SetEndPoint( aPosition );
}


int SCH_LINE_WIRE_BUS_TOOL::doDrawSegments( const std::string& aTool, int aType )
{
    SCH_SCREEN*      screen = m_frame->GetScreen();
    EE_POINT_EDITOR* pointEditor = m_toolMgr->GetTool<EE_POINT_EDITOR>();
    SCH_LINE*        segment = nullptr;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    Activate();

    // Add the new label to the selection so the rotate command operates on it
    if( m_busUnfold.label )
        m_selectionTool->AddItemToSel( m_busUnfold.label, true );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( !pointEditor->HasPoint() )  // Set wxCursor shape when starting the tool
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_PENCIL );

        wxPoint cursorPos = (wxPoint) getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );
        bool forceHV = m_frame->GetForceHVLines();

        //------------------------------------------------------------------------
        // Handle cancel:
        //
        auto cleanup = [&] () {
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            for( auto wire : m_wires )
                delete wire;

            m_wires.clear();
            segment = nullptr;

            if( m_busUnfold.entry )
                m_frame->RemoveFromScreen( m_busUnfold.entry );

            if( m_busUnfold.label && !m_busUnfold.label_placed )
                m_selectionTool->RemoveItemFromSel( m_busUnfold.label, true );

            if( m_busUnfold.label && m_busUnfold.label_placed )
                m_frame->RemoveFromScreen( m_busUnfold.label );

            delete m_busUnfold.entry;
            delete m_busUnfold.label;
            m_busUnfold = {};

            m_view->ClearPreview();
            m_view->ShowPreview( false );
        };

        if( evt->IsCancelInteractive() )
        {
            if( segment || m_busUnfold.in_progress )
                cleanup();
            else
            {
                m_frame->PopTool( aTool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( segment || m_busUnfold.in_progress )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( aTool );
                break;
            }
        }
        //------------------------------------------------------------------------
        // Handle finish:
        //
        else if( evt->IsAction( &EE_ACTIONS::finishLineWireOrBus )
                     || evt->IsAction( &EE_ACTIONS::finishWire )
                     || evt->IsAction( &EE_ACTIONS::finishBus )
                     || evt->IsAction( &EE_ACTIONS::finishLine ) )
        {
            if( segment || m_busUnfold.in_progress )
            {
                finishSegments();
                segment = nullptr;
            }
        }
        //------------------------------------------------------------------------
        // Handle click:
        //
        else if( evt->IsClick( BUT_LEFT ) || ( segment && evt->IsDblClick( BUT_LEFT ) ) )
        {
            // First click when unfolding places the label and wire-to-bus entry
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxASSERT( aType == LAYER_WIRE );

                m_frame->AddToScreen( m_busUnfold.label );
                m_selectionTool->RemoveItemFromSel( m_busUnfold.label, true );
                m_busUnfold.label_placed = true;
            }

            if( !segment )
            {
                segment = startSegments( aType, VECTOR2D( cursorPos ) );
            }
            // Create a new segment if we're out of previously-created ones
            else if( !segment->IsNull() || ( forceHV && !m_wires.end()[-2]->IsNull() ) )
            {
                // Terminate the command if the end point is on a pin, junction, or another
                // wire or bus.
                if( !m_busUnfold.in_progress
                        && screen->IsTerminalPoint( cursorPos, segment->GetLayer() ) )
                {
                    finishSegments();
                    segment = nullptr;
                }
                else
                {
                    segment->SetEndPoint( cursorPos );

                    // Create a new segment, and chain it after the current segment.
                    segment = new SCH_LINE( *segment );
                    segment->SetFlags( IS_NEW | IS_MOVED );
                    segment->SetStartPoint( cursorPos );
                    m_wires.push_back( segment );

                    m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );
                }
            }

            if( evt->IsDblClick( BUT_LEFT ) && segment )
            {
                if( forceHV && m_wires.size() >= 2 )
                    computeBreakPoint( { m_wires.end()[-2], segment }, cursorPos );

                finishSegments();
                segment = nullptr;
            }
        }
        //------------------------------------------------------------------------
        // Handle motion:
        //
        else if( evt->IsMotion() || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            m_view->ClearPreview();

            // Update the bus unfold posture based on the mouse movement
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxPoint cursor_delta = cursorPos - m_busUnfold.origin;
                SCH_BUS_WIRE_ENTRY* entry = m_busUnfold.entry;

                bool offset = ( cursor_delta.x < 0 );
                char shape = ( offset ? ( ( cursor_delta.y >= 0 ) ? '/' : '\\' )
                                      : ( ( cursor_delta.y >= 0 ) ? '\\' : '/' ) );

                // Erase and redraw if necessary
                if( shape != entry->GetBusEntryShape() || offset != m_busUnfold.offset )
                {
                    entry->SetBusEntryShape( shape );
                    wxPoint entry_pos = m_busUnfold.origin;

                    if( offset )
                        entry_pos -= entry->GetSize();

                    entry->SetPosition( entry_pos );
                    m_busUnfold.offset = offset;

                    m_frame->RefreshItem( entry );

                    wxPoint wire_start = offset ? entry->GetPosition() : entry->m_End();
                    m_wires.front()->SetStartPoint( wire_start );
                }

                // Update the label "ghost" position
                m_busUnfold.label->SetPosition( cursorPos );
                m_view->AddToPreview( m_busUnfold.label->Clone() );
            }

            if( segment )
            {
                // Coerce the line to vertical or horizontal if necessary
                if( forceHV && m_wires.size() >= 2 )
                    computeBreakPoint( { m_wires.end()[-2], segment }, cursorPos );
                else
                    segment->SetEndPoint( cursorPos );
            }

            for( auto wire : m_wires )
            {
                if( !wire->IsNull() )
                    m_view->AddToPreview( wire->Clone() );
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu:
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !segment )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_UNFOLD_BUS
                && evt->GetCommandId().get() <= ID_POPUP_SCH_UNFOLD_BUS_END )
            {
                wxASSERT_MSG( !segment, "Bus unfold event received when already drawing!" );

                aType = LAYER_WIRE;
                wxString net = *evt->Parameter<wxString*>();
                segment = doUnfoldBus( net );
            }
        }
        else
            evt->SetPassEvent();

        // Enable autopanning and cursor capture only when there is a segment to be placed
        getViewControls()->SetAutoPan( segment != nullptr );
        getViewControls()->CaptureCursor( segment != nullptr );
    }

    return 0;
}


SCH_LINE* SCH_LINE_WIRE_BUS_TOOL::startSegments( int aType, const VECTOR2D& aPos )
{
    SCH_LINE* segment = nullptr;

    switch ( aType )
    {
    default:
        segment = new SCH_LINE( aPos, LAYER_NOTES );
        break;
    case LAYER_WIRE:
        segment = new SCH_LINE( aPos, LAYER_WIRE );
        break;
    case LAYER_BUS:
        segment = new SCH_LINE( aPos, LAYER_BUS );
        break;
    }

    segment->SetFlags( IS_NEW | IS_MOVED );
    m_wires.push_back( segment );

    m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );

    // We need 2 segments to go from a given start pin to an end point when the
    // horizontal and vertical lines only switch is on.
    if( m_frame->GetForceHVLines() )
    {
        segment = new SCH_LINE( *segment );
        segment->SetFlags( IS_NEW | IS_MOVED );
        m_wires.push_back( segment );

        m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );
    }

    return segment;
}


/**
 * In a contiguous list of wires, remove wires that backtrack over the previous
 * wire. Example:
 *
 * Wire is added:
 * ---------------------------------------->
 *
 * A second wire backtracks over it:
 * -------------------<====================>
 *
 * simplifyWireList is called:
 * ------------------->
 */
void SCH_LINE_WIRE_BUS_TOOL::simplifyWireList()
{
    for( auto it = m_wires.begin(); it != m_wires.end(); )
    {
        SCH_LINE* line = *it;

        if( line->IsNull() )
        {
            delete line;
            it = m_wires.erase( it );
            continue;
        }

        auto next_it = it;
        ++next_it;

        if( next_it == m_wires.end() )
            break;

        SCH_LINE* next_line = *next_it;

        if( line->IsParallel( next_line ) )
        {
            if( SCH_LINE* merged = line->MergeOverlap( next_line ) )
            {
                delete line;
                delete next_line;
                it = m_wires.erase( it );
                *it = merged;
            }
        }

        ++it;
    }
}


void SCH_LINE_WIRE_BUS_TOOL::finishSegments()
{
    // Clear selection when done so that a new wire can be started.
    // NOTE: this must be done before simplifyWireList is called or we might end up with
    // freed selected items.
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    PICKED_ITEMS_LIST itemList;

    // Remove segments backtracking over others
    simplifyWireList();

    // Collect the possible connection points for the new lines
    std::vector< wxPoint > connections;
    std::vector< wxPoint > new_ends;
    m_frame->GetSchematicConnections( connections );

    // Check each new segment for possible junctions and add/split if needed
    for( auto wire : m_wires )
    {
        if( wire->HasFlag( SKIP_STRUCT ) )
            continue;

        wire->GetConnectionPoints( new_ends );

        for( auto i : connections )
        {
            if( IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), i ) )
                new_ends.push_back( i );
        }
        itemList.PushItem( ITEM_PICKER( wire, UR_NEW ) );
    }

    if( m_busUnfold.in_progress && m_busUnfold.label_placed )
    {
        wxASSERT( m_busUnfold.entry && m_busUnfold.label );

        itemList.PushItem( ITEM_PICKER( m_busUnfold.entry, UR_NEW ) );
        itemList.PushItem( ITEM_PICKER( m_busUnfold.label, UR_NEW ) );
        m_busUnfold.label->ClearEditFlags();
    }

    // Get the last non-null wire (this is the last created segment).
    if( !m_wires.empty() )
        m_frame->SaveCopyForRepeatItem( m_wires.back() );

    // Add the new wires
    for( auto wire : m_wires )
    {
        wire->ClearFlags( IS_NEW | IS_MOVED );
        m_frame->AddToScreen( wire );
    }

    m_wires.clear();
    m_view->ClearPreview();
    m_view->ShowPreview( false );

    getViewControls()->CaptureCursor( false );
    getViewControls()->SetAutoPan( false );

    m_frame->SaveCopyInUndoList( itemList, UR_NEW );

    // Correct and remove segments that need to be merged.
    m_frame->SchematicCleanUp();

    for( auto item : m_frame->GetScreen()->Items().OfType( SCH_COMPONENT_T ) )
    {
        std::vector< wxPoint > pts;
        item->GetConnectionPoints( pts );

        if( pts.size() > 2 )
            continue;

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                m_frame->TrimWire( *i, *j );
        }
    }

    for( auto i : new_ends )
    {
        if( m_frame->GetScreen()->IsJunctionNeeded( i, true ) )
            m_frame->AddJunction( i, true, false );
    }

    if( m_busUnfold.in_progress )
        m_busUnfold = {};

    m_frame->TestDanglingEnds();
    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    m_frame->OnModify();
}


int SCH_LINE_WIRE_BUS_TOOL::AddJunctionsIfNeeded( const TOOL_EVENT& aEvent )
{
    EE_SELECTION* aSelection = aEvent.Parameter<EE_SELECTION*>();

    std::vector<wxPoint> pts;
    std::vector<wxPoint> connections;

    m_frame->GetSchematicConnections( connections );

    for( unsigned ii = 0; ii < aSelection->GetSize(); ii++ )
    {
        SCH_ITEM*            item = dynamic_cast<SCH_ITEM*>( aSelection->GetItem( ii ) );
        std::vector<wxPoint> new_pts;

        if( !item || !item->IsConnectable() )
            continue;

        item->GetConnectionPoints( new_pts );
        pts.insert( pts.end(), new_pts.begin(), new_pts.end() );

        // If the item is a line, we also add any connection points from the rest of the schematic
        // that terminate on the line after it is moved.
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = (SCH_LINE*) item;
            for( auto i : connections )
            {
                if( IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), i ) )
                    pts.push_back( i );
            }
        }
        else
        {
            // Clean up any wires that short non-wire connections in the list
            for( auto point = new_pts.begin(); point != new_pts.end(); point++ )
            {
                for( auto second_point = point + 1; second_point != new_pts.end(); second_point++ )
                    m_frame->TrimWire( *point, *second_point );
            }
        }
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( pts.begin(), pts.end(), []( const wxPoint& a, const wxPoint& b ) -> bool {
        return a.x < b.x || ( a.x == b.x && a.y < b.y );
    } );

    pts.erase( unique( pts.begin(), pts.end() ), pts.end() );

    for( auto point : pts )
    {
        if( m_frame->GetScreen()->IsJunctionNeeded( point, true ) )
            m_frame->AddJunction( point, true, false );
    }

    return 0;
}


void SCH_LINE_WIRE_BUS_TOOL::setTransitions()
{
    Go( &SCH_LINE_WIRE_BUS_TOOL::AddJunctionsIfNeeded, EE_ACTIONS::addNeededJunctions.MakeEvent() );
    Go( &SCH_LINE_WIRE_BUS_TOOL::DrawSegments, EE_ACTIONS::drawWire.MakeEvent() );
    Go( &SCH_LINE_WIRE_BUS_TOOL::DrawSegments, EE_ACTIONS::drawBus.MakeEvent() );
    Go( &SCH_LINE_WIRE_BUS_TOOL::DrawSegments, EE_ACTIONS::drawLines.MakeEvent() );

    Go( &SCH_LINE_WIRE_BUS_TOOL::UnfoldBus,    EE_ACTIONS::unfoldBus.MakeEvent() );
}
