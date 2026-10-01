/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/git/git_advanced_dialog.h>

#include <bitmaps.h>
#include <dialogs/git/git_status_entries.h>

#include <memory>
#include <vector>

#include <wx/app.h>
#include <wx/arrstr.h>
#include <wx/bitmap.h>
#include <wx/bmpbndl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dataview.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/mstream.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/richmsgdlg.h>
#include <wx/textdlg.h>
#include <wx/treectrl.h>

#include <picosha2.h>

#include <confirm.h>
#include <eda_base_frame.h>
#include <frame_type.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <git/git_branch_handler.h>
#include <git/git_commit_handler.h>
#include <git/git_config_handler.h>
#include <git/git_pull_handler.h>
#include <git/git_push_handler.h>
#include <git/kicad_git_common.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <kiway.h>
#include <kiway_player.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <thread>
#include <widgets/git_commit_pane.h>
#include <widgets/ui_common.h>
#include <widgets/wx_treebook.h>


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


wxString relativeDate( const wxDateTime& aWhen )
{
    if( !aWhen.IsValid() )
        return wxEmptyString;

    wxTimeSpan diff = wxDateTime::Now() - aWhen;

    if( diff.GetSeconds() < 60 )
        return _( "just now" );

    if( diff.GetMinutes() < 60 )
        return wxString::Format( _( "%d minutes ago" ), static_cast<int>( diff.GetMinutes() ) );

    if( diff.GetHours() < 24 )
        return wxString::Format( _( "%d hours ago" ), static_cast<int>( diff.GetHours() ) );

    if( diff.GetDays() < 30 )
        return wxString::Format( _( "%d days ago" ), static_cast<int>( diff.GetDays() ) );

    return aWhen.FormatISODate();
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

    // Closing the window must not wait out those timeouts, so the fetch gives up when asked.
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


// The stock wxListCtrl keeps these pages looking like the rest of KiCad.
struct LIST_PAGE
{
    wxPanel*    panel;
    wxListCtrl* list;
    wxBoxSizer* sizer;
};


LIST_PAGE makeListPage( wxWindow* aBook )
{
    LIST_PAGE page;
    page.panel = new wxPanel( aBook );
    page.sizer = new wxBoxSizer( wxVERTICAL );

    page.list = new wxListCtrl( page.panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL );

    page.sizer->Add( page.list, 1, wxEXPAND | wxALL, 4 );
    page.panel->SetSizer( page.sizer );
    return page;
}


wxWindow* makeHistoryPage( wxWindow* aBook, wxDataViewListCtrl** aOutList )
{
    wxPanel*    panel = new wxPanel( aBook );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // A custom renderer draws the avatar at full size and fixes the row height, since the
    // native icon renderer caps the icon size.
    wxDataViewListCtrl* list = new wxDataViewListCtrl( panel, wxID_ANY );

    int rowHeight = list->FromDIP( 28 );
    int gap = list->FromDIP( 6 );

    // The row height must be set explicitly. A renderer's reported size does not grow the row.
    list->SetRowHeight( rowHeight );

    list->AppendColumn( new wxDataViewColumn( _( "Description" ), new HISTORY_AVATAR_RENDERER( rowHeight, gap ), 0,
                                              list->FromDIP( 320 ), wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE ),
                        wxT( "wxDataViewIconText" ) );
    list->AppendTextColumn( _( "Author" ), wxDATAVIEW_CELL_INERT, list->FromDIP( 120 ) );
    list->AppendTextColumn( _( "Date" ), wxDATAVIEW_CELL_INERT, list->FromDIP( 90 ) );
    list->AppendTextColumn( _( "Commit" ), wxDATAVIEW_CELL_INERT, list->FromDIP( 90 ) );

    sizer->Add( list, 1, wxEXPAND | wxALL, 4 );
    panel->SetSizer( sizer );

    *aOutList = list;
    return panel;
}


wxWindow* makeBranchesPage( wxWindow* aBook, wxListCtrl** aOutList )
{
    LIST_PAGE page = makeListPage( aBook );

    page.list->AppendColumn( _( "Branch" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 300 ) );
    page.list->AppendColumn( _( "Status" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 120 ) );

    *aOutList = page.list;
    return page.panel;
}


wxWindow* makeRemotesPage( wxWindow* aBook, wxListCtrl** aOutList )
{
    LIST_PAGE page = makeListPage( aBook );

    page.list->AppendColumn( _( "Name" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 110 ) );
    page.list->AppendColumn( _( "URL" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 420 ) );

    *aOutList = page.list;
    return page.panel;
}


wxWindow* makeStashesPage( wxWindow* aBook, wxListCtrl** aOutList )
{
    LIST_PAGE page = makeListPage( aBook );

    page.list->AppendColumn( _( "Description" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 400 ) );
    page.list->AppendColumn( _( "Date" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 130 ) );

    *aOutList = page.list;
    return page.panel;
}


wxWindow* makeTagsPage( wxWindow* aBook, wxListCtrl** aOutList )
{
    LIST_PAGE page = makeListPage( aBook );

    page.list->AppendColumn( _( "Tag" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 90 ) );
    page.list->AppendColumn( _( "Commit" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 90 ) );
    page.list->AppendColumn( _( "Description" ), wxLIST_FORMAT_LEFT, page.list->FromDIP( 330 ) );

    *aOutList = page.list;
    return page.panel;
}


wxWindow* makeRepositoryPage( wxWindow* aBook, wxTextCtrl** aName, wxTextCtrl** aEmail, wxStaticText** aRemote )
{
    wxPanel*    panel = new wxPanel( aBook );
    wxBoxSizer* outer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer* grid = new wxFlexGridSizer( 3, 2, 8, 12 );
    grid->AddGrowableCol( 1 );

    *aName = new wxTextCtrl( panel, wxID_ANY, wxEmptyString );
    *aEmail = new wxTextCtrl( panel, wxID_ANY, wxEmptyString );
    *aRemote = new wxStaticText( panel, wxID_ANY, wxEmptyString );

    grid->Add( new wxStaticText( panel, wxID_ANY, _( "Name:" ) ), 0, wxALIGN_CENTER_VERTICAL );
    grid->Add( *aName, 1, wxEXPAND );
    grid->Add( new wxStaticText( panel, wxID_ANY, _( "Email:" ) ), 0, wxALIGN_CENTER_VERTICAL );
    grid->Add( *aEmail, 1, wxEXPAND );
    grid->Add( new wxStaticText( panel, wxID_ANY, _( "Remote:" ) ), 0, wxALIGN_CENTER_VERTICAL );
    grid->Add( *aRemote, 1, wxEXPAND );

    outer->Add( grid, 0, wxEXPAND | wxALL, 4 );

    outer->Add( new wxStaticText( panel, wxID_ANY,
                                  _( "Name and email apply to this project only. Preferences holds the default "
                                     "used when these are empty." ) ),
                0, wxALL, 4 );

    wxBoxSizer* pad = new wxBoxSizer( wxVERTICAL );
    pad->Add( outer, 1, wxEXPAND | wxALL, 8 );
    panel->SetSizer( pad );
    return panel;
}

} // namespace


