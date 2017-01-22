/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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


/**
 * @file  class_action_plugin.cpp
 * @brief Class ACTION_PLUGIN and ACTION_PLUGINS
 */

#include "class_action_plugin.h"


ACTION_PLUGIN::~ACTION_PLUGIN()
{
}


void ACTION_PLUGIN::register_action()
{
    ACTION_PLUGINS::register_action( this );
}


std::vector<ACTION_PLUGIN*> ACTION_PLUGINS::m_Actions;
std::vector<int> ACTION_PLUGINS::m_ActionsMenu;


ACTION_PLUGIN* ACTION_PLUGINS::GetAction( int aIndex )
{
    return m_Actions[aIndex];
}


ACTION_PLUGIN* ACTION_PLUGINS::GetActionByMenu( int menu )
{
    int max = GetActionsCount();

    for( int i = 0; i<max; i++ )
    {
        if( m_ActionsMenu[i] == menu )
            return m_Actions[i];
    }

    return NULL;
}


void ACTION_PLUGINS::SetActionMenu( int aIndex, int idMenu )
{
    m_ActionsMenu[aIndex] = idMenu;
}


int ACTION_PLUGINS::GetActionMenu( int aIndex )
{
    return m_ActionsMenu[aIndex];
}


ACTION_PLUGIN* ACTION_PLUGINS::GetAction( wxString aName )
{
    int max = GetActionsCount();

    for( int i = 0; i<max; i++ )
    {
        ACTION_PLUGIN* action = GetAction( i );

        wxString name = action->GetName();

        if( name.Cmp( aName )==0 )
            return action;
    }

    return NULL;
}


int ACTION_PLUGINS::GetActionsCount()
{
    return m_Actions.size();
}


void ACTION_PLUGINS::register_action( ACTION_PLUGIN* aAction )
{
    int updatedMenu = 0;

    // Search for this entry do not register twice this action:
    for( int ii = 0; ii < GetActionsCount(); ii++ )
    {
        if( aAction == GetAction( ii ) ) // Already registered
            return;
    }

    // Search for a action with the same name, and remove it if found
    for( int ii = 0; ii < GetActionsCount(); ii++ )
    {
        ACTION_PLUGIN* action = GetAction( ii );

        if( action->GetName() == aAction->GetName() )
        {
            updatedMenu = GetActionMenu( ii );
            m_Actions.erase( m_Actions.begin() + ii );
            m_ActionsMenu.erase( m_ActionsMenu.begin() + ii );

            delete action;

            break;
        }
    }

    m_Actions.push_back( aAction );
    m_ActionsMenu.push_back( updatedMenu );
}


bool ACTION_PLUGINS::deregister_object( void* aObject )
{
    int max = GetActionsCount();

    for( int i = 0; i<max; i++ )
    {
        ACTION_PLUGIN* action = GetAction( i );

        if( action->GetObject() == aObject )
        {
            m_Actions.erase( m_Actions.begin() + i );
            m_ActionsMenu.erase( m_ActionsMenu.begin() + i );
            delete action;
            return true;
        }
    }

    return false;
}
