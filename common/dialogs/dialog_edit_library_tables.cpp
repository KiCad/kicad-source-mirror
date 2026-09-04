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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <wx/button.h>
#include <wx/sizer.h>
#include <confirm.h>
#include <lib_table_notebook_panel.h>

#include <dialogs/dialog_edit_library_tables.h>


DIALOG_EDIT_LIBRARY_TABLES::DIALOG_EDIT_LIBRARY_TABLES( wxWindow* aParent, const wxString& aTitle ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_GlobalTableChanged( false ),
        m_ProjectTableChanged( false ),
        m_infoBar( nullptr ),
        m_contentPanel( nullptr )
{
    // Construction delayed until after panel is installed
}


void DIALOG_EDIT_LIBRARY_TABLES::InstallPanel( wxPanel* aPanel, wxAuiNotebook* aNotebook )
{
    m_contentPanel = aPanel;
    m_notebook = aNotebook;

    // Now perform the body of the constructor
    auto mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    mainSizer->Add( m_contentPanel, 1, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 5 );
    m_contentPanel->SetMinSize( FromDIP( wxSize( 1000, 600 ) ) );

    auto sdbSizer = new wxStdDialogButtonSizer();
    auto sdbSizerOK = new wxButton( this, wxID_OK );
    sdbSizer->AddButton( sdbSizerOK );
    auto sdbSizerCancel = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( sdbSizerCancel );
    sdbSizer->Realize();

    mainSizer->Add( sdbSizer, 0, wxALL|wxEXPAND, 5 );

    SetupStandardButtons();

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_EDIT_LIBRARY_TABLES::onCloseWindow, this );
    Bind( wxEVT_BUTTON, &DIALOG_EDIT_LIBRARY_TABLES::onCancelButton, this, wxID_CANCEL );

    finishDialogSettings();

    // On some windows manager (Unity, XFCE), this dialog is not always raised, depending
    // on how the dialog is run.
    Raise();
}


bool DIALOG_EDIT_LIBRARY_TABLES::TransferDataToWindow()
{
    return m_contentPanel->TransferDataToWindow();
}


bool DIALOG_EDIT_LIBRARY_TABLES::TransferDataFromWindow()
{
    /* Transfer tables edited in dialog to the global and project tables:
     * A good way is to use m_contentPanel->TransferDataFromWindow to do that.
     * But be careful:
     * Since wxWidgets 3.1, it is called by wxDialog::TransferDataFromWindow()
     * Before wxWidgets 3.1, it is *not* called by wxDialog::TransferDataFromWindow()
     * m_contentPanel->TransferDataFromWindow do not works with two calls.
     * Therefore *do not* call wxDialog::TransferDataFromWindow(),
     * because m_contentPanel->TransferDataFromWindow() is called here.
     */
    return m_contentPanel->TransferDataFromWindow();
}


bool DIALOG_EDIT_LIBRARY_TABLES::CanClose()
{
    if( m_closeOKed )
        return true;

    std::vector<LIB_TABLE_NOTEBOOK_PANEL*> pages;
    pages.reserve( m_notebook->GetPageCount() );

    for( int ii = 0; ii < (int) m_notebook->GetPageCount(); ++ii )
        pages.push_back( static_cast<LIB_TABLE_NOTEBOOK_PANEL*>( m_notebook->GetPage( ii ) ) );

    bool modified = false;

    for( LIB_TABLE_NOTEBOOK_PANEL* page : pages )
    {
        page->GetGrid()->CommitPendingChanges();
        modified |= page->TableModified();
    }

    if( modified && !::HandleUnsavedChanges( this, _( "Save changes to library tables?" ),
            [&]() -> bool
            {
                for( LIB_TABLE_NOTEBOOK_PANEL* page : pages )
                    modified |= page->SaveTable();

                m_GlobalTableChanged = true;
                m_ProjectTableChanged = true;

                return true;
            } ) )
    {
        return false;
    };

    m_closeOKed = true;
    return true;
}


void DIALOG_EDIT_LIBRARY_TABLES::onCloseWindow( wxCloseEvent& aEvent )
{
    if( !CanClose() )
    {
        aEvent.Veto();
        return;
    }

    aEvent.Skip();
}


void DIALOG_EDIT_LIBRARY_TABLES::onCancelButton( wxCommandEvent& aEvent )
{
    if( !CanClose() )
        return;

    aEvent.Skip();
}