GIT_ADVANCED_DIALOG::GIT_ADVANCED_DIALOG( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Git" ), wxDefaultPosition, wxSize( 780, 560 ),
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, wxT( "GitAdvancedDialog" ) ),
        m_parent( aParent ),
        m_pane( aPane ),
        m_book( nullptr ),
        m_historyList( nullptr ),
        m_branchesList( nullptr ),
        m_remotesList( nullptr ),
        m_stashesList( nullptr ),
        m_tagsList( nullptr ),
        m_repoUserName( nullptr ),
        m_repoUserEmail( nullptr ),
        m_repoRemoteUrl( nullptr ),
        m_repoName( nullptr ),
        m_branchChoice( nullptr ),
        m_fetchButton( nullptr ),
        m_pullButton( nullptr ),
        m_pushButton( nullptr )
{
    m_hash_key = "GitAdvancedDialog";

    m_reloadRetryTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                ReloadPages();
            },
            m_reloadRetryTimer.GetId() );

    m_historyScrollTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                onHistoryScrollTick();
            },
            m_historyScrollTimer.GetId() );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_book = new WX_TREEBOOK( this, wxID_ANY );
    m_book->SetFont( KIUI::GetControlFont( this ) );
    m_book->SetFitToCurrentPage( true );

    // One icon per page, in page order, as the About dialog does for its tabs.
    wxVector<wxBitmapBundle> pageIcons;
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_history, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_branches, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_remotes, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_stashes, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_tags, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_repository, 16 ) );
    m_book->SetImages( pageIcons );

    m_book->AddPage( makeHistoryPage( m_book, &m_historyList ), _( "History" ), true, 0 );

    wxWindow* branchesPage = makeBranchesPage( m_book, &m_branchesList );
    addButtonRow( branchesPage, { { _( "New Branch..." ),
                                    [this]()
                                    {
                                        onNewBranch();
                                    } },
                                  { _( "Delete" ),
                                    [this]()
                                    {
                                        onDeleteBranch();
                                    } },
                                  { _( "Force Push..." ), [this]()
                                    {
                                        onForcePush();
                                    } } } );
    m_book->AddPage( branchesPage, _( "Branches" ), false, 1 );

    wxWindow* remotesPage = makeRemotesPage( m_book, &m_remotesList );
    addButtonRow( remotesPage, { { _( "Add..." ),
                                   [this]()
                                   {
                                       onAddRemote();
                                   } },
                                 { _( "Edit URL..." ),
                                   [this]()
                                   {
                                       onEditRemote();
                                   } },
                                 { _( "Remove" ), [this]()
                                   {
                                       onRemoveRemote();
                                   } } } );
    m_book->AddPage( remotesPage, _( "Remotes" ), false, 2 );

    wxWindow* stashesPage = makeStashesPage( m_book, &m_stashesList );
    addButtonRow( stashesPage, { { _( "Stash Changes..." ),
                                   [this]()
                                   {
                                       onStashSave();
                                   } },
                                 { _( "Apply" ),
                                   [this]()
                                   {
                                       onStashApply();
                                   } },
                                 { _( "Pop" ),
                                   [this]()
                                   {
                                       onStashPop();
                                   } },
                                 { _( "Drop" ), [this]()
                                   {
                                       onStashDrop();
                                   } } } );
    m_book->AddPage( stashesPage, _( "Stashes" ), false, 3 );

    wxWindow* tagsPage = makeTagsPage( m_book, &m_tagsList );
    addButtonRow( tagsPage, { { _( "New Tag..." ),
                                [this]()
                                {
                                    onNewTag();
                                } },
                              { _( "Check Out" ),
                                [this]()
                                {
                                    onCheckoutTag();
                                } },
                              { _( "Delete" ), [this]()
                                {
                                    onDeleteTag();
                                } } } );
    m_book->AddPage( tagsPage, _( "Tags" ), false, 4 );

    wxWindow* repositoryPage = makeRepositoryPage( m_book, &m_repoUserName, &m_repoUserEmail, &m_repoRemoteUrl );
    addButtonRow( repositoryPage, { { _( "Save" ), [this]()
                                    {
                                        onSaveRepositoryConfig();
                                    } } } );
    m_book->AddPage( repositoryPage, _( "Repository" ), false, 5 );

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
                                 ShowGitCommitDiff( m_parent, hash, this );
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

                             menu.Append( 2, _( "Restore This Version..." ) );

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

    // Double-clicking a branch switches the working tree to it.
    m_branchesList->SetToolTip( _( "Double click a branch to switch the project to it." ) );
    m_branchesList->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                          [this]( wxListEvent& aEvent )
                          {
                              switchToBranch( m_branchesList->GetItemText( aEvent.GetIndex(), 0 ) );
                          } );

    // The top bar names the repository and branch and holds the remote actions. Pull and Push
    // run through the docked panel, which owns the merge and its conflict handling.
    wxBoxSizer* topBar = new wxBoxSizer( wxHORIZONTAL );

    m_repoName = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_repoName->SetFont( m_repoName->GetFont().Bold() );
    m_branchChoice = new wxChoice( this, wxID_ANY );

    m_fetchButton = makeBarButton( BITMAPS::git_fetch, _( "Fetch" ),
                                   [this]()
                                   {
                                       onFetch();
                                   } );
    m_pullButton = makeBarButton( BITMAPS::git_pull, _( "Pull" ),
                                  [this]()
                                  {
                                      m_pane->Pull();
                                  } );
    m_pushButton = makeBarButton( BITMAPS::git_push, _( "Push" ),
                                  [this]()
                                  {
                                      m_pane->Push();
                                  } );

    topBar->Add( new wxStaticText( this, wxID_ANY, _( "Repository:" ) ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4 );
    topBar->Add( m_repoName, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16 );
    topBar->Add( new wxStaticText( this, wxID_ANY, _( "Branch:" ) ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4 );
    topBar->Add( m_branchChoice, 0, wxALIGN_CENTER_VERTICAL );
    topBar->AddStretchSpacer();
    topBar->Add( m_fetchButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4 );
    topBar->Add( m_pullButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4 );
    topBar->Add( m_pushButton, 0, wxALIGN_CENTER_VERTICAL );

    m_branchChoice->Bind( wxEVT_CHOICE,
                          [this]( wxCommandEvent& )
                          {
                              switchToBranch( m_branchChoice->GetStringSelection() );

                              // A refused switch leaves the choice on a branch the tree is not
                              // on, so put it back on the real one.
                              ReloadPages();
                          } );

    mainSizer->Add( topBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8 );

    mainSizer->Add( m_book, 1, wxEXPAND | wxALL, 4 );

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    buttons->AddButton( new wxButton( this, wxID_CANCEL, _( "Close" ) ) );
    buttons->Realize();
    mainSizer->Add( buttons, 0, wxEXPAND | wxALL, 4 );

    SetSizer( mainSizer );

    // The sidebar tree does not invalidate its best size when pages are added, so lay it
    // out and pin its width, matching how PAGED_DIALOG builds its navigation.
    m_book->GetTreeCtrl()->InvalidateBestSize();

    for( size_t i = 0; i < m_book->GetPageCount(); ++i )
        m_book->GetPage( i )->Layout();

    m_book->Layout();
    m_book->Fit();

    wxSize treeSize = m_book->GetTreeCtrl()->GetSize();
    treeSize.x += 20;
    m_book->GetTreeCtrl()->SetMinSize( treeSize );

    // A page is a bare list whose natural size is a few rows, so without a floor the first
    // open is a stamp. A saved size still wins once there is one.
    m_book->SetMinSize( FromDIP( wxSize( 800, 600 ) ) );
    m_book->Layout();

    Bind(
            wxEVT_BUTTON,
            [this]( wxCommandEvent& )
            {
                Close();
            },
            wxID_CANCEL );

    ReloadPages();

    // A commit, push or pull from any surface broadcasts a change, so the window never goes stale.
    syncListener();

    finishDialogSettings();
}


// One window per process. KiCad has one project open at a time and every frame shares it.
static GIT_ADVANCED_DIALOG* activeDialog()
{
    return static_cast<GIT_ADVANCED_DIALOG*>( KIGIT::ActiveAdvancedGitWindow() );
}


static void setActiveDialog( GIT_ADVANCED_DIALOG* aDialog )
{
    KIGIT::ActiveAdvancedGitWindow() = aDialog;
}


GIT_ADVANCED_DIALOG* GIT_ADVANCED_DIALOG::Active()
{
    return activeDialog();
}


void GIT_ADVANCED_DIALOG::ShowForFrame( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane )
{
    if( !activeDialog() )
    {
        GIT_ADVANCED_DIALOG* dialog = new GIT_ADVANCED_DIALOG( aParent, aPane );

        dialog->Bind( wxEVT_CLOSE_WINDOW,
                      [dialog]( wxCloseEvent& )
                      {
                          if( activeDialog() == dialog )
                              setActiveDialog( nullptr );

                          // Hiding through DIALOG_SHIM records the size for the next open. This
                          // handler takes the close event, so the base class's own save never
                          // runs.
                          dialog->Show( false );
                          dialog->Destroy();
                      } );

        setActiveDialog( dialog );
    }

    activeDialog()->Show( true );
    activeDialog()->Raise();
}


void GIT_ADVANCED_DIALOG::CloseActive()
{
    // Destroy() only schedules the delete, so the pointer is dropped here. Reopening before the
    // event loop gets round to it then builds a new window instead of raising a dying one.
    if( GIT_ADVANCED_DIALOG* dialog = activeDialog() )
    {
        setActiveDialog( nullptr );
        dialog->Destroy();
    }
}


void GIT_ADVANCED_DIALOG::CloseForPane( GIT_COMMIT_PANE* aPane )
{
    if( activeDialog() && activeDialog()->m_pane == aPane )
        CloseActive();
}


GIT_ADVANCED_DIALOG::~GIT_ADVANCED_DIALOG()
{
    if( activeDialog() == this )
        setActiveDialog( nullptr );

    // A push or fetch still running holds this window, so stop it and wait.
    bool transferStarted = m_pushWorker.joinable() || m_fetchWorker.joinable();

    if( m_pushCancel )
        m_pushCancel->store( true );

    if( m_fetchCancel )
        m_fetchCancel->store( true );

    if( m_pushWorker.joinable() )
        m_pushWorker.join();

    if( m_fetchWorker.joinable() )
        m_fetchWorker.join();

    m_avatarCancel = true;

    for( std::thread& worker : m_avatarWorkers )
        worker.join();

    if( std::shared_ptr<KIGIT_COMMON> session = m_listenerSession.lock() )
    {
        session->RemoveRefreshListener( this );

        // The completion queued on this window goes with it, so the others are told here.
        if( transferStarted )
            session->NotifyRefreshListeners();
    }
}


void GIT_ADVANCED_DIALOG::syncListener()
{
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    if( m_listenerSession.lock() == common )
        return;

    // Loading a project replaces the git session, so move the listener to the new one.
    if( std::shared_ptr<KIGIT_COMMON> previous = m_listenerSession.lock() )
        previous->RemoveRefreshListener( this );

    common->AddRefreshListener( this,
                                [this]()
                                {
                                    ReloadPages();
                                } );
    m_listenerSession = common;
}


void GIT_ADVANCED_DIALOG::ReloadPages()
{
    syncListener();

    // Reading the repository takes the shared lock. Retry shortly rather than wait, because the
    // background fetch can hold it for a network round trip.
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    if( !lock.owns_lock() )
    {
        m_reloadRetryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }

    populateTopBar();
    populateHistory();
    populateBranches();
    populateRemotes();
    populateStashes();
    populateTags();
    populateRepository();
}


void GIT_ADVANCED_DIALOG::populateHistory()
{
    if( !m_historyList )
        return;

    // The caller holds the git action lock.
    std::vector<KIGIT_COMMIT_INFO> commits = m_parent->Prj().GetGitCommon()->GetRecentCommits( m_historyLimit );

    m_historyList->DeleteAllItems();
    m_historyEmails.clear();

    appendHistoryRows( commits, 0 );

    m_historyComplete = commits.size() < m_historyLimit;

    // Scrolling near the end grows the window. Poll for it, because native wxDataViewCtrl
    // ports deliver no portable scroll events.
    if( m_historyComplete )
        m_historyScrollTimer.Stop();
    else if( !m_historyScrollTimer.IsRunning() )
        m_historyScrollTimer.Start( 250 );
}


void GIT_ADVANCED_DIALOG::appendHistoryRows( const std::vector<KIGIT_COMMIT_INFO>& aCommits, size_t aFrom )
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
        row.push_back( wxVariant( relativeDate( c.when ) ) );
        row.push_back( wxVariant( c.hash ) );
        m_historyList->AppendItem( row );

        m_historyEmails.push_back( c.email );

        if( showAvatars )
            requestAvatar( c.email, diameter );
    }
}


