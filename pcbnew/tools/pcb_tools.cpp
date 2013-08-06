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
#include <wx/event.h>

#include <wxPcbStruct.h>
#include <wxBasePcbFrame.h>

#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>

#include <class_drawpanel_gal.h>
#include <pcbnew_id.h>

#include "selection_tool.h"

void PCB_EDIT_FRAME::setupTools()
{
	// create the manager and dispatcher. Route draw panel events to the dispatcher.
	m_toolManager = new TOOL_MANAGER;
	m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, this );
	m_galCanvas->SetEventDispatcher( m_toolDispatcher );

	// register our selection tool.
	m_toolManager->RegisterTool( new SELECTION_TOOL );
}


void PCB_EDIT_FRAME::onGenericCommand( wxCommandEvent &aEvent )
{
	m_toolDispatcher->DispatchWxCommand( aEvent );
}

