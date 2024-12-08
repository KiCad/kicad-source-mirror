/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_update_check_prompt.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <pgm_base.h>

DIALOG_UPDATE_CHECK_PROMPT::DIALOG_UPDATE_CHECK_PROMPT( wxWindow* aWindow ) :
        DIALOG_UPDATE_CHECK_PROMPT_BASE( aWindow )
{
#ifndef KICAD_UPDATE_CHECK
    m_cbKiCadUpdates->Hide();
#endif
}


bool DIALOG_UPDATE_CHECK_PROMPT::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    settings->m_PcmUpdateCheck = m_cbPCMUpdates->GetValue();
#ifndef KICAD_UPDATE_CHECK
    settings->m_KiCadUpdateCheck = m_cbKiCadUpdates->GetValue();
#endif

    return true;
}


bool DIALOG_UPDATE_CHECK_PROMPT::TransferDataToWindow()
{
    // Since this is a first time start dialog, just default to both checks true
    m_cbPCMUpdates->SetValue( true );
    m_cbKiCadUpdates->SetValue( true );

    return true;
}