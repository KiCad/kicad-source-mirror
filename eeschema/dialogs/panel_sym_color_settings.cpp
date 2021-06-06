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

#include <eeschema_settings.h>
#include <symbol_edit_frame.h>
#include <symbol_editor_settings.h>
#include <sch_painter.h>
#include <settings/settings_manager.h>
#include <view/view.h>

#include "panel_sym_color_settings.h"


PANEL_SYM_COLOR_SETTINGS::PANEL_SYM_COLOR_SETTINGS( SYMBOL_EDIT_FRAME* aFrame,
                                                    wxWindow* aWindow )
        : PANEL_SYM_COLOR_SETTINGS_BASE( aWindow ), m_frame( aFrame )
{
}


bool PANEL_SYM_COLOR_SETTINGS::TransferDataToWindow()
{
    SYMBOL_EDITOR_SETTINGS* cfg = m_frame->GetSettings();

    m_useEeschemaTheme->SetValue( cfg->m_UseEeschemaColorSettings );

    COLOR_SETTINGS* current = m_frame->GetSettingsManager()->GetColorSettings( cfg->m_ColorTheme );

    int width    = 0;
    int height   = 0;
    int minwidth = width;

    m_themeSelection->Clear();

    for( COLOR_SETTINGS* settings : m_frame->GetSettingsManager()->GetColorSettingsList() )
    {
        int pos = m_themeSelection->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings == current )
            m_themeSelection->SetSelection( pos );

        m_themeSelection->GetTextExtent( settings->GetName(), &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_themeSelection->SetMinSize( wxSize( minwidth + 50, -1 ) );

    m_txtTheme->Enable( !m_useEeschemaTheme->GetValue() );
    m_themeSelection->Enable( !m_useEeschemaTheme->GetValue() );

    Fit();

    return true;
}


bool PANEL_SYM_COLOR_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER* mgr = m_frame->GetSettingsManager();

    auto selected = static_cast<COLOR_SETTINGS*>(
            m_themeSelection->GetClientData( m_themeSelection->GetSelection() ) );

    SYMBOL_EDITOR_SETTINGS* cfg = mgr->GetAppSettings<SYMBOL_EDITOR_SETTINGS>();

    cfg->m_UseEeschemaColorSettings = m_useEeschemaTheme->GetValue();

    if( !cfg->m_UseEeschemaColorSettings )
        cfg->m_ColorTheme = selected->GetFilename();

    if( cfg->m_UseEeschemaColorSettings )
    {
        EESCHEMA_SETTINGS* eecfg = mgr->GetAppSettings<EESCHEMA_SETTINGS>();
        selected = mgr->GetColorSettings( eecfg->m_ColorTheme );
    }

    auto settings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    settings->LoadColors( selected );

    return true;
}


void PANEL_SYM_COLOR_SETTINGS::OnUseEeschemaThemeChanged( wxCommandEvent& event )
{
    bool useEeschema = m_useEeschemaTheme->GetValue();

    m_txtTheme->Enable( !useEeschema );
    m_themeSelection->Enable( !useEeschema );
}
