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

#include <fctsys.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <widgets/paged_dialog.h>
#include <footprint_edit_frame.h>

#include <panel_modedit_settings.h>


PANEL_MODEDIT_SETTINGS::PANEL_MODEDIT_SETTINGS( FOOTPRINT_EDIT_FRAME* aFrame,
                                                PAGED_DIALOG* aParent) :
        PANEL_MODEDIT_SETTINGS_BASE( aParent->GetTreebook() ),
        m_frame( aFrame )
{}


bool PANEL_MODEDIT_SETTINGS::TransferDataToWindow()
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();

    // Display options
    m_PolarDisplay->SetSelection( displ_opts->m_DisplayPolarCood ? 1 : 0 );
    m_UnitsSelection->SetSelection( m_frame->GetUserUnits() == INCHES ? 0 : 1 );

    // Editing options
    m_Segments_45_Only_Ctrl->SetValue( m_frame->Settings().m_use45DegreeGraphicSegments );
    m_MagneticPads->SetValue( m_frame->Settings().m_magneticPads == CAPTURE_ALWAYS );
    m_dragSelects->SetValue( m_frame->Settings().m_dragSelects );

    return true;
}


bool PANEL_MODEDIT_SETTINGS::TransferDataFromWindow()
{
    // Display options
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();
    displ_opts->m_DisplayPolarCood = m_PolarDisplay->GetSelection() != 0;

    m_frame->SetUserUnits( m_UnitsSelection->GetSelection() == 0 ? INCHES : MILLIMETRES );

    // Editing options
    m_frame->Settings().m_use45DegreeGraphicSegments = m_Segments_45_Only_Ctrl->GetValue();
    m_frame->Settings().m_magneticPads = m_MagneticPads->GetValue() ? CAPTURE_ALWAYS : NO_EFFECT;
    m_frame->Settings().m_dragSelects = m_dragSelects->GetValue();

    return true;
}
