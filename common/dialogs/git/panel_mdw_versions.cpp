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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <dialogs/git/panel_mdw_versions.h>

#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/workspace_item_files.h>
#include <confirm.h>
#include <diff_merge/merge_dispatch.h>
#include <eda_base_frame.h>
#include <project.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_revision_copy.h>
#include <mdw/client/mdw_source.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/filefn.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include <wx/weakref.h>


namespace
{
std::vector<long> selectedRows( const wxListCtrl& aList )
{
    std::vector<long> rows;

    for( long row = aList.GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ); row >= 0;
         row = aList.GetNextItem( row, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) )
    {
        rows.push_back( row );
    }

    return rows;
}


wxString dateText( const wxDateTime& aTime )
{
    return aTime.IsValid() ? aTime.Format( wxS( "%x %H:%M" ) ) : wxString();
}


/// What the diff viewer is opened on, kept until the modal viewer closes
struct COMPARISON
{
    std::shared_ptr<GIT_DIFF_WORKSPACE> scratch;
    wxString                            older;
    wxString                            newer;
    wxString                            olderLabel;
    wxString                            newerLabel;
    KICAD_DIFF::DOC_KIND                kind = KICAD_DIFF::DOC_KIND::UNKNOWN;
};
} // namespace


PANEL_MDW_VERSIONS::PANEL_MDW_VERSIONS( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize& aSize,
                                        long aStyle ) :
        wxPanel( aParent, aId, aPos, aSize, aStyle )
{
    wxBoxSizer*   sizer = new wxBoxSizer( wxVERTICAL );
    wxStaticText* label = new wxStaticText( this, wxID_ANY, _( "Versions:" ) );

    m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
    m_copyStatus = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                    wxST_ELLIPSIZE_MIDDLE | wxST_NO_AUTORESIZE );
    m_compareButton = new wxButton( this, wxID_ANY, _( "Compare" ) );
    m_saveCopyButton = new wxButton( this, wxID_ANY, _( "Save Copy..." ) );

    wxBoxSizer* buttons = new wxBoxSizer( wxHORIZONTAL );

    buttons->Add( m_saveCopyButton, 0, wxRIGHT, FromDIP( 8 ) );
    buttons->Add( m_compareButton, 0 );

    sizer->Add( label, 0, wxBOTTOM, FromDIP( 4 ) );
    sizer->Add( m_list, 1, wxEXPAND );
    sizer->Add( m_copyStatus, 0, wxEXPAND | wxTOP, FromDIP( 4 ) );
    sizer->Add( buttons, 0, wxALIGN_RIGHT | wxTOP, FromDIP( 8 ) );
    SetSizer( sizer );

    m_list->AppendColumn( _( "Version" ), wxLIST_FORMAT_LEFT, FromDIP( 80 ) );
    m_list->AppendColumn( _( "Author" ), wxLIST_FORMAT_LEFT, FromDIP( 110 ) );
    m_list->AppendColumn( _( "Date" ), wxLIST_FORMAT_LEFT, FromDIP( 130 ) );
    m_list->AppendColumn( _( "Message" ), wxLIST_FORMAT_LEFT, FromDIP( 220 ) );

    m_list->Bind( wxEVT_SIZE,
                  [this]( wxSizeEvent& aEvent )
                  {
                      fitMessageColumn();
                      aEvent.Skip();
                  } );

    m_compareButton->SetToolTip( _( "Select a version to compare it with the previous one." ) );
    m_compareButton->Disable();

    m_saveCopyButton->SetToolTip( _( "Save the selected version to a file." ) );
    m_saveCopyButton->Disable();
    m_copyStatus->Hide();

    auto selectionChanged = [this]( wxListEvent& )
    {
        updateCompare();
    };

    m_list->Bind( wxEVT_LIST_ITEM_SELECTED, selectionChanged );
    m_list->Bind( wxEVT_LIST_ITEM_DESELECTED, selectionChanged );

    m_list->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK,
                  [this]( wxListEvent& aEvent )
                  {
                      onContextMenu( aEvent );
                  } );

    m_compareButton->Bind( wxEVT_BUTTON,
                           [this]( wxCommandEvent& )
                           {
                               onCompare();
                           } );

    m_saveCopyButton->Bind( wxEVT_BUTTON,
                            [this]( wxCommandEvent& )
                            {
                                if( std::optional<long> row = singleRevisionRow() )
                                    onSaveCopy( *row );
                            } );
}


