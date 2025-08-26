/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "tool/action_menu.h"

#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/textentry.h>
#include <wx/stc/stc.h>

#include <bitmaps.h>
#include <eda_base_frame.h>
#include <functional>
#include <id.h>
#include <kiface_base.h>
#include <tool/actions.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <trace_helpers.h>
#include <textentry_tricks.h>
#include <widgets/ui_common.h>

using namespace std::placeholders;


ACTION_MENU::ACTION_MENU( bool isContextMenu, TOOL_INTERACTIVE* aTool ) :
    m_isForcedPosition( false ),
    m_dirty( true ),
    m_titleDisplayed( false ),
    m_isContextMenu( isContextMenu ),
    m_icon( BITMAPS::INVALID_BITMAP ),
    m_selected( -1 ),
    m_tool( aTool )
{
    setupEvents();
}


ACTION_MENU::~ACTION_MENU()
{
    Disconnect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ACTION_MENU::OnMenuEvent ), nullptr, this );
    Disconnect( wxEVT_IDLE, wxIdleEventHandler( ACTION_MENU::OnIdle ), nullptr, this );

    // Explicitly release the GDI resources
    for( auto subitem : GetMenuItems() )
        subitem->SetBitmap( wxNullBitmap );

    // Set parent to NULL to prevent submenus from unregistering from a nonexistent object
    for( ACTION_MENU* menu : m_submenus )
    {
        for( auto menuItem : GetMenuItems() )
            menuItem->SetBitmap( wxNullBitmap );

        menu->SetParent( nullptr );
    }

    ACTION_MENU* parent = dynamic_cast<ACTION_MENU*>( GetParent() );

    if( parent )
        parent->m_submenus.remove( this );
}


void ACTION_MENU::SetIcon( BITMAPS aIcon )
{
    m_icon = aIcon;
}


void ACTION_MENU::setupEvents()
{
    Connect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ACTION_MENU::OnMenuEvent ), nullptr, this );
    Connect( wxEVT_IDLE, wxIdleEventHandler( ACTION_MENU::OnIdle ), nullptr, this );
}


void ACTION_MENU::SetTitle( const wxString& aTitle )
{
    // Unfortunately wxMenu::SetTitle() does not work very well, so this is an alternative version
    m_title = aTitle;

    // Update the menu title
    if( m_titleDisplayed )
        DisplayTitle( true );
}


void ACTION_MENU::DisplayTitle( bool aDisplay )
{
    if( ( !aDisplay || m_title.IsEmpty() ) && m_titleDisplayed )
    {
        // Destroy the menu entry keeping the title..
        wxMenuItem* item = FindItemByPosition( 0 );
        wxASSERT( item->GetItemLabelText() == GetTitle() );
        item->SetBitmap( wxNullBitmap );
        Destroy( item );

        // ..and separator
        item = FindItemByPosition( 0 );
        wxASSERT( item->IsSeparator() );
        Destroy( item );
        m_titleDisplayed = false;
    }

    else if( aDisplay && !m_title.IsEmpty() )
    {
        if( m_titleDisplayed )
        {
            // Simply update the title
            FindItemByPosition( 0 )->SetItemLabel( m_title );
        }
        else
        {
            // Add a separator and a menu entry to display the title
            InsertSeparator( 0 );
            Insert( 0, new wxMenuItem( this, wxID_NONE, m_title, wxEmptyString, wxITEM_NORMAL ) );

            if( !!m_icon )
                KIUI::AddBitmapToMenuItem( FindItemByPosition( 0 ), KiBitmapBundle( m_icon ) );

            m_titleDisplayed = true;
        }
    }
}


wxMenuItem* ACTION_MENU::Add( const wxString& aLabel, int aId, BITMAPS aIcon )
{
    wxASSERT_MSG( FindItem( aId ) == nullptr, wxS( "Duplicate menu IDs!" ) );

    wxMenuItem* item = new wxMenuItem( this, aId, aLabel, wxEmptyString, wxITEM_NORMAL );

    if( !!aIcon )
        KIUI::AddBitmapToMenuItem( item, KiBitmapBundle( aIcon ) );

    return Append( item );
}