void GIT_ADVANCED_DIALOG::onHistoryScrollTick()
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

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    // Busy: try again on the next tick.
    if( !lock.owns_lock() )
        return;

    m_historyLimit += 50;

    std::vector<KIGIT_COMMIT_INFO> commits = common->GetRecentCommits( m_historyLimit );

    lock.unlock();

    // Append only the new rows, so the scroll position stays where the user is reading.
    appendHistoryRows( commits, itemCount );

    m_historyComplete = commits.size() < m_historyLimit;

    if( m_historyComplete )
        m_historyScrollTimer.Stop();
}


void GIT_ADVANCED_DIALOG::requestAvatar( const wxString& aEmail, int aDiameter )
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


void GIT_ADVANCED_DIALOG::applyAvatar( const wxString& aEmail, int aDiameter, const std::string& aBytes )
{
    if( aBytes.empty() )
        return;

    wxMemoryInputStream stream( aBytes.data(), aBytes.size() );
    wxImage             image;

    if( !image.LoadFile( stream, wxBITMAP_TYPE_ANY ) )
        return;

    wxBitmap circle = makeCircularBitmap( image, aDiameter );
    m_avatarCache[aEmail] = circle;

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


void GIT_ADVANCED_DIALOG::populateBranches()
{
    if( !m_branchesList )
        return;

    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    wxString                      current = common->GetCurrentBranchName();

    m_branchesList->DeleteAllItems();

    for( const auto& [name, isRemote] : common->GetAllBranches() )
    {
        long     idx = m_branchesList->InsertItem( m_branchesList->GetItemCount(), name );
        wxString status;

        if( name == current )
            status = _( "current" );
        else if( isRemote )
            status = _( "remote" );

        m_branchesList->SetItem( idx, 1, status );
    }
}


void GIT_ADVANCED_DIALOG::populateRemotes()
{
    if( !m_remotesList )
        return;

    // The caller holds the git action lock.
    m_remotesList->DeleteAllItems();

    for( const auto& [name, url] : m_parent->Prj().GetGitCommon()->GetRemotes() )
    {
        long idx = m_remotesList->InsertItem( m_remotesList->GetItemCount(), name );
        m_remotesList->SetItem( idx, 1, url );
    }
}


// A stash index is only valid for the listing it came from. Another program adding or dropping
// a stash shifts every index below it, so act on the stash itself.
static bool findStashByIdUnlocked( KIGIT_COMMON& aCommon, const wxString& aId, size_t& aIndexOut )
{
    if( aId.IsEmpty() )
        return false;

    for( const KIGIT_STASH_INFO& s : aCommon.GetStashes() )
    {
        if( s.id == aId )
        {
            aIndexOut = s.index;
            return true;
        }
    }

    return false;
}


void GIT_ADVANCED_DIALOG::populateStashes()
{
    if( !m_stashesList )
        return;

    // The caller holds the git action lock.
    m_stashesList->DeleteAllItems();

    m_stashIds.clear();

    for( const KIGIT_STASH_INFO& s : m_parent->Prj().GetGitCommon()->GetStashes() )
    {
        long idx = m_stashesList->InsertItem( m_stashesList->GetItemCount(), s.message );
        m_stashesList->SetItem( idx, 1, relativeDate( s.when ) );
        m_stashesList->SetItemData( idx, static_cast<long>( s.index ) );
        m_stashIds.push_back( s.id );
    }
}


void GIT_ADVANCED_DIALOG::populateTags()
{
    if( !m_tagsList )
        return;

    // The caller holds the git action lock.
    m_tagsList->DeleteAllItems();

    for( const KIGIT_TAG_INFO& t : m_parent->Prj().GetGitCommon()->GetTags() )
    {
        long idx = m_tagsList->InsertItem( m_tagsList->GetItemCount(), t.name );
        m_tagsList->SetItem( idx, 1, t.hash );
        m_tagsList->SetItem( idx, 2, t.summary );
    }
}


void GIT_ADVANCED_DIALOG::populateRepository()
{
    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    GIT_CONFIG_HANDLER configHandler( common.get() );
    GitUserConfig      userConfig = configHandler.GetUserConfig();

    if( m_repoUserName )
        m_repoUserName->SetValue( userConfig.authorName );

    if( m_repoUserEmail )
        m_repoUserEmail->SetValue( userConfig.authorEmail );

    if( m_repoRemoteUrl )
        m_repoRemoteUrl->SetLabel( common->GetRemoteUrl() );
}


void GIT_ADVANCED_DIALOG::switchToBranch( const wxString& aBranch )
{
    if( !sessionStillCurrent() )
        return;

    if( aBranch.IsEmpty() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    wxString                      current;

    // Reading the current branch touches the repository, so do it under the lock too.
    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        current = common->GetCurrentBranchName();
    }

    if( aBranch == current )
        return;

    // Switching refuses over uncommitted work, so saving the editors first would only turn
    // unsaved edits into the very changes that block it. Ask for a commit up front instead.
    if( !refuseOverUncommittedWork() )
        return;

    if( !IsOK( this, wxString::Format( _( "Switch to branch '%s'? This replaces the project files "
                                          "on disk with that branch's version." ),
                                       aBranch ) ) )
    {
        return;
    }

    if( !confirmWorkingTreeChange() )
        return;

    BranchResult result = BranchResult::Error;
    wxString     resultError;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        GIT_BRANCH_HANDLER handler( common.get() );
        result = handler.SwitchToBranch( aBranch );
        resultError = handler.GetErrorString();
    }

    if( result == BranchResult::Conflict )
    {
        DisplayInfoMessage( this, resultError );
        return;
    }

    if( result != BranchResult::Success )
    {
        DisplayErrorMessage( this, wxString::Format( _( "Could not switch to branch '%s'." ), aBranch ),
                             KIGIT_COMMON::GetLastGitError() );
        return;
    }

    finishWorkingTreeChange();
}