void PANEL_MDW_VERSIONS::Clear()
{
    m_historyOp = MDW_OP();
    m_olderOp = MDW_OP();
    m_newerOp = MDW_OP();
    m_newestOp = MDW_OP();
    m_copyOp = MDW_OP();
    m_copying = false;
    m_itemGeneration++;
    setCopyStatus( wxEmptyString );
    m_newestScratch.reset();
    m_newestFor.clear();
    m_item = MDW_ITEM_REF();
    m_checkoutPath.clear();
    m_workingFiles.clear();
    m_workingCopy.clear();
    m_revisions.clear();
    m_olderUnread = false;
    m_comparing = false;
    m_rowKeys.clear();
    m_list->DeleteAllItems();
    updateCompare();
}


void PANEL_MDW_VERSIONS::forgetChanges()
{
    m_changesOp = MDW_OP();
    m_changesState = CHANGES::UNREAD;
    m_modified.clear();
}


void PANEL_MDW_VERSIONS::RefreshChanges()
{
    forgetChanges();

    if( !m_source || m_item.item_id.IsEmpty() )
        return;

    // The shown rows stay until the new reads land, so the list does not flicker
    readHistory();

    if( !m_localRoot.IsEmpty() )
        readChanges();
}


void PANEL_MDW_VERSIONS::SetItem( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                                  const wxString& aLocalRoot, const std::vector<wxString>& aWorkingFiles,
                                  EDA_BASE_FRAME* aFrame, const wxString& aMemberName )
{
    Clear();

    // The modified files are read once for every item shown from the same source and root
    if( aSource != m_source || aLocalRoot != m_localRoot )
    {
        forgetChanges();
        m_newest.clear();
    }

    m_source = aSource;
    m_localRoot = aLocalRoot;
    m_item = aItem;
    m_workingFiles = aWorkingFiles;
    m_checkoutPath = aWorkingFiles.empty() ? wxString() : aWorkingFiles.front();
    m_memberName = aMemberName;
    m_frame = aFrame;

    if( !m_source )
        return;

    readHistory();

    if( m_changesState == CHANGES::UNREAD && !m_localRoot.IsEmpty() )
        readChanges();

    applyWorkingCopy();
}


void PANEL_MDW_VERSIONS::readHistory()
{
    m_historyOp = MdwReadAllPages<MDW_REVISION>(
            [this]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_REVISION>> aDone )
            {
                return m_source->History( m_item, aPage, std::move( aDone ) );
            },
            0,
            [this]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_REVISION>& aRead )
            {
                // The shown rows stay when the first page failed
                if( !aResult.IsOk() && aRead.rows.empty() )
                    return;

                m_revisions = aRead.rows;

                // A short list must not pass for the whole history
                m_olderUnread = !aResult.IsOk() || !aRead.next_page_token.IsEmpty();

                if( aResult.IsOk() )
                    applyWorkingCopy();
                else
                    fillList();
            },
            MDW_READ_CAP::PARTIAL );
}


void PANEL_MDW_VERSIONS::readChanges()
{
    m_changesState = CHANGES::READING;

    m_changesOp = MdwReadAllPages<MDW_LOCAL_CHANGE>(
            [this]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aDone )
            {
                return m_source->LocalChanges( m_localRoot, aPage, std::move( aDone ) );
            },
            0,
            [this]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_LOCAL_CHANGE>& aRead )
            {
                if( !aResult.IsOk() )
                {
                    m_workingCopy.clear();

                    // An upload backend cannot say, so the newest version decides
                    m_changesState = aResult.error == MDW_ERROR_CODE::NOT_SUPPORTED ? CHANGES::UNSUPPORTED
                                                                                      : CHANGES::UNREAD;
                    applyWorkingCopy();
                    return;
                }

                m_modified.clear();

                for( const MDW_LOCAL_CHANGE& change : aRead.rows )
                {
                    if( change.change == MDW_CHANGE_TYPE::MODIFIED )
                        m_modified.insert( change.member.item_id );
                }

                m_changesState = CHANGES::KNOWN;
                applyWorkingCopy();
            } );
}


