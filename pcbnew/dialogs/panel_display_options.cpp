/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcbnew_settings.h>
#include <config_map.h>
#include <panel_display_options.h>
#include <widgets/gal_options_panel.h>


static const UTIL::CFG_MAP<TRACK_CLEARANCE_MODE> clearanceModeMap =
{
    { SHOW_WITH_VIA_WHILE_ROUTING,             2 },     // Default
    { DO_NOT_SHOW_CLEARANCE,                   0 },
    { SHOW_WHILE_ROUTING,                      1 },
    { SHOW_WITH_VIA_WHILE_ROUTING_OR_DRAGGING, 3 },
    { SHOW_WITH_VIA_ALWAYS,                    4 },
};


PANEL_DISPLAY_OPTIONS::PANEL_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
    PANEL_DISPLAY_OPTIONS_BASE( aParent ),
    m_isPCBEdit( dynamic_cast<PCBNEW_SETTINGS*>( aAppSettings ) != nullptr )
{
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, aAppSettings );
    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );

    m_optionsBook->SetSelection( m_isPCBEdit ? 1 : 0 );
}


void PANEL_DISPLAY_OPTIONS::loadPCBSettings( PCBNEW_SETTINGS* aCfg )
{
    int i = UTIL::GetConfigForVal( clearanceModeMap, aCfg->m_Display.m_TrackClearance );
    m_OptDisplayTracksClearance->SetSelection( i );

    m_OptDisplayPadClearence->SetValue( aCfg->m_Display.m_PadClearance );
    m_OptDisplayPadNumber->SetValue( aCfg->m_ViewersDisplay.m_DisplayPadNumbers );
    m_ShowNetNamesOption->SetSelection( aCfg->m_Display.m_NetNames );
    m_live3Drefresh->SetValue( aCfg->m_Display.m_Live3DRefresh );
    m_checkCrossProbeOnSelection->SetValue( aCfg->m_CrossProbing.on_selection );
    m_checkCrossProbeCenter->SetValue( aCfg->m_CrossProbing.center_on_items );
    m_checkCrossProbeZoom->SetValue( aCfg->m_CrossProbing.zoom_to_fit );
    m_checkCrossProbeAutoHighlight->SetValue( aCfg->m_CrossProbing.auto_highlight );
}


bool PANEL_DISPLAY_OPTIONS::TransferDataToWindow()
{
    if( m_isPCBEdit )
    {
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
        PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>();

        loadPCBSettings( cfg );
    }

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


/*
 * Update variables with new options
 */
bool PANEL_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    m_galOptsPanel->TransferDataFromWindow();

    if( m_isPCBEdit )
    {
        PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

        int i = m_OptDisplayTracksClearance->GetSelection();
        cfg->m_Display.m_TrackClearance = UTIL::GetValFromConfig( clearanceModeMap, i );

        cfg->m_Display.m_PadClearance = m_OptDisplayPadClearence->GetValue();
        cfg->m_ViewersDisplay.m_DisplayPadNumbers = m_OptDisplayPadNumber->GetValue();
        cfg->m_Display.m_NetNames = m_ShowNetNamesOption->GetSelection();
        cfg->m_Display.m_Live3DRefresh = m_live3Drefresh->GetValue();
        cfg->m_CrossProbing.on_selection = m_checkCrossProbeOnSelection->GetValue();
        cfg->m_CrossProbing.center_on_items = m_checkCrossProbeCenter->GetValue();
        cfg->m_CrossProbing.zoom_to_fit = m_checkCrossProbeZoom->GetValue();
        cfg->m_CrossProbing.auto_highlight = m_checkCrossProbeAutoHighlight->GetValue();
    }

    return true;
}


void PANEL_DISPLAY_OPTIONS::ResetPanel()
{
    PCBNEW_SETTINGS cfg;
    cfg.Load();             // Loading without a file will init to defaults

    if( m_isPCBEdit )
        loadPCBSettings( &cfg );

    m_galOptsPanel->ResetPanel( &cfg );
}


