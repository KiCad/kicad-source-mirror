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


#include "dialog_git_switch.h"

#include <git/kicad_git_memory.h>
#include <trace_helpers.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <git2.h>


DIALOG_GIT_SWITCH::DIALOG_GIT_SWITCH( wxWindow* aParent, git_repository* aRepository ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Git Branch Switch" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_timer( this ), m_repository( aRepository )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // Add explanation text
    wxStaticText* explanationText =
            new wxStaticText( this, wxID_ANY, _( "Select or enter a branch name:" ) );
    sizer->Add( explanationText, 0, wxALL, 10 );

    // Add branch list with three columns
    m_branchList = new wxListView( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_SINGLE_SEL );
    m_branchList->InsertColumn( 0, _( "Branch" ) );
    m_branchList->InsertColumn( 1, _( "Last Commit" ) );
    m_branchList->InsertColumn( 2, _( "Last Updated" ) );
    sizer->Add( m_branchList, 1, wxALL | wxEXPAND, 10 );

    // Add branch name text box
    m_branchNameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                       wxDefaultSize, wxTE_PROCESS_ENTER );
    sizer->Add( m_branchNameText, 0, wxALL | wxEXPAND, 10 );

    // Add buttons
    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
    m_switchButton = new wxButton( this, wxID_OK, _( "Switch" ) );
    buttonSizer->AddButton( m_switchButton );
    m_switchButton->Disable();
    wxButton* cancelButton = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    buttonSizer->AddButton( cancelButton );
    buttonSizer->Realize();
    sizer->Add( buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10 );

    // Bind events
    Bind( wxEVT_LIST_ITEM_SELECTED, &DIALOG_GIT_SWITCH::OnBranchListSelection, this, m_branchList->GetId() );
    Bind( wxEVT_LIST_ITEM_ACTIVATED, &DIALOG_GIT_SWITCH::OnBranchListDClick, this, m_branchList->GetId() );
    Bind( wxEVT_BUTTON, &DIALOG_GIT_SWITCH::OnSwitchButton, this, m_switchButton->GetId() );
    Bind( wxEVT_BUTTON, &DIALOG_GIT_SWITCH::OnCancelButton, this, cancelButton->GetId() );
    Bind( wxEVT_TEXT, &DIALOG_GIT_SWITCH::OnTextChanged, this, m_branchNameText->GetId() );
    Bind( wxEVT_TIMER, &DIALOG_GIT_SWITCH::OnTimer, this, m_timer.GetId() );

    // Populate branch list
    PopulateBranchList();

    // Set sizer for the dialog
    SetSizerAndFit( sizer );

    finishDialogSettings();

    m_existingBranch = false;
}

DIALOG_GIT_SWITCH::~DIALOG_GIT_SWITCH()
{
    StopTimer();
    Unbind( wxEVT_TIMER, &DIALOG_GIT_SWITCH::OnTimer, this, m_timer.GetId() );
}

void DIALOG_GIT_SWITCH::PopulateBranchList()
{
    m_branchList->DeleteAllItems();

    // Get the branches
    GetBranches();

    // Populate the list
    for( auto& [ name, data ] : m_branches )
    {
        wxDateTime lastUpdated( data.lastUpdated );
        wxString   lastUpdatedString = lastUpdated.Format();

        long itemIndex = m_branchList->InsertItem( m_branchList->GetItemCount(), name );
        m_branchList->SetItem( itemIndex, 1, data.commitString );
        m_branchList->SetItem( itemIndex, 2, lastUpdatedString );
    }

    m_branchList->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_branchList->SetColumnWidth( 1, wxLIST_AUTOSIZE );
    m_branchList->SetColumnWidth( 2, wxLIST_AUTOSIZE );

}


void DIALOG_GIT_SWITCH::OnBranchListDClick( wxListEvent& aEvent )
{
    int selection = aEvent.GetIndex();

    if( selection != wxNOT_FOUND )
    {
        wxString branchName = m_branchList->GetItemText( selection );
        m_branchNameText->SetValue( branchName );

        if( branchName != m_currentBranch )
            EndModal( wxID_OK );
    }
}


