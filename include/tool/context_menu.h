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

#include <wx/string.h>

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
	~CONTEXT_MENU();

	void SetTitle( const wxString& aTitle );
	void Add( const wxString& aItem, int aId );
	
	// fixme: unimplemented
	// void Add ( const TOOL_ACTION& aAction, int aId = -1 );
	
	void Clear();

	wxMenu* GetMenu() const
	{
		return m_menu;
	}

private:
	class CMEventHandler;
	
	friend class TOOL_INTERACTIVE;

	void setTool( TOOL_INTERACTIVE* aTool )
	{
		m_tool = aTool;
	}

	bool m_titleSet;

	wxMenu* m_menu;
	CMEventHandler* m_handler;
	TOOL_INTERACTIVE* m_tool;
};

#endif
