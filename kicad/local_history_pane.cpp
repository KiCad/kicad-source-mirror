/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "local_history_pane.h"
#include "kicad_manager_frame.h"

#include <git2.h>
#include <project.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/menu.h>
#include <wx/settings.h>

wxDEFINE_EVENT( EVT_LOCAL_HISTORY_REFRESH, wxCommandEvent );

LOCAL_HISTORY_PANE::LOCAL_HISTORY_PANE( KICAD_MANAGER_FRAME* aParent ) :
        wxPanel( aParent ),
        m_frame( aParent ),
        m_list( nullptr ),
        m_refreshTimer( this )
{
    m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL );
    m_list->AppendColumn( _( "Title" ) );
    m_list->AppendColumn( _( "Time" ) );
    m_list->SetColumnWidth( 0, FromDIP( 200 ) );
    m_list->SetColumnWidth( 1, FromDIP( 150 ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( m_list, 1, wxEXPAND );
    SetSizer( sizer );

    m_list->Bind( wxEVT_MOTION, &LOCAL_HISTORY_PANE::OnMotion, this );
    m_list->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK, &LOCAL_HISTORY_PANE::OnRightClick, this );
    Bind( wxEVT_TIMER, &LOCAL_HISTORY_PANE::OnRefreshTimer, this, m_refreshTimer.GetId() );
    Bind( EVT_LOCAL_HISTORY_REFRESH, &LOCAL_HISTORY_PANE::OnRefreshEvent, this );

    m_refreshTimer.Start( 10000 );
}

LOCAL_HISTORY_PANE::~LOCAL_HISTORY_PANE()
{
    m_refreshTimer.Stop();
}

void LOCAL_HISTORY_PANE::RefreshHistory( const wxString& aProjectPath )
{
    if( !IsShownOnScreen() )
        return;

    m_list->DeleteAllItems();
    m_commits.clear();

    wxFileName hist( aProjectPath, wxS( ".history" ) );

    git_repository* repo = nullptr;
    if( git_repository_open( &repo, hist.GetFullPath().mb_str().data() ) != 0 )
        return;

    git_revwalk* walk = nullptr;
    if( git_revwalk_new( &walk, repo ) != 0 )
    {
        git_repository_free( repo );
        return;
    }

    if( git_revwalk_push_head( walk ) != 0 )
    {
        git_revwalk_free( walk );
        git_repository_free( repo );
        return;
    }

    wxDateTime now = wxDateTime::Now();

    git_oid oid;
    while( git_revwalk_next( &oid, walk ) == 0 )
    {
        git_commit* commit = nullptr;
        if( git_commit_lookup( &commit, repo, &oid ) != 0 )
            continue;

        LOCAL_COMMIT_INFO info;
        info.hash = wxString::FromUTF8( git_oid_tostr_s( &oid ) );
        info.summary = wxString::FromUTF8( git_commit_summary( commit ) );
        info.message = wxString::FromUTF8( git_commit_message( commit ) );
        info.date = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );

        // Calculate relative time
        wxTimeSpan elapsed = now - info.date;
        wxString timeStr;

        if( elapsed.GetMinutes() < 1 )
        {
            timeStr = _( "Moments ago" );
        }
        else if( elapsed.GetMinutes() < 91 )
        {
            int minutes = elapsed.GetMinutes();
            timeStr = wxString::Format( _( "%d minutes ago" ), minutes );
        }
        else if( elapsed.GetHours() < 24 )
        {
            int hours = elapsed.GetHours();
            timeStr = wxString::Format( _( "%d hours ago" ), hours );
        }
        else
        {
            timeStr = info.date.Format();
        }

        wxString title = info.message.BeforeFirst( '\n' );
        long     row = m_list->InsertItem( m_list->GetItemCount(), title );
        m_list->SetItem( row, 1, timeStr );

        // Tint by foreground colour so light and dark themes both stay readable.
        if( info.summary.StartsWith( wxS( "Autosave" ) ) )
            m_list->SetItemTextColour( row, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
        else if( info.summary.StartsWith( wxS( "Backup" ) ) )
            m_list->SetItemTextColour( row, wxColour( 80, 120, 200 ) );

        m_commits.emplace_back( std::move( info ) );
        git_commit_free( commit );
    }

    git_revwalk_free( walk );
    git_repository_free( repo );
}

void LOCAL_HISTORY_PANE::OnMotion( wxMouseEvent& aEvent )
{
    int  flags = 0;
    long item = m_list->HitTest( aEvent.GetPosition(), flags );

    if( item >= 0 && item < static_cast<long>( m_commits.size() ) )
    {
        const LOCAL_COMMIT_INFO& info = m_commits[item];
        wxString                 tip = info.message;

        if( !tip.EndsWith( wxS( "\n" ) ) )
            tip << wxS( "\n" );

        tip << wxS( "\n" ) << info.date.FormatISOCombined();

        if( m_list->GetToolTipText() != tip )
            m_list->SetToolTip( tip );
    }
    else
    {
        m_list->UnsetToolTip();
    }

    aEvent.Skip();
}

void LOCAL_HISTORY_PANE::OnRightClick( wxListEvent& aEvent )
{
    long item = aEvent.GetIndex();

    if( item < 0 || item >= static_cast<long>( m_commits.size() ) )
        return;

    wxMenu menu;
    wxMenuItem* restore = menu.Append( wxID_ANY, _( "Restore Commit" ) );
    menu.Bind( wxEVT_MENU,
               [this, item]( wxCommandEvent& )
               {
                   m_frame->RestoreCommitFromHistory( m_commits[item].hash );
               },
               restore->GetId() );
    PopupMenu( &menu );
}


void LOCAL_HISTORY_PANE::OnRefreshEvent( wxCommandEvent& aEvent )
{
    wxString projectPath = aEvent.GetString();

    if( projectPath.IsEmpty() )
        projectPath = m_frame->Prj().GetProjectPath();

    RefreshHistory( projectPath );
}


void LOCAL_HISTORY_PANE::OnRefreshTimer( wxTimerEvent& aEvent )
{
    // Refresh the history to show updates from autosave
    RefreshHistory( m_frame->Prj().GetProjectPath() );
}
