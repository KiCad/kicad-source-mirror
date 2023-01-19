/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_pcm_settings.h"

#include <pgm_base.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <widgets/ui_common.h>

PANEL_PCM_SETTINGS::PANEL_PCM_SETTINGS( wxWindow* parent ) : PANEL_PCM_SETTINGS_BASE( parent )
{
    wxSize minSize = m_libPrefix->GetMinSize();
    int    minWidth = m_libPrefix->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_libPrefix->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
}


bool PANEL_PCM_SETTINGS::TransferDataToWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>();

    m_updateCheck->SetValue( settings->m_PcmUpdateCheck );
    m_libAutoAdd->SetValue( settings->m_PcmLibAutoAdd );
    m_libAutoRemove->SetValue( settings->m_PcmLibAutoRemove );
    m_libPrefix->SetValue( settings->m_PcmLibPrefix );

    return true;
}


bool PANEL_PCM_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>();

    settings->m_PcmUpdateCheck = m_updateCheck->GetValue();
    settings->m_PcmLibAutoAdd = m_libAutoAdd->GetValue();
    settings->m_PcmLibAutoRemove = m_libAutoRemove->GetValue();
    settings->m_PcmLibPrefix = m_libPrefix->GetValue();

    return true;
}