void GIT_ADVANCED_DIALOG::onAmendLastCommit()
{
    if( !sessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    wxString lastMessage;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        // Rewriting a commit that is already on the remote would diverge the branch, so only an
        // unpushed commit can be amended.
        if( !common->HasLocalCommits() )
        {
            DisplayInfoMessage( this, _( "The last commit has already been pushed, so it can not be amended." ) );
            return;
        }

        lastMessage = common->GetLastCommitMessage();
        lastMessage.Trim();
    }

    wxTextEntryDialog dlg( this, _( "Commit message:" ), _( "Amend Last Commit" ), lastMessage,
                           wxTextEntryDialogStyle | wxTE_MULTILINE );

    if( dlg.ShowModal() != wxID_OK || dlg.GetValue().Trim().IsEmpty() )
        return;

    wxString message = dlg.GetValue();

    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    runGitAction(
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


void GIT_ADVANCED_DIALOG::onForcePush()
{
    if( !sessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    wxString branch;
    wxString upstream;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        if( !common->HasPushAndPullRemote() )
        {
            DisplayInfoMessage( this, _( "No remote is configured to push to." ) );
            return;
        }

        branch = common->GetCurrentBranchName();

        // Push sends the branch to its same-named ref on the remote, regardless of any
        // differently named upstream, so the warning must name that actual target.
        upstream = common->GetRemoteNameOrDefault() + wxS( "/" ) + branch;
    }

    wxRichMessageDialog confirm( this,
                                 wxString::Format( _( "Force push replaces '%s' with your local '%s'.\n\n"
                                                      "Commits that exist only on the remote will be lost." ),
                                                   upstream, branch ),
                                 _( "Force Push" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
    confirm.SetYesNoLabels( _( "Force &Push" ), _( "Cancel" ) );

    if( confirm.ShowModal() != wxID_YES )
        return;

    // Never the shared compute pool. A libgit2 network read parked there stalls DRC, zone
    // filling and library loading.
    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    m_pushCancel = token;

    if( m_pushWorker.joinable() )
        m_pushWorker.join();

    m_pushWorker = std::thread(
            [this, common, token]()
            {
                common->ClearAuthFailure();

                GIT_PUSH_HANDLER handler( common.get() );
                handler.SetCancelToken( token );
                PushResult       result = handler.PerformPush( true );
                wxString         error = handler.GetErrorString();
                bool             authFailure = common->WasAuthFailure();

                // This operation's own token, so a force push that never became active cannot
                // consume a cancel belonging to the transfer that is running.
                bool cancelled = handler.IsThisOperationCancelled();

                CallAfter(
                        [this, common, result, error, authFailure, cancelled]()
                        {
                            if( cancelled )
                                return;

                            if( result == PushResult::Success )
                            {
                                common->NotifyRefreshListeners();
                                return;
                            }

                            if( result == PushResult::Busy )
                            {
                                DisplayInfoMessage( this, _( "Another git operation is in progress. "
                                                             "Try again in a moment." ) );
                                return;
                            }

                            if( authFailure && KIGIT::PromptForCredentials( this, common.get(), &m_parent->Prj() ) )
                            {
                                onForcePush();
                                return;
                            }

                            DisplayErrorMessage( this, _( "Force push failed." ), error );
                        } );
            } );
}


wxButton* GIT_ADVANCED_DIALOG::makeBarButton( BITMAPS aIcon, const wxString& aLabel,
                                              const std::function<void()>& aHandler )
{
    wxButton* button = new wxButton( this, wxID_ANY, aLabel );
    button->SetBitmap( KiBitmapBundle( aIcon ) );
    button->Bind( wxEVT_BUTTON,
                  [aHandler]( wxCommandEvent& )
                  {
                      aHandler();
                  } );
    return button;
}


void GIT_ADVANCED_DIALOG::populateTopBar()
{
    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    wxString                      current = common->GetCurrentBranchName();
    wxFileName                    root = wxFileName::DirName( common->GetGitRootDirectory() );

    m_repoName->SetLabel( root.GetDirs().empty() ? root.GetFullPath() : root.GetDirs().Last() );

    m_branchChoice->Clear();

    // Only a local branch can be checked out from here. Remote ones are on the Branches page.
    for( const auto& [name, isRemote] : common->GetAllBranches() )
    {
        if( !isRemote )
            m_branchChoice->Append( name );
    }

    m_branchChoice->SetStringSelection( current );
    m_branchChoice->InvalidateBestSize();
    Layout();
}


void GIT_ADVANCED_DIALOG::onFetch()
{
    if( !sessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        if( !common->HasPushAndPullRemote() )
        {
            DisplayInfoMessage( this, _( "No remote is configured to fetch from." ) );
            return;
        }
    }

    m_fetchButton->SetLabel( _( "Fetching..." ) );
    m_fetchButton->Disable();
    Layout();

    // Never the shared compute pool. A libgit2 network read parked there stalls DRC, zone
    // filling and library loading.
    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    m_fetchCancel = token;

    if( m_fetchWorker.joinable() )
        m_fetchWorker.join();

    m_fetchWorker = std::thread(
            [this, common, token]()
            {
                common->ClearAuthFailure();

                GIT_PULL_HANDLER handler( common.get() );
                handler.SetCancelToken( token );
                bool     fetched = handler.PerformFetch();
                wxString error = handler.GetErrorStrings().empty() ? wxString() : handler.GetErrorString();
                bool     authFailure = common->WasAuthFailure();

                // This operation's own token, so a fetch that never became active cannot
                // consume a cancel belonging to the transfer that is running.
                bool cancelled = handler.IsThisOperationCancelled();

                CallAfter(
                        [this, common, fetched, error, authFailure, cancelled]()
                        {
                            m_fetchButton->SetLabel( _( "Fetch" ) );
                            m_fetchButton->Enable();
                            Layout();

                            if( cancelled )
                                return;

                            if( fetched )
                            {
                                common->NotifyRefreshListeners();
                                return;
                            }

                            if( authFailure && KIGIT::PromptForCredentials( this, common.get(), &m_parent->Prj() ) )
                            {
                                onFetch();
                                return;
                            }

                            // A held lock comes back as a failure with no message.
                            if( error.IsEmpty() )
                            {
                                DisplayInfoMessage( this, _( "Another git operation is in progress. "
                                                             "Try again in a moment." ) );
                                return;
                            }

                            DisplayErrorMessage( this, _( "Fetch failed." ), error );
                        } );
            } );
}


bool GIT_ADVANCED_DIALOG::refuseOverUncommittedWork()
{
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    bool                          dirty = false;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return false;
        }

        dirty = common->HasUncommittedTrackedChanges();
    }

    if( dirty || DocumentEditorsDirty( m_parent->Kiway() )
        || !LibraryEditorsWithUnsavedChanges( m_parent->Kiway() ).empty() )
    {
        DisplayInfoMessage( this, _( "There are changes that are not committed yet, and this would run over "
                                     "them. Commit them first, or set them aside with Stash Changes." ) );
        return false;
    }

    return true;
}


bool GIT_ADVANCED_DIALOG::confirmWorkingTreeChange()
{
    // Save rather than close: this window is parented to the frame hosting the panel, so a close
    // would destroy that frame and this dialog with it, mid-call.
    if( !PrepareEditorsForGitOperation( m_parent, GIT_EDITOR_PREP::WORKING_TREE ) )
        return false;

    m_docsBeforeChange = CaptureOpenEditorDocs( m_parent->Kiway() );
    return true;
}


void GIT_ADVANCED_DIALOG::finishWorkingTreeChange()
{
    ReloadOpenEditors( m_parent->Kiway(), m_docsBeforeChange );

    // Refreshes this window and the docked panel through the shared registry.
    m_parent->Prj().GetGitCommon()->NotifyRefreshListeners();
}


void GIT_ADVANCED_DIALOG::addButtonRow( wxWindow*                                                      aPage,
                                        const std::vector<std::pair<wxString, std::function<void()>>>& aButtons )
{
    wxBoxSizer* strip = new wxBoxSizer( wxHORIZONTAL );

    for( const auto& [label, handler] : aButtons )
    {
        wxButton* button = new wxButton( aPage, wxID_ANY, label );
        button->Bind( wxEVT_BUTTON,
                      [handler]( wxCommandEvent& )
                      {
                          handler();
                      } );
        strip->Add( button, 0, wxRIGHT, 4 );
    }

    aPage->GetSizer()->Add( strip, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4 );
}


void GIT_ADVANCED_DIALOG::onRestoreVersion( const wxString& aCommitHash )
{
    if( !sessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    // Edits still in an editor's memory pass the backend's disk check, and the reload prompt would
    // then ask about them only after the restore commit exists, too late to choose.
    for( FRAME_T type : { FRAME_SCH, FRAME_PCB_EDITOR } )
    {
        KIWAY_PLAYER* player = m_parent->Kiway().Player( type, false );

        if( player && player->IsContentModified() )
        {
            DisplayInfoMessage( this, _( "There are unsaved changes in the open editors. Commit them "
                                         "first, so the restore stays a separate step that can be "
                                         "undone." ) );
            return;
        }
    }

    if( !LibraryEditorsWithUnsavedChanges( m_parent->Kiway() ).empty() )
    {
        DisplayInfoMessage( this, _( "A library editor has unsaved changes. Save or discard them first, so "
                                     "the restore does not overwrite what it is editing." ) );
        return;
    }

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        // A restore commit made on a detached HEAD belongs to no branch and is lost on the
        // next switch.
        if( common->IsHeadDetached() )
        {
            DisplayInfoMessage( this, _( "This project is showing an old snapshot rather than a branch. "
                                         "Double click a branch on the Branches page first, then restore." ) );
            return;
        }
    }

    // Restore only this project's folder. A repository can hold several projects, and rewinding
    // the whole tree from one project's window would rewind the others too.
    wxString projectPrefix;
    bool     prefixKnown = false;

    {
        wxString workDir = common->GetProjectDir();
        wxString projectPath = m_parent->Prj().GetProjectPath();

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
    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( m_parent->Kiway() );

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
        ReloadOpenEditors( m_parent->Kiway(), docsBefore );
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
                outside = ChangedTrackedFilesOutsideProject( m_parent->Prj() );
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


bool GIT_ADVANCED_DIALOG::sessionStillCurrent()
{
    // The rows were filled from the previous session. Acting on the new one could delete an
    // identically named branch, or the same numbered stash, in a project that never showed them.
    if( m_listenerSession.lock() == m_parent->Prj().GetGitCommon() )
        return true;

    Close();
    return false;
}


void GIT_ADVANCED_DIALOG::runGitAction( const std::function<bool( KIGIT_COMMON& )>& aFn, const wxString& aError,
                                        bool aRewritesWorkingTree )
{
    if( !sessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    bool                          ok = false;

    {
        // libgit2 is not safe for concurrent use of one repository, so take the git action lock
        // for the mutation. It is released before the refresh broadcast.
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        ok = aFn( *common );
    }

    if( !ok )
    {
        // A failed action can still have written files, so the editors and the panels are
        // brought up to date first. Nothing reloads when nothing changed on disk.
        if( aRewritesWorkingTree )
        {
            ReloadOpenEditors( m_parent->Kiway(), m_docsBeforeChange );
            common->NotifyRefreshListeners();
        }

        DisplayErrorMessage( this, aError, KIGIT_COMMON::GetLastGitError() );
        return;
    }

    if( aRewritesWorkingTree )
    {
        // The files the editors have open were just rewritten, so pull them back in from disk.
        ReloadOpenEditors( m_parent->Kiway(), m_docsBeforeChange );
    }

    common->NotifyRefreshListeners();
}


wxString GIT_ADVANCED_DIALOG::selectedText( wxListCtrl* aList, int aColumn ) const
{
    long sel = aList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( sel == -1 )
        return wxEmptyString;

    return aList->GetItemText( sel, aColumn );
}


void GIT_ADVANCED_DIALOG::onNewBranch()
{
    wxString name = wxGetTextFromUser( _( "New branch name:" ), _( "New Branch" ), wxEmptyString, this );

    if( name.IsEmpty() )
        return;

    runGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.CreateBranch( name );
            },
            wxString::Format( _( "Could not create branch '%s'." ), name ) );
}


void GIT_ADVANCED_DIALOG::onDeleteBranch()
{
    if( !sessionStillCurrent() )
        return;

    wxString name = selectedText( m_branchesList, 0 );

    if( name.IsEmpty() )
        return;

    // Deleting a remote-tracking branch would mean deleting it on the server, and the lookup
    // below could otherwise hit a local branch of the same name.
    if( selectedText( m_branchesList, 1 ) == _( "remote" ) )
    {
        DisplayInfoMessage( this, _( "This branch lives on the remote, so it can not be deleted from here." ) );
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    wxString                      current;
    size_t                        unmerged = 0;

    // Reading the current branch touches the repository, so do it under the lock.
    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        current = common->GetCurrentBranchName();
        unmerged = common->GetUnmergedCommitCount( name );
    }

    if( name == current )
    {
        DisplayErrorMessage( this, _( "You cannot delete the branch you are on." ) );
        return;
    }

    // Deleting drops any commit no other branch reaches, and git offers no way back a
    // beginner would find, so say what is at stake before asking.
    wxString question;

    if( unmerged == 1 )
    {
        question = wxString::Format( _( "Branch '%s' has one commit that is not on the current branch. "
                                        "Deleting it loses that commit.\n\nDelete it anyway?" ),
                                     name );
    }
    else if( unmerged > 1 )
    {
        question = wxString::Format( _( "Branch '%s' has %zu commits that are not on the current branch. "
                                        "Deleting it loses them.\n\nDelete it anyway?" ),
                                     name, unmerged );
    }
    else
    {
        question = wxString::Format( _( "Delete branch '%s'?" ), name );
    }

    if( !IsOK( this, question ) )
        return;

    runGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.DeleteBranch( name );
            },
            wxString::Format( _( "Could not delete branch '%s'." ), name ) );
}


void GIT_ADVANCED_DIALOG::onStashSave()
{
    if( !sessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    // Stashing writes a commit object, so it needs an author identity like a commit does.
    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    // Unsaved editor changes are written to disk here, so they are part of what gets
    // stashed instead of surviving in memory.
    if( !confirmWorkingTreeChange() )
        return;

    bool hasTrackedChanges = false;
    bool hasNewFilesOnly = false;
    bool busy = false;

    {
        // Reading the status touches the repository, so it runs under the same lock the
        // background fetch and the transfer workers take. Only plain answers leave this scope,
        // because the messages below open modal dialogs.
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            busy = true;
        }
        else
        {
            hasTrackedChanges = common->HasUncommittedTrackedChanges();
            hasNewFilesOnly = !hasTrackedChanges && !BuildGitChangeEntries( m_parent->Prj() ).empty();
        }
    }

    if( busy )
    {
        DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
        return;
    }

    if( !hasTrackedChanges )
    {
        // A stash only includes files that have been committed before. When the only changes
        // are brand new files, saying "no changes" would contradict what the panel shows.
        if( hasNewFilesOnly )
        {
            DisplayInfoMessage( this, _( "The changed files have never been committed, and a stash only "
                                         "includes files that have been. Commit them from the Version "
                                         "Control panel first, or discard them there." ) );
        }
        else
        {
            DisplayInfoMessage( this, _( "There are no changes to stash." ) );
        }

        return;
    }

    wxTextEntryDialog dlg( this, _( "Optional description for this stash:" ), _( "Stash Changes" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString message = dlg.GetValue().Trim( true ).Trim( false );

    runGitAction(
            [message]( KIGIT_COMMON& aCommon )
            {
                return aCommon.StashSave( message );
            },
            _( "Could not stash the changes." ), /* rewrites working tree */ true );
}


wxString GIT_ADVANCED_DIALOG::stashIdForRow( long aRow ) const
{
    if( aRow < 0 || static_cast<size_t>( aRow ) >= m_stashIds.size() )
        return wxEmptyString;

    return m_stashIds[aRow];
}


void GIT_ADVANCED_DIALOG::restoreSelectedStash( bool aRemoveAfterApply )
{
    long sel = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( sel == -1 )
        return;

    // Applying needs a clean tree, so saving the editors first would only turn unsaved edits
    // into the very changes that block it. Ask for a commit up front instead.
    if( !refuseOverUncommittedWork() )
        return;

    wxString question = aRemoveAfterApply ? _( "Apply this stash and remove it from the list?" )
                                          : _( "Apply this stash to the working tree?" );

    if( !IsOK( this, question ) )
        return;

    if( !confirmWorkingTreeChange() )
        return;

    wxString id = stashIdForRow( sel );

    runGitAction(
            [id, aRemoveAfterApply]( KIGIT_COMMON& aCommon )
            {
                size_t index = 0;

                if( !findStashByIdUnlocked( aCommon, id, index ) )
                    return false;

                return aRemoveAfterApply ? aCommon.StashPop( index ) : aCommon.StashApply( index );
            },
            aRemoveAfterApply ? _( "Could not pop the stash. Another program may have changed the stash "
                                   "list, so nothing was applied." )
                              : _( "Could not apply the stash. Another program may have changed the stash "
                                   "list, so nothing was applied." ),
            /* rewrites working tree */ true );
}


void GIT_ADVANCED_DIALOG::onStashApply()
{
    restoreSelectedStash( false );
}


void GIT_ADVANCED_DIALOG::onStashPop()
{
    restoreSelectedStash( true );
}


void GIT_ADVANCED_DIALOG::onStashDrop()
{
    long sel = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( sel == -1 )
        return;

    if( !IsOK( this, _( "Drop this stash? This cannot be undone." ) ) )
        return;

    wxString id = stashIdForRow( sel );

    runGitAction(
            [id]( KIGIT_COMMON& aCommon )
            {
                size_t index = 0;

                if( !findStashByIdUnlocked( aCommon, id, index ) )
                    return false;

                return aCommon.StashDrop( index );
            },
            _( "Could not drop the stash. Another program may have changed the stash list, so nothing "
               "was dropped." ) );
}


void GIT_ADVANCED_DIALOG::onNewTag()
{
    wxString name = wxGetTextFromUser( _( "New tag name:" ), _( "New Tag" ), wxEmptyString, this );

    if( name.IsEmpty() )
        return;

    runGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.CreateTag( name );
            },
            wxString::Format( _( "Could not create tag '%s'." ), name ) );
}


void GIT_ADVANCED_DIALOG::onCheckoutTag()
{
    wxString name = selectedText( m_tagsList, 0 );

    if( name.IsEmpty() )
        return;

    if( !refuseOverUncommittedWork() )
        return;

    if( !IsOK( this, wxString::Format( _( "Check out tag '%s'? This shows an old snapshot of the "
                                          "project. To keep any commits you make here, create a new "
                                          "branch before committing. Switching away without one "
                                          "leaves those commits outside every branch." ),
                                       name ) ) )
    {
        return;
    }

    if( !confirmWorkingTreeChange() )
        return;

    runGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.CheckoutTag( name );
            },
            wxString::Format( _( "Could not check out tag '%s'." ), name ), /* rewrites working tree */ true );
}


