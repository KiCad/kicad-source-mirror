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
    double trackOpacityPercent = aCfg->m_ProjectDefaults.m_TrackOpacity * 100.0;
    m_OptTrackOpacity->SetValue( wxString::Format( "%.0f", trackOpacityPercent ) );
    double viaOpacityPercent = aCfg->m_ProjectDefaults.m_ViaOpacity * 100.0;
    m_OptViaOpacity->SetValue( wxString::Format( "%.0f", viaOpacityPercent ) );
    double padOpacityPercent = aCfg->m_ProjectDefaults.m_PadOpacity * 100.0;
    m_OptPadOpacity->SetValue( wxString::Format( "%.0f", padOpacityPercent ) );
    double zoneOpacityPercent = aCfg->m_ProjectDefaults.m_ZoneOpacity * 100.0;
    m_OptZoneOpacity->SetValue( wxString::Format( "%.0f", zoneOpacityPercent ) );
    double imageOpacityPercent = aCfg->m_ProjectDefaults.m_ImageOpacity * 100.0;
    m_OptImageOpacity->SetValue( wxString::Format( "%.0f", imageOpacityPercent ) );
    double filledShapeOpacityPercent = aCfg->m_ProjectDefaults.m_FilledShapeOpacity * 100.0;
    m_OptFilledShapeOpacity->SetValue( wxString::Format( "%.0f", filledShapeOpacityPercent ) );
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
            double trackOpacityPercent = cfg->m_ProjectDefaults.m_TrackOpacity * 100.0;
            m_OptTrackOpacity->GetValue().ToDouble( &trackOpacityPercent );
            double viaOpacityPercent = cfg->m_ProjectDefaults.m_ViaOpacity * 100.0;
            m_OptViaOpacity->GetValue().ToDouble( &viaOpacityPercent );
            double padOpacityPercent = cfg->m_ProjectDefaults.m_PadOpacity * 100.0;
            m_OptPadOpacity->GetValue().ToDouble( &padOpacityPercent );
            double zoneOpacityPercent = cfg->m_ProjectDefaults.m_ZoneOpacity * 100.0;
            m_OptZoneOpacity->GetValue().ToDouble( &zoneOpacityPercent );
            double imageOpacityPercent = cfg->m_ProjectDefaults.m_ImageOpacity * 100.0;
            m_OptImageOpacity->GetValue().ToDouble( &imageOpacityPercent );
            double filledShapeOpacityPercent = cfg->m_ProjectDefaults.m_FilledShapeOpacity * 100.0;
            m_OptFilledShapeOpacity->GetValue().ToDouble( &filledShapeOpacityPercent );

            cfg->m_ProjectDefaults.m_TrackOpacity = trackOpacityPercent / 100.0;
            cfg->m_ProjectDefaults.m_ViaOpacity = viaOpacityPercent / 100.0;
            cfg->m_ProjectDefaults.m_PadOpacity = padOpacityPercent / 100.0;
            cfg->m_ProjectDefaults.m_ZoneOpacity = zoneOpacityPercent / 100.0;
            cfg->m_ProjectDefaults.m_ImageOpacity = imageOpacityPercent / 100.0;
            cfg->m_ProjectDefaults.m_FilledShapeOpacity = filledShapeOpacityPercent / 100.0;
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