void PANEL_MDW_VERSIONS::applyWorkingCopy()
{
    // The last answer stands until the read in flight lands
    if( m_changesState == CHANGES::UNREAD || m_changesState == CHANGES::READING )
    {
        fillList();
        return;
    }

    bool differs = false;

    if( wxFileName::FileExists( m_checkoutPath ) )
    {
        if( m_changesState == CHANGES::KNOWN )
            differs = m_modified.contains( m_item.item_id );
        else if( m_changesState == CHANGES::UNSUPPORTED && !m_revisions.empty() )
            differs = differsFromNewest();
    }

    m_workingCopy = differs ? m_checkoutPath : wxString();
    fillList();
}


bool PANEL_MDW_VERSIONS::differsFromNewest()
{
    const wxString&  revisionId = m_revisions.front().revision_id;
    const wxDateTime modified = wxFileName( m_checkoutPath ).GetModificationTime();

    // The answer holds until a newer version is listed or the file is saved again
    if( auto it = m_newest.find( m_item.item_id );
        it != m_newest.end() && it->second.revisionId == revisionId && it->second.modified == modified )
    {
        return it->second.differs;
    }

    // Until the answer lands, the row stays as it was. A failed download is not retried until the item is shown again
    if( m_newestFor == revisionId )
        return hasWorkingCopy();

    m_newestFor = revisionId;
    m_newestScratch = std::make_shared<GIT_DIFF_WORKSPACE>();

    if( !m_newestScratch->IsOk() )
        return hasWorkingCopy();

    wxString scratch = m_newestScratch->Root() + wxS( "/" ) + compareDocument().fileName;

    m_newestOp = m_source->Download(
            m_item, revisionId, scratch,
            [this, revisionId, modified, scratch]( const MDW_RESULT& aResult, const MDW_REVISION& )
            {
                if( aResult.IsOk() )
                {
                    m_newest[m_item.item_id] = { revisionId, modified, MdwFilesDiffer( m_checkoutPath, scratch ) };
                    m_newestFor.clear();
                }
                else
                {
                    m_workingCopy.clear();
                }

                m_newestScratch.reset();
                applyWorkingCopy();
            } );

    return hasWorkingCopy();
}


