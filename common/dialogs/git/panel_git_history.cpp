/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/git/panel_git_history.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <wx/arrstr.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/mstream.h>
#include <wx/richmsgdlg.h>
#include <wx/textdlg.h>

#include <picosha2.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_file_diff_area.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_ui_utils.h>
#include <diff_merge/diff_scene.h>
#include <eda_base_frame.h>
#include <frame_type.h>
#include <git/git_commit_handler.h>
#include <git/git_config_handler.h>
#include <git/kicad_git_common.h>
#include <git/project_git_utils.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <kiplatform/ui.h>
#include <kiway.h>
#include <kiway_player.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>


namespace
{

wxColour avatarColour( const wxString& aName )
{
    static const wxColour palette[] = {
        wxColour( 0x3b, 0x7d, 0xd8 ), wxColour( 0x8a, 0x5c, 0xd0 ), wxColour( 0x2f, 0x9e, 0x52 ),
        wxColour( 0xc0, 0x85, 0x1a ), wxColour( 0xcf, 0x3b, 0x3b ), wxColour( 0x0e, 0x9a, 0xa0 ),
    };

    unsigned hash = 0;

    for( wxUniChar c : aName )
        hash = hash * 31 + static_cast<unsigned>( c.GetValue() );

    return palette[hash % ( sizeof( palette ) / sizeof( palette[0] ) )];
}


wxString initialsOf( const wxString& aName )
{
    wxString result;

    for( const wxString& part : wxSplit( aName, ' ' ) )
    {
        if( !part.IsEmpty() )
        {
            result += wxString( part[0] ).Upper();

            if( result.length() >= 2 )
                break;
        }
    }

    return result.IsEmpty() ? wxString( "?" ) : result;
}


// Draw the avatar circle of aDiameter centered in an aSlot square bitmap. The slot is taller
// than the circle so the row has room and the circle never clips against the row edge.
wxBitmap makeAvatarBitmap( wxWindow* aCtx, const wxString& aName, int aSlot, int aDiameter )
{
    wxBitmap bmp( aSlot, aSlot, 32 );
    bmp.UseAlpha();

    wxMemoryDC mdc( bmp );
    wxGCDC     dc( mdc );

    dc.SetBackground( *wxTRANSPARENT_BRUSH );
    dc.Clear();

    int offset = ( aSlot - aDiameter ) / 2;

    dc.SetPen( *wxTRANSPARENT_PEN );
    dc.SetBrush( wxBrush( avatarColour( aName ) ) );
    dc.DrawEllipse( offset, offset, aDiameter, aDiameter );

    wxFont font = aCtx->GetFont();
    font.SetWeight( wxFONTWEIGHT_BOLD );
    font.SetPixelSize( wxSize( 0, aDiameter / 2 ) );
    dc.SetFont( font );
    dc.SetTextForeground( *wxWHITE );

    wxString ini = initialsOf( aName );
    wxSize   te = dc.GetTextExtent( ini );
    dc.DrawText( ini, ( aSlot - te.x ) / 2, ( aSlot - te.y ) / 2 );

    mdc.SelectObject( wxNullBitmap );
    return bmp;
}


// Fetch a contributor's Gravatar over HTTPS using the SHA-256 of their email. Returns the raw
// image bytes, or empty on any failure or when the contributor has no Gravatar. Runs on a
// worker thread, so it does no wx GUI work.
std::string fetchGravatarBytes( const wxString& aEmail, int aPixels, const std::atomic<bool>& aCancel )
{
    wxString normalized = aEmail;
    normalized.Trim( true ).Trim( false );
    normalized.MakeLower();

    std::string hash = picosha2::hash256_hex_string( std::string( normalized.utf8_string() ) );

    KICAD_CURL_EASY curl;
    curl.SetURL( "https://www.gravatar.com/avatar/" + hash + "?s=" + std::to_string( aPixels ) + "&d=404" );
    curl.SetUserAgent( "KiCad" );
    curl.SetFollowRedirects( true );

    // A photo is decoration, so it must not sit on an unresponsive host for the default 30
    // second connect timeout.
    curl.SetConnectTimeout( 5 );
    curl.SetTimeout( 10 );

    // Closing the page must not wait out those timeouts, so the fetch gives up when asked.
    curl.SetTransferCallback(
            [&aCancel]( size_t, size_t, size_t, size_t ) -> int
            {
                return aCancel ? 1 : 0;
            },
            0 );

    if( curl.Perform() != 0 || curl.GetResponseStatusCode() != 200 )
        return {};

    return curl.GetBuffer();
}


// Scale a fetched photo to aDiameter and clip it to a circle so it matches the generated
// initials avatars.
wxBitmap makeCircularBitmap( const wxImage& aImage, int aDiameter )
{
    wxImage scaled = aImage.Scale( aDiameter, aDiameter, wxIMAGE_QUALITY_HIGH );

    if( !scaled.HasAlpha() )
        scaled.InitAlpha();

    // Clear the alpha outside the inscribed circle so the photo shows as a disc, matching the
    // generated initials avatars.
    const double centre = ( aDiameter - 1 ) / 2.0;
    const double radius = aDiameter / 2.0;

    for( int y = 0; y < aDiameter; ++y )
    {
        for( int x = 0; x < aDiameter; ++x )
        {
            double dx = x - centre;
            double dy = y - centre;

            if( dx * dx + dy * dy > radius * radius )
                scaled.SetAlpha( x, y, 0 );
        }
    }

    return wxBitmap( scaled );
}


// The native icon renderer caps the icon size, so the avatar and summary share one cell here
// to let the avatar show at full size.
class HISTORY_AVATAR_RENDERER : public wxDataViewCustomRenderer
{
public:
    HISTORY_AVATAR_RENDERER( int aRowHeight, int aGap ) :
            wxDataViewCustomRenderer( wxT( "wxDataViewIconText" ), wxDATAVIEW_CELL_INERT ),
            m_rowHeight( aRowHeight ),
            m_gap( aGap )
    {
    }