wxMenuItem* ACTION_MENU::Add( const wxString& aLabel, const wxString& aTooltip, int aId,
                              BITMAPS aIcon, bool aIsCheckmarkEntry )
{
    wxASSERT_MSG( FindItem( aId ) == nullptr, wxS( "Duplicate menu IDs!" ) );

    wxMenuItem* item = new wxMenuItem( this, aId, aLabel, aTooltip, aIsCheckmarkEntry ? wxITEM_CHECK
                                                                                      : wxITEM_NORMAL );

    if( !!aIcon )
        KIUI::AddBitmapToMenuItem( item, KiBitmapBundle( aIcon ) );

    return Append( item );
}


wxMenuItem* ACTION_MENU::Add( const TOOL_ACTION& aAction, bool aIsCheckmarkEntry,
                              const wxString& aOverrideLabel )
{
    // ID numbers for tool actions are assigned above ACTION_BASE_UI_ID inside TOOL_EVENT
    BITMAPS icon = aAction.GetIcon();

    // Allow the label to be overridden at point of use
    wxString menuLabel = aOverrideLabel.IsEmpty() ? aAction.GetMenuItem() : aOverrideLabel;

    wxMenuItem* item = new wxMenuItem( this, aAction.GetUIId(), menuLabel, aAction.GetTooltip(),
                                       aIsCheckmarkEntry ? wxITEM_CHECK : wxITEM_NORMAL );
    if( !!icon )
        KIUI::AddBitmapToMenuItem( item, KiBitmapBundle( icon ) );

    m_toolActions[aAction.GetUIId()] = &aAction;

    return Append( item );
}


wxMenuItem* ACTION_MENU::Add( ACTION_MENU* aMenu )
{
    m_submenus.push_back( aMenu );

    wxASSERT_MSG( !aMenu->m_title.IsEmpty(), wxS( "Set a title for ACTION_MENU using SetTitle()" ) );

    if( !!aMenu->m_icon )
    {
        wxMenuItem* newItem = new wxMenuItem( this, -1, aMenu->m_title );
        KIUI::AddBitmapToMenuItem( newItem, KiBitmapBundle( aMenu->m_icon ) );
        newItem->SetSubMenu( aMenu );
        return Append( newItem );
    }
    else
    {
        return AppendSubMenu( aMenu, aMenu->m_title );
    }
}


void ACTION_MENU::AddClose( const wxString& aAppname )
{
#ifdef __WINDOWS__
    Add( _( "Close" ),
         wxString::Format( _( "Close %s" ), aAppname ),
         wxID_CLOSE,
         BITMAPS::exit );
#else
    Add( _( "Close" ) + wxS( "\tCtrl+W" ),
         wxString::Format( _( "Close %s" ), aAppname ),
         wxID_CLOSE,
         BITMAPS::exit );
#endif
}


void ACTION_MENU::AddQuitOrClose( KIFACE_BASE* aKiface, wxString aAppname )
{
    if( !aKiface || aKiface->IsSingle() ) // not when under a project mgr
    {
        // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via
        // wxID_EXIT
        Add( _( "Quit" ) + wxS( "\tCtrl+Q" ),
             wxString::Format( _( "Quit %s" ), aAppname ),
             wxID_EXIT,
             BITMAPS::exit );
    }
    else
    {
        AddClose( aAppname );
    }
}


void ACTION_MENU::AddQuit( const wxString& aAppname )
{
    // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via
    // wxID_EXIT
    Add( _( "Quit" ) + wxS( "\tCtrl+Q" ),
         wxString::Format( _( "Quit %s" ), aAppname ),
         wxID_EXIT,
         BITMAPS::exit );
}


void ACTION_MENU::Clear()
{
    m_titleDisplayed = false;

    for( int i = GetMenuItemCount() - 1; i >= 0; --i )
        Destroy( FindItemByPosition( i ) );

    m_toolActions.clear();
    m_submenus.clear();

    wxASSERT( GetMenuItemCount() == 0 );
}


bool ACTION_MENU::HasEnabledItems() const
{
    for( wxMenuItem* item : GetMenuItems() )
    {
        if( item->IsEnabled() && !item->IsSeparator() )
            return true;
    }

    return false;
}


void ACTION_MENU::UpdateAll()
{
    try
    {
        update();
    }
    catch( std::exception& )
    {
    }

    if( m_tool )
        updateHotKeys();

    runOnSubmenus( std::bind( &ACTION_MENU::UpdateAll, _1 ) );
}


void ACTION_MENU::ClearDirty()
{
    m_dirty = false;
    runOnSubmenus( std::bind( &ACTION_MENU::ClearDirty, _1 ) );
}


void ACTION_MENU::SetDirty()
{
    m_dirty = true;
    runOnSubmenus( std::bind( &ACTION_MENU::SetDirty, _1 ) );
}


