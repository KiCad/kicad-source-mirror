/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <painter.h>
#include <view/view.h>

#include "panel_pl_editor_color_settings.h"
#include "pl_editor_frame.h"
#include "pl_editor_settings.h"


PANEL_PL_EDITOR_COLOR_SETTINGS::PANEL_PL_EDITOR_COLOR_SETTINGS( PL_EDITOR_FRAME* aFrame,
                                                                wxWindow* aWindow )
        : PANEL_PL_EDITOR_COLOR_SETTINGS_BASE( aWindow ),
          m_frame( aFrame )
{
}


bool PANEL_PL_EDITOR_COLOR_SETTINGS::TransferDataToWindow()
{
    PL_EDITOR_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PL_EDITOR_SETTINGS>();

    COLOR_SETTINGS* current = Pgm().GetSettingsManager().GetColorSettings( cfg->m_ColorTheme );

    int width    = 0;
    int height   = 0;
    int minwidth = width;

    m_themeSelection->Clear();

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        int pos = m_themeSelection->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings == current )
            m_themeSelection->SetSelection( pos );

        m_themeSelection->GetTextExtent( settings->GetName(), &width, &height );
        minwidth = std::max( minwidth, width );
    }

    m_themeSelection->SetMinSize( wxSize( minwidth + 50, -1 ) );

    Fit();

    return true;
}


bool PANEL_PL_EDITOR_COLOR_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    auto selected = static_cast<COLOR_SETTINGS*>(
            m_themeSelection->GetClientData( m_themeSelection->GetSelection() ) );

    PL_EDITOR_SETTINGS* cfg = mgr.GetAppSettings<PL_EDITOR_SETTINGS>();

    cfg->m_ColorTheme = selected->GetFilename();

    auto settings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    settings->LoadColors( selected );

    return true;
}