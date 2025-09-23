/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialogs/dialog_book_reporter.h>
#include <widgets/wx_html_report_box.h>
#include <wx/event.h>
#include <kiway_player.h>


wxDEFINE_EVENT( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, wxCommandEvent );


DIALOG_BOOK_REPORTER::DIALOG_BOOK_REPORTER( KIWAY_PLAYER* aParent, const wxString& aName,
                                            const wxString& aTitle ) :
    DIALOG_BOOK_REPORTER_BASE( aParent, wxID_ANY, aTitle ),
    m_frame( aParent )
{
    SetName( aName );
    SetupStandardButtons();
    m_sdbSizerApply->Hide();
    finishDialogSettings();
}


void DIALOG_BOOK_REPORTER::DeleteAllPages()
{
    m_notebook->DeleteAllPages();
}


void DIALOG_BOOK_REPORTER::OnErrorLinkClicked( wxHtmlLinkEvent& aEvent )
{
    m_frame->ExecuteRemoteCommand( aEvent.GetLinkInfo().GetHref().ToStdString().c_str() );
}


WX_HTML_REPORT_BOX* DIALOG_BOOK_REPORTER::AddHTMLPage( const wxString& aTitle )
{
    wxPanel* panel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTAB_TRAVERSAL  );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    WX_HTML_REPORT_BOX* reporter = new WX_HTML_REPORT_BOX( panel, wxID_ANY, wxDefaultPosition,
                                                           wxDefaultSize,
                                                           wxHW_SCROLLBAR_AUTO | wxBORDER_SIMPLE );

   	sizer->Add( reporter, 1, wxEXPAND | wxALL, 5 );
   	panel->SetSizer( sizer );
   	panel->Layout();
    m_notebook->AddPage( panel, aTitle );

    reporter->SetUnits( m_frame->GetUserUnits() );
    reporter->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED,
                       wxHtmlLinkEventHandler( DIALOG_BOOK_REPORTER::OnErrorLinkClicked ),
                       nullptr, this );

    return reporter;
}


wxPanel* DIALOG_BOOK_REPORTER::AddBlankPage( const wxString& aTitle )
{
    wxPanel* panel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTAB_TRAVERSAL  );
    m_notebook->AddPage( panel, aTitle );

    return panel;
}


int DIALOG_BOOK_REPORTER::GetPageCount() const
{
    return m_notebook->GetPageCount();
}


void DIALOG_BOOK_REPORTER::OnClose( wxCloseEvent& aEvent )
{
    // Dialog is mode-less so let the parent know that it needs to be destroyed.
    if( !IsModal() && !IsQuasiModal() )
    {
        wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_DIALOG_BOOK_REPORTER, aEvent.GetId() );

        evt->SetEventObject( this );
        evt->SetString( GetName() );
        wxWindow* parent = GetParent();

        if( parent )
            wxQueueEvent( parent, evt );
    }

    aEvent.Skip();
}


void DIALOG_BOOK_REPORTER::OnApply( wxCommandEvent& event )
{
    wxCloseEvent closeEvent;
    closeEvent.SetId( m_sdbSizerApply->GetId() );
    OnClose( closeEvent );
}


void DIALOG_BOOK_REPORTER::OnOK( wxCommandEvent& event )
{
    wxCloseEvent closeEvent;
    closeEvent.SetId( m_sdbSizerOK->GetId() );
    OnClose( closeEvent );
}