void ACTION_MENU::SetTool( TOOL_INTERACTIVE* aTool )
{
    m_tool = aTool;
    runOnSubmenus( std::bind( &ACTION_MENU::SetTool, _1, aTool ) );
}


ACTION_MENU* ACTION_MENU::Clone() const
{
    ACTION_MENU* clone = create();
    clone->Clear();
    clone->copyFrom( *this );
    return clone;
}


ACTION_MENU* ACTION_MENU::create() const
{
    ACTION_MENU* menu = new ACTION_MENU( false );

    wxASSERT_MSG( typeid( *this ) == typeid( *menu ),
                  wxString::Format( "You need to override create() method for class %s", typeid( *this ).name() ) );

    return menu;
}


TOOL_MANAGER* ACTION_MENU::getToolManager() const
{
    return m_tool ? m_tool->GetManager() : nullptr;
}


void ACTION_MENU::updateHotKeys()
{
    TOOL_MANAGER* toolMgr = getToolManager();

    wxASSERT( toolMgr );

    for( std::pair<const int, const TOOL_ACTION*>& ii : m_toolActions )
    {
        int                id = ii.first;
        const TOOL_ACTION& action = *ii.second;
        int                key = toolMgr->GetHotKey( action ) & ~MD_MODIFIER_MASK;

        if( key > 0 )
        {
            int mod = toolMgr->GetHotKey( action ) & MD_MODIFIER_MASK;
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


// wxWidgets doesn't tell us when a menu command was generated from a hotkey or from
// a menu selection.  It's important to us because a hotkey can be an immediate action
// while the menu selection can not (as it has no associated position).
//
// We get around this by storing the last highlighted menuId.  If it matches the command
// id then we know this is a menu selection.  (You might think we could use the menuOpen
// menuClose events, but these are actually generated for hotkeys as well.)

static int g_last_menu_highlighted_id = 0;


// We need to store the position of the mouse when the menu was opened so it can be passed
// to the command event generated when the menu item is selected.
static VECTOR2D g_menu_open_position;


void ACTION_MENU::OnIdle( wxIdleEvent& event )
{
    g_last_menu_highlighted_id = 0;
    g_menu_open_position.x = 0.0;
    g_menu_open_position.y = 0.0;
}


void ACTION_MENU::OnMenuEvent( wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT evt;
    wxString       menuText;
    wxEventType    type    = aEvent.GetEventType();
    wxWindow*      focus   = wxWindow::FindFocus();
    TOOL_MANAGER*  toolMgr = getToolManager();

    if( type == wxEVT_MENU_OPEN )
    {
        if( m_dirty && toolMgr )
            toolMgr->RunAction<ACTION_MENU*>( ACTIONS::updateMenu, this );

        wxMenu* parent = dynamic_cast<wxMenu*>( GetParent() );

        // Don't update the position if this menu has a parent or is a menubar menu
        if( !parent && !IsAttached() && toolMgr )
            g_menu_open_position = toolMgr->GetMousePosition();

        g_last_menu_highlighted_id = 0;
    }
    else if( type == wxEVT_MENU_HIGHLIGHT )
    {
        if( aEvent.GetId() > 0 )
            g_last_menu_highlighted_id = aEvent.GetId();

        evt = TOOL_EVENT( TC_COMMAND, TA_CHOICE_MENU_UPDATE, aEvent.GetId() );
    }
    else if( type == wxEVT_COMMAND_MENU_SELECTED )
    {
        // Despite our attempts to catch the theft of text editor CHAR_HOOK and CHAR events
        // in TOOL_DISPATCHER::DispatchWxEvent, wxWidgets sometimes converts those it knows
        // about into menu commands without ever generating the appropriate CHAR_HOOK and CHAR
        // events first.
        if( dynamic_cast<wxTextEntry*>( focus )
                || dynamic_cast<wxStyledTextCtrl*>( focus )
                || dynamic_cast<wxListView*>( focus )
                || dynamic_cast<wxGrid*>( focus ) )
        {
            // Original key event has been lost, so we have to re-create it from the menu's
            // wxAcceleratorEntry.
            wxMenuItem* menuItem = FindItem( aEvent.GetId() );
            wxAcceleratorEntry* acceleratorKey = menuItem ? menuItem->GetAccel() : nullptr;

            if( acceleratorKey )
            {
                wxKeyEvent keyEvent( wxEVT_CHAR_HOOK );
                keyEvent.m_keyCode = acceleratorKey->GetKeyCode();
                keyEvent.m_controlDown = ( acceleratorKey->GetFlags() & wxMOD_CONTROL ) > 0;
                keyEvent.m_shiftDown = ( acceleratorKey->GetFlags() & wxMOD_SHIFT ) > 0;
                keyEvent.m_altDown = ( acceleratorKey->GetFlags() & wxMOD_ALT ) > 0;

                if( wxTextEntry* ctrl = dynamic_cast<wxTextEntry*>( focus ) )
                    TEXTENTRY_TRICKS::OnCharHook( ctrl, keyEvent );
                else
                    focus->HandleWindowEvent( keyEvent );

                if( keyEvent.GetSkipped() )
                {
                    keyEvent.SetEventType( wxEVT_CHAR );
                    focus->HandleWindowEvent( keyEvent );
                }

                // Don't bubble-up dangerous actions; the target may be behind a modeless dialog.
                // Cf. https://gitlab.com/kicad/code/kicad/-/issues/17229
                if( keyEvent.GetKeyCode() == WXK_BACK || keyEvent.GetKeyCode() == WXK_DELETE )
                    return;

                // If the event was used as a KEY event (not skipped) by the focused window,
                // just finish.  Otherwise this is actually a wxEVT_COMMAND_MENU_SELECTED (or the
                // focused window is read only).
                if( !keyEvent.GetSkipped() )
                    return;
            }
        }

        // Store the selected position, so it can be checked by the tools
        m_selected = aEvent.GetId();

        ACTION_MENU* parent = dynamic_cast<ACTION_MENU*>( GetParent() );

        while( parent )
        {
            parent->m_selected = m_selected;
            parent = dynamic_cast<ACTION_MENU*>( parent->GetParent() );
        }

        // Check if there is a TOOL_ACTION for the given UI ID
        if( toolMgr && toolMgr->GetActionManager()->IsActionUIId( m_selected ) )
            evt = findToolAction( m_selected );

        if( !evt )
        {
#ifdef __WINDOWS__
            if( !evt )
            {
                // Try to find the submenu which holds the selected item
                wxMenu* menu = nullptr;
                FindItem( m_selected, &menu );

                // This conditional compilation is probably not needed.
                if( menu )
                {
                    ACTION_MENU* cxmenu = static_cast<ACTION_MENU*>( menu );
                    evt = cxmenu->eventHandler( aEvent );
                }
            }
#else
            if( !evt )
                runEventHandlers( aEvent, evt );
#endif

            // Handling non-ACTION menu entries.  Two ranges of ids are supported:
            //   between 0 and ID_CONTEXT_MENU_ID_MAX
            //   between ID_POPUP_MENU_START and ID_POPUP_MENU_END

            #define ID_CONTEXT_MENU_ID_MAX wxID_LOWEST  /* = 100 should be plenty */

            if( !evt && (   ( m_selected >= 0 && m_selected < ID_CONTEXT_MENU_ID_MAX )
                         || ( m_selected >= ID_POPUP_MENU_START && m_selected <= ID_POPUP_MENU_END ) ) )
            {
                ACTION_MENU* actionMenu = dynamic_cast<ACTION_MENU*>( GetParent() );

                if( actionMenu && actionMenu->PassHelpTextToHandler() )
                    menuText = GetHelpString( aEvent.GetId() );
                else
                    menuText = GetLabelText( aEvent.GetId() );

                evt = TOOL_EVENT( TC_COMMAND, TA_CHOICE_MENU_CHOICE, m_selected, AS_GLOBAL );
                evt->SetParameter( &menuText );
            }
        }
    }

    // forward the action/update event to the TOOL_MANAGER
    // clients that don't supply a tool will have to check GetSelected() themselves
    if( evt && toolMgr )
    {
        wxLogTrace( kicadTraceToolStack, wxS( "ACTION_MENU::OnMenuEvent %s" ), evt->Format() );

        // WARNING: if you're squeamish, look away.
        // What follows is a series of egregious hacks necessitated by a lack of information from
        // wxWidgets on where context-menu-commands and command-key-events originated.

        // If it's a context menu then fetch the mouse position from our context-menu-position
        // hack.
        if( m_isContextMenu )
        {
            evt->SetMousePosition( g_menu_open_position );
        }
        // Check if it is a menubar event, and don't get any position if it is.  Note that we
        // can't  use IsAttached() here, as it only differentiates a context menu, and we also
        // want a hotkey to generate a position.
        else if( g_last_menu_highlighted_id == aEvent.GetId() )
        {
            evt->SetHasPosition( false );
        }
        // Otherwise it's a command-key-event and we need to get the mouse position from the tool
        // manager so that immediate actions work.
        else
        {
            evt->SetMousePosition( toolMgr->GetMousePosition() );
        }

        toolMgr->ProcessEvent( *evt );
    }
    else
    {
        aEvent.Skip();
    }
}


void ACTION_MENU::runEventHandlers( const wxMenuEvent& aMenuEvent, OPT_TOOL_EVENT& aToolEvent )
{
    aToolEvent = eventHandler( aMenuEvent );

    if( !aToolEvent )
        runOnSubmenus( std::bind( &ACTION_MENU::runEventHandlers, _1, aMenuEvent, aToolEvent ) );
}


void ACTION_MENU::runOnSubmenus( std::function<void(ACTION_MENU*)> aFunction )
{
    try
    {
        std::for_each( m_submenus.begin(), m_submenus.end(),
                       [&]( ACTION_MENU* m )
                       {
                           aFunction( m );
                           m->runOnSubmenus( aFunction );
                       } );
    }
    catch( std::exception& )
    {
    }
}


OPT_TOOL_EVENT ACTION_MENU::findToolAction( int aId )
{
    OPT_TOOL_EVENT evt;

    auto findFunc =
            [&]( ACTION_MENU* m )
            {
                if( evt )
                    return;

                const auto it = m->m_toolActions.find( aId );

                if( it != m->m_toolActions.end() )
                    evt = it->second->MakeEvent();
            };

    findFunc( this );

    if( !evt )
        runOnSubmenus( findFunc );

    return evt;
}


void ACTION_MENU::copyFrom( const ACTION_MENU& aMenu )
{
    m_icon = aMenu.m_icon;
    m_title = aMenu.m_title;
    m_titleDisplayed = aMenu.m_titleDisplayed;
    m_selected = -1; // aMenu.m_selected;
    m_tool = aMenu.m_tool;
    m_toolActions = aMenu.m_toolActions;

    // Copy all menu entries
    for( int i = 0; i < (int) aMenu.GetMenuItemCount(); ++i )
    {
        wxMenuItem* item = aMenu.FindItemByPosition( i );
        appendCopy( item );
    }
}


wxMenuItem* ACTION_MENU::appendCopy( const wxMenuItem* aSource )
{
    wxMenuItem* newItem = new wxMenuItem( this, aSource->GetId(), aSource->GetItemLabel(),
                                          aSource->GetHelp(), aSource->GetKind() );

    // Add the source bitmap if it is not the wxNullBitmap
    // On Windows, for Checkable Menu items, adding a bitmap adds also
    // our predefined checked alternate bitmap
    // On other OS, wxITEM_CHECK and wxITEM_RADIO Menu items do not use custom bitmaps.

#if defined( __WXMSW__ )
    // On Windows, AddBitmapToMenuItem() uses the unchecked bitmap for wxITEM_CHECK and
    // wxITEM_RADIO menuitems and automatically adds a checked bitmap.
    // For other menuitrms, use the "checked" bitmap.
    bool use_checked_bm = ( aSource->GetKind() == wxITEM_CHECK ||
                            aSource->GetKind() == wxITEM_RADIO ) ? false : true;
    const wxBitmap& src_bitmap = aSource->GetBitmap( use_checked_bm );
#else
    const wxBitmap& src_bitmap = aSource->GetBitmap();
#endif

    if( src_bitmap.IsOk() && src_bitmap.GetHeight() > 1 )    // a null bitmap has a 0 size
        KIUI::AddBitmapToMenuItem( newItem, src_bitmap );

    if( aSource->IsSubMenu() )
    {
        ACTION_MENU* menu = dynamic_cast<ACTION_MENU*>( aSource->GetSubMenu() );
        wxASSERT_MSG( menu, wxS( "Submenus are expected to be a ACTION_MENU" ) );

        if( menu )
        {
            ACTION_MENU* menuCopy = menu->Clone();
            newItem->SetSubMenu( menuCopy );
            m_submenus.push_back( menuCopy );
        }
    }

    // wxMenuItem has to be added before enabling/disabling or checking
    Append( newItem );

    if( aSource->IsCheckable() )
        newItem->Check( aSource->IsChecked() );

    newItem->Enable( aSource->IsEnabled() );

    return newItem;
}
