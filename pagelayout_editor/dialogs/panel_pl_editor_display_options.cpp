/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <settings/settings_manager.h>
#include <pl_editor_settings.h>
#include <panel_pl_editor_display_options.h>
#include <widgets/gal_options_panel.h>
#include <widgets/ui_common.h>
#include <wx/sizer.h>


PANEL_PL_EDITOR_DISPLAY_OPTIONS::PANEL_PL_EDITOR_DISPLAY_OPTIONS( wxWindow* aParent,
                                                                  APP_SETTINGS_BASE* aAppSettings ) :
        RESETTABLE_PANEL( aParent )
{
    wxBoxSizer* bPanelSizer = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* bLeftCol = new wxBoxSizer( wxVERTICAL );

    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, aAppSettings );
    bLeftCol->Add( m_galOptsPanel, 1, wxLEFT|wxBOTTOM|wxEXPAND, 5 );

    bPanelSizer->Add( bLeftCol, 0, wxEXPAND, 0 );

   	this->SetSizer( bPanelSizer );
   	this->Layout();
   	bPanelSizer->Fit( this );
}


bool PANEL_PL_EDITOR_DISPLAY_OPTIONS::TransferDataToWindow()
{
    m_galOptsPanel->TransferDataToWindow();

    return true;
}


bool PANEL_PL_EDITOR_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    m_galOptsPanel->TransferDataFromWindow();

    return true;
}


void PANEL_PL_EDITOR_DISPLAY_OPTIONS::ResetPanel()
{
    PL_EDITOR_SETTINGS cfg;
    cfg.Load();               // Loading without a file will init to defaults

    m_galOptsPanel->ResetPanel( &cfg );
}


