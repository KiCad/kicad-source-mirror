/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/panel_spacemouse.h>
#include <pgm_base.h>
#include <settings/common_settings.h>

PANEL_SPACEMOUSE::PANEL_SPACEMOUSE( wxWindow* aParent ) :
        PANEL_SPACEMOUSE_BASE( aParent )
{
#ifdef __WXMAC__
    wxSizerItem* sizerItem = m_gbSizer->FindItemAtPosition( wxGBPosition( 0, 0 ) );
    sizerItem->SetBorder( 6 );
    sizerItem->SetFlag( wxBOTTOM );

    sizerItem = m_gbSizer->FindItemAtPosition( wxGBPosition( 3, 0 ) );
    sizerItem->SetBorder( 6 );
    sizerItem->SetFlag( wxBOTTOM );
#endif
}

bool PANEL_SPACEMOUSE::TransferDataToWindow()
{
    const COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    applySettingsToPanel( *cfg );

    return true;
}

bool PANEL_SPACEMOUSE::TransferDataFromWindow()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    cfg->m_SpaceMouse.rotate_speed = m_rotationSpeed->GetValue();
    cfg->m_SpaceMouse.pan_speed = m_autoPanSpeed->GetValue();
    cfg->m_SpaceMouse.reverse_rotate = m_checkEnablePanH->GetValue();
    cfg->m_SpaceMouse.reverse_pan_y = m_reverseY->GetValue();
    cfg->m_SpaceMouse.reverse_pan_x = m_reverseX->GetValue();
    cfg->m_SpaceMouse.reverse_zoom = m_reverseZ->GetValue();

    return true;
}

void PANEL_SPACEMOUSE::ResetPanel()
{
    COMMON_SETTINGS defaultSettings;

    defaultSettings.ResetToDefaults();

    applySettingsToPanel( defaultSettings );
}

void PANEL_SPACEMOUSE::applySettingsToPanel( const COMMON_SETTINGS& aSettings )
{
    m_rotationSpeed->SetValue( aSettings.m_SpaceMouse.rotate_speed );
    m_autoPanSpeed->SetValue( aSettings.m_SpaceMouse.pan_speed );
    m_checkEnablePanH->SetValue( aSettings.m_SpaceMouse.reverse_rotate );
    m_reverseY->SetValue( aSettings.m_SpaceMouse.reverse_pan_y );
    m_reverseX->SetValue( aSettings.m_SpaceMouse.reverse_pan_x );
    m_reverseZ->SetValue( aSettings.m_SpaceMouse.reverse_zoom );
}
