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

#include "sch_action_plugin.h"
#include <wx/log.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <pgm_base.h>


SCH_ACTION_PLUGIN::~SCH_ACTION_PLUGIN()
{
}


void SCH_ACTION_PLUGIN::register_action()
{
    SCH_ACTION_PLUGINS::register_action( this );
}


std::vector<SCH_ACTION_PLUGIN*> SCH_ACTION_PLUGINS::m_actionsList;
bool SCH_ACTION_PLUGINS::m_actionRunning = false;


SCH_ACTION_PLUGIN* SCH_ACTION_PLUGINS::GetAction( int aIndex )
{
    return m_actionsList[aIndex];
}


SCH_ACTION_PLUGIN* SCH_ACTION_PLUGINS::GetActionByMenu( int aMenu )
{
    for( int i = 0; i < GetActionsCount(); i++ )
    {
        if( m_actionsList[i]->m_actionMenuId == aMenu )
            return m_actionsList[i];
    }

    return nullptr;
}


void SCH_ACTION_PLUGINS::SetActionMenu( int aIndex, int idMenu )
{
    m_actionsList[aIndex]->m_actionMenuId = idMenu;
}


SCH_ACTION_PLUGIN* SCH_ACTION_PLUGINS::GetActionByButton( int aButton )
{
    for( int i = 0; i < GetActionsCount(); i++ )
    {
        if( m_actionsList[i]->m_actionButtonId == aButton )
            return m_actionsList[i];
    }

    return nullptr;
}


void SCH_ACTION_PLUGINS::SetActionButton( SCH_ACTION_PLUGIN* aAction, int idButton )
{
    aAction->m_actionButtonId = idButton;
}


SCH_ACTION_PLUGIN* SCH_ACTION_PLUGINS::GetActionByPath( const wxString& aPath )
{
    for( int i = 0; i < GetActionsCount(); i++ )
    {
        if( m_actionsList[i]->GetPluginPath() == aPath )
            return m_actionsList[i];
    }

    return nullptr;
}


SCH_ACTION_PLUGIN* SCH_ACTION_PLUGINS::GetAction( const wxString& aName )
{
    for( int i = 0; i < GetActionsCount(); i++ )
    {
        if( m_actionsList[i]->GetName() == aName )
            return m_actionsList[i];
    }

    return nullptr;
}


int SCH_ACTION_PLUGINS::GetActionsCount()
{
    return m_actionsList.size();
}


void SCH_ACTION_PLUGINS::register_action( SCH_ACTION_PLUGIN* aAction )
{
    // Don't register twice
    for( int ii = 0; ii < GetActionsCount(); ii++ )
    {
        if( aAction == GetAction( ii ) )
            return;
    }

    // Replace existing action with same name
    for( int ii = 0; ii < GetActionsCount(); ii++ )
    {
        SCH_ACTION_PLUGIN* action = GetAction( ii );

        if( action->GetName() == aAction->GetName() )
        {
            m_actionsList.erase( m_actionsList.begin() + ii );
            delete action;
            break;
        }
    }

    wxASSERT( PgmOrNull() );

    if( PgmOrNull() )
    {
        wxString icon_file_name = aAction->GetIconFileName( GetBitmapStore()->IsDarkTheme() );

        if( !icon_file_name.IsEmpty() )
        {
            {
                wxLogNull eat_errors;
                aAction->iconBitmap.LoadFile( icon_file_name, wxBITMAP_TYPE_PNG );
            }

            if( !aAction->iconBitmap.IsOk() )
            {
                wxLogVerbose( wxT( "Failed to load icon " ) + icon_file_name
                              + wxT( " for eeschema action plugin " ) );
            }
        }
    }

    m_actionsList.push_back( aAction );
}


bool SCH_ACTION_PLUGINS::deregister_object( void* aObject )
{
    for( int i = 0; i < GetActionsCount(); i++ )
    {
        SCH_ACTION_PLUGIN* action = GetAction( i );

        if( action->GetObject() == aObject )
        {
            m_actionsList.erase( m_actionsList.begin() + i );
            delete action;
            return true;
        }
    }

    return false;
}


bool SCH_ACTION_PLUGINS::IsActionRunning()
{
    return m_actionRunning;
}


void SCH_ACTION_PLUGINS::SetActionRunning( bool aRunning )
{
    m_actionRunning = aRunning;
}


void SCH_ACTION_PLUGINS::UnloadAll()
{
    for( SCH_ACTION_PLUGIN* plugin : m_actionsList )
        delete plugin;

    m_actionsList.clear();
}