void PANEL_MDW_VERSIONS::fillList()
{
    // The working copy keeps an empty key, a version its revision id
    std::set<wxString> selected;

    for( long row : selectedRows( *m_list ) )
    {
        if( row < static_cast<long>( m_rowKeys.size() ) )
            selected.insert( m_rowKeys[row] );
    }

    m_list->DeleteAllItems();
    m_rowKeys.clear();

    auto keep = [&]( long aRow )
    {
        if( selected.contains( m_rowKeys.back() ) )
            m_list->SetItemState( aRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    };

    if( hasWorkingCopy() )
    {
        long row = m_list->InsertItem( 0, _( "Working copy" ) );

        m_list->SetItem( row, 2, dateText( wxFileName( m_workingCopy ).GetModificationTime() ) );
        m_rowKeys.emplace_back();
        keep( row );
    }

    for( const MDW_REVISION& revision : m_revisions )
    {
        long row = m_list->InsertItem( m_list->GetItemCount(), WorkspaceRevisionText( revision ) );

        m_rowKeys.push_back( revision.revision_id );
        keep( row );

        m_list->SetItem( row, 1, revision.author );
        m_list->SetItem( row, 2, dateText( revision.timestamp ) );
        m_list->SetItem( row, 3, revision.comment.BeforeFirst( '\n' ) );
    }

    if( m_olderUnread )
        m_list->InsertItem( m_list->GetItemCount(), _( "Older versions could not be read" ) );

    // Rows enough to scroll narrow the list without a size event
    fitMessageColumn();
    updateCompare();
}


std::optional<std::pair<long, long>> PANEL_MDW_VERSIONS::comparePair() const
{
    return MdwComparePair( static_cast<long>( m_revisions.size() ), selectedRows( *m_list ), hasWorkingCopy() );
}


std::optional<long> PANEL_MDW_VERSIONS::singleRevisionRow() const
{
    std::vector<long> rows = selectedRows( *m_list );

    if( rows.size() != 1 || rows[0] >= static_cast<long>( m_rowKeys.size() ) || m_rowKeys[rows[0]].IsEmpty() )
        return std::nullopt;

    return rows[0];
}


void PANEL_MDW_VERSIONS::fitMessageColumn()
{
    const int last = m_list->GetColumnCount() - 1;
    int       used = 0;

    for( int col = 0; col < last; ++col )
        used += m_list->GetColumnWidth( col );

    // MSW draws the width past the last column as an empty header with no row highlight
    m_list->SetColumnWidth( last, std::max( FromDIP( 220 ), m_list->GetClientSize().x - used ) );
}


void PANEL_MDW_VERSIONS::setCopyStatus( const wxString& aText, const wxString& aPath )
{
    m_copyStatus->SetLabel( aText );
    m_copyStatus->Show( !aText.IsEmpty() );

    // The label is ellipsized to fit, so the whole path is only in the tooltip
    if( aPath.IsEmpty() )
        m_copyStatus->UnsetToolTip();
    else
        m_copyStatus->SetToolTip( aPath );

    Layout();
}


void PANEL_MDW_VERSIONS::updateCompare()
{
    m_compareButton->Enable( !m_comparing && comparePair().has_value()
                             && compareDocument().kind != KICAD_DIFF::DOC_KIND::UNKNOWN );
    m_saveCopyButton->Enable( !m_copying && singleRevisionRow().has_value() );
}


MDW_COMPARE_DOC PANEL_MDW_VERSIONS::compareDocument() const
{
    return MdwCompareDocument( m_item.item_id, m_checkoutPath );
}


PANEL_MDW_VERSIONS::SIDE PANEL_MDW_VERSIONS::sideAt( long aRow ) const
{
    SIDE side;

    // A member is named alone, as its library is the file all its versions share
    MDW_ITEM_REF shown = m_item;

    if( !m_memberName.IsEmpty() )
        shown.path_hint = m_memberName;

    if( hasWorkingCopy() && aRow == 0 )
    {
        side.label = wxString::Format( _( "%s (working copy)" ), MdwItemFileName( shown ) );
        return side;
    }

    const MDW_REVISION& revision = m_revisions[aRow - ( hasWorkingCopy() ? 1 : 0 )];

    side.revisionId = revision.revision_id;
    side.label = MdwVersionSideLabel( shown, revision );
    return side;
}


bool PANEL_MDW_VERSIONS::saveWorkingCopy( const wxString& aPath ) const
{
    // The downloaded versions of a symbol hold only that symbol, so its working copy must too
    if( m_memberName.IsEmpty() || compareDocument().kind != KICAD_DIFF::DOC_KIND::SYM_LIB )
        return wxCopyFile( m_workingCopy, aPath );

    std::ifstream     in( m_workingCopy.fn_str(), std::ios::binary );
    std::stringstream text;

    text << in.rdbuf();

    const std::string member = MdwSymbolMemberText( text.str(), m_memberName );

    if( member.empty() )
        return false;

    std::ofstream out( aPath.fn_str(), std::ios::binary );

    out << member;
    return out.good();
}


MDW_OP PANEL_MDW_VERSIONS::fetch( const SIDE& aSide, std::function<void( const MDW_RESULT& )> aDone )
{
    if( aSide.revisionId.IsEmpty() )
    {
        aDone( saveWorkingCopy( aSide.path ) ? MDW_RESULT::Ok() : MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL ) );
        return MDW_OP();
    }

    return m_source->Download( m_item, aSide.revisionId, aSide.path,
                               [aDone]( const MDW_RESULT& aResult, const MDW_REVISION& )
                               {
                                   aDone( aResult );
                               } );
}


