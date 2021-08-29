/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


static const UTIL::CFG_MAP<PCB_DISPLAY_OPTIONS::TRACE_CLEARANCE_DISPLAY_MODE_T> traceClearanceSelectMap =
{
    { PCB_DISPLAY_OPTIONS::SHOW_TRACK_CLEARANCE_WITH_VIA_WHILE_ROUTING, 2 },     // Default
    { PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE,                       0 },
    { PCB_DISPLAY_OPTIONS::SHOW_TRACK_CLEARANCE_WHILE_ROUTING,          1 },
    { PCB_DISPLAY_OPTIONS::SHOW_WHILE_ROUTING_OR_DRAGGING,              3 },
    { PCB_DISPLAY_OPTIONS::SHOW_TRACK_CLEARANCE_WITH_VIA_ALWAYS,        4 },
};


PANEL_DISPLAY_OPTIONS::PANEL_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
    PANEL_DISPLAY_OPTIONS_BASE( aParent ),
    m_isPCBEdit( dynamic_cast<PCBNEW_SETTINGS*>( aAppSettings ) != nullptr )
{
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, aAppSettings );
    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );

    m_optionsBook->SetSelection( m_isPCBEdit ? 1 : 0 );
}


bool PANEL_DISPLAY_OPTIONS::TransferDataToWindow()
{
    if( m_isPCBEdit )
    {
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
        PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>();

        m_OptDisplayTracksClearance->SetSelection( UTIL::GetConfigForVal(
                                                        traceClearanceSelectMap,
                                                        cfg->m_Display.m_ShowTrackClearanceMode ) );

        m_OptDisplayPadClearence->SetValue( cfg->m_Display.m_DisplayPadClearance );
        m_OptDisplayPadNumber->SetValue( cfg->m_Display.m_DisplayPadNum );
        m_OptDisplayPadNoConn->SetValue( cfg->m_Display.m_DisplayPadNoConnects );
        m_ShowNetNamesOption->SetSelection( cfg->m_Display.m_DisplayNetNamesMode );
        m_live3Drefresh->SetValue( cfg->m_Display.m_Live3DRefresh );
        m_checkCrossProbeCenter->SetValue( cfg->m_CrossProbing.center_on_items );
        m_checkCrossProbeZoom->SetValue( cfg->m_CrossProbing.zoom_to_fit );
        m_checkCrossProbeAutoHighlight->SetValue( cfg->m_CrossProbing.auto_highlight );
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
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
        PCBNEW_SETTINGS*  cfg = mgr.GetAppSettings<PCBNEW_SETTINGS>();

        cfg->m_Display.m_ShowTrackClearanceMode = UTIL::GetValFromConfig(
                                                    traceClearanceSelectMap,
                                                    m_OptDisplayTracksClearance->GetSelection() );

        cfg->m_Display.m_DisplayPadClearance = m_OptDisplayPadClearence->GetValue();
        cfg->m_Display.m_DisplayPadNum = m_OptDisplayPadNumber->GetValue();
        cfg->m_Display.m_DisplayPadNoConnects = m_OptDisplayPadNoConn->GetValue();
        cfg->m_Display.m_DisplayNetNamesMode = m_ShowNetNamesOption->GetSelection();
        cfg->m_Display.m_Live3DRefresh = m_live3Drefresh->GetValue();
        cfg->m_CrossProbing.center_on_items = m_checkCrossProbeCenter->GetValue();
        cfg->m_CrossProbing.zoom_to_fit = m_checkCrossProbeZoom->GetValue();
        cfg->m_CrossProbing.auto_highlight = m_checkCrossProbeAutoHighlight->GetValue();
    }

    return true;
}


