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

wxDEFINE_EVENT( EVT_LOCAL_HISTORY_REFRESH, wxCommandEvent );

LOCAL_HISTORY_PANE::LOCAL_HISTORY_PANE( KICAD_MANAGER_FRAME* aParent ) : wxPanel( aParent ),
        m_frame( aParent ), m_list( nullptr ), m_timer( this ), m_refreshTimer( this ),
        m_hoverItem( -1 ), m_tip( nullptr )
{
    m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxLC_REPORT | wxLC_SINGLE_SEL );
    m_list->AppendColumn( _( "Title" ) );
    m_list->AppendColumn( _( "Time" ) );
    m_list->SetColumnWidth( 0, 200 );
    m_list->SetColumnWidth( 1, 120 );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( m_list, 1, wxEXPAND );
    SetSizer( sizer );

    m_list->Bind( wxEVT_MOTION, &LOCAL_HISTORY_PANE::OnMotion, this );
    m_list->Bind( wxEVT_LEAVE_WINDOW, &LOCAL_HISTORY_PANE::OnLeave, this );
    m_list->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK, &LOCAL_HISTORY_PANE::OnRightClick, this );
    Bind( wxEVT_TIMER, &LOCAL_HISTORY_PANE::OnTimer, this, m_timer.GetId() );
    Bind( wxEVT_TIMER, &LOCAL_HISTORY_PANE::OnRefreshTimer, this, m_refreshTimer.GetId() );
    Bind( EVT_LOCAL_HISTORY_REFRESH, &LOCAL_HISTORY_PANE::OnRefreshEvent, this );

    // Start refresh timer to check for updates every 10 seconds
    m_refreshTimer.Start( 10000 );
}

LOCAL_HISTORY_PANE::~LOCAL_HISTORY_PANE()
{
    m_timer.Stop();
    m_refreshTimer.Stop();

    if( m_tip )
    {
        m_tip->Destroy();
        m_tip = nullptr;
    }
}

void LOCAL_HISTORY_PANE::RefreshHistory( const wxString& aProjectPath )
{
    std::lock_guard<std::mutex> lock( m_mutex );

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
            timeStr = info.date.FormatISOCombined();
        }

        long row = m_list->InsertItem( m_list->GetItemCount(), info.summary );
        m_list->SetItem( row, 1, timeStr );

        if( info.summary.StartsWith( wxS( "Autosave" ) ) )
            m_list->SetItemBackgroundColour( row, wxColour( 230, 255, 230 ) );
        else if( info.summary.StartsWith( wxS( "Backup" ) ) )
            m_list->SetItemBackgroundColour( row, wxColour( 230, 230, 255 ) );

        m_commits.emplace_back( std::move( info ) );
        git_commit_free( commit );
    }

    git_revwalk_free( walk );
    git_repository_free( repo );
}

void LOCAL_HISTORY_PANE::OnMotion( wxMouseEvent& aEvent )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    int flags = 0;
    long item = m_list->HitTest( aEvent.GetPosition(), flags );

    if( item != m_hoverItem )
    {
        if( m_hoverItem != -1 )
            m_list->SetItemState( m_hoverItem, 0, wxLIST_STATE_SELECTED );

        m_hoverItem = item;
        m_hoverPos = aEvent.GetPosition();

        if( m_hoverItem != -1 )
            m_list->SetItemState( m_hoverItem, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

        m_timer.StartOnce( 500 );
    }

    aEvent.Skip();
}

void LOCAL_HISTORY_PANE::OnLeave( wxMouseEvent& aEvent )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    m_timer.Stop();

    if( m_hoverItem != -1 )
        m_list->SetItemState( m_hoverItem, 0, wxLIST_STATE_SELECTED );

    m_hoverItem = -1;

    if( m_tip )
    {
        m_tip->Destroy();
        m_tip = nullptr;
    }

    aEvent.Skip();
}

void LOCAL_HISTORY_PANE::OnTimer( wxTimerEvent& aEvent )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if( m_hoverItem < 0 || m_hoverItem >= static_cast<long>( m_commits.size() ) )
        return;

    if( m_tip )
    {
        m_tip->Destroy();
        m_tip = nullptr;
    }

    wxString msg = m_commits[m_hoverItem].message + wxS( "\n" )
                    + m_commits[m_hoverItem].date.FormatISOCombined();
    m_tip = new wxTipWindow( this, msg );
    m_tip->SetTipWindowPtr( &m_tip );
    m_tip->Position( ClientToScreen( m_hoverPos ), wxDefaultSize );
    SetFocus();
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
