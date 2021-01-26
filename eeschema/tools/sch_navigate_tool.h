/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef SCH_NAVIGATE_TOOL_H
#define SCH_NAVIGATE_TOOL_H

#include <sch_base_frame.h>
#include <tools/ee_tool_base.h>
#include <status_popup.h>

class SCH_EDIT_FRAME;

/**
 * Handle actions specific to the schematic editor.
 */
class SCH_NAVIGATE_TOOL : public wxEvtHandler, public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_NAVIGATE_TOOL()  :
            EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.NavigateTool" )
    { }

    ~SCH_NAVIGATE_TOOL() { }

    int EnterSheet( const TOOL_EVENT& aEvent );
    int LeaveSheet( const TOOL_EVENT& aEvent );
    int NavigateHierarchy( const TOOL_EVENT& aEvent );
    int HypertextCommand( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    std::stack<wxString> m_hypertextStack;
};


#endif // SCH_NAVIGATE_TOOL_H
