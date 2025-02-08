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

#include "dialog_git_commit.h"

#include <bitmaps/bitmaps_list.h>
#include <bitmaps/bitmap_types.h>
#include <git/kicad_git_common.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

DIALOG_GIT_COMMIT::DIALOG_GIT_COMMIT( wxWindow* parent, git_repository* repo,
                                      const wxString&                defaultAuthorName,
                                      const wxString&                defaultAuthorEmail,
                                      const std::map<wxString, int>& filesToCommit ) :
        DIALOG_SHIM( parent, wxID_ANY, _( "Commit Changes" ) )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );


    // List Control for files to commit
    m_listCtrl = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL );

    // Set up columns
    m_listCtrl->EnableCheckBoxes();
    m_listCtrl->AppendColumn( _( "Filename" ) );
    m_listCtrl->AppendColumn( _( "Status" ) );

    // Set column widths
    m_listCtrl->SetColumnWidth( 0, 200 );
    m_listCtrl->SetColumnWidth( 1, 200 );

    // Set up image list for icons
#ifdef __WXMAC__
    // HiDPI-aware API; will be generally available in wxWidgets 3.4
    wxVector<wxBitmapBundle> stateImages;
    stateImages.push_back( wxBitmapBundle() );                          // GIT_STATUS_UNTRACKED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_good_check ) ); // GIT_STATUS_CURRENT
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_modified ) ); // GIT_STATUS_MODIFIED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_add ) );    // GIT_STATUS_ADDED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_delete ) ); // GIT_STATUS_DELETED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_out_of_date ) );   // GIT_STATUS_BEHIND
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_changed_ahead ) ); // GIT_STATUS_AHEAD
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_conflict ) ); // GIT_STATUS_CONFLICTED

    m_listCtrl->SetNormalImages( stateImages );
    m_listCtrl->SetSmallImages( stateImages );
#else
    wxImageList* imageList = new wxImageList(
            16, 16, true, static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_LAST ) );

    imageList->Add( KiBitmap( BITMAPS::git_good_check ) );    // PLACEHOLDER
    imageList->Add( KiBitmap( BITMAPS::git_good_check ) );    // GIT_STATUS_CURRENT
    imageList->Add( KiBitmap( BITMAPS::git_modified ) );      // GIT_STATUS_MODIFIED
    imageList->Add( KiBitmap( BITMAPS::git_add ) );           // GIT_STATUS_ADDED
    imageList->Add( KiBitmap( BITMAPS::git_delete ) );        // GIT_STATUS_DELETED
    imageList->Add( KiBitmap( BITMAPS::git_out_of_date ) );   // GIT_STATUS_BEHIND
    imageList->Add( KiBitmap( BITMAPS::git_changed_ahead ) ); // GIT_STATUS_AHEAD
    imageList->Add( KiBitmap( BITMAPS::git_conflict ) );      // GIT_STATUS_CONFLICTED

    // Assign the image list to the list control
    m_listCtrl->SetImageList( imageList, wxIMAGE_LIST_SMALL );
