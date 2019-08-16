/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
 * Copyright (C) 2013-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_base_frame.h>
#include <functional>
#include <id.h>
#include <menus_helpers.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <trace_helpers.h>
#include <wx/log.h>


using namespace std::placeholders;


ACTION_MENU::ACTION_MENU( bool isContextMenu ) :
    m_dirty( true ),
    m_titleDisplayed( false ),
    m_isContextMenu( isContextMenu ),
    m_icon( nullptr ),
    m_selected( -1 ),
    m_tool( nullptr )
{
    setupEvents();
}


ACTION_MENU::~ACTION_MENU()
{
    // Set parent to NULL to prevent submenus from unregistering from a notexisting object
    for( auto menu : m_submenus )
        menu->SetParent( nullptr );

    ACTION_MENU* parent = dynamic_cast<ACTION_MENU*>( GetParent() );

    if( parent )
        parent->m_submenus.remove( this );
}


void ACTION_MENU::SetIcon( const BITMAP_OPAQUE* aIcon )
{
    m_icon = aIcon;
}


void ACTION_MENU::setupEvents()
{
// See wxWidgets hack in EDA_BASE_FRAME::OnMenuOpen().
//    Connect( wxEVT_MENU_OPEN, wxMenuEventHandler( ACTION_MENU::OnMenuEvent ), NULL, this );
//    Connect( wxEVT_MENU_HIGHLIGHT, wxMenuEventHandler( ACTION_MENU::OnMenuEvent ), NULL, this );
//    Connect( wxEVT_MENU_CLOSE, wxMenuEventHandler( ACTION_MENU::OnMenuEvent ), NULL, this );

    Connect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ACTION_MENU::OnMenuEvent ), NULL, this );
    Connect( wxEVT_IDLE, wxIdleEventHandler( ACTION_MENU::OnIdle ), NULL, this );
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

            if( m_icon )
                AddBitmapToMenuItem( FindItemByPosition( 0 ), KiBitmap( m_icon ) );

            m_titleDisplayed = true;
        }
    }
}


wxMenuItem* ACTION_MENU::Add( const wxString& aLabel, int aId, const BITMAP_OPAQUE* aIcon )
{
    wxASSERT_MSG( FindItem( aId ) == nullptr, "Duplicate menu IDs!" );

    wxMenuItem* item = new wxMenuItem( this, aId, aLabel, wxEmptyString, wxITEM_NORMAL );

    if( aIcon )
        AddBitmapToMenuItem( item, KiBitmap( aIcon ) );

    return Append( item );
}


wxMenuItem* ACTION_MENU::Add( const wxString& aLabel, const wxString& aTooltip, int aId,
                              const BITMAP_OPAQUE* aIcon )
{
    wxASSERT_MSG( FindItem( aId ) == nullptr, "Duplicate menu IDs!" );

    wxMenuItem* item = new wxMenuItem( this, aId, aLabel, aTooltip, wxITEM_NORMAL );

    if( aIcon )
        AddBitmapToMenuItem( item, KiBitmap( aIcon ) );

    return Append( item );
}


wxMenuItem* ACTION_MENU::Add( const TOOL_ACTION& aAction, bool aIsCheckmarkEntry )
{
    /// ID numbers for tool actions need to have a value higher than ACTION_ID
    const BITMAP_OPAQUE* icon = aAction.GetIcon();

    wxMenuItem* item = new wxMenuItem( this, getMenuId( aAction ), aAction.GetMenuItem(),
                                       aAction.GetDescription(),
                                       aIsCheckmarkEntry ? wxITEM_CHECK : wxITEM_NORMAL );

    if( icon )
        AddBitmapToMenuItem( item, KiBitmap( icon ) );

    m_toolActions[getMenuId( aAction )] = &aAction;

    return Append( item );
}


wxMenuItem* ACTION_MENU::Add( ACTION_MENU* aMenu )
{
    ACTION_MENU* menuCopy = aMenu->Clone();
    m_submenus.push_back( menuCopy );

    wxASSERT_MSG( !menuCopy->m_title.IsEmpty(), "Set a title for ACTION_MENU using SetTitle()" );

    if( aMenu->m_icon )
    {
        wxMenuItem* newItem = new wxMenuItem( this, -1, menuCopy->m_title );
        AddBitmapToMenuItem( newItem, KiBitmap( aMenu->m_icon ) );
        newItem->SetSubMenu( menuCopy );
        return Append( newItem );
    }
    else
    {
        return AppendSubMenu( menuCopy, menuCopy->m_title );
    }
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
    bool hasEnabled = false;

    auto& items = GetMenuItems();

    for( auto item : items )
    {
        if( item->IsEnabled() && !item->IsSeparator() )
        {
            hasEnabled = true;
            break;
        }
    }

    return hasEnabled;
}


void ACTION_MENU::UpdateAll()
{
    try
    {
        update();
    }
    catch( std::exception& e )
    {
        wxLogDebug( wxString::Format( "ACTION_MENU update handler exception: %s", e.what() ) );
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
            wxString::Format( "You need to override create() method for class %s", typeid(*this).name() ) );

    return menu;
}


TOOL_MANAGER* ACTION_MENU::getToolManager() const
{
    wxASSERT( m_tool );
    return m_tool ? m_tool->GetManager() : nullptr;
}


