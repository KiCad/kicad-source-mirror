/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_design_settings.h>
#include <fctsys.h>
#include <footprint_edit_frame.h>
#include <pcb_edit_frame.h>
#include <pcbnew.h>
#include <pcbnew_settings.h>
#include <widgets/paged_dialog.h>

#include <panel_modedit_settings.h>


PANEL_MODEDIT_SETTINGS::PANEL_MODEDIT_SETTINGS( FOOTPRINT_EDIT_FRAME* aFrame,
                                                PAGED_DIALOG* aParent) :
        PANEL_MODEDIT_SETTINGS_BASE( aParent->GetTreebook() ),
        m_frame( aFrame )
{}


bool PANEL_MODEDIT_SETTINGS::TransferDataToWindow()
{
    // Display options
    m_PolarDisplay->SetSelection( m_frame->GetShowPolarCoords() ? 1 : 0 );
    m_UnitsSelection->SetSelection( m_frame->GetUserUnits() == EDA_UNITS::INCHES ? 0 : 1 );

    // Editing options
    m_Segments_45_Only_Ctrl->SetValue( m_frame->Settings().m_Use45DegreeGraphicSegments );
    m_MagneticPads->SetValue(
            m_frame->GetMagneticItemsSettings()->pads == MAGNETIC_OPTIONS::CAPTURE_ALWAYS );

    return true;
}


bool PANEL_MODEDIT_SETTINGS::TransferDataFromWindow()
{
    // Display options
    m_frame->SetShowPolarCoords( m_PolarDisplay->GetSelection() != 0 );
    m_frame->SetUserUnits(
            m_UnitsSelection->GetSelection() == 0 ? EDA_UNITS::INCHES : EDA_UNITS::MILLIMETRES );

    // Editing options
    m_frame->Settings().m_Use45DegreeGraphicSegments = m_Segments_45_Only_Ctrl->GetValue();
    m_frame->GetMagneticItemsSettings()->pads = m_MagneticPads->GetValue() ?
            MAGNETIC_OPTIONS::CAPTURE_ALWAYS : MAGNETIC_OPTIONS::NO_EFFECT;

    return true;
}
