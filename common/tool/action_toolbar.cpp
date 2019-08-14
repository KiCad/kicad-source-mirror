/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.txt for contributors.
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

#include <functional>
#include <eda_draw_frame.h>
#include <tool/actions.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/tool_interactive.h>
#include <tool/action_toolbar.h>


ACTION_TOOLBAR::ACTION_TOOLBAR( EDA_BASE_FRAME* parent, wxWindowID id, const wxPoint& pos,
                                const wxSize& size, long style ) :
    wxAuiToolBar( parent, id, pos, size, style ),
    m_toolManager( parent->GetToolManager() )
{
    Connect( wxEVT_COMMAND_TOOL_CLICKED, wxAuiToolBarEventHandler( ACTION_TOOLBAR::onToolEvent ), NULL, this );
}


void ACTION_TOOLBAR::Add( const TOOL_ACTION& aAction, bool aIsToggleEntry )
{
    EDA_BASE_FRAME* editFrame = m_toolManager->GetEditFrame();
    int toolId = aAction.GetId() + ACTION_ID;

    AddTool( toolId, wxEmptyString, KiScaledBitmap( aAction.GetIcon(), editFrame ),
             aAction.GetDescription(), aIsToggleEntry ? wxITEM_CHECK : wxITEM_NORMAL );

    m_toolKinds[ toolId ] = aIsToggleEntry;
    m_toolActions[ toolId ] = &aAction;
}


void ACTION_TOOLBAR::AddButton( const TOOL_ACTION& aAction )
{
    EDA_BASE_FRAME* editFrame = m_toolManager->GetEditFrame();
    int toolId = aAction.GetId() + ACTION_ID;

    AddTool( toolId, wxEmptyString, KiScaledBitmap( aAction.GetIcon(), editFrame ),
             aAction.GetName(), wxITEM_NORMAL );

    m_toolKinds[ toolId ] = false;
    m_toolActions[ toolId ] = &aAction;
}


void ACTION_TOOLBAR::SetToolBitmap( const TOOL_ACTION& aAction, const wxBitmap& aBitmap )
{
    int toolId = aAction.GetId() + ACTION_ID;
    wxAuiToolBar::SetToolBitmap( toolId, aBitmap );

    // Set the disabled bitmap: we use the disabled bitmap version
    // of aBitmap.
    wxAuiToolBarItem* tb_item = wxAuiToolBar::FindTool( toolId );

    if( tb_item )
        tb_item->SetDisabledBitmap( aBitmap.ConvertToDisabled() );
}


void ACTION_TOOLBAR::Toggle( const TOOL_ACTION& aAction, bool aState )
{
    int toolId = aAction.GetId() + ACTION_ID;

    if( m_toolKinds[ toolId ] )
        ToggleTool( toolId, aState );
    else
        EnableTool( toolId, aState );
}


void ACTION_TOOLBAR::Toggle( const TOOL_ACTION& aAction, bool aEnabled, bool aChecked )
{
    int toolId = aAction.GetId() + ACTION_ID;

    EnableTool( toolId, aEnabled );
    ToggleTool( toolId, aEnabled && aChecked );
}


void ACTION_TOOLBAR::onToolEvent( wxAuiToolBarEvent& aEvent )
{
    OPT_TOOL_EVENT evt;
    wxString menuText;

    wxEventType type = aEvent.GetEventType();

    if( type == wxEVT_COMMAND_TOOL_CLICKED && aEvent.GetId() >= ACTION_ID )
    {
        const auto it = m_toolActions.find( aEvent.GetId() );

        if( it != m_toolActions.end() )
            evt = it->second->MakeEvent();
    }

    // forward the action/update event to the TOOL_MANAGER
    if( evt && m_toolManager )
    {
        evt->SetHasPosition( false );
        m_toolManager->ProcessEvent( *evt );
    }
    else
    {
        aEvent.Skip();
    }
}

