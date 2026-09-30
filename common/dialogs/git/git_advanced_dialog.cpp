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
#include <dialogs/git/git_status_entries.h>

#include <memory>
#include <vector>

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>

#include <confirm.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
#include <kiway.h>
#include <project.h>
#include <widgets/ui_common.h>
#include <widgets/wx_treebook.h>


namespace
{

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

} // namespace


GIT_ADVANCED_DIALOG::GIT_ADVANCED_DIALOG( EDA_BASE_FRAME* aParent ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Git" ), wxDefaultPosition, wxSize( 780, 560 ),
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, wxT( "GitAdvancedDialog" ) ),
        m_parent( aParent ),
        m_book( nullptr ),
        m_historyList( nullptr ),
        m_branchesList( nullptr ),
        m_remotesList( nullptr ),
        m_stashesList( nullptr ),
        m_tagsList( nullptr ),
        m_repoUserName( nullptr ),
        m_repoUserEmail( nullptr ),
        m_repoRemoteUrl( nullptr )
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

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_book = new WX_TREEBOOK( this, wxID_ANY );
    m_book->SetFont( KIUI::GetControlFont( this ) );
    m_book->SetFitToCurrentPage( true );

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


void GIT_ADVANCED_DIALOG::ShowForFrame( EDA_BASE_FRAME* aParent )
{
    if( !activeDialog() )
    {
        GIT_ADVANCED_DIALOG* dialog = new GIT_ADVANCED_DIALOG( aParent );

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


GIT_ADVANCED_DIALOG::~GIT_ADVANCED_DIALOG()
{
    if( activeDialog() == this )
        setActiveDialog( nullptr );

    if( std::shared_ptr<KIGIT_COMMON> session = m_listenerSession.lock() )
        session->RemoveRefreshListener( this );
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
    // background fetch can hold it for a network round trip without broadcasting a refresh.
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    if( !lock.owns_lock() )
    {
        m_reloadRetryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }
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
