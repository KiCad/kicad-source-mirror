/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
    m_titleSet( false ), m_selected( -1 ), m_tool( NULL ), m_parent( NULL ), m_icon( NULL ),
    m_menu_handler( CONTEXT_MENU::menuHandlerStub ),
    m_update_handler( CONTEXT_MENU::updateHandlerStub )
{
    setupEvents();
}


CONTEXT_MENU::CONTEXT_MENU( const CONTEXT_MENU& aMenu )
{
    copyFrom( aMenu );
    setupEvents();
}


CONTEXT_MENU::~CONTEXT_MENU()
{
    // Set parent to NULL to prevent submenus from unregistering from a notexisting object
    for( std::list<CONTEXT_MENU*>::iterator it = m_submenus.begin(); it != m_submenus.end(); ++it )
        (*it)->m_parent = NULL;

    if( m_parent )
        m_parent->m_submenus.remove( this );
}


CONTEXT_MENU& CONTEXT_MENU::operator=( const CONTEXT_MENU& aMenu )
{
    Clear();

    copyFrom( aMenu );

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
    const BITMAP_OPAQUE* icon = aAction.GetIcon();

    wxMenuItem* item = new wxMenuItem( this, getMenuId( aAction ), aAction.GetMenuItem(),
                                       aAction.GetDescription(), wxITEM_NORMAL );

    if( icon )
        item->SetBitmap( KiBitmap( icon ) );

    m_toolActions[getMenuId( aAction )] = &aAction;

    return Append( item );
}


std::list<wxMenuItem*> CONTEXT_MENU::Add( CONTEXT_MENU* aMenu, const wxString& aLabel, bool aExpand )
{
    std::list<wxMenuItem*> items;

    if( aExpand )
    {
        for( int i = 0; i < (int) aMenu->GetMenuItemCount(); ++i )
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

    m_submenus.push_back( aMenu );
    aMenu->m_parent = this;

    return items;
}


void CONTEXT_MENU::Clear()
{
    m_titleSet = false;

    for( int i = GetMenuItemCount() - 1; i >= 0; --i )
        Destroy( FindItemByPosition( i ) );

    m_toolActions.clear();
    m_submenus.clear();
    m_parent = NULL;

    assert( GetMenuItemCount() == 0 );
}


void CONTEXT_MENU::UpdateAll()
{
    m_update_handler();
    updateHotKeys();

    runOnSubmenus( boost::bind( &CONTEXT_MENU::UpdateAll, _1 ) );
}


TOOL_MANAGER* CONTEXT_MENU::getToolManager()
{
    assert( m_tool );
    return m_tool->GetManager();
}


void CONTEXT_MENU::updateHotKeys()
{
    for( std::map<int, const TOOL_ACTION*>::const_iterator it = m_toolActions.begin();
            it != m_toolActions.end(); ++it )
    {
        int id = it->first;
        const TOOL_ACTION& action = *it->second;

        if( action.HasHotKey() )
        {
            int key = action.GetHotKey() & ~MD_MODIFIER_MASK;
            int mod = action.GetHotKey() & MD_MODIFIER_MASK;
            int flags = 0;
            wxMenuItem* item = FindChildItem( id );

            if( item )
            {
                flags |= ( mod & MD_ALT ) ? wxACCEL_ALT : 0;
                flags |= ( mod & MD_CTRL ) ? wxACCEL_CTRL : 0;
                flags |= ( mod & MD_SHIFT ) ? wxACCEL_SHIFT : 0;

                if( !flags )
                    flags = wxACCEL_NORMAL;

                wxAcceleratorEntry accel( flags, key, id, item );
                item->SetAccel( &accel );
            }
        }
    }
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
            runEventHandlers( aEvent, evt );

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

    runOnSubmenus( boost::bind( &CONTEXT_MENU::setTool, _1, aTool ) );
}


void CONTEXT_MENU::runEventHandlers( const wxMenuEvent& aMenuEvent, OPT_TOOL_EVENT& aToolEvent )
{
    aToolEvent = m_menu_handler( aMenuEvent );

    if( !aToolEvent )
        runOnSubmenus( boost::bind( &CONTEXT_MENU::runEventHandlers, _1, aMenuEvent, aToolEvent ) );
}


void CONTEXT_MENU::runOnSubmenus( boost::function<void(CONTEXT_MENU*)> aFunction )
{
    std::for_each( m_submenus.begin(), m_submenus.end(), aFunction );
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

        m_submenus.push_back( menu );
        menu->m_parent = this;
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
    m_parent = NULL; // aMenu.m_parent;
    m_menu_handler = aMenu.m_menu_handler;
    m_update_handler = aMenu.m_update_handler;

    // Copy all the menu entries
    for( int i = 0; i < (int) aMenu.GetMenuItemCount(); ++i )
    {
        wxMenuItem* item = aMenu.FindItemByPosition( i );
        appendCopy( item );
    }
}


OPT_TOOL_EVENT CONTEXT_MENU::menuHandlerStub( const wxMenuEvent& )
{
    return OPT_TOOL_EVENT();
}


void CONTEXT_MENU::updateHandlerStub()
{
}
