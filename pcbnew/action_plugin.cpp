/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  action_plugin.cpp
 * @brief Class ACTION_PLUGIN and ACTION_PLUGINS
 */

#include <wx/log.h>

#include "action_plugin.h"
#include "bitmaps.h"
#include "bitmap_store.h"
#include <pgm_base.h>


ACTION_PLUGIN::~ACTION_PLUGIN()
{
}


void ACTION_PLUGIN::register_action()
{
    ACTION_PLUGINS::register_action( this );
}


std::vector<ACTION_PLUGIN*> ACTION_PLUGINS::m_actionsList;


bool ACTION_PLUGINS::m_actionRunning = false;


ACTION_PLUGIN* ACTION_PLUGINS::GetAction( int aIndex )
{
    return m_actionsList[aIndex];
}


ACTION_PLUGIN* ACTION_PLUGINS::GetActionByMenu( int aMenu )
{
    int max = GetActionsCount();

    for( int i = 0; i < max; i++ )
    {
        if( m_actionsList[i]->m_actionMenuId == aMenu )
            return m_actionsList[i];
    }

    return nullptr;
}


void ACTION_PLUGINS::SetActionMenu( int aIndex, int idMenu )
{
    m_actionsList[aIndex]->m_actionMenuId = idMenu;
}


ACTION_PLUGIN* ACTION_PLUGINS::GetActionByButton( int aButton )
{
    int max = GetActionsCount();

    for( int i = 0; i < max; i++ )
    {
        if( m_actionsList[i]->m_actionButtonId == aButton )
            return m_actionsList[i];
    }

    return nullptr;
}


void ACTION_PLUGINS::SetActionButton( ACTION_PLUGIN* aAction, int idButton )
{
    aAction->m_actionButtonId = idButton;
}


ACTION_PLUGIN* ACTION_PLUGINS::GetActionByPath(const wxString& aPath)
{
    for( int i = 0; i < GetActionsCount() ; i++ )
    {
        if( m_actionsList[i]->GetPluginPath() == aPath)
        {
            return m_actionsList[i];
        }
    }

    return nullptr;
}


ACTION_PLUGIN* ACTION_PLUGINS::GetAction( const wxString& aName )
{
    int max = GetActionsCount();

    for( int i = 0; i<max; i++ )
    {
        ACTION_PLUGIN* action = GetAction( i );

        wxString name = action->GetName();

        if( name.Cmp( aName )==0 )
            return action;
    }

    return nullptr;
}


int ACTION_PLUGINS::GetActionsCount()
{
    return m_actionsList.size();
}


void ACTION_PLUGINS::register_action( ACTION_PLUGIN* aAction )
{
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
            m_actionsList.erase( m_actionsList.begin() + ii );

            delete action;

            break;
        }
    }

    wxASSERT( PgmOrNull() );    // PgmOrNull() returning nullptr should never happen,
                                // but it sometimes happens on msys2 build

    if( PgmOrNull() )           // Hack for msys2. Must be removed when the root cause is fixed
    {
        // Load icon if supplied
        wxString icon_file_name = aAction->GetIconFileName( GetBitmapStore()->IsDarkTheme() );

        if( !icon_file_name.IsEmpty() )
        {
            {
                wxLogNull eat_errors;
                aAction->iconBitmap.LoadFile( icon_file_name, wxBITMAP_TYPE_PNG );
            }

            if ( !aAction->iconBitmap.IsOk() )
            {
                wxLogVerbose( wxT( "Failed to load icon " ) + icon_file_name + wxT( " for action plugin " ) );
            }
        }
    }

    m_actionsList.push_back( aAction );
}


bool ACTION_PLUGINS::deregister_object( void* aObject )
{
    int max = GetActionsCount();

    for( int i = 0; i<max; i++ )
    {
        ACTION_PLUGIN* action = GetAction( i );

        if( action->GetObject() == aObject )
        {
            m_actionsList.erase( m_actionsList.begin() + i );

            //m_actionsListMenu.erase( m_actionsListMenu.begin() + i );
            delete action;
            return true;
        }
    }

    return false;
}


bool ACTION_PLUGINS::IsActionRunning()
{
    return ACTION_PLUGINS::m_actionRunning;
}


void ACTION_PLUGINS::SetActionRunning( bool aRunning )
{
    ACTION_PLUGINS::m_actionRunning = aRunning;
}


void ACTION_PLUGINS::UnloadAll()
{
    for( ACTION_PLUGIN* plugin : m_actionsList )
        delete plugin;

    m_actionsList.clear();
}
