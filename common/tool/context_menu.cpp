/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/tool_interactive.h>
#include <tool/context_menu.h>
#include <cassert>

CONTEXT_MENU::CONTEXT_MENU() :
    m_titleSet( false ), m_selected( -1 ), m_handler( this ), m_tool( NULL )
{
    m_menu.Connect( wxEVT_MENU_HIGHLIGHT, wxEventHandler( CMEventHandler::onEvent ),
                     NULL, &m_handler );
    m_menu.Connect( wxEVT_COMMAND_MENU_SELECTED, wxEventHandler( CMEventHandler::onEvent ),
                     NULL, &m_handler );

    // Workaround for the case when mouse cursor never reaches menu (it hangs up tools using menu)
    wxMenuEvent menuEvent( wxEVT_MENU_HIGHLIGHT, -1, &m_menu );
    m_menu.AddPendingEvent( menuEvent );
}


CONTEXT_MENU::CONTEXT_MENU( const CONTEXT_MENU& aMenu ) :
    m_titleSet( aMenu.m_titleSet ), m_selected( -1 ), m_handler( this ), m_tool( aMenu.m_tool )
{
    m_menu.Connect( wxEVT_MENU_HIGHLIGHT, wxEventHandler( CMEventHandler::onEvent ),
                     NULL, &m_handler );
    m_menu.Connect( wxEVT_COMMAND_MENU_SELECTED, wxEventHandler( CMEventHandler::onEvent ),
                     NULL, &m_handler );

    // Workaround for the case when mouse cursor never reaches menu (it hangs up tools using menu)
    wxMenuEvent menuEvent( wxEVT_MENU_HIGHLIGHT, -1, &m_menu );
    m_menu.AddPendingEvent( menuEvent );

    // Copy all the menu entries
    for( unsigned i = 0; i < aMenu.m_menu.GetMenuItemCount(); ++i )
    {
        wxMenuItem* item = aMenu.m_menu.FindItemByPosition( i );
        m_menu.Append( new wxMenuItem( &m_menu, item->GetId(), item->GetItemLabel(),
                        wxEmptyString, wxITEM_NORMAL ) );
    }

    // Copy tool actions that are available to choose from context menu
    m_toolActions = aMenu.m_toolActions;
}


void CONTEXT_MENU::SetTitle( const wxString& aTitle )
{
    // TODO handle an empty string (remove title and separator)

    // Unfortunately wxMenu::SetTitle() does nothing..
    if( m_titleSet )
    {
        m_menu.FindItemByPosition( 0 )->SetItemLabel( aTitle );
    }
    else
    {
        m_menu.InsertSeparator( 0 );
        m_menu.Insert( 0, new wxMenuItem( &m_menu, -1, aTitle, wxEmptyString, wxITEM_NORMAL ) );
        m_titleSet = true;
    }
}


void CONTEXT_MENU::Add( const wxString& aLabel, int aId )
{
#ifdef DEBUG

    if( m_menu.FindItem( aId ) != NULL )
        wxLogWarning( wxT( "Adding more than one menu entry with the same ID may result in"
                "undefined behaviour" ) );
#endif
    m_menu.Append( new wxMenuItem( &m_menu, aId, aLabel, wxEmptyString, wxITEM_NORMAL ) );
}


void CONTEXT_MENU::Add( const TOOL_ACTION& aAction )
{
    /// ID numbers for tool actions need to have a value higher than m_actionId
    int id = m_actionId + aAction.GetId();

    wxMenuItem* item = new wxMenuItem( &m_menu, id,
        wxString( aAction.GetMenuItem().c_str(), wxConvUTF8 ),
        wxString( aAction.GetDescription().c_str(), wxConvUTF8 ), wxITEM_NORMAL );

    if( aAction.HasHotKey() )
    {
        int key = aAction.GetHotKey() & ~MD_MODIFIER_MASK;
        int mod = aAction.GetHotKey() & MD_MODIFIER_MASK;
        int flags = wxACCEL_NORMAL;

        switch( mod )
        {
        case MD_ALT:    flags = wxACCEL_ALT;    break;
        case MD_CTRL:   flags = wxACCEL_CTRL;   break;
        case MD_SHIFT:  flags = wxACCEL_SHIFT;  break;
        }

        item->SetAccel( new wxAcceleratorEntry( flags, key, id, item ) );
    }

    m_menu.Append( item );
    m_toolActions[id] = &aAction;
}


void CONTEXT_MENU::Clear()
{
    m_titleSet = false;

    // Remove all the entries from context menu
    for( unsigned i = 0; i < m_menu.GetMenuItemCount(); ++i )
        m_menu.Destroy( m_menu.FindItemByPosition( 0 ) );

    m_toolActions.clear();
}


void CONTEXT_MENU::CMEventHandler::onEvent( wxEvent& aEvent )
{
    TOOL_EVENT evt;
    wxEventType type = aEvent.GetEventType();

    // When the currently chosen item in the menu is changed, an update event is issued.
    // For example, the selection tool can use this to dynamically highlight the current item
    // from selection clarification popup.
    if( type == wxEVT_MENU_HIGHLIGHT )
        evt = TOOL_EVENT( TC_COMMAND, TA_CONTEXT_MENU_UPDATE, aEvent.GetId() );

    // One of menu entries was selected..
    else if( type == wxEVT_COMMAND_MENU_SELECTED )
    {
        // Store the selected position
        m_menu->m_selected = aEvent.GetId();

        // Check if there is a TOOL_ACTION for the given ID
        if( m_menu->m_toolActions.count( aEvent.GetId() ) == 1 )
        {
            evt = m_menu->m_toolActions[aEvent.GetId()]->MakeEvent();
        }
        else
        {
            // Handling non-action menu entries (e.g. items in clarification list)
            evt = TOOL_EVENT( TC_COMMAND, TA_CONTEXT_MENU_CHOICE, aEvent.GetId() );
        }
    }

    // forward the action/update event to the TOOL_MANAGER
    if( m_menu->m_tool )
        m_menu->m_tool->GetManager()->ProcessEvent( evt );
}
