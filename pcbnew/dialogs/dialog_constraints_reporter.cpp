/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <dialog_constraints_reporter.h>
#include <pcb_base_frame.h>
#include <wx_html_report_box.h>


DIALOG_CONSTRAINTS_REPORTER::DIALOG_CONSTRAINTS_REPORTER( PCB_BASE_FRAME* aParent ) :
        DIALOG_CONSTRAINTS_REPORTER_BASE( aParent ),
        m_frame( aParent )
{
}


void DIALOG_CONSTRAINTS_REPORTER::FinishInitialization()
{
    FinishDialogSettings();
}


void DIALOG_CONSTRAINTS_REPORTER::DeleteAllPages()
{
    m_notebook->DeleteAllPages();
}


WX_HTML_REPORT_BOX* DIALOG_CONSTRAINTS_REPORTER::AddPage( const wxString& aTitle )
{
    wxPanel* panel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTAB_TRAVERSAL  );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    WX_HTML_REPORT_BOX* reporter = new WX_HTML_REPORT_BOX( panel, wxID_ANY, wxDefaultPosition,
                                                           wxDefaultSize,
                                                           wxHW_SCROLLBAR_AUTO|wxBORDER_SIMPLE );

   	sizer->Add( reporter, 1, wxEXPAND|wxALL, 5 );
   	panel->SetSizer( sizer );
   	panel->Layout();
    m_notebook->AddPage( panel, aTitle );

    reporter->SetUnits( m_frame->GetUserUnits() );

    return reporter;
}

