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
#include <dialogs/panel_setup_zone_hatch_offsets.h>


PANEL_SETUP_ZONES::PANEL_SETUP_ZONES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                      BOARD_DESIGN_SETTINGS& aBrdSettings ) :
        PANEL_SETUP_ZONES_BASE( aParentWindow ),
        m_brdSettings( aBrdSettings ),
        m_zoneSettingsBag( nullptr, &aBrdSettings.GetDefaultZoneSettings() )
{
    m_panelZoneProperties = new PANEL_ZONE_PROPERTIES( m_scrolledWindow, aFrame, m_zoneSettingsBag, false );
    m_panelZoneProperties->SetZone( nullptr );
    m_scrolledWindow->GetSizer()->Add( m_panelZoneProperties, 1, wxEXPAND, 5 );

    m_panelHatchOffsets = new PANEL_SETUP_ZONE_HATCH_OFFSETS( m_scrolledWindow, aFrame, aBrdSettings );
    m_scrolledWindow->GetSizer()->AddSpacer( 10 );
    m_scrolledWindow->GetSizer()->Add( m_panelHatchOffsets, 0, wxEXPAND, 5 );
}


bool PANEL_SETUP_ZONES::TransferDataToWindow()
{
    if( !m_panelZoneProperties->TransferZoneSettingsToWindow() )
        return false;

    return m_panelHatchOffsets->TransferDataToWindow();
}


bool PANEL_SETUP_ZONES::TransferDataFromWindow()
{
    if( m_panelZoneProperties->TransferZoneSettingsFromWindow() )
    {
        ZONE_SETTINGS settings = *m_zoneSettingsBag.GetZoneSettings( nullptr );
        settings.m_Netcode = NETINFO_LIST::ORPHANED;
        m_brdSettings.SetDefaultZoneSettings( settings );

        return m_panelHatchOffsets->TransferDataFromWindow();
    }

    return false;
}


bool PANEL_SETUP_ZONES::CommitPendingChanges()
{
    return m_panelZoneProperties->CommitPendingChanges();
}


void PANEL_SETUP_ZONES::ImportSettingsFrom( BOARD* aBoard )
{
    // The bag holds a copy of the default zone settings keyed by nullptr.
    // Overwrite that copy with the other board's defaults and refresh the window.
    *m_zoneSettingsBag.GetZoneSettings( nullptr ) = aBoard->GetDesignSettings().GetDefaultZoneSettings();
    m_panelZoneProperties->TransferZoneSettingsToWindow();
}


void PANEL_SETUP_ZONES::ImportHatchOffsetsFrom( BOARD* aBoard )
{
    m_panelHatchOffsets->ImportSettingsFrom( aBoard );
}


void PANEL_SETUP_ZONES::SyncCopperLayers( int aCopperLayerCount )
{
    m_panelHatchOffsets->SyncCopperLayers( aCopperLayerCount );
    m_scrolledWindow->Layout();
    m_scrolledWindow->FitInside();
}
