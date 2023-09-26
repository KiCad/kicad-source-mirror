/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_footprint_chooser.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <pcb_base_frame.h>
#include <widgets/panel_footprint_chooser.h>


DIALOG_FOOTPRINT_CHOOSER::DIALOG_FOOTPRINT_CHOOSER( PCB_BASE_FRAME* aParent,
                                                    const LIB_ID& aPreselect,
                                                    const wxArrayString& aFootprintHistoryList ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Choose Footprint" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_chooserPanel = new PANEL_FOOTPRINT_CHOOSER( aParent, this, aFootprintHistoryList,
                                                  [this]()
                                                  {
                                                      EndQuasiModal( wxID_OK );
                                                  } );

    sizer->Add( m_chooserPanel, 1, wxEXPAND, 5 );

    if( aPreselect.IsValid() )
        m_chooserPanel->SetPreselect( aPreselect );

    SetTitle( GetTitle() + wxString::Format( _( " (%d items loaded)" ),
                                             m_chooserPanel->GetItemCount() ) );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( this, wxID_OK );
    wxButton*               cancelButton = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    sizer->Add( sdbSizer, 0, wxEXPAND | wxALL, 5 );
    SetSizer( sizer );

    SetInitialFocus( m_chooserPanel->GetFocusTarget() );
    SetupStandardButtons();

    m_chooserPanel->FinishSetup();
    Layout();
}


LIB_ID DIALOG_FOOTPRINT_CHOOSER::GetSelectedLibId() const
{
    return m_chooserPanel->GetSelectedLibId();
}
