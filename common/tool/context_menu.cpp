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
#include <boost/bind.hpp>
#include <cassert>

CONTEXT_MENU::CONTEXT_MENU() :
    m_titleSet( false ), m_selected( -1 ), m_tool( NULL )
{
    setCustomEventHandler( boost::bind( &CONTEXT_MENU::handleCustomEvent, this, _1 ) );

    setupEvents();
}


CONTEXT_MENU::CONTEXT_MENU( const CONTEXT_MENU& aMenu ) :
    m_titleSet( aMenu.m_titleSet ), m_selected( -1 ), m_tool( aMenu.m_tool ),
    m_toolActions( aMenu.m_toolActions ), m_customHandler( aMenu.m_customHandler )
{
    // Copy all the menu entries
    for( unsigned i = 0; i < aMenu.GetMenuItemCount(); ++i )
    {
        wxMenuItem* item = aMenu.FindItemByPosition( i );

        if( item->IsSubMenu() )
        {
#ifdef DEBUG
            // Submenus of a CONTEXT_MENU are supposed to be CONTEXT_MENUs as well
            assert( dynamic_cast<CONTEXT_MENU*>( item->GetSubMenu() ) );
#endif

            CONTEXT_MENU* menu = new CONTEXT_MENU( static_cast<const CONTEXT_MENU&>( *item->GetSubMenu() ) );
            AppendSubMenu( menu, item->GetItemLabel(), wxEmptyString );
        }
        else
        {
            wxMenuItem* newItem = new wxMenuItem( this, item->GetId(), item->GetItemLabel(),
                    wxEmptyString, item->GetKind() );

            Append( newItem );
            copyItem( item, newItem );
        }
    }

    setupEvents();
}


void CONTEXT_MENU::setupEvents()
{
    Connect( wxEVT_MENU_HIGHLIGHT, wxEventHandler( CONTEXT_MENU::onMenuEvent ),
                     NULL, this );
    Connect( wxEVT_COMMAND_MENU_SELECTED, wxEventHandler( CONTEXT_MENU::onMenuEvent ),
                     NULL, this );

    // Workaround for the case when mouse cursor never reaches menu (it hangs up tools using menu)
    wxMenuEvent menuEvent( wxEVT_MENU_HIGHLIGHT, -1, this );
    AddPendingEvent( menuEvent );
}


void CONTEXT_MENU::SetTitle( const wxString& aTitle )
{
    // TODO handle an empty string (remove title and separator)

    // Unfortunately wxMenu::SetTitle() does nothing.. (at least wxGTK)

    if( m_titleSet )
    {
        FindItemByPosition( 0 )->SetItemLabel( aTitle );
    }
    else
    {
        InsertSeparator( 0 );
        Insert( 0, new wxMenuItem( this, -1, aTitle, wxEmptyString, wxITEM_NORMAL ) );
        m_titleSet = true;
    }
}


void CONTEXT_MENU::Add( const wxString& aLabel, int aId )
{
#ifdef DEBUG

    if( FindItem( aId ) != NULL )
        wxLogWarning( wxT( "Adding more than one menu entry with the same ID may result in"
                "undefined behaviour" ) );
#endif
    Append( new wxMenuItem( this, aId, aLabel, wxEmptyString, wxITEM_NORMAL ) );
}


void CONTEXT_MENU::Add( const TOOL_ACTION& aAction )
{
    /// ID numbers for tool actions need to have a value higher than m_actionId
    int id = m_actionId + aAction.GetId();

    wxMenuItem* item = new wxMenuItem( this, id,
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

        wxAcceleratorEntry accel( flags, key, id, item );
        item->SetAccel( &accel );
    }

    Append( item );
    m_toolActions[id] = &aAction;
}


void CONTEXT_MENU::Clear()
{
    m_titleSet = false;

    // Remove all the entries from context menu
    for( unsigned i = 0; i < GetMenuItemCount(); ++i )
        Destroy( FindItemByPosition( 0 ) );

    m_toolActions.clear();
}


void CONTEXT_MENU::onMenuEvent( wxEvent& aEvent )
{
    OPT_TOOL_EVENT evt;

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
        m_selected = aEvent.GetId();

        // Check if there is a TOOL_ACTION for the given ID
        if( m_toolActions.count( aEvent.GetId() ) == 1 )
        {
            evt = m_toolActions[aEvent.GetId()]->MakeEvent();
        }
        else
        {
            evt = m_customHandler( aEvent );

            // Handling non-action menu entries (e.g. items in clarification list)
            if( !evt )
                evt = TOOL_EVENT( TC_COMMAND, TA_CONTEXT_MENU_CHOICE, aEvent.GetId() );
        }
    }

    // forward the action/update event to the TOOL_MANAGER
    TOOL_MANAGER::Instance().ProcessEvent( *evt );
}


void CONTEXT_MENU::copyItem( const wxMenuItem* aSource, wxMenuItem* aDest ) const
{
    assert( !aSource->IsSubMenu() );    // it does not transfer submenus

    aDest->SetKind( aSource->GetKind() );
    aDest->SetHelp( aSource->GetHelp() );
    aDest->Enable( aSource->IsEnabled() );

    if( aSource->IsCheckable() )
        aDest->Check( aSource->IsChecked() );

    if( aSource->GetKind() == wxITEM_NORMAL )
        aDest->SetBitmap( aSource->GetBitmap() );
}
