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

#include <cstring>
#include <string>

#include <base_struct.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>

#include <boost/foreach.hpp>

using namespace std;

struct FlagString
{
	int flag;		
	std::string str;
};


static const std::string flag2string( int flag, const FlagString* exps )
{
	std::string rv;

	for( int i = 0; exps[i].str.length(); i++ )
	{
		if( exps[i].flag & flag )
			rv += exps[i].str + " ";
	}

	return rv;
}


const std::string TOOL_EVENT::Format() const
{
	std::string ev;

	const FlagString categories[] = {
		{ TC_Mouse, "mouse" },
		{ TC_Keyboard, "keyboard" },
		{ TC_Command, "command" },
		{ TC_Message, "message" },
		{ TC_View, "view" },
		{ 0, "" }
	};

	const FlagString actions[] = {
		{ TA_MouseClick, "click" },
		{ TA_MouseUp, "button-up" },
		{ TA_MouseDown, "button-down" },
		{ TA_MouseDrag, "drag" },
		{ TA_MouseMotion, "motion" },
		{ TA_MouseWheel, "wheel" },
		{ TA_KeyUp, "key-up" },
		{ TA_KeyDown, "key-down" },
		{ TA_ViewRefresh, "view-refresh" },
		{ TA_ViewZoom, "view-zoom" },
		{ TA_ViewPan, "view-pan" },
		{ TA_ViewDirty, "view-dirty" },
		{ TA_ChangeLayer, "change-layer" },
		{ TA_CancelTool, "cancel-tool" },
		{ TA_ActivateTool, "activate-tool" },
		{ TA_ContextMenuUpdate, "context-menu-update" },
		{ TA_ContextMenuChoice, "context-menu-choice" },
		{ 0, "" }
	};

	const FlagString buttons[] = {
		{ MB_None, "none" },
		{ MB_Left, "left" },
		{ MB_Right, "right" },
		{ MB_Middle, "middle" },
		{ 0, "" }
	};

	const FlagString modifiers[] = {
        { MD_ModShift, "shift" },
        { MD_ModCtrl, "ctrl" },
        { MD_ModAlt, "alt" },
        { 0, "" }
	};

	ev = "category: ";
	ev += flag2string( m_category, categories );
	ev += " action: ";
	ev += flag2string( m_actions, actions );
	
	if( m_actions & TA_Mouse )
	{
		ev += " btns: ";
		ev += flag2string( m_mouseButtons, buttons );
	}

	if( m_actions & TA_Keyboard )
	{
        char tmp[128];
        sprintf( tmp, "key: %d", m_keyCode );
        ev += tmp;
	}
	
	if( m_actions & ( TA_Mouse | TA_Keyboard ) )
	{
	    ev += " mods: ";
	    ev += flag2string( m_modifiers, modifiers );
	}

	if( m_commandId )
	{
		char tmp[128];
		sprintf( tmp, "cmd-id: %d", *m_commandId );
		ev += tmp;
	}

	if( m_commandStr )
		ev += "cmd-str: " + ( *m_commandStr );
		
	return ev;
}


const std::string TOOL_EVENT_LIST::Format() const
{
	string s;

	BOOST_FOREACH( TOOL_EVENT e, m_events )
		s += e.Format() + " ";
	
	return s;
}