void GIT_ADVANCED_DIALOG::onDeleteTag()
{
    wxString name = selectedText( m_tagsList, 0 );

    if( name.IsEmpty() )
        return;

    if( !IsOK( this, wxString::Format( _( "Delete tag '%s'?" ), name ) ) )
        return;

    runGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.DeleteTag( name );
            },
            wxString::Format( _( "Could not delete tag '%s'." ), name ) );
}


void GIT_ADVANCED_DIALOG::runRemoteAction( const std::function<bool( KIGIT_COMMON& )>& aMutation,
                                           const wxString&                             aError )
{
    runGitAction(
            [&aMutation]( KIGIT_COMMON& aCommon )
            {
                if( !aMutation( aCommon ) )
                    return false;

                // Refresh the cached URL and credentials, so a later push does not reach the new
                // remote authenticating as the old one.
                aCommon.UpdateCurrentBranchInfo();
                return true;
            },
            aError );
}


void GIT_ADVANCED_DIALOG::onAddRemote()
{
    wxString name = wxGetTextFromUser( _( "Remote name:" ), _( "Add Remote" ), wxEmptyString, this );

    if( name.IsEmpty() )
        return;

    wxString url = wxGetTextFromUser( _( "Remote URL:" ), _( "Add Remote" ), wxEmptyString, this );

    if( url.IsEmpty() )
        return;

    runRemoteAction(
            [name, url]( KIGIT_COMMON& aCommon )
            {
                return aCommon.AddRemote( name, url );
            },
            _( "Could not add the remote." ) );
}