    bool SetValue( const wxVariant& aValue ) override
    {
        m_value << aValue;
        return true;
    }

    bool GetValue( wxVariant& aValue ) const override
    {
        aValue << m_value;
        return true;
    }

    wxSize GetSize() const override { return wxSize( GetOwner()->GetWidth(), m_rowHeight ); }

    bool Render( wxRect aRect, wxDC* aDc, int aState ) override
    {
        RenderBackground( aDc, aRect );

        wxBitmap bmp = m_value.GetBitmapBundle().GetBitmap( wxDefaultSize );
        int      textX = aRect.x;

        if( bmp.IsOk() )
        {
            int y = aRect.y + ( aRect.height - bmp.GetHeight() ) / 2;
            aDc->DrawBitmap( bmp, aRect.x, y, true );
            textX += bmp.GetWidth() + m_gap;
        }

        wxRect textRect( textX, aRect.y, aRect.GetRight() - textX, aRect.height );
        RenderText( m_value.GetText(), 0, textRect, aDc, aState );
        return true;
    }

private:
    wxDataViewIconText m_value;
    int                m_rowHeight;
    int                m_gap;
};


wxString joinNames( const std::vector<wxString>& aNames )
{
    wxString joined;

    for( const wxString& name : aNames )
        joined += ( joined.IsEmpty() ? wxString() : wxString( wxS( ", " ) ) ) + name;

    return joined;
}


wxString fileStatusLabel( KIGIT::FILE_CHANGE_STATUS aStatus )
{
    switch( aStatus )
    {
    case KIGIT::FILE_CHANGE_STATUS::ADDED: return _( "added" );
    case KIGIT::FILE_CHANGE_STATUS::REMOVED: return _( "removed" );
    case KIGIT::FILE_CHANGE_STATUS::RENAMED: return _( "renamed" );
    case KIGIT::FILE_CHANGE_STATUS::COPIED: return _( "copied" );
    case KIGIT::FILE_CHANGE_STATUS::TYPECHANGE: return _( "type changed" );
    default: return _( "modified" );
    }
}

} // namespace


