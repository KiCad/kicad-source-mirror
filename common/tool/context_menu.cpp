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
    m_titleSet( false ), m_handler( this ), m_tool( NULL )
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
    m_titleSet( aMenu.m_titleSet ), m_handler( this ), m_tool( aMenu.m_tool )
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
}


void CONTEXT_MENU::SetTitle( const wxString& aTitle )
{
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
	m_menu.Append( new wxMenuItem( &m_menu, aId, aLabel, wxEmptyString, wxITEM_NORMAL ) );
}


void CONTEXT_MENU::Add( const TOOL_ACTION& aAction, int aId )
{
    m_menu.Append( new wxMenuItem( &m_menu, aId,
                    wxString( aAction.GetDescription() + '\t' + getHotKeyDescription( aAction ) ),
                    wxEmptyString, wxITEM_NORMAL ) );
}


void CONTEXT_MENU::Clear()
{
	m_titleSet = false;

	for( unsigned i = 0; i < m_menu.GetMenuItemCount(); ++i )
	    m_menu.Destroy( m_menu.FindItemByPosition( 0 ) );
}


std::string CONTEXT_MENU::getHotKeyDescription( const TOOL_ACTION& aAction ) const
{
    int hotkey = aAction.GetHotKey();

    std::string description = "";

    if( hotkey & MD_ModAlt )
        description += "ALT+";
    if( hotkey & MD_ModCtrl )
        description += "CTRL+";
    if( hotkey & MD_ModShift )
        description += "SHIFT+";

    // TODO dispatch keys such as Fx, TAB, PG_UP/DN, HOME, END, etc.
    description += char( hotkey & ~MD_ModifierMask );

    return description;
}


void CONTEXT_MENU::CMEventHandler::onEvent( wxEvent& aEvent )
{
    TOOL_EVENT evt;
    wxEventType type = aEvent.GetEventType();

    if( type == wxEVT_MENU_HIGHLIGHT )
        evt = TOOL_EVENT( TC_Command, TA_ContextMenuUpdate, aEvent.GetId() );
    else if( type == wxEVT_COMMAND_MENU_SELECTED )
        evt = TOOL_EVENT( TC_Command, TA_ContextMenuChoice, aEvent.GetId() );

    if( m_menu->m_tool )
        m_menu->m_tool->GetManager()->ProcessEvent( evt );
}
