/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 CERN
 * Copyright (C) 2015-2019 KiCad Developers, see CHANGELOG.txt for contributors.
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

#include <tool/conditional_menu.h>
#include <tool/action_menu.h>
#include <menus_helpers.h>
#include <kiface_i.h>


CONDITIONAL_MENU::CONDITIONAL_MENU( bool isContextMenu, TOOL_INTERACTIVE* aTool ) :
        ACTION_MENU( isContextMenu )
{
    m_tool = aTool;
}


ACTION_MENU* CONDITIONAL_MENU::create() const
{
    CONDITIONAL_MENU* clone = new CONDITIONAL_MENU( m_isContextMenu, m_tool );
    clone->m_entries = m_entries;
    return clone;
}


void CONDITIONAL_MENU::AddItem( const TOOL_ACTION& aAction, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    wxASSERT( aAction.GetId() > 0 ); // Check if action was previously registered in ACTION_MANAGER
    addEntry( ENTRY( &aAction, aCondition, aOrder, false ) );
}


void CONDITIONAL_MENU::AddCheckItem( const TOOL_ACTION& aAction,
                                     const SELECTION_CONDITION& aCondition, int aOrder )
{
    wxASSERT( aAction.GetId() > 0 ); // Check if action was previously registered in ACTION_MANAGER
    addEntry( ENTRY( &aAction, aCondition, aOrder, true ) );
}


void CONDITIONAL_MENU::AddItem( int aId, const wxString& aText, const wxString& aTooltip,
                                BITMAP_DEF aIcon, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    wxMenuItem* item = new wxMenuItem( nullptr, aId, aText, aTooltip, wxITEM_NORMAL );

    if( aIcon )
        AddBitmapToMenuItem( item, KiBitmap( aIcon ) );

    addEntry( ENTRY( item, aIcon, aCondition, aOrder, false ) );
}


void CONDITIONAL_MENU::AddCheckItem( int aId, const wxString& aText, const wxString& aTooltip,
                                     BITMAP_DEF aIcon, const SELECTION_CONDITION& aCondition,
                                     int aOrder )
{
    wxMenuItem* item = new wxMenuItem( nullptr, aId, aText, aTooltip, wxITEM_CHECK );

    if( aIcon )
        AddBitmapToMenuItem( item, KiBitmap( aIcon ) );

    addEntry( ENTRY( item, aIcon, aCondition, aOrder, true ) );
}


void CONDITIONAL_MENU::AddMenu( ACTION_MENU* aMenu, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    addEntry( ENTRY( aMenu, aCondition, aOrder ) );
}


void CONDITIONAL_MENU::AddSeparator( int aOrder )
{
    addEntry( ENTRY( SELECTION_CONDITIONS::ShowAlways, aOrder ) );
}


void CONDITIONAL_MENU::AddClose( wxString aAppname )
{
    AddItem( wxID_CLOSE, _( "Close\tCTRL+W" ), wxString::Format( "Close %s", aAppname ), exit_xpm,
            SELECTION_CONDITIONS::ShowAlways );
}


void CONDITIONAL_MENU::AddQuitOrClose( KIFACE_I* aKiface, wxString aAppname )
{
    if( !aKiface || aKiface->IsSingle() ) // not when under a project mgr
    {
        // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via
        // wxID_EXIT
        AddItem( wxID_EXIT, _( "Quit" ), wxString::Format( "Quit %s", aAppname ), exit_xpm,
                SELECTION_CONDITIONS::ShowAlways );
    }
    else
    {
        AddClose( aAppname );
    }
}


SELECTION g_resolveDummySelection;


void CONDITIONAL_MENU::Resolve()
{
    Evaluate( g_resolveDummySelection );
    UpdateAll();

    runOnSubmenus( [] ( ACTION_MENU* aMenu ) {
        CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( aMenu );

        if( conditionalMenu )
            conditionalMenu->Resolve();
    } );
}


void CONDITIONAL_MENU::Evaluate( SELECTION& aSelection )
{
    Clear();

    // We try to avoid adding useless separators (when no menuitems between separators)
    int menu_count = 0;     // number of menus since the latest separator

    for( const ENTRY& entry : m_entries )
    {
        const SELECTION_CONDITION& cond = entry.Condition();
        bool                       result;
        wxMenuItem*                menuItem = nullptr;

        try
        {
            result = cond( aSelection );
        }
        catch( std::exception& )
        {
            continue;
        }

        if( m_isContextMenu && !result )
            continue;

        switch( entry.Type() )
        {
            case ENTRY::ACTION:
                menuItem = Add( *entry.Action(), entry.IsCheckmarkEntry() );
                menu_count++;
                break;

            case ENTRY::MENU:
                menuItem = Add( entry.Menu() );
                menu_count++;
                break;

            case ENTRY::WXITEM:
#ifdef __WXMAC__
                // Instantiate the Preferences item only on the first Resolve(); after that
                // wxWidgets will have moved it to the Application menu
                if( entry.wxItem()->GetId() == wxID_PREFERENCES )
                {
                    if( &aSelection != &g_resolveDummySelection )
                        continue;
                }
#endif
                menuItem = new wxMenuItem( this,
                                           entry.wxItem()->GetId(),
                                           entry.wxItem()->GetItemLabel(),
                                           entry.wxItem()->GetHelp(),
                                           entry.wxItem()->GetKind() );

                if( entry.GetIcon() )
                    AddBitmapToMenuItem( menuItem, KiBitmap( entry.GetIcon() ) );

                // the wxMenuItem must be append only after the bitmap is set:
                Append( menuItem );

                menu_count++;
                break;

            case ENTRY::SEPARATOR:
                if( menu_count )
                    menuItem = AppendSeparator();

                menu_count = 0;
                break;

            default:
                wxASSERT( false );
                break;
        }

        if( menuItem )
        {
            if( entry.IsCheckmarkEntry() )
                menuItem->Check( result );
            else
                menuItem->Enable( result );
        }
    }

    // Recursively call Evaluate on all the submenus that are CONDITIONAL_MENUs to ensure
    // they are updated. This is also required on GTK to make sure the menus have the proper
    // size when created.
    runOnSubmenus(
        [&aSelection]( ACTION_MENU* aMenu )
        {
            CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( aMenu );

            if( conditionalMenu )
                conditionalMenu->Evaluate( aSelection );
        } );
}


void CONDITIONAL_MENU::addEntry( ENTRY aEntry )
{
    if( aEntry.Order() < 0 )        // Any order, so give it any order number
        aEntry.SetOrder( m_entries.size() );

    std::list<ENTRY>::iterator it = m_entries.begin();

    // Find the right spot for the entry
    while( it != m_entries.end() && it->Order() <= aEntry.Order() )
        ++it;

    m_entries.insert( it, aEntry );
}