PANEL_GIT_HISTORY::PANEL_GIT_HISTORY( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_HISTORY_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame ),
        m_historyRefsColumn( nullptr ),
        m_fileDiff( nullptr )
{
    m_historySearch->SetDescriptiveText( _( "Search history" ) );
    m_historyPathRow->ShowItems( false );

    // A custom renderer draws the avatar at full size and fixes the row height, since the
    // native icon renderer caps the icon size.
    int rowHeight = m_historyList->FromDIP( 28 );
    int gap = m_historyList->FromDIP( 6 );

    // The row height must be set explicitly. A renderer's reported size does not grow the row.
    m_historyList->SetRowHeight( rowHeight );

    m_historyList->AppendColumn(
            new wxDataViewColumn( _( "Description" ), new HISTORY_AVATAR_RENDERER( rowHeight, gap ), 0,
                                  m_historyList->FromDIP( 320 ), wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE ),
            wxT( "wxDataViewIconText" ) );
    m_historyList->AppendTextColumn( _( "Author" ), wxDATAVIEW_CELL_INERT, m_historyList->FromDIP( 120 ) );
    m_historyList->AppendTextColumn( _( "Date" ), wxDATAVIEW_CELL_INERT, m_historyList->FromDIP( 90 ) );
    m_historyList->AppendTextColumn( _( "Commit" ), wxDATAVIEW_CELL_INERT, m_historyList->FromDIP( 90 ) );
    m_historyRefsColumn =
            m_historyList->AppendTextColumn( _( "Refs" ), wxDATAVIEW_CELL_INERT, m_historyList->FromDIP( 120 ) );

    m_commitFiles->AppendColumn( _( "File" ), wxLIST_FORMAT_LEFT, FromDIP( 220 ) );
    m_commitFiles->AppendColumn( _( "Status" ), wxLIST_FORMAT_LEFT, FromDIP( 90 ) );
    m_commitFiles->AppendColumn( wxS( "+" ), wxLIST_FORMAT_RIGHT, FromDIP( 50 ) );
    m_commitFiles->AppendColumn( wxS( "-" ), wxLIST_FORMAT_RIGHT, FromDIP( 50 ) );

    // The line counts take the colours the diff uses, lifted a little on a dark background.
    KICAD_DIFF::DIFF_COLOR_THEME theme;
    bool                         dark = KIPLATFORM::UI::IsDarkTheme();
    m_commitAdded->SetForegroundColour( ( dark ? theme.added.Brightened( 0.4 ) : theme.added ).ToColour() );
    m_commitRemoved->SetForegroundColour( ( dark ? theme.removed.Brightened( 0.4 ) : theme.removed ).ToColour() );

    // The detail pane appears once a commit is selected, and the diff area once a file is picked.
    m_historySashPos = FromDIP( 460 );
    m_historySplitter->Unsplit( m_commitDetail );
    m_detailSashPos = FromDIP( 140 );
    m_fileDiff = new GIT_FILE_DIFF_AREA( m_detailSplitter, m_frame );
    m_fileDiff->Hide();

    m_retryTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                refresh();
            },
            m_retryTimer.GetId() );

    m_historySearchTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                reloadHistory();
            },
            m_historySearchTimer.GetId() );

    m_historyScrollTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                onHistoryScrollTick();
            },
            m_historyScrollTimer.GetId() );

    m_showAllButton->Bind( wxEVT_BUTTON,
                           [this]( wxCommandEvent& )
                           {
                               ShowHistoryForFile( wxEmptyString );
                           } );

    // Walking the whole history for a search text that matches nothing takes a moment in a big
    // repository, so wait for the typing to pause.
    m_historySearch->Bind( wxEVT_TEXT,
                           [this]( wxCommandEvent& )
                           {
                               m_historySearchTimer.Start( 300, wxTIMER_ONE_SHOT );
                           } );

    m_historyBranch->Bind( wxEVT_CHOICE,
                           [this]( wxCommandEvent& )
                           {
                               int sel = m_historyBranch->GetSelection();
                               m_historyBranchName = sel > 0 ? m_historyBranch->GetString( sel ) : wxString();
                               reloadHistory();
                           } );

    m_historyShowRefs->Bind( wxEVT_CHECKBOX,
                             [this]( wxCommandEvent& )
                             {
                                 m_historyRefsColumn->SetHidden( !m_historyShowRefs->IsChecked() );
                             } );

    m_historyList->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED,
                         [this]( wxDataViewEvent& )
                         {
                             if( m_fillingHistory )
                                 return;

                             int      row = m_historyList->GetSelectedRow();
                             wxString hash = row == wxNOT_FOUND ? wxString() : m_historyList->GetTextValue( row, 3 );
                             showCommitDetail( hash );
                         } );

    // Double-clicking a commit shows what it changed against its parent.
    m_historyList->SetToolTip( _( "Double click a commit to see what it changed. Right click for more." ) );
    m_historyList->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED,
                         [this]( wxDataViewEvent& aEvent )
                         {
                             int row = m_historyList->ItemToRow( aEvent.GetItem() );

                             if( row == wxNOT_FOUND )
                                 return;

                             wxString hash = m_historyList->GetTextValue( row, 3 );

                             if( !hash.IsEmpty() )
                                 ShowGitCommitDiff( m_frame, hash, this );
                         } );

    // Amending is offered on the newest commit only, and only while it has not been pushed.
    m_historyList->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                         [this]( wxDataViewEvent& aEvent )
                         {
                             int row = m_historyList->ItemToRow( aEvent.GetItem() );

                             if( row == wxNOT_FOUND )
                                 return;

                             wxString hash = m_historyList->GetTextValue( row, 3 );

                             if( hash.IsEmpty() )
                                 return;

                             wxMenu menu;

                             // Amending rewrites the newest commit, so it is offered there only.
                             if( row == 0 )
                                 menu.Append( 1, _( "Amend Last Commit..." ) );

                             menu.Append( 2, _( "Restore This Version" ) );

                             menu.Bind(
                                     wxEVT_MENU,
                                     [this]( wxCommandEvent& )
                                     {
                                         onAmendLastCommit();
                                     },
                                     1 );
                             menu.Bind(
                                     wxEVT_MENU,
                                     [this, hash]( wxCommandEvent& )
                                     {
                                         onRestoreVersion( hash );
                                     },
                                     2 );
                             m_historyList->PopupMenu( &menu );
                         } );

    // Picking a changed file shows what the commit did to it under the list.
    m_commitFiles->Bind( wxEVT_LIST_ITEM_SELECTED,
                         [this]( wxListEvent& aEvent )
                         {
                             if( !m_fillingHistory )
                                 showFileDiff( m_commitFiles->GetItemText( aEvent.GetIndex(), 0 ) );
                         } );

    m_commitFiles->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK,
                         [this]( wxListEvent& aEvent )
                         {
                             wxMenu menu;
                             menu.Append( 1, _( "Show History for This File" ) );

                             if( GetPopupMenuSelectionFromUser( menu ) == 1 )
                                 ShowHistoryForFile( m_commitFiles->GetItemText( aEvent.GetIndex(), 0 ) );
                         } );

    // Double-clicking opens the full comparison window for it.
    m_commitFiles->SetToolTip( _( "Double click a file to see what the commit changed in it." ) );
    m_commitFiles->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                         [this]( wxListEvent& aEvent )
                         {
                             ShowGitCommitDiff( m_frame, m_detailHash, this,
                                                m_commitFiles->GetItemText( aEvent.GetIndex(), 0 ) );
                         } );
}


