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

#include <dialogs/git/dialog_design_workspace.h>

#include <dialogs/git/panel_git_workspace.h>
#include <eda_base_frame.h>

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/intl.h>


static const wxString WINDOW_NAME = wxS( "DesignWorkspaceWindow" );


DIALOG_DESIGN_WORKSPACE::DIALOG_DESIGN_WORKSPACE( EDA_BASE_FRAME* aFrame ) :
        DIALOG_SHIM( aFrame, wxID_ANY, _( "Design Workspace" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, WINDOW_NAME )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    m_page = new PANEL_GIT_WORKSPACE( this, nullptr, aFrame );
    m_page->SetMinSize( FromDIP( wxSize( 820, 460 ) ) );
    topSizer->Add( m_page, 1, wxEXPAND | wxALL, FromDIP( 4 ) );

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    buttons->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttons->Realize();
    topSizer->Add( buttons, 0, wxEXPAND | wxALL, FromDIP( 8 ) );

    SetSizer( topSizer );
    SetupStandardButtons( { { wxID_CANCEL, _( "Close" ) } } );

    // Modeless, so closing destroys rather than ending a modal loop
    Bind( wxEVT_BUTTON,
          [this]( wxCommandEvent& )
          {
              Close();
          },
          wxID_CANCEL );

    Bind( wxEVT_CLOSE_WINDOW,
          [this]( wxCloseEvent& )
          {
              Show( false );
              Destroy();
          } );

    finishDialogSettings();
}


void DIALOG_DESIGN_WORKSPACE::ShowForFrame( EDA_BASE_FRAME* aFrame, const wxString& aItemId )
{
    DIALOG_DESIGN_WORKSPACE* dialog =
            dynamic_cast<DIALOG_DESIGN_WORKSPACE*>( wxWindow::FindWindowByName( WINDOW_NAME, aFrame ) );

    if( !dialog )
        dialog = new DIALOG_DESIGN_WORKSPACE( aFrame );

    dialog->m_page->Populate();

    if( !aItemId.IsEmpty() )
        dialog->m_page->SelectItem( aItemId );

    dialog->Show( true );
    dialog->Raise();
}
