/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "panel_zone_conversion.h"
#include <pcbnew_settings.h>


PANEL_ZONE_CONVERSION::PANEL_ZONE_CONVERSION( wxWindow* aParent, EDA_DRAW_FRAME* aFrame,
                                              CONVERT_SETTINGS& aSettings ) :
        PANEL_ZONE_CONVERSION_BASE( aParent ),
        m_convertSettings( aSettings ),
        m_gap( aFrame, m_gapLabel, m_gapCtrl, m_gapUnits )
{
    // Set the default before DIALOG_SHIM restores saved control values.
    m_gap.SetValue( m_convertSettings.m_Gap );
}


bool PANEL_ZONE_CONVERSION::TransferDataToWindow()
{
    if( m_convertSettings.m_Strategy == BOUNDING_HULL )
        m_rbEnvelope->SetValue( true );
    else
        m_rbCenterline->SetValue( true );

    m_cbDeleteOriginals->SetValue( m_convertSettings.m_DeleteOriginals );
    m_gap.Enable( m_rbEnvelope->GetValue() );
    return true;
}


bool PANEL_ZONE_CONVERSION::TransferDataFromWindow()
{
    if( m_rbEnvelope->GetValue() )
        m_convertSettings.m_Strategy = BOUNDING_HULL;
    else
        m_convertSettings.m_Strategy = CENTERLINE;

    m_convertSettings.m_DeleteOriginals = m_cbDeleteOriginals->GetValue();
    m_convertSettings.m_Gap = m_gap.GetIntValue();
    return true;
}


void PANEL_ZONE_CONVERSION::OnUpdateUI( wxUpdateUIEvent& aEvent )
{
    m_gap.Enable( m_rbEnvelope->GetValue() );
}
