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

#ifndef __CONTEXT_MENU_H
#define __CONTEXT_MENU_H

#include <wx/menu.h>
#include <tool/tool_action.h>

class wxMenu;
class TOOL_INTERACTIVE;

/**
 * Class CONTEXT_MENU
 *
 * Defines the structure of a context (usually right-click) popup menu
 * for a given tool.
 */
class CONTEXT_MENU 
{

public:
	CONTEXT_MENU();
	CONTEXT_MENU( const CONTEXT_MENU& aMenu );

	void SetTitle( const wxString& aTitle );
	void Add( const wxString& aLabel, int aId );
	void Add( const TOOL_ACTION& aAction, int aId = -1 );
	void Clear();

	wxMenu* GetMenu() const
	{
		return const_cast<wxMenu*>( &m_menu );
	}

private:
	class CMEventHandler : public wxEvtHandler
	{
	public:
	    CMEventHandler( CONTEXT_MENU* aMenu ) : m_menu( aMenu ) {};

	    void onEvent( wxEvent& aEvent );

	private:
	    CONTEXT_MENU* m_menu;
	};
	
	friend class TOOL_INTERACTIVE;

	void setTool( TOOL_INTERACTIVE* aTool )
	{
		m_tool = aTool;
	}

	/**
	 * Returns a hot key in the string format accepted by wxMenu.
	 *
	 * @param aAction is the action with hot key to be converted.
	 * @return Hot key in the string format compatible with wxMenu.
	 */
	std::string getHotKeyDescription( const TOOL_ACTION& aAction ) const;

	/// Flag indicating that the menu title was set up
	bool m_titleSet;

	wxMenu m_menu;
	CMEventHandler m_handler;
	TOOL_INTERACTIVE* m_tool;
};

#endif
