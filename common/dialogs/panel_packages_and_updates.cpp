/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Andrew Lutsenko, anlutsenko at gmail dot com
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

#include "panel_packages_and_updates.h"

#include <pgm_base.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <widgets/ui_common.h>

PANEL_PACKAGES_AND_UPDATES::PANEL_PACKAGES_AND_UPDATES( wxWindow* parent ) :
        PANEL_PACKAGES_AND_UPDATES_BASE( parent )
{
    wxSize minSize = m_libPrefix->GetMinSize();
    int    minWidth = m_libPrefix->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_libPrefix->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

#ifndef KICAD_UPDATE_CHECK
    m_generalLabel->Hide();
    m_staticline3->Hide();
    m_cbKicadUpdate->Hide();
#endif
}


bool PANEL_PACKAGES_AND_UPDATES::TransferDataToWindow()
{
    if( KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" ) )
    {
        m_cbKicadUpdate->SetValue( cfg->m_KiCadUpdateCheck );
        m_cbPcmUpdate->SetValue( cfg->m_PcmUpdateCheck );
        m_libAutoAdd->SetValue( cfg->m_PcmLibAutoAdd );
        m_libAutoRemove->SetValue( cfg->m_PcmLibAutoRemove );
        m_libPrefix->SetValue( cfg->m_PcmLibPrefix );
    }

    return true;
}


bool PANEL_PACKAGES_AND_UPDATES::TransferDataFromWindow()
{
    if( KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" ) )
    {
        cfg->m_KiCadUpdateCheck = m_cbKicadUpdate->GetValue();
        cfg->m_PcmUpdateCheck = m_cbPcmUpdate->GetValue();
        cfg->m_PcmLibAutoAdd = m_libAutoAdd->GetValue();
        cfg->m_PcmLibAutoRemove = m_libAutoRemove->GetValue();
        cfg->m_PcmLibPrefix = m_libPrefix->GetValue();
    }

    return true;
}
