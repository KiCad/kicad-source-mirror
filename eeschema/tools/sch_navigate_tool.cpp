/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <schematic.h>
#include <eeschema_id.h>
#include <tools/ee_actions.h>
#include <tools/sch_navigate_tool.h>


int SCH_NAVIGATE_TOOL::NavigateHierarchy( const TOOL_EVENT& aEvent )
{
    m_frame->UpdateHierarchyNavigator( true );
    return 0;
}


int SCH_NAVIGATE_TOOL::HypertextCommand( const TOOL_EVENT& aEvent )
{
    wxString* page = aEvent.Parameter<wxString*>();

    wxCHECK( page, 0 );

    auto goToPage =
            [&]( wxString* aPage )
            {
                for( const SCH_SHEET_PATH& sheet : m_frame->Schematic().GetSheets() )
                {
                    if( sheet.GetPageNumber() == *aPage )
                    {
                        m_frame->GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );
                        m_frame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

                        m_frame->SetCurrentSheet( sheet );
                        m_frame->DisplayCurrentSheet();

                        return;
                    }
                }
            };

    if( *page == "HYPERTEXT_BACK" )
    {
        if( m_hypertextStack.size() > 0 )
        {
            goToPage( &m_hypertextStack.top() );
            m_hypertextStack.pop();
        }
    }
    else
    {
        m_hypertextStack.push( m_frame->GetCurrentSheet().GetPageNumber() );
        goToPage( page );
    }

    return 0;
}


int SCH_NAVIGATE_TOOL::EnterSheet( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    const EE_SELECTION& selection = selTool->RequestSelection( EE_COLLECTOR::SheetsOnly );

    if( selection.GetSize() == 1 )
    {
        SCH_SHEET* sheet = (SCH_SHEET*) selection.Front();

        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->GetCurrentSheet().push_back( sheet );
        m_frame->DisplayCurrentSheet();
    }

    return 0;
}


int SCH_NAVIGATE_TOOL::LeaveSheet( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetCurrentSheet().Last() != &m_frame->Schematic().Root() )
    {
        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->GetCurrentSheet().pop_back();
        m_frame->DisplayCurrentSheet();
    }

    return 0;
}


void SCH_NAVIGATE_TOOL::setTransitions()
{
    Go( &SCH_NAVIGATE_TOOL::EnterSheet,            EE_ACTIONS::enterSheet.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::LeaveSheet,            EE_ACTIONS::leaveSheet.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::NavigateHierarchy,     EE_ACTIONS::navigateHierarchy.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::HypertextCommand,      EE_ACTIONS::hypertextCommand.MakeEvent() );
}
