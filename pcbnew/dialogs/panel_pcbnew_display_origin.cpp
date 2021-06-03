/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board.h>
#include <panel_pcbnew_display_origin.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_view.h>
#include <pcbnew_id.h>
#include <widgets/paged_dialog.h>

#include <wx/treebook.h>


PANEL_PCBNEW_DISPLAY_ORIGIN::PANEL_PCBNEW_DISPLAY_ORIGIN(
        PCB_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent )
        : PANEL_PCBNEW_DISPLAY_ORIGIN_BASE( aParent->GetTreebook() ),
          m_Frame( aFrame )
{
}


bool PANEL_PCBNEW_DISPLAY_ORIGIN::TransferDataToWindow()
{
    const PCB_DISPLAY_OPTIONS& displ_opts = m_Frame->GetDisplayOptions();

    int origin = 0;

    switch( displ_opts.m_DisplayOrigin )
    {
    case PCB_DISPLAY_OPTIONS::PCB_ORIGIN_PAGE: origin = 0; break;
    case PCB_DISPLAY_OPTIONS::PCB_ORIGIN_AUX:  origin = 1; break;
    case PCB_DISPLAY_OPTIONS::PCB_ORIGIN_GRID: origin = 2; break;
    }

    m_DisplayOrigin->SetSelection( origin );
    m_XAxisDirection->SetSelection( displ_opts.m_DisplayInvertXAxis ? 1 : 0 );
    m_YAxisDirection->SetSelection( displ_opts.m_DisplayInvertYAxis ? 0 : 1 );

    return true;
}


bool PANEL_PCBNEW_DISPLAY_ORIGIN::TransferDataFromWindow()
{
    PCB_DISPLAY_OPTIONS displ_opts = m_Frame->GetDisplayOptions();

    switch( m_DisplayOrigin->GetSelection() )
    {
    case 0: displ_opts.m_DisplayOrigin = PCB_DISPLAY_OPTIONS::PCB_ORIGIN_PAGE; break;
    case 1: displ_opts.m_DisplayOrigin = PCB_DISPLAY_OPTIONS::PCB_ORIGIN_AUX;  break;
    case 2: displ_opts.m_DisplayOrigin = PCB_DISPLAY_OPTIONS::PCB_ORIGIN_GRID; break;
    }

    displ_opts.m_DisplayInvertXAxis = m_XAxisDirection->GetSelection() != 0;
    displ_opts.m_DisplayInvertYAxis = m_YAxisDirection->GetSelection() == 0;

    m_Frame->SetDisplayOptions( displ_opts );

    return true;
}