void PANEL_MDW_VERSIONS::onCompare()
{
    std::optional<std::pair<long, long>> pair = comparePair();

    if( !m_source || !pair )
        return;

    auto comparison = std::make_shared<COMPARISON>();

    const MDW_COMPARE_DOC doc = compareDocument();

    comparison->kind = doc.kind;

    comparison->scratch = std::make_shared<GIT_DIFF_WORKSPACE>();

    if( !comparison->scratch->IsOk() )
    {
        DisplayErrorMessage( this, _( "Could not download the versions to compare." ) );
        return;
    }

    // Same name in two folders, since the diff viewer routes on the file name
    wxFileName older( comparison->scratch->Root() + wxS( "/older/" ) + doc.fileName );
    wxFileName newer( comparison->scratch->Root() + wxS( "/newer/" ) + doc.fileName );

    older.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    newer.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    SIDE olderSide = sideAt( pair->first );
    SIDE newerSide = sideAt( pair->second );

    olderSide.path = comparison->older = older.GetFullPath();
    newerSide.path = comparison->newer = newer.GetFullPath();
    comparison->olderLabel = olderSide.label;
    comparison->newerLabel = newerSide.label;

    m_comparing = true;
    updateCompare();

    auto finish = [this, comparison]( const MDW_RESULT& aStatus )
    {
        if( !aStatus.IsOk() )
        {
            m_comparing = false;
            updateCompare();
            DisplayErrorMessage( this, _( "Could not download the versions to compare." ), MdwErrorText( aStatus ) );
            return;
        }

        WX_STRING_REPORTER reporter;
        KICAD_DIFF::DispatchOpenDiffDialog( m_frame->Kiway(), comparison->kind, comparison->older, comparison->newer,
                                            comparison->olderLabel, comparison->newerLabel, this, &reporter );

        if( reporter.HasMessage() )
            DisplayErrorMessage( this, _( "Could not open the comparison." ), reporter.GetMessages() );

        // Compare stays off while the modal viewer is up
        m_comparing = false;
        updateCompare();
    };

    m_olderOp = fetch( olderSide,
                       [this, newerSide, finish]( const MDW_RESULT& aStatus )
                       {
                           if( aStatus.IsOk() )
                               m_newerOp = fetch( newerSide, finish );
                           else
                               finish( aStatus );
                       } );
}


wxString PANEL_MDW_VERSIONS::copyFolder() const
{
    if( m_memberName.IsEmpty() )
        return wxFileName( m_checkoutPath ).GetPath();

    // A member's file lives in the team library's checkout, where a copy would sit as an untracked file
    if( m_frame && !m_frame->Prj().IsNullProject() )
        return m_frame->Prj().GetProjectPath();

    return wxStandardPaths::Get().GetDocumentsDir();
}


void PANEL_MDW_VERSIONS::onSaveCopy( long aRow )
{
    if( !m_source || m_copying || aRow < 0 || aRow >= static_cast<long>( m_rowKeys.size() )
        || m_rowKeys[aRow].IsEmpty() )
    {
        return;
    }

    // The modal dialog keeps history reads and item changes running, so nothing below may point into members
    const MDW_REVISION                revision = m_revisions[aRow - ( hasWorkingCopy() ? 1 : 0 )];
    const std::shared_ptr<MDW_SOURCE> source = m_source;
    const MDW_ITEM_REF                item = m_item;
    const std::vector<wxString>       workingFiles = m_workingFiles;
    const wxString                    memberName = m_memberName;
    const unsigned                    generation = m_itemGeneration;
    const wxFileName                  checkout( m_checkoutPath );
    wxString                          wildcard;

    switch( compareDocument().kind )
    {
    case KICAD_DIFF::DOC_KIND::PCB: wildcard = FILEEXT::PcbFileWildcard(); break;
    case KICAD_DIFF::DOC_KIND::SCH: wildcard = FILEEXT::KiCadSchematicFileWildcard(); break;
    case KICAD_DIFF::DOC_KIND::SYM_LIB: wildcard = FILEEXT::KiCadSymbolLibFileWildcard(); break;
    case KICAD_DIFF::DOC_KIND::FP_LIB:
    case KICAD_DIFF::DOC_KIND::FOOTPRINT: wildcard = FILEEXT::KiCadFootprintLibFileWildcard(); break;
    default: break;
    }

    wildcard = wildcard.IsEmpty() ? FILEEXT::AllFilesWildcard() : wildcard + wxS( "|" ) + FILEEXT::AllFilesWildcard();

    wxFileDialog dialog( this, _( "Save Copy" ), copyFolder(),
                         MdwRevisionCopyName( checkout.GetFullName(), memberName, revision ), wildcard,
                         wxFD_SAVE );

    wxWeakRef<PANEL_MDW_VERSIONS> self( this );

    if( dialog.ShowModal() != wxID_OK || !self )
        return;

    // Another item may have been shown, or another copy started, while the dialog was up
    if( generation != m_itemGeneration || m_copying )
        return;

    const wxString target = dialog.GetPath();
    const wxString refusal = MdwCopyTargetRefusal( target, workingFiles );

    if( !refusal.IsEmpty() )
    {
        DisplayErrorMessage( this, refusal );
        return;
    }

    // Only a file there now was offered for replacing, so one made during the download is kept
    const bool replace = wxFileName::FileExists( target );

#ifndef __WXMAC__
    // Asked only after the refusal, so the working file is never offered for replacing first. The macOS save panel
    // always asks itself
    if( replace && !IsOK( this, wxString::Format( _( "The file '%s' already exists. Overwrite it?" ), target ) ) )
        return;
#endif

    // The question is modal as well, so the panel may be gone or showing another item
    if( !self || generation != m_itemGeneration || m_copying )
        return;

    MDW_ITEM_REF shown = item;

    if( !memberName.IsEmpty() )
        shown.path_hint = memberName;

    const wxString label = MdwVersionSideLabel( shown, revision );

    m_copying = true;
    setCopyStatus( wxString::Format( _( "Saving %s…" ), label ) );
    updateCompare();

    m_copyOp = MdwSaveRevisionCopy( source, item, revision.revision_id, target, replace,
                                    [this, label, target]( const MDW_RESULT& aResult, const MDW_REVISION& )
                                    {
                                        m_copying = false;

                                        if( aResult.IsOk() )
                                        {
                                            setCopyStatus( wxString::Format( _( "Saved %s" ),
                                                                             wxFileName( target ).GetFullName() ),
                                                           target );
                                            updateCompare();
                                            return;
                                        }

                                        setCopyStatus( wxEmptyString );
                                        updateCompare();

                                        // The modal error can let the panel be destroyed, so nothing may follow it
                                        DisplayErrorMessage( this,
                                                             wxString::Format( _( "Could not save a copy of %s." ),
                                                                               label ),
                                                             MdwErrorText( aResult ) );
                                    } );
}


