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

#include <wx/wx.h>
#include <wx/menu.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/tool_interactive.h>

#include <tool/context_menu.h>

class CONTEXT_MENU::CMEventHandler : public wxEvtHandler
{
public:
	CMEventHandler( CONTEXT_MENU* aMenu ):
		m_menu(aMenu) {};

	void onEvent( wxEvent & aEvent )
	{
		TOOL_EVENT evt;
		wxEventType type = aEvent.GetEventType();

		if( type == wxEVT_MENU_HIGHLIGHT )
			evt = TOOL_EVENT( TC_Command, TA_ContextMenuUpdate, aEvent.GetId() );
		else if ( type == wxEVT_COMMAND_MENU_SELECTED )
			evt = TOOL_EVENT( TC_Command, TA_ContextMenuChoice, aEvent.GetId() );
		
		m_menu->m_tool->GetManager()->ProcessEvent( evt );
	}

private:
	CONTEXT_MENU* m_menu;
};


CONTEXT_MENU::CONTEXT_MENU()
{
	m_tool = NULL;
	m_menu = new wxMenu();
	m_handler = new CMEventHandler( this );
	m_menu->Connect( wxEVT_MENU_HIGHLIGHT, wxEventHandler( CMEventHandler::onEvent ), NULL, m_handler );
	m_menu->Connect( wxEVT_COMMAND_MENU_SELECTED, wxEventHandler( CMEventHandler::onEvent ), NULL, m_handler );
	m_titleSet = false;
}


CONTEXT_MENU::~CONTEXT_MENU()
{
	delete m_menu;
	delete m_handler;
}


void CONTEXT_MENU::SetTitle( const wxString& aTitle )
{
	if( m_titleSet )
	{
        m_menu->Delete( m_menu->FindItemByPosition( 0 ) ); // fixme: this is LAME!
        m_menu->Delete( m_menu->FindItemByPosition( 0 ) );
	}
	
	m_menu->InsertSeparator( 0 );
	m_menu->Insert( 0, new wxMenuItem( m_menu, -1, aTitle, wxEmptyString, wxITEM_NORMAL ) );
	m_titleSet = true;
}


void CONTEXT_MENU::Add ( const wxString& aItem, int aId )
{
	m_menu->Append( new wxMenuItem( m_menu, aId, aItem, wxEmptyString, wxITEM_NORMAL ) );
}


void CONTEXT_MENU::Clear()
{
	m_titleSet = false;
}
