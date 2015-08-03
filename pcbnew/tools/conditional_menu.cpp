/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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

#include "conditional_menu.h"
#include <tool/context_menu.h>

void CONDITIONAL_MENU::AddItem( const TOOL_ACTION& aAction, const SELECTION_CONDITION& aCondition,
                                int aOrder )
{
    assert( aAction.GetId() > 0 ); // Check if action was previously registered in ACTION_MANAGER
    addEntry( ENTRY( &aAction, aCondition, aOrder ) );
}


void CONDITIONAL_MENU::AddMenu( CONTEXT_MENU* aMenu, const wxString& aLabel, bool aExpand,
                                const SELECTION_CONDITION& aCondition, int aOrder )
{
    addEntry( ENTRY( aMenu, aLabel, aExpand, aCondition, aOrder ) );
}


void CONDITIONAL_MENU::AddSeparator( const SELECTION_CONDITION& aCondition, int aOrder )
{
    addEntry( ENTRY( aCondition, aOrder ) );
}


CONTEXT_MENU* CONDITIONAL_MENU::Generate( SELECTION& aSelection )
{
    CONTEXT_MENU* m_menu = new CONTEXT_MENU;
    m_menu->SetTool( m_tool );

    for( std::list<ENTRY>::iterator it = m_entries.begin(); it != m_entries.end(); ++it )
    {
        const SELECTION_CONDITION& cond = it->Condition();

        if( !cond( aSelection ) )
            continue;

        switch( it->Type() )
        {
            case ENTRY::ACTION:
                m_menu->Add( *it->Action() );
                break;

            case ENTRY::MENU:
                it->Menu()->UpdateAll();
                m_menu->Add( it->Menu(), it->Label(), it->Expand() );
                break;

            case ENTRY::WXITEM:
                m_menu->Append( it->wxItem() );
                break;

            case ENTRY::SEPARATOR:
                m_menu->AppendSeparator();
                break;

            default:
                assert( false );
                break;
        }
    }

    return m_menu;
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
