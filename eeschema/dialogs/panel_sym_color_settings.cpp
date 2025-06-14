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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>
#include "panel_sym_color_settings.h"


PANEL_SYM_COLOR_SETTINGS::PANEL_SYM_COLOR_SETTINGS( wxWindow* aWindow ) :
        PANEL_SYM_COLOR_SETTINGS_BASE( aWindow )
{
}


bool PANEL_SYM_COLOR_SETTINGS::TransferDataToWindow()
{
    SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );

    if( cfg && cfg->m_UseEeschemaColorSettings )
        m_eeschemaRB->SetValue( true );
    else
        m_themeRB->SetValue( true );

    COLOR_SETTINGS* current = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

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


bool PANEL_SYM_COLOR_SETTINGS::TransferDataFromWindow()
{
    if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
    {
        cfg->m_UseEeschemaColorSettings = m_eeschemaRB->GetValue();

        if( !cfg->m_UseEeschemaColorSettings )
        {
            int             sel = m_themes->GetSelection();
            COLOR_SETTINGS* colors = static_cast<COLOR_SETTINGS*>( m_themes->GetClientData( sel ) );

            cfg->m_ColorTheme = colors->GetFilename();
        }
    }

    return true;
}


void PANEL_SYM_COLOR_SETTINGS::OnThemeChanged( wxCommandEvent& event )
{
    m_themeRB->SetValue( true );
}