void ACTION_MENU::updateHotKeys()
{
    TOOL_MANAGER* toolMgr = getToolManager();

    for( auto& ii : m_toolActions )
    {
        int                id = ii.first;
        const TOOL_ACTION& action = *ii.second;
        int                key = toolMgr->GetHotKey( action ) & ~MD_MODIFIER_MASK;

        if( key )
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


void ACTION_MENU::OnIdle( wxIdleEvent& event )
{
    g_last_menu_highlighted_id = 0;
}


void ACTION_MENU::OnMenuEvent( wxMenuEvent& aEvent )
{
    OPT_TOOL_EVENT evt;
    wxString       menuText;
    wxEventType    type = aEvent.GetEventType();

    if( type == wxEVT_MENU_OPEN )
    {
        if( m_dirty && m_tool )
            getToolManager()->RunAction( ACTIONS::updateMenu, true, this );

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
        // Store the selected position, so it can be checked by the tools
        m_selected = aEvent.GetId();

        ACTION_MENU* parent = dynamic_cast<ACTION_MENU*>( GetParent() );

        while( parent )
        {
            parent->m_selected = m_selected;
            parent = dynamic_cast<ACTION_MENU*>( parent->GetParent() );
        }

        // Check if there is a TOOL_ACTION for the given ID
        if( m_selected >= ACTION_ID )
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
    // It will be removed later, for the Kicad V 6.x version.
    // But in "old" 3.0 version, the "&& menu != this" contition was added to avoid hang
    // This hang is no longer encountered in wxWidgets 3.0.4 version, and this condition is no longer needed.
    // And in 3.1.2, we have to remove it, as "menu != this" never happens
    // ("menu != this" always happens in 3.1.1 and older!).
    #if wxCHECK_VERSION(3, 1, 2)
                if( menu )
    #else
                if( menu && menu != this )
    #endif
                {
                    ACTION_MENU* cxmenu = static_cast<ACTION_MENU*>( menu );
                    evt = cxmenu->eventHandler( aEvent );
                }
            }
#else
            if( !evt )
                runEventHandlers( aEvent, evt );
#endif

            // Handling non-action menu entries (e.g. items in clarification list)
            // in some context menus, that have IDs explicitly chosen between
            // ID_POPUP_MENU_START and ID_POPUP_MENU_END
            // Note also items in clarification list have an id >= 0 and < wxID_LOWEST
            // in fact 0 to n-1 for n items in clarification list)
            // id < 0 are automatically created for menuitems created with wxID_ANY
            #define ID_CONTEXT_MENU_ID_MAX wxID_LOWEST  /* = 100 should be enough and better */

            if( !evt &&
                    ( ( m_selected >= 0 && m_selected < ID_CONTEXT_MENU_ID_MAX ) ||
                      ( m_selected >= ID_POPUP_MENU_START && m_selected <= ID_POPUP_MENU_END ) ) )
            {
                menuText = GetLabelText( aEvent.GetId() );
                evt = TOOL_EVENT( TC_COMMAND, TA_CHOICE_MENU_CHOICE, m_selected, AS_GLOBAL,
                                  &menuText );
            }
        }
    }

    // forward the action/update event to the TOOL_MANAGER
    // clients that don't supply a tool will have to check GetSelected() themselves
    if( evt && m_tool )
    {

        wxLogTrace( kicadTraceToolStack, "ACTION_MENU::OnMenuEvent %s", evt->Format() );

        TOOL_MANAGER* toolMgr = m_tool->GetManager();

        if( g_last_menu_highlighted_id == aEvent.GetId() && !m_isContextMenu )
            evt->SetHasPosition( false );

        if( toolMgr->GetEditFrame() && !toolMgr->GetEditFrame()->GetDoImmediateActions() )
        {
            // An tool-selection-event has no position
            if( evt->GetCommandStr().is_initialized()
                    && evt->GetCommandStr().get() != toolMgr->GetEditFrame()->CurrentToolName() )
            {
                evt->SetHasPosition( false );
            }
        }

        if( m_tool->GetManager() )
            m_tool->GetManager()->ProcessEvent( *evt );
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
        std::for_each( m_submenus.begin(), m_submenus.end(), [&]( ACTION_MENU* m ) {
            aFunction( m );
            m->runOnSubmenus( aFunction );
        } );
    }
    catch( std::exception& e )
    {
        wxLogDebug( wxString::Format( "ACTION_MENU runOnSubmenus exception: %s", e.what() ) );
    }
}


OPT_TOOL_EVENT ACTION_MENU::findToolAction( int aId )
{
    OPT_TOOL_EVENT evt;

    auto findFunc = [&]( ACTION_MENU* m ) {
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
    // On Windows, for Checkable Menu items, adding a null bitmap adds also
    // our predefined checked alternate bitmap
    const wxBitmap& src_bitmap = aSource->GetBitmap();

    if( src_bitmap.IsOk() && src_bitmap.GetHeight() > 1 )    // a null bitmap has a 0 size
        AddBitmapToMenuItem( newItem, src_bitmap );

    if( aSource->IsSubMenu() )
    {
        ACTION_MENU* menu = dynamic_cast<ACTION_MENU*>( aSource->GetSubMenu() );
        wxASSERT_MSG( menu, "Submenus are expected to be a ACTION_MENU" );

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
