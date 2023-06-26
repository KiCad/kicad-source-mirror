/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "eda_doc.h"


wxString SCH_NAVIGATE_TOOL::g_BackLink = wxT( "HYPERTEXT_BACK" );


void SCH_NAVIGATE_TOOL::ResetHistory()
{
    m_navHistory.clear();
    m_navHistory.push_back( m_frame->GetCurrentSheet() );
    m_navIndex = m_navHistory.begin();
}


void SCH_NAVIGATE_TOOL::CleanHistory()
{
    SCH_SHEET_LIST sheets = m_frame->Schematic().GetSheets();

    // Search through our history, and removing any entries
    // that the no longer point to a sheet on the schematic
    auto entry = m_navHistory.begin();

    while( entry != m_navHistory.end() )
    {
        if( std::find( sheets.begin(), sheets.end(), *entry ) != sheets.end() )
            ++entry;
        else
            entry = m_navHistory.erase( entry );
    }
}


void SCH_NAVIGATE_TOOL::HypertextCommand( const wxString& href )
{
    wxString destPage;

    if( href == SCH_NAVIGATE_TOOL::g_BackLink )
    {
        TOOL_EVENT dummy;
        Back( dummy );
    }
    else if( EDA_TEXT::IsGotoPageHref( href, &destPage ) && !destPage.IsEmpty() )
    {
        for( const SCH_SHEET_PATH& sheet : m_frame->Schematic().GetSheets() )
        {
            if( sheet.GetPageNumber() == destPage )
            {
                changeSheet( sheet );
                return;
            }
        }

        m_frame->ShowInfoBarError( wxString::Format( _( "Page '%s' not found." ), destPage ) );
    }
    else
    {
        wxMenu menu;

        menu.Append( 1, wxString::Format( _( "Open %s" ), href ) );

        if( m_frame->GetPopupMenuSelectionFromUser( menu ) == 1 )
            GetAssociatedDocument( m_frame, href, &m_frame->Prj() );
    }
}


int SCH_NAVIGATE_TOOL::Up( const TOOL_EVENT& aEvent )
{
    // Checks for CanGoUp()
    LeaveSheet( aEvent );
    return 0;
}


int SCH_NAVIGATE_TOOL::Forward( const TOOL_EVENT& aEvent )
{
    if( CanGoForward() )
    {
        m_navIndex++;

        m_frame->GetToolManager()->RunAction( ACTIONS::cancelInteractive );
        m_frame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

        m_frame->SetCurrentSheet( *m_navIndex );
        m_frame->DisplayCurrentSheet();
    }
    else
    {
        wxBell();
    }

    return 0;
}


int SCH_NAVIGATE_TOOL::Back( const TOOL_EVENT& aEvent )
{
    if( CanGoBack() )
    {
        m_navIndex--;

        m_frame->GetToolManager()->RunAction( ACTIONS::cancelInteractive );
        m_frame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

        m_frame->SetCurrentSheet( *m_navIndex );
        m_frame->DisplayCurrentSheet();
    }
    else
    {
        wxBell();
    }

    return 0;
}


int SCH_NAVIGATE_TOOL::Previous( const TOOL_EVENT& aEvent )
{
    if( CanGoPrevious() )
    {
        int targetSheet = m_frame->GetCurrentSheet().GetVirtualPageNumber() - 1;
        changeSheet( m_frame->Schematic().GetSheets().at( targetSheet - 1 ) );
    }
    else
    {
        wxBell();
    }

    return 0;
}


int SCH_NAVIGATE_TOOL::Next( const TOOL_EVENT& aEvent )
{
    if( CanGoNext() )
    {
        int targetSheet = m_frame->GetCurrentSheet().GetVirtualPageNumber() + 1;
        changeSheet( m_frame->Schematic().GetSheets().at( targetSheet - 1 ) );
    }
    else
    {
        wxBell();
    }

    return 0;
}


bool SCH_NAVIGATE_TOOL::CanGoBack()
{
    return m_navHistory.size() > 0 && m_navIndex != m_navHistory.begin();
}


bool SCH_NAVIGATE_TOOL::CanGoForward()
{
    return m_navHistory.size() > 0 && m_navIndex != --m_navHistory.end();
}


bool SCH_NAVIGATE_TOOL::CanGoUp()
{
    return m_frame->GetCurrentSheet().Last() != &m_frame->Schematic().Root();
}


bool SCH_NAVIGATE_TOOL::CanGoPrevious()
{
    return m_frame->GetCurrentSheet().GetVirtualPageNumber() > 1;
}


bool SCH_NAVIGATE_TOOL::CanGoNext()
{
    return m_frame->GetCurrentSheet().GetVirtualPageNumber()
           < (int) m_frame->Schematic().GetSheets().size();
}


int SCH_NAVIGATE_TOOL::ChangeSheet( const TOOL_EVENT& aEvent )
{
    SCH_SHEET_PATH* path = aEvent.Parameter<SCH_SHEET_PATH*>();
    wxCHECK( path, 0 );

    changeSheet( *path );

    return 0;
}


int SCH_NAVIGATE_TOOL::EnterSheet( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    const EE_SELECTION& selection = selTool->RequestSelection( { SCH_SHEET_T } );

    if( selection.GetSize() == 1 )
    {
        SCH_SHEET_PATH pushed = m_frame->GetCurrentSheet();
        pushed.push_back( (SCH_SHEET*) selection.Front() );

        changeSheet( pushed );
    }

    return 0;
}


int SCH_NAVIGATE_TOOL::LeaveSheet( const TOOL_EVENT& aEvent )
{
    if( CanGoUp() )
    {
        SCH_SHEET_PATH popped = m_frame->GetCurrentSheet();
        popped.pop_back();

        changeSheet( popped );
    }
    else
    {
        wxBell();
    }

    return 0;
}


void SCH_NAVIGATE_TOOL::setTransitions()
{
    Go( &SCH_NAVIGATE_TOOL::ChangeSheet,           EE_ACTIONS::changeSheet.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::EnterSheet,            EE_ACTIONS::enterSheet.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::LeaveSheet,            EE_ACTIONS::leaveSheet.MakeEvent() );

    Go( &SCH_NAVIGATE_TOOL::Up,                    EE_ACTIONS::navigateUp.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::Forward,               EE_ACTIONS::navigateForward.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::Back,                  EE_ACTIONS::navigateBack.MakeEvent() );

    Go( &SCH_NAVIGATE_TOOL::Previous,              EE_ACTIONS::navigatePrevious.MakeEvent() );
    Go( &SCH_NAVIGATE_TOOL::Next,                  EE_ACTIONS::navigateNext.MakeEvent() );
}


void SCH_NAVIGATE_TOOL::pushToHistory( SCH_SHEET_PATH aPath )
{
    if( CanGoForward() )
        m_navHistory.erase( std::next( m_navIndex ), m_navHistory.end() );

    m_navHistory.push_back( aPath );
    m_navIndex = --m_navHistory.end();
}


void SCH_NAVIGATE_TOOL::changeSheet( SCH_SHEET_PATH aPath )
{
    m_frame->GetToolManager()->RunAction( ACTIONS::cancelInteractive );
    m_frame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

    // Store the current zoom level into the current screen before switching
    m_frame->GetScreen()->m_LastZoomLevel = m_frame->GetCanvas()->GetView()->GetScale();

    pushToHistory( aPath );

    m_frame->FocusOnItem( nullptr );
    m_frame->Schematic().SetCurrentSheet( aPath );
    m_frame->DisplayCurrentSheet();
}
