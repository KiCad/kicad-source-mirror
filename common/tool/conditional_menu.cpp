/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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


CONDITIONAL_MENU::CONDITIONAL_MENU( bool isContextMenu, TOOL_INTERACTIVE* aTool ) :
        m_isContextMenu( isContextMenu )
{
    m_tool = aTool;

    // wxWidgets 3.0.4 on MSW checks for an empty menu before running the MENU_OPEN
    // event.  Add a dummy item to ensure that the event is dispatched.  Evaluate()
    // will clear the menu before evaluating all the items anyway.
    Append( wxID_ANY, wxT( "dummy menu for MSW" ) );
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
        item->SetBitmap( KiBitmap( aIcon ) );

    addEntry( ENTRY( item, aCondition, aOrder, false ) );
}


void CONDITIONAL_MENU::AddCheckItem( int aId, const wxString& aText, const wxString& aTooltip,
                                     BITMAP_DEF aIcon, const SELECTION_CONDITION& aCondition,
                                     int aOrder )
{
    wxMenuItem* item = new wxMenuItem( nullptr, aId, aText, aTooltip, wxITEM_CHECK );

#if !defined(__WXGTK__)  // wxGTK does not support bitmaps on checkable menu items

    if( aIcon )
        item->SetBitmap( KiBitmap( aIcon ) );

#endif

    addEntry( ENTRY( item, aCondition, aOrder, true ) );
}


void CONDITIONAL_MENU::AddMenu( ACTION_MENU* aMenu, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    addEntry( ENTRY( aMenu, aCondition, aOrder ) );
}


void CONDITIONAL_MENU::AddSeparator( const SELECTION_CONDITION& aCondition, int aOrder )
{
    addEntry( ENTRY( aCondition, aOrder ) );
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
                menuItem = Append( entry.wxItem()->GetId(), entry.wxItem()->GetItemLabel(),
                                   entry.wxItem()->GetHelp(), entry.wxItem()->GetKind() );
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
