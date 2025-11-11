/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "panel_setup_zones.h"

#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <dialogs/panel_zone_properties.h>


PANEL_SETUP_ZONES::PANEL_SETUP_ZONES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                      BOARD_DESIGN_SETTINGS& aBrdSettings ) :
        PANEL_SETUP_ZONES_BASE( aParentWindow ),
        m_brdSettings( aBrdSettings ),
        m_zoneSettingsBag( nullptr, &aBrdSettings.GetDefaultZoneSettings() )
{
    m_panelZoneProperties = new PANEL_ZONE_PROPERTIES( this, aFrame, m_zoneSettingsBag, false );
    m_panelZoneProperties->SetZone( nullptr );
    m_mainSizer->Add( m_panelZoneProperties, 1, 0, 5 );
}


bool PANEL_SETUP_ZONES::TransferDataToWindow()
{
    return m_panelZoneProperties->TransferZoneSettingsToWindow();
}


bool PANEL_SETUP_ZONES::TransferDataFromWindow()
{
    if( m_panelZoneProperties->TransferZoneSettingsFromWindow() )
    {
        ZONE_SETTINGS settings = *m_zoneSettingsBag.GetZoneSettings( nullptr );
        settings.m_Netcode = NETINFO_LIST::ORPHANED;
        m_brdSettings.SetDefaultZoneSettings( settings );
        return true;
    }

    return false;
}


bool PANEL_SETUP_ZONES::CommitPendingChanges()
{
    return m_panelZoneProperties->CommitPendingChanges();
}