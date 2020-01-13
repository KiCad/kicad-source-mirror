/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <filehistory.h>
#include <id.h>
#include <settings/app_settings.h>
#include <tool/action_menu.h>
#include <tool/selection_conditions.h>
#include <wx/menu.h>

#include <functional>
using namespace std::placeholders;


FILE_HISTORY::FILE_HISTORY( size_t aMaxFiles, int aBaseFileId ) :
        wxFileHistory( std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE ) )
{
    SetBaseId( aBaseFileId );
}


void FILE_HISTORY::Load( const APP_SETTINGS_BASE& aSettings )
{
    m_fileHistory.clear();

    // file_history stores the most recent file first
    for( auto it = aSettings.m_System.file_history.rbegin();
         it != aSettings.m_System.file_history.rend(); ++it )
        AddFileToHistory( *it );
}


void FILE_HISTORY::Load( const std::vector<wxString>& aList )
{
    m_fileHistory.clear();

    for( const auto& file : aList )
        AddFileToHistory( file );
}


void FILE_HISTORY::Save( APP_SETTINGS_BASE& aSettings )
{
    aSettings.m_System.file_history.clear();

    for( const auto& file : m_fileHistory )
        aSettings.m_System.file_history.insert( aSettings.m_System.file_history.begin(),
                                                file.ToStdString() );
}


void FILE_HISTORY::Save( std::vector<wxString>* aList )
{
    aList->clear();

    for( const auto& file : m_fileHistory )
        aList->push_back( file );
}


void FILE_HISTORY::SetMaxFiles( size_t aMaxFiles )
{
    m_fileMaxFiles = std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE );

    size_t numFiles = m_fileHistory.size();

    while( numFiles > m_fileMaxFiles )
        RemoveFileFromHistory( --numFiles );
}


void FILE_HISTORY::AddFileToHistory( const wxString &aFile )
{
    wxFileHistory::AddFileToHistory( aFile );

    // Iterate over each menu associated with this file history, and if it is one of our
    // FILE_HISTORY_MENUs, we force it to be refreshed (so that the items are all in the
    // correct locations).
    for( wxList::compatibility_iterator node = m_fileMenus.GetFirst();
            node; node = node->GetNext() )
    {
        wxMenu* menu = static_cast<wxMenu*>( node->GetData() );

        FILE_HISTORY_MENU* fileMenu = dynamic_cast<FILE_HISTORY_MENU*>( menu );

        if( fileMenu )
            fileMenu->RefreshMenu();
    }
}


SELECTION_CONDITION FILE_HISTORY::FileHistoryNotEmpty( const FILE_HISTORY& aHistory )
{
    return std::bind( &FILE_HISTORY::isHistoryNotEmpty, _1, std::cref( aHistory ) );
}


bool FILE_HISTORY::isHistoryNotEmpty( const SELECTION& aSelection, const FILE_HISTORY& aHistory )
{
    return aHistory.GetCount() != 0;
}


FILE_HISTORY_MENU::FILE_HISTORY_MENU( FILE_HISTORY& aHistory, wxString aClearText ) :
    ACTION_MENU( false ),
    m_fileHistory( aHistory ),
    m_clearText( aClearText )
{
    m_fileHistory.UseMenu( this );
    buildMenu();
}


FILE_HISTORY_MENU::~FILE_HISTORY_MENU()
{
    m_fileHistory.RemoveMenu( this );
}


void FILE_HISTORY_MENU::RefreshMenu()
{
    // We have to manually delete all menu items before we rebuild the menu
    for( int i = GetMenuItemCount() - 1; i >= 0; --i )
        Destroy( FindItemByPosition( i ) );

    buildMenu();
}


void FILE_HISTORY_MENU::buildMenu()
{
    if( m_fileHistory.GetCount() == 0 )
    {
        // If the history is empty, we create an item to say there are no files
        wxMenuItem* item = new wxMenuItem( this, wxID_ANY, _( "No Files" ) );

        Append( item );
        Enable( item->GetId(), false );
    }
    else
        m_fileHistory.AddFilesToMenu( this );

    wxMenuItem* clearItem = new wxMenuItem( this, ID_FILE_LIST_CLEAR, m_clearText );

    AppendSeparator();
    Append( clearItem );
    Connect( ID_FILE_LIST_CLEAR, wxEVT_COMMAND_MENU_SELECTED,
            wxMenuEventHandler( FILE_HISTORY_MENU::onClearEntries ), NULL, this );
}


void FILE_HISTORY_MENU::onClearEntries( wxMenuEvent& aEvent )
{
    while( m_fileHistory.GetCount() > 0 )
        m_fileHistory.RemoveFileFromHistory( 0 );

    RefreshMenu();
}


ACTION_MENU* FILE_HISTORY_MENU::create() const
{
    return new FILE_HISTORY_MENU( m_fileHistory, m_clearText );
}
