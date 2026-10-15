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

#include <dialogs/git/dialog_mdw_part_history.h>

#include <dialog_shim.h>
#include <dialogs/git/panel_mdw_versions.h>
#include <eda_base_frame.h>
#include <mdw/client/mdw_library_sources.h>
#include <widgets/mdw_part_controller.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/intl.h>
#include <wx/sizer.h>


void ShowMdwPartHistory( EDA_BASE_FRAME* aFrame, const MDW_PART_TARGET& aTarget, const wxString& aName )
{
    // Modeless, so the diff viewer Compare opens takes input beside it
    DIALOG_SHIM* dlg = new DIALOG_SHIM( aFrame, wxID_ANY, wxString::Format( _( "History of %s" ), aName ),
                                        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    PANEL_MDW_VERSIONS*     versions = new PANEL_MDW_VERSIONS( dlg );
    wxBoxSizer*             sizer = new wxBoxSizer( wxVERTICAL );
    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();

    buttons->AddButton( new wxButton( dlg, wxID_CANCEL, _( "Close" ) ) );
    buttons->Realize();

    sizer->Add( versions, 1, wxEXPAND | wxALL, KIUI::GetStdMargin() );
    sizer->Add( buttons, 0, wxEXPAND | wxALL, KIUI::GetStdMargin() );
    sizer->SetMinSize( dlg->FromDIP( wxSize( 560, 360 ) ) );
    dlg->SetSizerAndFit( sizer );

    std::shared_ptr<const MDW_LIBRARY_COPY> copy = MDW_LIBRARY_SOURCES::Get().SharedCopy( aTarget.marker );

    versions->SetItem( aTarget.source, aTarget.item, copy ? copy->local_root : wxString(),
                       WORKSPACE_WORKING_FILES( aTarget.localPath ), aFrame, aName );

    dlg->Bind( wxEVT_CLOSE_WINDOW,
               [dlg]( wxCloseEvent& )
               {
                   dlg->Destroy();
               } );

    dlg->Bind( wxEVT_BUTTON,
               [dlg]( wxCommandEvent& )
               {
                   dlg->Close();
               },
               wxID_CANCEL );

    dlg->Centre();
    dlg->Show( true );
}
