/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <config_map.h>
#include <panel_project_defaults.h>
#include <math/util.h>


PANEL_PROJECT_DEFAULTS::PANEL_PROJECT_DEFAULTS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
        PANEL_PROJECT_DEFAULTS_BASE( aParent ),
        m_isPCBEdit( dynamic_cast<PCBNEW_SETTINGS*>( aAppSettings ) != nullptr )
{
}


PANEL_PROJECT_DEFAULTS::~PANEL_PROJECT_DEFAULTS()
{
}


void PANEL_PROJECT_DEFAULTS::loadPCBSettings( PCBNEW_SETTINGS* aCfg )
{
    m_OptTrackOpacity->SetValue( KiROUND( aCfg->m_ProjectDefaults.m_TrackOpacity * 100.0 ) );
    m_OptViaOpacity->SetValue( KiROUND( aCfg->m_ProjectDefaults.m_ViaOpacity * 100.0 ) );
    m_OptPadOpacity->SetValue( KiROUND( aCfg->m_ProjectDefaults.m_PadOpacity * 100.0 ) );
    m_OptZoneOpacity->SetValue( KiROUND( aCfg->m_ProjectDefaults.m_ZoneOpacity * 100.0 ) );
    m_OptImageOpacity->SetValue( KiROUND( aCfg->m_ProjectDefaults.m_ImageOpacity * 100.0 ) );
    m_OptFilledShapeOpacity->SetValue( KiROUND( aCfg->m_ProjectDefaults.m_FilledShapeOpacity * 100.0 ) );
}


bool PANEL_PROJECT_DEFAULTS::TransferDataToWindow()
{
    if( m_isPCBEdit )
        loadPCBSettings( GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) );

    return true;
}


/*
 * Update variables with new options
 */
bool PANEL_PROJECT_DEFAULTS::TransferDataFromWindow()
{
    if( m_isPCBEdit )
    {
        if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        {
            cfg->m_ProjectDefaults.m_TrackOpacity = m_OptTrackOpacity->GetValue() / 100.0;
            cfg->m_ProjectDefaults.m_ViaOpacity = m_OptViaOpacity->GetValue() / 100.0;
            cfg->m_ProjectDefaults.m_PadOpacity = m_OptPadOpacity->GetValue() / 100.0;
            cfg->m_ProjectDefaults.m_ZoneOpacity = m_OptZoneOpacity->GetValue() / 100.0;
            cfg->m_ProjectDefaults.m_ImageOpacity = m_OptImageOpacity->GetValue() / 100.0;
            cfg->m_ProjectDefaults.m_FilledShapeOpacity = m_OptFilledShapeOpacity->GetValue() / 100.0;
        }
    }

    return true;
}


void PANEL_PROJECT_DEFAULTS::ResetPanel()
{
    if( m_isPCBEdit )
    {
        PCBNEW_SETTINGS cfg;
        cfg.Load(); // Loading without a file will init to defaults

        loadPCBSettings( &cfg );
    }
}