void DIALOG_GIT_SWITCH::OnBranchListSelection( wxListEvent& aEvent )
{
    int selection = aEvent.GetIndex();

    if( selection != wxNOT_FOUND )
    {
        wxString branchName = m_branchList->GetItemText( selection );
        m_branchNameText->SetValue( branchName );
        m_switchButton->SetLabel( _( "Switch" ) );
        m_switchButton->Enable( branchName != m_currentBranch );
    }
    else
    {
        // Deselect all elements in the list
        for( int ii = 0; ii < m_branchList->GetItemCount(); ++ii )
            m_branchList->SetItemState( ii, 0, 0 );
    }
}

void DIALOG_GIT_SWITCH::OnSwitchButton(wxCommandEvent& aEvent)
{
    wxString branchName = m_branchNameText->GetValue();

    // Check if the branch name exists
    bool branchExists = m_branches.count(branchName);

    if (branchExists)
    {
        EndModal(wxID_OK); // Return Switch code
    }
    else
    {
        EndModal(wxID_ADD); // Return Add code
    }
}


void DIALOG_GIT_SWITCH::OnCancelButton(wxCommandEvent& aEvent)
{
    EndModal(wxID_CANCEL); // Return Cancel code
}


wxString DIALOG_GIT_SWITCH::GetBranchName() const
{
    return m_branchNameText->GetValue();
}


void DIALOG_GIT_SWITCH::StartTimer()
{
    m_timer.Start( 500, true );
}


void DIALOG_GIT_SWITCH::StopTimer()
{
    m_timer.Stop();
}


void DIALOG_GIT_SWITCH::OnTimer( wxTimerEvent& aEvt )
{
    wxString branchName = m_branchNameText->GetValue();

    if( branchName == m_lastEnteredText )
        return;

    m_lastEnteredText = branchName;

    // Check if the branch name exists
    bool branchExists = m_branches.count( branchName );

    if( branchExists )
    {
        m_switchButton->SetLabel( _( "Switch" ) );
        m_switchButton->Enable( branchName != m_currentBranch );
    }
    else
    {
        m_switchButton->SetLabel( _( "Add" ) );
        m_switchButton->Enable();
    }
}


void DIALOG_GIT_SWITCH::OnTextChanged( wxCommandEvent& aEvt )
{
    StartTimer();
}


void DIALOG_GIT_SWITCH::GetBranches()
{
    // Clear the branch list
    m_branches.clear();

    git_branch_iterator* branchIterator = nullptr;
    git_branch_t         branchType;

    // Get Current Branch
    git_reference* currentBranchReference = nullptr;
    git_repository_head( &currentBranchReference, m_repository );

    if( !currentBranchReference )
    {
        wxLogTrace( traceGit, "Failed to get current branch" );
        return;
    }

    KIGIT::GitReferencePtr currentBranch( currentBranchReference );
    m_currentBranch = git_reference_shorthand( currentBranchReference );

    // Initialize branch iterator
    git_branch_iterator_new( &branchIterator, m_repository, GIT_BRANCH_ALL );
    KIGIT::GitBranchIteratorPtr branchIteratorPtr( branchIterator );

    // Iterate over local branches
    git_reference* branchReference = nullptr;
    while( git_branch_next( &branchReference, &branchType, branchIterator ) == 0 )
    {
        KIGIT::GitReferencePtr branchReferencePtr( branchReference );

        // Get the branch OID
        const git_oid* branchOid = git_reference_target( branchReference );

        // Skip this branch if it doesn't have an OID
        if( !branchOid )
            continue;

        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, m_repository, branchOid ) )
        {
            // Skip this branch if it doesn't have a commit
            continue;
        }

        KIGIT::GitCommitPtr commitPtr( commit );

        // Retrieve commit details
        BranchData branchData;
        branchData.commitString = git_commit_message( commit );
        branchData.lastUpdated = static_cast<time_t>( git_commit_time( commit ) );
        branchData.isRemote = branchType == GIT_BRANCH_REMOTE;

        m_branches[git_reference_shorthand( branchReference )] = branchData;
    }
}