void PANEL_MDW_VERSIONS::onContextMenu( wxListEvent& aEvent )
{
    const long row = aEvent.GetIndex();

    if( row < 0 || row >= static_cast<long>( m_rowKeys.size() ) )
        return;

    auto select = [this]( std::initializer_list<long> aRows )
    {
        for( long selected : selectedRows( *m_list ) )
            m_list->SetItemState( selected, 0, wxLIST_STATE_SELECTED );

        for( long selected : aRows )
            m_list->SetItemState( selected, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    };

    // The menu acts on the clicked row alone, as the buttons would, which a right click does not always select
    select( { row } );

    const long total = static_cast<long>( m_rowKeys.size() );
    const bool isVersion = !m_rowKeys[row].IsEmpty();
    const bool canCompare = !m_comparing && compareDocument().kind != KICAD_DIFF::DOC_KIND::UNKNOWN;

    // PopupMenu runs events, so a read may refill the rows before an entry is chosen
    auto rowNow = [this, generation = m_itemGeneration, key = m_rowKeys[row]]() -> std::optional<long>
    {
        if( generation != m_itemGeneration )
            return std::nullopt;

        auto it = std::find( m_rowKeys.begin(), m_rowKeys.end(), key );

        if( it == m_rowKeys.end() )
            return std::nullopt;

        return static_cast<long>( it - m_rowKeys.begin() );
    };

    wxMenu menu;

    wxMenuItem* previous = menu.Append( wxID_ANY, _( "Compare with Previous" ) );
    wxMenuItem* working = hasWorkingCopy() ? menu.Append( wxID_ANY, _( "Compare with Working Copy" ) ) : nullptr;
    wxMenuItem* save = menu.Append( wxID_ANY, _( "Save Copy..." ) );

    previous->Enable( canCompare && row + 1 < total );

    if( working )
        working->Enable( canCompare && isVersion );

    save->Enable( !m_copying && isVersion );

    menu.Bind( wxEVT_MENU,
               [this, rowNow, select]( wxCommandEvent& )
               {
                   if( std::optional<long> now = rowNow() )
                   {
                       select( { *now } );
                       onCompare();
                   }
               },
               previous->GetId() );

    if( working )
    {
        menu.Bind( wxEVT_MENU,
                   [this, rowNow, select]( wxCommandEvent& )
                   {
                       if( std::optional<long> now = rowNow(); now && hasWorkingCopy() )
                       {
                           select( { *now, 0 } );
                           onCompare();
                       }
                   },
                   working->GetId() );
    }

    menu.Bind( wxEVT_MENU,
               [this, rowNow]( wxCommandEvent& )
               {
                   if( std::optional<long> now = rowNow() )
                       onSaveCopy( *now );
               },
               save->GetId() );

    m_list->PopupMenu( &menu );
}
