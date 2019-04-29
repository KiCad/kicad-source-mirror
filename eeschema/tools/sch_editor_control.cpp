/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <sch_view.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <connection_graph.h>
#include <erc.h>
#include <eeschema_id.h>
#include <netlist_object.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_picker_tool.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_selection_tool.h>
#include <project.h>
#include <hotkeys.h>
#include <advanced_config.h>
#include <status_popup.h>
#include <simulation_cursors.h>
#include <sim/sim_plot_frame.h>

TOOL_ACTION SCH_ACTIONS::refreshPreview( "eeschema.EditorControl.refreshPreview",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::simProbe( "eeschema.Simulation.probe",
        AS_GLOBAL, 0,
        _( "Add a simulator probe" ), "" );

TOOL_ACTION SCH_ACTIONS::simTune( "eeschema.Simulation.tune",
        AS_GLOBAL, 0,
        _( "Select a value to be tuned" ), "" );

TOOL_ACTION SCH_ACTIONS::highlightNet( "eeschema.EditorControl.highlightNet",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::clearHighlight( "eeschema.EditorControl.clearHighlight",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::highlightNetSelection( "eeschema.EditorControl.highlightNetSelection",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::highlightNetCursor( "eeschema.EditorControl.highlightNetCursor",
        AS_GLOBAL, 0,
        _( "Highlight Net" ), _( "Highlight wires and pins of a net" ), NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::deleteItemCursor( "eeschema.EditorControl.deleteItemCursor",
        AS_GLOBAL, 0,
        _( "Delete Items" ), _( "Delete clicked items" ), NULL, AF_ACTIVATE );


SCH_EDITOR_CONTROL::SCH_EDITOR_CONTROL() :
        TOOL_INTERACTIVE( "eeschema.EditorControl" ),
        m_frame( nullptr ),
        m_menu( *this )
{
}


SCH_EDITOR_CONTROL::~SCH_EDITOR_CONTROL()
{
}


void SCH_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
}


bool SCH_EDITOR_CONTROL::Init()
{
    auto activeToolCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto inactiveStateCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_NO_TOOL_SELECTED && aSel.Size() == 0 );
    };

    auto& ctxMenu = m_menu.GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1000 );
    ctxMenu.AddSeparator( activeToolCondition, 1000 );

    // Finally, add the standard zoom & grid items
    m_menu.AddStandardSubMenus( *getEditFrame<SCH_BASE_FRAME>() );

    /*
    auto lockMenu = std::make_shared<LOCK_CONTEXT_MENU>();
    lockMenu->SetTool( this );

    // Add the SCH control menus to relevant other tools
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        menu.AddSeparator( inactiveStateCondition );
        toolMenu.AddSubMenu( lockMenu );

        menu.AddMenu( lockMenu.get(), false,
                      SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::LockableItems ), 200 );
    }
    */

    return true;
}


int SCH_EDITOR_CONTROL::CrossProbeSchToPcb( const TOOL_EVENT& aEvent )
{
    // Don't get in an infinite loop SCH -> PCB -> SCH -> PCB -> SCH -> ...
    if( m_probingSchToPcb )
    {
        m_probingSchToPcb = false;
        return 0;
    }

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    if( selection.Size() == 1 )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.Front() );
        SCH_COMPONENT* component;

        switch( item->Type() )
        {
        case SCH_FIELD_T:
        case LIB_FIELD_T:
            component = (SCH_COMPONENT*) item->GetParent();
            m_frame->SendMessageToPCBNEW( item, component );
            break;

        case SCH_COMPONENT_T:
            component = (SCH_COMPONENT*) item;
            m_frame->SendMessageToPCBNEW( item, component );
            break;

        case SCH_PIN_T:
            component = (SCH_COMPONENT*) item->GetParent();
            m_frame->SendMessageToPCBNEW( static_cast<SCH_PIN*>( item ), component );
            break;

#if 0   // This is too slow on larger projects
            case SCH_SHEET_T:
        SendMessageToPCBNEW( item, nullptr );
        break;
#endif
        default:
            ;
        }
    }

    return 0;
}