PANEL_GIT_HISTORY::~PANEL_GIT_HISTORY()
{
    m_avatarCancel = true;

    for( std::thread& worker : m_avatarWorkers )
        worker.join();
}


void PANEL_GIT_HISTORY::ShowHistoryForFile( const wxString& aPath )
{
    m_historyPath = aPath;
    m_historyPathLabel->SetLabel( aPath );
    m_historyPathRow->ShowItems( !aPath.IsEmpty() );
    Layout();
    reloadHistory();
}


KIGIT_HISTORY_FILTER PANEL_GIT_HISTORY::historyFilter() const
{
    KIGIT_HISTORY_FILTER filter;
    filter.branch = m_historyBranchName;
    filter.search = m_historySearch->GetValue();
    filter.path = m_historyPath;
    return filter;
}


void PANEL_GIT_HISTORY::reloadHistory()
{
    m_historyLimit = 50;
    refresh();
}


void PANEL_GIT_HISTORY::refresh()
{
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    if( !lock.owns_lock() )
    {
        m_retryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }

    Populate();
}


void PANEL_GIT_HISTORY::SetBranches( const std::vector<std::pair<wxString, bool>>& aBranches )
{
    m_historyBranch->Clear();
    m_historyBranch->Append( _( "All Branches" ) );

    for( const auto& [name, isRemote] : aBranches )
        m_historyBranch->Append( name );

    if( m_historyBranchName.IsEmpty() || !m_historyBranch->SetStringSelection( m_historyBranchName ) )
    {
        m_historyBranch->SetSelection( 0 );

        // The branch the list was narrowed to is gone, so the list is empty. Show everything.
        if( !m_historyBranchName.IsEmpty() )
        {
            m_historyBranchName.clear();
            Populate();
        }
    }

    m_historyBranch->InvalidateBestSize();
    m_historyBranch->GetParent()->Layout();
}


