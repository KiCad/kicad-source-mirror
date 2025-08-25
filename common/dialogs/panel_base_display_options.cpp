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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/panel_gal_options.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_base_display_options.h>


PANEL_BASE_DISPLAY_OPTIONS::PANEL_BASE_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
        wxPanel( aParent, wxID_ANY )
{
    auto mainSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( mainSizer );

    // install GAL options pane
    m_galOptsPanel = new PANEL_GAL_OPTIONS( this, aAppSettings );
    mainSizer->Add( m_galOptsPanel, 1, wxEXPAND | wxLEFT, 5 );

    // a spacer to take up the other half of the width
    auto spacer = new wxPanel( this, wxID_ANY );
    mainSizer->Add( spacer, 1, wxEXPAND | wxLEFT, 5 );
}


bool PANEL_BASE_DISPLAY_OPTIONS::TransferDataToWindow()
{
    m_galOptsPanel->TransferDataToWindow();
    return true;
}


bool PANEL_BASE_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    m_galOptsPanel->TransferDataFromWindow();
    return true;
}