#endif

    // Populate list control with items
    for( auto& [filename, status] : filesToCommit )
    {
        int i = m_listCtrl->GetItemCount();
        m_listCtrl->InsertItem( i, filename );

        if( status & ( GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW ) )
        {
            m_listCtrl->SetItem( i, 1, _( "New" ) );
            m_listCtrl->SetItemImage(
                    i, static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_ADDED ) );

            if( status & ( GIT_STATUS_INDEX_NEW ) )
                m_listCtrl->CheckItem( i, true );
        }
        else if( status & ( GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED ) )
        {
            m_listCtrl->SetItem( i, 1, _( "Modified" ) );
            m_listCtrl->SetItemImage(
                    i, static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_MODIFIED ) );

            if( status & ( GIT_STATUS_INDEX_MODIFIED ) )
                m_listCtrl->CheckItem( i, true );
        }
        else if( status & ( GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_DELETED ) )
        {
            m_listCtrl->SetItem( i, 1, _( "Deleted" ) );
            m_listCtrl->SetItemImage(
                    i, static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_DELETED ) );

            if( status & ( GIT_STATUS_INDEX_DELETED ) )
                m_listCtrl->CheckItem( i, true );
        }
        else
        {
            printf( " Unknown status: %d\n", status );
        }
    }

    sizer->Add( m_listCtrl, 1, wxEXPAND | wxALL, 5 );

    // Commit Message Text Control
    wxStaticText* commitMessageLabel = new wxStaticText( this, wxID_ANY, _( "Commit Message:" ) );
    m_commitMessageTextCtrl =
            new wxTextCtrl( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    sizer->Add( commitMessageLabel, 0, wxALL, 5 );
    sizer->Add( m_commitMessageTextCtrl, 1, wxEXPAND | wxALL, 5 );

    // Author Name and Email Text Control
    wxStaticText* authorLabel = new wxStaticText( this, wxID_ANY, _( "Author:" ) );
    wxString      defaultAuthor = defaultAuthorName + " <" + defaultAuthorEmail + ">";
    m_authorTextCtrl =
            new wxTextCtrl( this, wxID_ANY, defaultAuthor, wxDefaultPosition, wxDefaultSize, 0 );
    sizer->Add( authorLabel, 0, wxALL, 5 );
    sizer->Add( m_authorTextCtrl, 0, wxEXPAND | wxALL, 5 );

    // OK and Cancel Buttons

    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();

    m_okButton = new wxButton( this, wxID_OK, _( "OK" ) );
    wxButton* cancelButton = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    buttonSizer->Add( cancelButton, 0, wxALL, 5 );
    buttonSizer->Add( m_okButton, 0, wxALL, 5 );
    buttonSizer->Realize();

    sizer->Add( buttonSizer, 0, wxALIGN_RIGHT | wxALL, 5 );

    SetSizerAndFit( sizer );

    SetupStandardButtons( { { wxID_OK, _( "C&ommit" ) } } );

    // Bind events
    Bind( wxEVT_TEXT, &DIALOG_GIT_COMMIT::OnTextChanged, this, m_commitMessageTextCtrl->GetId() );

    // Set the repository and defaults
    m_repo = repo;
    m_defaultAuthorName = defaultAuthorName;
    m_defaultAuthorEmail = defaultAuthorEmail;
}


void DIALOG_GIT_COMMIT::OnTextChanged( wxCommandEvent& aEvent )
{
    if( m_commitMessageTextCtrl->GetValue().IsEmpty() )
    {
        m_okButton->Disable();
        m_okButton->SetToolTip( _( "Commit message cannot be empty" ) );
    }
    else
    {
        m_okButton->Enable();
        m_okButton->SetToolTip( wxEmptyString );
    }
}


wxString DIALOG_GIT_COMMIT::GetCommitMessage() const
{
    return m_commitMessageTextCtrl->GetValue();
}


wxString DIALOG_GIT_COMMIT::GetAuthorName() const
{
    wxString authorText = m_authorTextCtrl->GetValue();
    size_t   pos = authorText.find( '<' );

    if( pos != wxString::npos )
        return authorText.substr( 0, pos ).Trim();

    return wxEmptyString;
}


wxString DIALOG_GIT_COMMIT::GetAuthorEmail() const
{
    wxString authorText = m_authorTextCtrl->GetValue();
    size_t   startPos = authorText.find( '<' );
    size_t   endPos = authorText.find( '>' );

    if( startPos != wxString::npos && endPos != wxString::npos && startPos < endPos )
        return authorText.substr( startPos + 1, endPos - startPos - 1 ).Trim();

    return wxEmptyString;
}


std::vector<wxString> DIALOG_GIT_COMMIT::GetSelectedFiles() const
{
    std::vector<wxString> selectedFiles;

    long item = -1;
    while( ( item = m_listCtrl->GetNextItem( item, wxLIST_NEXT_ALL ) )
           != -1 )
    {
        if( m_listCtrl->IsItemChecked( item ) )
            selectedFiles.push_back( m_listCtrl->GetItemText( item ) );
    }

    return selectedFiles;
}