#ifdef KICAD_SPICE
static bool probeSimulation( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    constexpr KICAD_T wiresAndComponents[] = { SCH_LINE_T, SCH_COMPONENT_T, SCH_SHEET_PIN_T, EOT };
    SCH_SELECTION_TOOL* selTool = aFrame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();

    SCH_ITEM* item = selTool->SelectPoint( aPosition, wiresAndComponents );

    if( !item )
        return false;

    std::unique_ptr<NETLIST_OBJECT_LIST> netlist( aFrame->BuildNetListBase() );

    for( NETLIST_OBJECT* obj : *netlist )
    {
        if( obj->m_Comp == item )
        {
            auto simFrame = (SIM_PLOT_FRAME*) aFrame->Kiway().Player( FRAME_SIMULATOR, false );

            if( simFrame )
                simFrame->AddVoltagePlot( obj->GetNetName() );

            break;
        }
    }

    return true;
}


int SCH_EDITOR_CONTROL::SimProbe( const TOOL_EVENT& aEvent )
{
    Activate();

    SCH_PICKER_TOOL* picker = m_toolMgr->GetTool<SCH_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_SIM_PROBE, wxCURSOR_DEFAULT, _( "Add a simulator probe" ) );
    m_frame->GetCanvas()->SetCursor( SIMULATION_CURSORS::GetCursor( SIMULATION_CURSORS::CURSOR::PROBE ) );

    picker->SetClickHandler( std::bind( probeSimulation, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


static bool tuneSimulation( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    constexpr KICAD_T fieldsAndComponents[] = { SCH_COMPONENT_T, SCH_FIELD_T, EOT };
    SCH_SELECTION_TOOL* selTool = aFrame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_ITEM* item = selTool->SelectPoint( aPosition, fieldsAndComponents );

    if( !item )
        return false;

    if( item->Type() != SCH_COMPONENT_T )
    {
        item = static_cast<SCH_ITEM*>( item->GetParent() );

        if( item->Type() != SCH_COMPONENT_T )
            return false;
    }

    auto simFrame = (SIM_PLOT_FRAME*) aFrame->Kiway().Player( FRAME_SIMULATOR, false );

    if( simFrame )
        simFrame->AddTuner( static_cast<SCH_COMPONENT*>( item ) );

    return true;
}


int SCH_EDITOR_CONTROL::SimTune( const TOOL_EVENT& aEvent )
{
    Activate();

    SCH_PICKER_TOOL* picker = m_toolMgr->GetTool<SCH_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_SIM_TUNE, wxCURSOR_DEFAULT, _( "Select a value to be tuned" ) );
    m_frame->GetCanvas()->SetCursor( SIMULATION_CURSORS::GetCursor( SIMULATION_CURSORS::CURSOR::TUNE ) );

    picker->SetClickHandler( std::bind( tuneSimulation, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}
#endif /* KICAD_SPICE */


// A magic cookie token for clearing the highlight
static VECTOR2D CLEAR;


// TODO(JE) Probably use netcode rather than connection name here eventually
static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( aToolMgr->GetEditFrame() );
    wxString        netName;
    EDA_ITEMS       nodeList;
    bool            retVal = true;

    if( aPosition != CLEAR && editFrame->GetScreen()->GetNode( (wxPoint) aPosition, nodeList ) )
    {
        if( TestDuplicateSheetNames( false ) > 0 )
        {
            wxMessageBox( _( "Error: duplicate sub-sheet names found in current sheet." ) );
            retVal = false;
        }
        else
        {
            if( auto item = dynamic_cast<SCH_ITEM*>( nodeList[0] ) )
            {
                if( item->Connection( *g_CurrentSheet ) )
                {
                    netName = item->Connection( *g_CurrentSheet )->Name();
                    editFrame->SetStatusText( wxString::Format( _( "Highlighted net: %s" ), UnescapeString( netName ) ) );
                }
            }
        }
    }

    editFrame->SetSelectedNetName( netName );
    editFrame->SendCrossProbeNetName( netName );
    editFrame->SetStatusText( wxString::Format( _( "Selected net: %s" ), UnescapeString( netName ) ) );

    aToolMgr->RunAction( SCH_ACTIONS::highlightNetSelection, true );

    return retVal;
}


int SCH_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2D              gridPosition = controls->GetCursorPosition( true );

    highlightNet( m_toolMgr, gridPosition );

    return 0;
}


int SCH_EDITOR_CONTROL::ClearHighlight( const TOOL_EVENT& aEvent )
{
    highlightNet( m_toolMgr, CLEAR );

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetSelection( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*            screen = g_CurrentSheet->LastScreen();
    std::vector<EDA_ITEM*> itemsToRedraw;
    wxString               selectedNetName = m_frame->GetSelectedNetName();

    if( !screen )
        return 0;

    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
    {
        SCH_CONNECTION* conn = item->Connection( *g_CurrentSheet );
        bool redraw = item->IsBrightened();

        if( conn && conn->Name() == selectedNetName )
            item->SetBrightened();
        else
            item->ClearBrightened();

        redraw |= item->IsBrightened();

        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* comp = static_cast<SCH_COMPONENT*>( item );

            redraw |= comp->HasBrightenedPins();

            comp->ClearBrightenedPins();
            std::vector<LIB_PIN*> pins;
            comp->GetPins( pins );

            for( LIB_PIN* pin : pins )
            {
                auto pin_conn = comp->GetConnectionForPin( pin, *g_CurrentSheet );

                if( pin_conn && pin_conn->Name( false ) == selectedNetName )
                {
                    comp->BrightenPin( pin );
                    redraw = true;
                }
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN& pin : static_cast<SCH_SHEET*>( item )->GetPins() )
            {
                auto pin_conn = pin.Connection( *g_CurrentSheet );
                bool redrawPin = pin.IsBrightened();

                if( pin_conn && pin_conn->Name() == selectedNetName )
                    pin.SetBrightened();
                else
                    pin.ClearBrightened();

                redrawPin |= pin.IsBrightened();

                if( redrawPin )
                    itemsToRedraw.push_back( &pin );
            }
        }

        if( redraw )
            itemsToRedraw.push_back( item );
    }

    // Be sure hightlight change will be redrawn
    KIGFX::VIEW* view = getView();

    for( auto redrawItem : itemsToRedraw )
        view->Update( (KIGFX::VIEW_ITEM*)redrawItem, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        m_frame->RecalculateConnections();

    Activate();

    SCH_PICKER_TOOL* picker = m_toolMgr->GetTool<SCH_PICKER_TOOL>();
    assert( picker );

    m_frame->SetToolID( ID_HIGHLIGHT_BUTT, wxCURSOR_HAND, _( "Highlight specific net" ) );
    picker->SetClickHandler( std::bind( highlightNet, m_toolMgr, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


static bool deleteItem( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    SCH_SELECTION_TOOL* selectionTool = aFrame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selectionTool, false );

    aFrame->GetToolManager()->RunAction( SCH_ACTIONS::selectionClear, true );

    SCH_ITEM* item = selectionTool->SelectPoint( aPosition );

    if( item )
    {
        if( item->IsLocked() )
        {
            STATUS_TEXT_POPUP statusPopup( aFrame );
            statusPopup.SetText( _( "Item locked." ) );
            statusPopup.Expire( 2000 );
            statusPopup.Popup();
            statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
        }
        else
        {
            aFrame->GetToolManager()->RunAction( SCH_ACTIONS::remove, true );
        }
    }

    return true;
}


int SCH_EDITOR_CONTROL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    SCH_PICKER_TOOL* picker = m_toolMgr->GetTool<SCH_PICKER_TOOL>();
    wxCHECK( picker, 0 );

    m_frame->SetToolID( ID_SCHEMATIC_DELETE_ITEM_BUTT, wxCURSOR_BULLSEYE, _( "Delete item" ) );
    picker->SetClickHandler( std::bind( deleteItem, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


void SCH_EDITOR_CONTROL::setTransitions()
{
    /*
    Go( &SCH_EDITOR_CONTROL::ToggleLockSelected,    SCH_ACTIONS::toggleLock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::LockSelected,          SCH_ACTIONS::lock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UnlockSelected,        SCH_ACTIONS::unlock.MakeEvent() );
     */

    Go( &SCH_EDITOR_CONTROL::CrossProbeSchToPcb,    EVENTS::SelectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeSchToPcb,    EVENTS::UnselectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeSchToPcb,    EVENTS::ClearedEvent );
    /*
    Go( &SCH_EDITOR_CONTROL::CrossProbePcbToSch,    SCH_ACTIONS::crossProbeSchToPcb.MakeEvent() );
     */

#ifdef KICAD_SPICE
    Go( &SCH_EDITOR_CONTROL::SimProbe,              SCH_ACTIONS::simProbe.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::SimTune,               SCH_ACTIONS::simTune.MakeEvent() );
#endif /* KICAD_SPICE */

    Go( &SCH_EDITOR_CONTROL::HighlightNet,          SCH_ACTIONS::highlightNet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::ClearHighlight,        SCH_ACTIONS::clearHighlight.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetCursor,    SCH_ACTIONS::highlightNetCursor.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetSelection, SCH_ACTIONS::highlightNetSelection.MakeEvent() );

    Go( &SCH_EDITOR_CONTROL::DeleteItemCursor,      SCH_ACTIONS::deleteItemCursor.MakeEvent() );
}
