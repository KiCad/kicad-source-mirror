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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <panel_project_defaults.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <math/util.h>


PANEL_PROJECT_DEFAULTS::PANEL_PROJECT_DEFAULTS( wxWindow* aParent ) :
        PANEL_PROJECT_DEFAULTS_BASE( aParent )
{
}


void PANEL_PROJECT_DEFAULTS::applyOpacities( const BOARD_OPACITY_DEFAULTS& aOpacities )
{
    m_OptTrackOpacity->SetValue( KiROUND( aOpacities.m_TrackOpacity * 100.0 ) );
    m_OptViaOpacity->SetValue( KiROUND( aOpacities.m_ViaOpacity * 100.0 ) );
    m_OptPadOpacity->SetValue( KiROUND( aOpacities.m_PadOpacity * 100.0 ) );
    m_OptZoneOpacity->SetValue( KiROUND( aOpacities.m_ZoneOpacity * 100.0 ) );
    m_OptFilledShapeOpacity->SetValue( KiROUND( aOpacities.m_ShapeOpacity * 100.0 ) );
    m_OptImageOpacity->SetValue( KiROUND( aOpacities.m_ImageOpacity * 100.0 ) );
    m_OptOutsideGroupOpacity->SetValue( KiROUND( aOpacities.m_OutsideGroupOpacity * 100.0 ) );
}


bool PANEL_PROJECT_DEFAULTS::TransferDataToWindow()
{
    if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        applyOpacities( cfg->m_BoardOpacityDefaults );

    return true;
}


bool PANEL_PROJECT_DEFAULTS::TransferDataFromWindow()
{
    if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
    {
        BOARD_OPACITY_DEFAULTS& opacities = cfg->m_BoardOpacityDefaults;

        opacities.m_TrackOpacity = m_OptTrackOpacity->GetValue() / 100.0;
        opacities.m_ViaOpacity = m_OptViaOpacity->GetValue() / 100.0;
        opacities.m_PadOpacity = m_OptPadOpacity->GetValue() / 100.0;
        opacities.m_ZoneOpacity = m_OptZoneOpacity->GetValue() / 100.0;
        opacities.m_ShapeOpacity = m_OptFilledShapeOpacity->GetValue() / 100.0;
        opacities.m_ImageOpacity = m_OptImageOpacity->GetValue() / 100.0;
        opacities.m_OutsideGroupOpacity = m_OptOutsideGroupOpacity->GetValue() / 100.0;
    }

    return true;
}


void PANEL_PROJECT_DEFAULTS::ResetPanel()
{
    applyOpacities( BOARD_OPACITY_DEFAULTS() );
}
