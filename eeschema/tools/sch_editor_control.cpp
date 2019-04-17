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
#include <sch_view.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <erc.h>
#include <eeschema_id.h>
#include <netlist_object.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_picker_tool.h>
#include <project.h>
#include <tools/sch_editor_control.h>
#include <hotkeys.h>
#include <advanced_config.h>

TOOL_ACTION SCH_ACTIONS::refreshPreview( "eeschema.EditorControl.refreshPreview",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::highlightNet( "eeschema.EditorControl.highlightNet",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::highlightNetSelection( "eeschema.EditorControl.highlightNetSelection",
        AS_GLOBAL, 0, "", "" );

TOOL_ACTION SCH_ACTIONS::highlightNetCursor( "eeschema.EditorControl.highlightNetCursor",
        AS_GLOBAL, 0,
        _( "Highlight Net" ), _( "Highlight wires and pins of a net" ), NULL, AF_ACTIVATE );


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
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

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


// TODO(JE) Probably use netcode rather than connection name here eventually
static bool highlightNet( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( aToolMgr->GetEditFrame() );
    wxString        netName;
    EDA_ITEMS       nodeList;
    bool            retVal = true;

    if( editFrame->GetScreen()->GetNode( wxPoint( aPosition.x, aPosition.y ), nodeList ) )
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
                    editFrame->SetStatusText( _( "Highlighted net: " ) + UnescapeString( netName ) );
                }
            }
        }
    }

    editFrame->SetSelectedNetName( netName );
    editFrame->SendCrossProbeNetName( netName );
    editFrame->SetStatusText( _( "Selected net: " ) + UnescapeString( netName ) );

    aToolMgr->RunAction( SCH_ACTIONS::highlightNetSelection, true );

    return retVal;
}


int SCH_EDITOR_CONTROL::HighlightNet( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    VECTOR2I              gridPosition = controls->GetCursorPosition( true );

    highlightNet( m_toolMgr, gridPosition );

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetSelection( const TOOL_EVENT& aEvent )
{
    SCH_SCREEN*            screen = g_CurrentSheet->LastScreen();
    std::vector<EDA_ITEM*> itemsToRedraw;
    wxString               selectedNetName = m_frame->GetSelectedNetName();

    if( !screen )
        return 0;

    for( SCH_ITEM* ptr = screen->GetDrawItems(); ptr; ptr = ptr->Next() )
    {
        auto conn = ptr->Connection( *g_CurrentSheet );
        bool redraw = ptr->GetState( BRIGHTENED );

        ptr->SetState( BRIGHTENED, ( conn && conn->Name() == selectedNetName ) );

        redraw |= ptr->GetState( BRIGHTENED );

        if( ptr->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* comp = static_cast<SCH_COMPONENT*>( ptr );

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
        else if( ptr->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN& pin : static_cast<SCH_SHEET*>( ptr )->GetPins() )
            {
                auto pin_conn = pin.Connection( *g_CurrentSheet );
                bool redrawPin = pin.GetState( BRIGHTENED );

                pin.SetState( BRIGHTENED, ( pin_conn && pin_conn->Name() == selectedNetName ) );

                redrawPin |= pin.GetState( BRIGHTENED );

                if( redrawPin )
                    itemsToRedraw.push_back( &pin );
            }
        }

        if( redraw )
            itemsToRedraw.push_back( ptr );
    }

    // Be sure hightlight change will be redrawn
    KIGFX::VIEW* view = getView();

    for( auto item : itemsToRedraw )
        view->Update( (KIGFX::VIEW_ITEM*)item, KIGFX::VIEW_UPDATE_FLAGS::REPAINT );

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int SCH_EDITOR_CONTROL::HighlightNetCursor( const TOOL_EVENT& aEvent )
{
    // TODO(JE) remove once real-time connectivity is a given
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity )
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


void SCH_EDITOR_CONTROL::setTransitions()
{
    /*
    Go( &SCH_EDITOR_CONTROL::ToggleLockSelected,    SCH_ACTIONS::toggleLock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::LockSelected,          SCH_ACTIONS::lock.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::UnlockSelected,        SCH_ACTIONS::unlock.MakeEvent() );
     */

    /*
    Go( &SCH_EDITOR_CONTROL::CrossProbeSchToPcb,    SELECTION_TOOL::SelectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeSchToPcb,    SELECTION_TOOL::UnselectedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbeSchToPcb,    SELECTION_TOOL::ClearedEvent );
    Go( &SCH_EDITOR_CONTROL::CrossProbePcbToSch,    SCH_ACTIONS::crossProbeSchToPcb.MakeEvent() );
     */

    Go( &SCH_EDITOR_CONTROL::HighlightNet,          SCH_ACTIONS::highlightNet.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetCursor,    SCH_ACTIONS::highlightNetCursor.MakeEvent() );
    Go( &SCH_EDITOR_CONTROL::HighlightNetSelection, SCH_ACTIONS::highlightNetSelection.MakeEvent() );
}
