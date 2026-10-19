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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "dialog_git_switch.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <algorithm>
#include <utility>


DIALOG_GIT_SWITCH::DIALOG_GIT_SWITCH( wxWindow* aParent, std::vector<KIGIT_BRANCH_INFO> aBranches ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Git Branch Switch" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_branchNameDebounce(
                [this]()
                {
                    updateSwitchButton();
                },
                500 ),
        m_branches( std::move( aBranches ) )
{
    // Only local branches can be checked out. Keep the picker alphabetical.
    std::erase_if( m_branches,
                   []( const KIGIT_BRANCH_INFO& aBranch )
                   {
                       return aBranch.isRemote;
                   } );
    std::sort( m_branches.begin(), m_branches.end(),
               []( const KIGIT_BRANCH_INFO& aFirst, const KIGIT_BRANCH_INFO& aSecond )
               {
                   return aFirst.name < aSecond.name;
               } );

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

    // Populate branch list
    PopulateBranchList();

    // Set sizer for the dialog
    SetSizerAndFit( sizer );

    finishDialogSettings();
}


DIALOG_GIT_SWITCH::~DIALOG_GIT_SWITCH()
{
    m_branchNameDebounce.Cancel();
}


void DIALOG_GIT_SWITCH::PopulateBranchList()
{
    m_branchList->DeleteAllItems();
    m_currentBranch.clear();

    long currentIndex = -1;

    // Populate the list
    for( const KIGIT_BRANCH_INFO& branch : m_branches )
    {
        wxString displayName = ( branch.isCurrent ? wxS( "* " ) : wxS( "  " ) ) + branch.name;

        long itemIndex = m_branchList->InsertItem( m_branchList->GetItemCount(), displayName );
        m_branchList->SetItem( itemIndex, 1, branch.message );
        m_branchList->SetItem( itemIndex, 2, branch.when.Format() );

        if( branch.isCurrent )
        {
            m_currentBranch = branch.name;
            currentIndex = itemIndex;
        }
    }

    m_branchList->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_branchList->SetColumnWidth( 1, wxLIST_AUTOSIZE );
    m_branchList->SetColumnWidth( 2, wxLIST_AUTOSIZE );

    if( currentIndex >= 0 )
    {
        m_branchList->Select( currentIndex );
        m_branchList->EnsureVisible( currentIndex );
        m_branchNameText->SetValue( m_currentBranch );
    }
}


void DIALOG_GIT_SWITCH::OnBranchListDClick( wxListEvent& aEvent )
{
    int selection = aEvent.GetIndex();

    if( selection >= 0 && static_cast<size_t>( selection ) < m_branches.size() )
    {
        const wxString& branchName = m_branches[selection].name;
        m_branchNameText->SetValue( branchName );

        if( branchName != m_currentBranch )
            EndModal( wxID_OK );
    }
}


void DIALOG_GIT_SWITCH::OnBranchListSelection( wxListEvent& aEvent )
{
    int selection = aEvent.GetIndex();

    if( selection >= 0 && static_cast<size_t>( selection ) < m_branches.size() )
    {
        const wxString& branchName = m_branches[selection].name;
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


void DIALOG_GIT_SWITCH::OnSwitchButton( wxCommandEvent& aEvent )
{
    EndModal( branchExists( m_branchNameText->GetValue() ) ? wxID_OK : wxID_ADD );
}


void DIALOG_GIT_SWITCH::OnCancelButton(wxCommandEvent& aEvent)
{
    EndModal(wxID_CANCEL); // Return Cancel code
}


wxString DIALOG_GIT_SWITCH::GetBranchName() const
{
    return m_branchNameText->GetValue();
}


void DIALOG_GIT_SWITCH::updateSwitchButton()
{
    wxString branchName = m_branchNameText->GetValue();

    if( branchName == m_lastEnteredText )
        return;

    m_lastEnteredText = branchName;

    if( branchExists( branchName ) )
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
    m_branchNameDebounce.Restart();
}


bool DIALOG_GIT_SWITCH::branchExists( const wxString& aName ) const
{
    return std::any_of( m_branches.begin(), m_branches.end(),
                        [&aName]( const KIGIT_BRANCH_INFO& aBranch )
                        {
                            return aBranch.name == aName;
                        } );
}
