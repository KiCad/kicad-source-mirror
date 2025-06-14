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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pl_editor_settings.h>
#include "panel_pl_editor_color_settings.h"


PANEL_PL_EDITOR_COLOR_SETTINGS::PANEL_PL_EDITOR_COLOR_SETTINGS( wxWindow* aParent ) :
        PANEL_PL_EDITOR_COLOR_SETTINGS_BASE( aParent )
{
}


bool PANEL_PL_EDITOR_COLOR_SETTINGS::TransferDataToWindow()
{
    PL_EDITOR_SETTINGS* cfg = GetAppSettings<PL_EDITOR_SETTINGS>( "pl_editor" );
    COLOR_SETTINGS*     current = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    int width    = 0;
    int height   = 0;
    int minwidth = width;

    m_themes->Clear();

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        int pos = m_themes->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings == current )
            m_themes->SetSelection( pos );

        m_themes->GetTextExtent( settings->GetName(), &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_themes->SetMinSize( wxSize( minwidth + 50, -1 ) );

    Fit();

    return true;
}


bool PANEL_PL_EDITOR_COLOR_SETTINGS::TransferDataFromWindow()
{
    int sel = m_themes->GetSelection();

    if( sel >= 0 )
    {
        if( PL_EDITOR_SETTINGS* cfg = GetAppSettings<PL_EDITOR_SETTINGS>( "pl_editor" ) )
        {
            COLOR_SETTINGS* colors = static_cast<COLOR_SETTINGS*>( m_themes->GetClientData( sel ) );
            cfg->m_ColorTheme = colors->GetFilename();
        }
    }

    return true;
}


void PANEL_PL_EDITOR_COLOR_SETTINGS::ResetPanel()
{
    m_themes->SetStringSelection( _( "KiCad Default" ) );
}


