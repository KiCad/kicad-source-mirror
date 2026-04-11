/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_update_notice.h>
#include <build_version.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <pgm_base.h>

DIALOG_UPDATE_NOTICE::DIALOG_UPDATE_NOTICE( wxWindow* aWindow, const wxString& aNewVersion,
                                            const wxString& aDetailsUrl,
                                            const wxString& aDownloadsUrl ) :
        DIALOG_UPDATE_NOTICE_BASE( aWindow ),
        m_detailsUrl( aDetailsUrl ),
        m_downloadsUrl( aDownloadsUrl )
{
    wxString msg = wxString::Format( _( "KiCad %s is now available (you have %s). Would you like to download it now?" ), aNewVersion, GetMajorMinorPatchVersion()  );
    m_messageLine2->SetLabelText( msg );

    Fit();
    Layout();
}


void DIALOG_UPDATE_NOTICE::OnSkipThisVersionClicked( wxCommandEvent& aEvent )
{
    EndModal( wxID_NO );
}


void DIALOG_UPDATE_NOTICE::OnBtnRemindMeClicked( wxCommandEvent& aEvent )
{
    EndModal( wxID_RETRY );
}


void DIALOG_UPDATE_NOTICE::OnBtnDetailsPageClicked( wxCommandEvent& aEvent )
{
    wxLaunchDefaultBrowser( m_detailsUrl, wxBROWSER_NEW_WINDOW );
}


void DIALOG_UPDATE_NOTICE::OnBtnDownloadsPageClicked( wxCommandEvent& aEvent )
{
    wxLaunchDefaultBrowser( m_downloadsUrl, wxBROWSER_NEW_WINDOW );
    EndModal( wxID_YES );
}