void PANEL_GIT_HISTORY::showCommitDetail( const wxString& aHash )
{
    m_detailHash = aHash;

    if( aHash.IsEmpty() )
    {
        clearCommitDetail();
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    // The reload puts the selection back on this commit and fills the pane then.
    if( !lock.owns_lock() )
    {
        m_retryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }

    fillCommitDetail( common->GetCommitDetail( aHash ) );
}


void PANEL_GIT_HISTORY::fillCommitDetail( const KIGIT_COMMIT_DETAIL& aDetail )
{
    if( !aDetail.found )
    {
        clearCommitDetail();
        return;
    }

    wxString title = aDetail.shortHash;

    for( const wxString& ref : aDetail.refs )
        title += wxS( "  [" ) + ref + wxS( "]" );

    m_commitTitle->SetLabel( title );
    m_commitAuthor->SetLabel( wxString::Format( wxS( "%s <%s>, %s" ), aDetail.author, aDetail.email,
                                                KIGIT::RelativeDate( aDetail.when ) ) );

    // The same size the list uses, so a photo fetched for a row serves the pane too.
    bool showAvatars = Pgm().GetCommonSettings()->m_Git.showContributorAvatars;
    int  diameter = m_historyList->FromDIP( 20 );
    auto cached = m_avatarCache.find( aDetail.email );

    m_detailEmail = aDetail.email;

    if( showAvatars && cached != m_avatarCache.end() && cached->second.IsOk() )
        m_commitAvatar->SetBitmap( cached->second );
    else
        m_commitAvatar->SetBitmap( makeAvatarBitmap( m_commitDetail, aDetail.author, diameter, diameter ) );

    if( showAvatars )
        requestAvatar( aDetail.email, diameter );

    m_commitMessage->ChangeValue( aDetail.message );

    wxString parents = aDetail.parents.empty() ? _( "No parent" )
                                               : wxString::Format( _( "Parent: %s" ), joinNames( aDetail.parents ) );

    m_detailParent = aDetail.parents.empty() ? wxString() : aDetail.parents.front();
    hideFileDiff();

    int      fileCount = static_cast<int>( aDetail.files.size() );
    wxString files = fileCount == 1 ? _( "1 changed file" ) : wxString::Format( _( "%d changed files" ), fileCount );

    m_commitStats->SetLabel( parents );
    m_commitFileCount->SetLabel( files );
    m_commitAdded->SetLabel( wxString::Format( wxS( "+%d" ), aDetail.added ) );
    m_commitRemoved->SetLabel( wxString::Format( wxS( "-%d" ), aDetail.removed ) );

    m_commitFiles->DeleteAllItems();

    for( const KIGIT_COMMIT_FILE& file : aDetail.files )
    {
        long idx = m_commitFiles->InsertItem( m_commitFiles->GetItemCount(), file.path );

        wxString status = fileStatusLabel( file.status );

        if( !file.oldPath.IsEmpty() )
            status = wxString::Format( _( "%s from %s" ), status, file.oldPath );

        m_commitFiles->SetItem( idx, 1, status );

        if( file.binary )
        {
            m_commitFiles->SetItem( idx, 2, _( "binary" ) );
        }
        else
        {
            m_commitFiles->SetItem( idx, 2, wxString::Format( wxS( "%d" ), file.added ) );
            m_commitFiles->SetItem( idx, 3, wxString::Format( wxS( "%d" ), file.removed ) );
        }
    }

    // The caller holds the git action lock and the render needs it too, so the file is picked
    // with the handler quiet and the render follows once the lock is free.
    if( !m_historyPath.IsEmpty() )
    {
        long row = m_commitFiles->FindItem( -1, m_historyPath );

        if( row != wxNOT_FOUND )
        {
            m_fillingHistory = true;
            m_commitFiles->SetItemState( row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
            m_fillingHistory = false;

            CallAfter(
                    [this, path = m_historyPath]()
                    {
                        showFileDiff( path );
                    } );
        }
    }

    if( !m_historySplitter->IsSplit() )
        m_historySplitter->SplitVertically( m_historyListPanel, m_commitDetail, m_historySashPos );

    m_commitDetail->Layout();
}


void PANEL_GIT_HISTORY::clearCommitDetail()
{
    KIGIT::UnsplitKeepingSash( m_historySplitter, m_commitDetail, m_historySashPos );
    hideFileDiff();
    m_detailEmail.clear();
    m_commitTitle->SetLabel( wxEmptyString );
    m_commitAuthor->SetLabel( wxEmptyString );
    m_commitMessage->ChangeValue( wxEmptyString );
    m_commitStats->SetLabel( wxEmptyString );
    m_commitFileCount->SetLabel( wxEmptyString );
    m_commitAdded->SetLabel( wxEmptyString );
    m_commitRemoved->SetLabel( wxEmptyString );
    m_commitFiles->DeleteAllItems();
}


void PANEL_GIT_HISTORY::hideFileDiff()
{
    m_fileDiff->Clear();
    KIGIT::UnsplitKeepingSash( m_detailSplitter, m_fileDiff, m_detailSashPos );
}


void PANEL_GIT_HISTORY::showFileDiff( const wxString& aPath )
{
    if( aPath.IsEmpty() || m_detailHash.IsEmpty() )
    {
        hideFileDiff();
        return;
    }

    wxString beforeLabel =
            m_detailParent.IsEmpty() ? _( "BASE (nothing)" ) : wxString::Format( _( "BASE (%s)" ), m_detailParent );
    wxString afterLabel = wxString::Format( _( "WORKING (%s)" ), m_detailHash );

    m_fileDiff->ShowFile(
            [&]( GIT_DIFF_WORKSPACE& aWorkspace, GIT_FILE_SIDES& aSides )
            {
                return MaterializeCommitFile( m_frame, aWorkspace, m_detailHash, aPath, aSides );
            },
            beforeLabel, afterLabel );

    if( !m_detailSplitter->IsSplit() )
        m_detailSplitter->SplitHorizontally( m_commitFilesPanel, m_fileDiff, m_detailSashPos );
}


void PANEL_GIT_HISTORY::Populate()
{
    // The caller holds the git action lock.
    std::vector<KIGIT_COMMIT_INFO> commits =
            m_frame->Prj().GetGitCommon()->GetRecentCommits( m_historyLimit, historyFilter() );

    // Emptying the list drops its selection, which clears the shown hash, so keep it aside.
    wxString shown = m_detailHash;

    m_fillingHistory = true;
    m_historyList->DeleteAllItems();
    m_historyEmails.clear();

    appendHistoryRows( commits, 0 );

    m_historyComplete = commits.size() < m_historyLimit;

    // Put the selection back on the commit the detail pane showed, if it is still listed, or
    // start on the newest commit so the pane is never empty while there is something to show.
    m_detailHash.clear();

    for( int row = 0; row < m_historyList->GetItemCount() && !shown.IsEmpty(); ++row )
    {
        if( m_historyList->GetTextValue( row, 3 ) == shown )
        {
            m_historyList->SelectRow( row );
            m_detailHash = shown;
            break;
        }
    }

    if( m_detailHash.IsEmpty() && m_historyList->GetItemCount() > 0 )
    {
        m_historyList->SelectRow( 0 );
        m_detailHash = m_historyList->GetTextValue( 0, 3 );
    }

    m_fillingHistory = false;

    if( m_detailHash.IsEmpty() )
        clearCommitDetail();
    else
        fillCommitDetail( m_frame->Prj().GetGitCommon()->GetCommitDetail( m_detailHash ) );

    // Scrolling near the end grows the window. Poll for it, because native wxDataViewCtrl
    // ports deliver no portable scroll events.
    if( m_historyComplete )
        m_historyScrollTimer.Stop();
    else if( !m_historyScrollTimer.IsRunning() )
        m_historyScrollTimer.Start( 250 );
}


void PANEL_GIT_HISTORY::appendHistoryRows( const std::vector<KIGIT_COMMIT_INFO>& aCommits, size_t aFrom )
{
    bool showAvatars = Pgm().GetCommonSettings()->m_Git.showContributorAvatars;

    // The custom renderer centers this avatar in the row, so the bitmap is just the circle.
    int diameter = m_historyList->FromDIP( 20 );

    for( size_t ii = aFrom; ii < aCommits.size(); ++ii )
    {
        const KIGIT_COMMIT_INFO& c = aCommits[ii];

        wxBitmap avatar;
        auto     cached = m_avatarCache.find( c.email );

        if( showAvatars && cached != m_avatarCache.end() && cached->second.IsOk() )
            avatar = cached->second;
        else
            avatar = makeAvatarBitmap( m_historyList, c.author, diameter, diameter );

        wxVariant iconText;
        iconText << wxDataViewIconText( c.summary, wxBitmapBundle::FromBitmap( avatar ) );

        wxVector<wxVariant> row;
        row.push_back( iconText );
        row.push_back( wxVariant( c.author ) );
        row.push_back( wxVariant( KIGIT::RelativeDate( c.when ) ) );
        row.push_back( wxVariant( c.hash ) );
        row.push_back( wxVariant( joinNames( c.refs ) ) );
        m_historyList->AppendItem( row );

        m_historyEmails.push_back( c.email );

        if( showAvatars )
            requestAvatar( c.email, diameter );
    }
}


void PANEL_GIT_HISTORY::onHistoryScrollTick()
{
    if( m_historyComplete || !m_historyList || !m_historyList->IsShownOnScreen() )
        return;

    int itemCount = m_historyList->GetItemCount();

    if( itemCount == 0 )
        return;

    // Which row sits at the bottom of the view right now? HitTest works the same way whether
    // the user scrolled with the wheel, the scrollbar or the keyboard.
    wxDataViewItem    item;
    wxDataViewColumn* column = nullptr;
    wxSize            client = m_historyList->GetClientSize();

    m_historyList->HitTest( wxPoint( 8, client.y - 8 ), item, column );

    int row = item.IsOk() ? m_historyList->ItemToRow( item ) : wxNOT_FOUND;

    // Past the last row (short list or overscroll) also counts as "at the end".
    if( row != wxNOT_FOUND && row < itemCount - 5 )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    // Busy: try again on the next tick.
    if( !lock.owns_lock() )
        return;

    m_historyLimit += 50;

    std::vector<KIGIT_COMMIT_INFO> commits = common->GetRecentCommits( m_historyLimit, historyFilter() );

    lock.unlock();

    // Append only the new rows, so the scroll position stays where the user is reading.
    appendHistoryRows( commits, itemCount );

    m_historyComplete = commits.size() < m_historyLimit;

    if( m_historyComplete )
        m_historyScrollTimer.Stop();
}


void PANEL_GIT_HISTORY::requestAvatar( const wxString& aEmail, int aDiameter )
{
    if( aEmail.IsEmpty() || m_avatarCache.count( aEmail ) || m_avatarRequested.count( aEmail ) )
        return;

    // Each fetch is its own network round trip, so cap how many a history page can start. The
    // rest keep their generated initials avatar.
    const size_t maxAvatarFetches = 24;

    if( m_avatarRequested.size() >= maxAvatarFetches )
        return;

    m_avatarRequested.insert( aEmail );

    wxString email = aEmail;
    int      fetchPx = aDiameter * 2;

    // Never the shared compute pool. A network read parked there stalls DRC, zone filling
    // and library loading.
    m_avatarWorkers.emplace_back(
            [this, email, aDiameter, fetchPx]()
            {
                std::string bytes = fetchGravatarBytes( email, fetchPx, m_avatarCancel );

                CallAfter(
                        [this, email, aDiameter, bytes]()
                        {
                            applyAvatar( email, aDiameter, bytes );
                        } );
            } );
}


void PANEL_GIT_HISTORY::applyAvatar( const wxString& aEmail, int aDiameter, const std::string& aBytes )
{
    if( aBytes.empty() )
        return;

    wxMemoryInputStream stream( aBytes.data(), aBytes.size() );
    wxImage             image;

    if( !image.LoadFile( stream, wxBITMAP_TYPE_ANY ) )
        return;

    wxBitmap circle = makeCircularBitmap( image, aDiameter );
    m_avatarCache[aEmail] = circle;

    if( aEmail == m_detailEmail )
        m_commitAvatar->SetBitmap( circle );

    for( unsigned row = 0; row < m_historyEmails.size(); ++row )
    {
        if( m_historyEmails[row] != aEmail )
            continue;

        wxVariant value;
        m_historyList->GetValue( value, row, 0 );

        wxDataViewIconText iconText;
        iconText << value;

        wxDataViewIconText updated( iconText.GetText(), wxBitmapBundle::FromBitmap( circle ) );
        wxVariant          updatedValue;
        updatedValue << updated;
        m_historyList->SetValue( updatedValue, row, 0 );
    }
}


void PANEL_GIT_HISTORY::onAmendLastCommit()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    wxString lastMessage;
    bool     hasLocalCommits = false;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        hasLocalCommits = common->HasLocalCommits();
        lastMessage = common->GetLastCommitMessage();
        lastMessage.Trim();
    }

    // Rewriting a commit that is already on the remote would diverge the branch, so only an
    // unpushed commit can be amended.
    if( !hasLocalCommits )
    {
        DisplayInfoMessage( this, _( "The last commit has already been pushed, so it cannot be amended." ) );
        return;
    }

    wxTextEntryDialog dlg( this, _( "Commit message:" ), _( "Amend Last Commit" ), lastMessage,
                           wxTextEntryDialogStyle | wxTE_MULTILINE );

    if( dlg.ShowModal() != wxID_OK || dlg.GetValue().Trim().IsEmpty() )
        return;

    wxString message = dlg.GetValue();

    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    m_dialog->RunGitAction(
            [message]( KIGIT_COMMON& aCommon )
            {
                GIT_CONFIG_HANDLER configHandler( &aCommon );
                GitUserConfig      userConfig = configHandler.GetUserConfig();

                GIT_COMMIT_HANDLER commitHandler( aCommon.GetRepo() );
                CommitResult       result =
                        commitHandler.PerformAmend( {}, message, userConfig.authorName, userConfig.authorEmail );

                // The amend exists on the stale-index path, so it is not a failure.
                return result == CommitResult::Success || result == CommitResult::SuccessIndexStale;
            },
            _( "Could not amend the commit." ) );
}


