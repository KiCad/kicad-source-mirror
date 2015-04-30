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
    m_titleSet( false ), m_selected( -1 ), m_tool( NULL ), m_icon( NULL )
{
    setupEvents();
}


CONTEXT_MENU::CONTEXT_MENU( const CONTEXT_MENU& aMenu )
{
    copyFrom( aMenu );
    setupEvents();
}


CONTEXT_MENU& CONTEXT_MENU::operator=( const CONTEXT_MENU& aMenu )
{
    Clear();

    copyFrom( aMenu );
    setupEvents();

    return *this;
}


void CONTEXT_MENU::setupEvents()
{
    Connect( wxEVT_MENU_HIGHLIGHT, wxMenuEventHandler( CONTEXT_MENU::onMenuEvent ), NULL, this );
    Connect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( CONTEXT_MENU::onMenuEvent ), NULL, this );
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
        Insert( 0, new wxMenuItem( this, wxID_NONE, aTitle, wxEmptyString, wxITEM_NORMAL ) );
        m_titleSet = true;
    }
}


wxMenuItem* CONTEXT_MENU::Add( const wxString& aLabel, int aId, const BITMAP_OPAQUE* aIcon )
{
#ifdef DEBUG

    if( FindItem( aId ) != NULL )
        wxLogWarning( wxT( "Adding more than one menu entry with the same ID may result in"
                "undefined behaviour" ) );
#endif
    wxMenuItem* item = new wxMenuItem( this, aId, aLabel, wxEmptyString, wxITEM_NORMAL );

    if( aIcon )
        item->SetBitmap( KiBitmap( aIcon ) );

    return Append( item );
}


wxMenuItem* CONTEXT_MENU::Add( const TOOL_ACTION& aAction )
{
    /// ID numbers for tool actions need to have a value higher than ACTION_ID
    int id = ACTION_ID + aAction.GetId();
    const BITMAP_OPAQUE* icon = aAction.GetIcon();

    wxMenuItem* item = new wxMenuItem( this, id, aAction.GetMenuItem(),
                                       aAction.GetDescription(), wxITEM_NORMAL );

    if( icon )
        item->SetBitmap( KiBitmap( icon ) );

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

    m_toolActions[id] = &aAction;

    return Append( item );
}


std::list<wxMenuItem*> CONTEXT_MENU::Add( CONTEXT_MENU* aMenu, const wxString& aLabel, bool aExpand )
{
    std::list<wxMenuItem*> items;

    if( aExpand )
    {
        unsigned int i = 0;

        for( i = 0; i < aMenu->GetMenuItemCount(); ++i )
        {
            wxMenuItem* item = aMenu->FindItemByPosition( i );
            items.push_back( appendCopy( item ) );
        }
    }
    else
    {
        if( aMenu->m_icon )
        {
            wxMenuItem* newItem = new wxMenuItem( this, -1, aLabel, wxEmptyString, wxITEM_NORMAL );
            newItem->SetBitmap( KiBitmap( aMenu->m_icon ) );
            newItem->SetSubMenu( aMenu );
            items.push_back( Append( newItem ) );
        }
        else
        {
            items.push_back( AppendSubMenu( aMenu, aLabel ) );
        }
    }

    m_toolActions.insert( aMenu->m_toolActions.begin(), aMenu->m_toolActions.end() );
    m_handlers.insert( m_handlers.end(), aMenu->m_handlers.begin(), aMenu->m_handlers.end() );

    return items;
}


void CONTEXT_MENU::Clear()
{
    m_titleSet = false;

    GetMenuItems().DeleteContents( true );
    GetMenuItems().Clear();
    m_toolActions.clear();
    m_handlers.clear();
    GetMenuItems().DeleteContents( false ); // restore the default so destructor does not go wild

    assert( GetMenuItemCount() == 0 );
}


void CONTEXT_MENU::onMenuEvent( wxMenuEvent& aEvent )
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
        // Under Linux, every submenu can have a separate event handler, under
        // Windows all submenus are handled by the main menu.
#ifdef __WINDOWS__
            if( !evt )
            {
                // Try to find the submenu which holds the selected item
                wxMenu* menu = NULL;
                FindItem( m_selected, &menu );

                if( menu && menu != this )
                {
                    menu->ProcessEvent( aEvent );
                    return;
                }
            }
#endif
            for( std::list<CUSTOM_MENU_HANDLER>::iterator it = m_handlers.begin();
                    it != m_handlers.end(); ++it )
            {
                evt = (*it)( aEvent );

                if( evt )
                    break;
            }

            // Handling non-action menu entries (e.g. items in clarification list)
            if( !evt )
                evt = TOOL_EVENT( TC_COMMAND, TA_CONTEXT_MENU_CHOICE, aEvent.GetId() );
        }
    }

    assert( m_tool );   // without tool & tool manager we cannot handle events

    // forward the action/update event to the TOOL_MANAGER
    if( evt && m_tool )
        m_tool->GetManager()->ProcessEvent( *evt );
}


void CONTEXT_MENU::setTool( TOOL_INTERACTIVE* aTool )
{
    m_tool = aTool;

    for( unsigned i = 0; i < GetMenuItemCount(); ++i )
    {
        wxMenuItem* item = FindItemByPosition( i );

        if( item->IsSubMenu() )
        {
            CONTEXT_MENU* menu = static_cast<CONTEXT_MENU*>( item->GetSubMenu() );
            menu->setTool( aTool );
        }
    }
}


wxMenuItem* CONTEXT_MENU::appendCopy( const wxMenuItem* aSource )
{
    wxMenuItem* newItem = new wxMenuItem( this, aSource->GetId(), aSource->GetItemLabel(),
                                          aSource->GetHelp(), aSource->GetKind() );

    if( aSource->GetKind() == wxITEM_NORMAL )
        newItem->SetBitmap( aSource->GetBitmap() );

    if( aSource->IsSubMenu() )
    {
#ifdef DEBUG
        // Submenus of a CONTEXT_MENU are supposed to be CONTEXT_MENUs as well
        assert( dynamic_cast<CONTEXT_MENU*>( aSource->GetSubMenu() ) );
#endif

        CONTEXT_MENU* menu = new CONTEXT_MENU( static_cast<const CONTEXT_MENU&>( *aSource->GetSubMenu() ) );
        newItem->SetSubMenu( menu );
        Append( newItem );

        m_toolActions.insert( menu->m_toolActions.begin(), menu->m_toolActions.end() );
        m_handlers.insert( m_handlers.end(), menu->m_handlers.begin(), menu->m_handlers.end() );
    }
    else
    {
        Append( newItem );
        newItem->SetKind( aSource->GetKind() );
        newItem->SetHelp( aSource->GetHelp() );
        newItem->Enable( aSource->IsEnabled() );

        if( aSource->IsCheckable() )
            newItem->Check( aSource->IsChecked() );
    }

    return newItem;
}


void CONTEXT_MENU::copyFrom( const CONTEXT_MENU& aMenu )
{
    m_icon = aMenu.m_icon;
    m_titleSet = aMenu.m_titleSet;
    m_selected = -1; // aMenu.m_selected;
    m_tool = aMenu.m_tool;
    m_toolActions = aMenu.m_toolActions;
    m_handlers = aMenu.m_handlers;

    // Copy all the menu entries
    for( unsigned i = 0; i < aMenu.GetMenuItemCount(); ++i )
    {
        wxMenuItem* item = aMenu.FindItemByPosition( i );
        appendCopy( item );
    }
}