void GIT_ADVANCED_DIALOG::onEditRemote()
{
    wxString name = selectedText( m_remotesList, 0 );

    if( name.IsEmpty() )
        return;

    wxString current = selectedText( m_remotesList, 1 );
    wxString url = wxGetTextFromUser( _( "Remote URL:" ), _( "Edit Remote" ), current, this );

    if( url.IsEmpty() )
        return;

    // The list shows the address with any user:password removed, so writing it back unchanged
    // would delete credentials the config still holds.
    if( url == current )
        return;

    runRemoteAction(
            [name, url]( KIGIT_COMMON& aCommon )
            {
                return aCommon.SetRemoteUrl( name, url );
            },
            _( "Could not update the remote." ) );
}


void GIT_ADVANCED_DIALOG::onRemoveRemote()
{
    wxString name = selectedText( m_remotesList, 0 );

    if( name.IsEmpty() )
        return;

    if( !IsOK( this, wxString::Format( _( "Remove remote '%s'?" ), name ) ) )
        return;

    runRemoteAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.RemoveRemote( name );
            },
            _( "Could not remove the remote." ) );
}


void GIT_ADVANCED_DIALOG::onSaveRepositoryConfig()
{
    wxString name = m_repoUserName->GetValue().Trim().Trim( false );
    wxString email = m_repoUserEmail->GetValue().Trim().Trim( false );

    if( name.IsEmpty() || email.IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Enter both a name and an email. Git needs them to record who "
                                      "made each commit." ) );
        return;
    }

    runGitAction(
            [name, email]( KIGIT_COMMON& aCommon )
            {
                return aCommon.SetUserConfig( name, email );
            },
            _( "Could not save the git configuration." ) );
}
