/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
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

#include <file_history.h>
#include <id.h>
#include <settings/app_settings.h>
#include <tool/action_menu.h>
#include <tool/selection_conditions.h>
#include <wx/menu.h>

#include <functional>
using namespace std::placeholders;


FILE_HISTORY::FILE_HISTORY( size_t aMaxFiles, int aBaseFileId, int aClearId, wxString aClearText )
        : wxFileHistory( std::min( aMaxFiles, (size_t) MAX_FILE_HISTORY_SIZE ) ),
          m_clearId( aClearId ),
          m_clearText( aClearText )
{
    SetBaseId( aBaseFileId );
}


void FILE_HISTORY::Load( const APP_SETTINGS_BASE& aSettings )
{
    ClearFileHistory();

    // file_history stores the most recent file first
    for( auto it = aSettings.m_System.file_history.rbegin();
         it != aSettings.m_System.file_history.rend(); ++it )
    {
        AddFileToHistory( *it );
    }
}


void FILE_HISTORY::Load( const std::vector<wxString>& aList )
{
    ClearFileHistory();

    for( const auto& file : aList )
        AddFileToHistory( file );
}


void FILE_HISTORY::Save( APP_SETTINGS_BASE& aSettings )
{
    aSettings.m_System.file_history.clear();

    for( const wxString& filename : m_fileHistory )
        aSettings.m_System.file_history.emplace_back( filename );
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
    // Iterate over each menu removing our custom items
    for( wxList::compatibility_iterator node = m_fileMenus.GetFirst();
            node; node = node->GetNext() )
    {
        wxMenu* menu = static_cast<wxMenu*>( node->GetData() );
        doRemoveClearitem( menu );
    }

    // Let wx add the items in the file history
    wxFileHistory::AddFileToHistory( aFile );

    // Add our custom items back
    for( wxList::compatibility_iterator node = m_fileMenus.GetFirst();
            node; node = node->GetNext() )
    {
        wxMenu* menu = static_cast<wxMenu*>( node->GetData() );
        doAddClearItem( menu );
    }
}


void FILE_HISTORY::AddFilesToMenu( wxMenu* aMenu )
{
    doRemoveClearitem( aMenu );
    wxFileHistory::AddFilesToMenu( aMenu );
    doAddClearItem( aMenu );
}


void FILE_HISTORY::doRemoveClearitem( wxMenu* aMenu )
{
    size_t      itemPos;
    wxMenuItem* clearItem = aMenu->FindChildItem( m_clearId, &itemPos );

    // Remove the separator if there is one
    if( clearItem && itemPos > 1 )
    {
        wxMenuItem* sepItem = aMenu->FindItemByPosition( itemPos - 1 );

        if( sepItem )
            aMenu->Destroy( sepItem );
    }

    // Remove the clear and placeholder menu items
    if( clearItem )
        aMenu->Destroy( m_clearId );

    if( aMenu->FindChildItem( ID_FILE_LIST_EMPTY ) )
        aMenu->Destroy( ID_FILE_LIST_EMPTY );
}


void FILE_HISTORY::doAddClearItem( wxMenu* aMenu )
{
    if( GetCount() == 0 )
    {
        // If the history is empty, we create an item to say there are no files
        wxMenuItem* item = new wxMenuItem( nullptr, ID_FILE_LIST_EMPTY, _( "No Files" ) );

        aMenu->Append( item );
        aMenu->Enable( item->GetId(), false );
    }

    wxMenuItem* clearItem = new wxMenuItem( nullptr, m_clearId, m_clearText );

    aMenu->AppendSeparator();
    aMenu->Append( clearItem );
}


void FILE_HISTORY::UpdateClearText( wxMenu* aMenu, wxString aClearText )
{
    size_t      itemPos;
    wxMenuItem* clearItem = aMenu->FindChildItem( m_clearId, &itemPos );

    if( clearItem && itemPos > 1 )      // clearItem is the last menu, after a separator
    {
        clearItem->SetItemLabel( aClearText );
    }
}


void FILE_HISTORY::ClearFileHistory()
{
    while( GetCount() > 0 )
        RemoveFileFromHistory( 0 );
}


SELECTION_CONDITION FILE_HISTORY::FileHistoryNotEmpty( const FILE_HISTORY& aHistory )
{
    return std::bind( &FILE_HISTORY::isHistoryNotEmpty, _1, std::cref( aHistory ) );
}


bool FILE_HISTORY::isHistoryNotEmpty( const SELECTION& aSelection, const FILE_HISTORY& aHistory )
{
    return aHistory.GetCount() != 0;
}