void PANEL_GIT_HISTORY::onRestoreVersion( const wxString& aCommitHash )
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    // Edits still in an editor's memory pass the backend's disk check, and the reload prompt would
    // then ask about them only after the restore commit exists, too late to choose.
    for( FRAME_T type : { FRAME_SCH, FRAME_PCB_EDITOR } )
    {
        KIWAY_PLAYER* player = m_frame->Kiway().Player( type, false );

        if( player && player->IsContentModified() )
        {
            DisplayInfoMessage( this, _( "There are unsaved changes in the open editors. Commit them "
                                         "first, so the restore stays a separate step that can be "
                                         "undone." ) );
            return;
        }
    }

    if( !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
    {
        DisplayInfoMessage( this, _( "A library editor has unsaved changes. Save or discard them first, so "
                                     "the restore does not overwrite what it is editing." ) );
        return;
    }

    bool     detached = false;
    wxString lfsExample;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        detached = common->IsHeadDetached();

        wxString prefix = KIGIT::PROJECT_GIT_UTILS::ProjectPrefix( common->GetProjectDir(),
                                                                   m_frame->Prj().GetProjectPath() );
        KIGIT::PROJECT_GIT_UTILS::ProjectUsesGitLfs( common->GetRepo(), prefix, &lfsExample );
    }

    // A restore writes the committed version over the files on disk, and for an LFS file that
    // is the placeholder rather than the design.
    if( !lfsExample.IsEmpty() )
    {
        DisplayErrorMessage( this, wxString::Format( _( "This project keeps '%s' in Git LFS, which KiCad "
                                                        "cannot read or write. Use git on the command line "
                                                        "for this repository." ),
                                                     lfsExample ) );
        return;
    }

    // A restore commit made on a detached HEAD belongs to no branch and is lost on the next
    // switch.
    if( detached )
    {
        DisplayInfoMessage( this, _( "This project is showing an old snapshot rather than a branch. "
                                     "Double click a branch on the Branches page first, then restore." ) );
        return;
    }

    // Restore only this project's folder. A repository can hold several projects, and rewinding
    // the whole tree from one project's window would rewind the others too.
    wxString projectPrefix;
    bool     prefixKnown = false;

    {
        wxString workDir = common->GetProjectDir();
        wxString projectPath = m_frame->Prj().GetProjectPath();

        workDir.Replace( wxS( "\\" ), wxS( "/" ) );
        projectPath.Replace( wxS( "\\" ), wxS( "/" ) );

        if( !workDir.EndsWith( wxS( "/" ) ) )
            workDir += wxS( "/" );

        if( projectPath.StartsWith( workDir ) )
        {
            projectPrefix = projectPath.Mid( workDir.length() );

            while( projectPrefix.EndsWith( wxS( "/" ) ) )
                projectPrefix.RemoveLast();

            prefixKnown = true;
        }
    }

    // An empty prefix means the whole tree, which is right for a project at the repository root
    // but wrong when the project's folder could not be worked out at all.
    if( !prefixKnown )
    {
        DisplayErrorMessage( this, _( "Could not work out which folder this project occupies in the "
                                      "repository, so nothing was restored." ) );
        return;
    }

    wxRichMessageDialog confirm( this,
                                 wxString::Format( _( "Restore this project's files as they were at version %s?\n\n"
                                                      "They are replaced with that version, recorded as a new "
                                                      "commit, so nothing in the history is lost. Files outside "
                                                      "the project folder, including any shared with other "
                                                      "projects, keep their current state." ),
                                                   aCommitHash ),
                                 _( "Restore Version" ), wxYES_NO | wxICON_QUESTION | wxCENTER );
    confirm.SetYesNoLabels( _( "&Restore" ), _( "Cancel" ) );

    if( confirm.ShowModal() != wxID_YES )
        return;

    // The restore commit needs an identity to be recorded with.
    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    // The restore rewrites files the editors have open, so snapshot them for a scoped reload.
    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( m_frame->Kiway() );

    RestoreResult result = RestoreResult::Error;
    wxString      error;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        result = common->RestoreVersion( aCommitHash, projectPrefix, error );
    }

    switch( result )
    {
    case RestoreResult::Restored:
        ReloadOpenEditors( m_frame->Kiway(), docsBefore );
        common->NotifyRefreshListeners();
        break;

    case RestoreResult::AlreadyCurrent:
        DisplayInfoMessage( this, _( "The project already matches that version." ) );
        break;

    case RestoreResult::Dirty:
    {
        // The restore checks the whole repository, while the checklist shows this project only.
        std::vector<wxString> outside;

        {
            std::unique_lock<std::mutex> lock = common->TryLockAction();

            if( lock.owns_lock() )
                outside = ChangedTrackedFilesOutsideProject( m_frame->Prj() );
        }

        if( !outside.empty() )
        {
            wxString listed;

            for( const wxString& path : outside )
                listed += wxS( "\n    " ) + path;

            DisplayInfoMessage( this, wxString::Format( _( "There are uncommitted changes elsewhere in this "
                                                           "repository:%s\n\nCommit or discard them from that "
                                                           "project first, or set them aside with Stash "
                                                           "Changes." ),
                                                        listed ) );
        }
        else
        {
            DisplayInfoMessage( this, _( "There are uncommitted changes. Commit them first, so the "
                                         "restore stays a separate step that can be undone." ) );
        }

        break;
    }

    case RestoreResult::Error: DisplayErrorMessage( this, _( "Could not restore that version." ), error ); break;
    }
}
