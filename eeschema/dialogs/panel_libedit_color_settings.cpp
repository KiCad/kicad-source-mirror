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
#include <lib_edit_frame.h>
#include <libedit_settings.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <settings/settings_manager.h>
#include <view/view.h>

#include "panel_libedit_color_settings.h"


PANEL_LIBEDIT_COLOR_SETTINGS::PANEL_LIBEDIT_COLOR_SETTINGS( LIB_EDIT_FRAME* aFrame,
                                                            wxWindow* aWindow )
        : PANEL_LIBEDIT_COLOR_SETTINGS_BASE( aWindow ), m_frame( aFrame )
{
}


bool PANEL_LIBEDIT_COLOR_SETTINGS::TransferDataToWindow()
{
    auto cfg = Pgm().GetSettingsManager().GetAppSettings<LIBEDIT_SETTINGS>();

    m_useEeschemaTheme->SetValue( cfg->m_UseEeschemaColorSettings );

    COLOR_SETTINGS* current = Pgm().GetSettingsManager().GetColorSettings( cfg->m_ColorTheme );

    for( COLOR_SETTINGS* settings : Pgm().GetSettingsManager().GetColorSettingsList() )
    {
        int pos = m_themeSelection->Append( settings->GetName(), static_cast<void*>( settings ) );

        if( settings == current )
            m_themeSelection->SetSelection( pos );
    }

    return true;
}


bool PANEL_LIBEDIT_COLOR_SETTINGS::TransferDataFromWindow()
{
    auto selected = static_cast<COLOR_SETTINGS*>(
            m_themeSelection->GetClientData( m_themeSelection->GetSelection() ) );

    auto cfg = Pgm().GetSettingsManager().GetAppSettings<LIBEDIT_SETTINGS>();

    cfg->m_UseEeschemaColorSettings = m_useEeschemaTheme->GetValue();
    cfg->m_ColorTheme               = selected->GetFilename();

    if( cfg->m_UseEeschemaColorSettings )
        selected = m_frame->GetColorSettings();

    auto settings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    settings->LoadColors( selected );

    return true;
}
