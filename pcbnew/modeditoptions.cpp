/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file modeditoptions.cpp
 * @brief Pcbnew footprint (module) editor options.
 */

#include <fctsys.h>
#include <class_drawpanel.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <class_board_design_settings.h>

#include <pcbnew_id.h>


void FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)GetDisplayOptions();
    bool state = m_optionsToolBar->GetToolToggled( id );

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        displ_opts->m_DisplayPadFill = !state;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        displ_opts->m_DisplayViaFill = !state;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        displ_opts->m_DisplayModText = state ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        displ_opts->m_DisplayModEdge = state ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        displ_opts->m_ContrastModeDisplay = state;
        m_canvas->Refresh( );
        break;

    default:
        wxMessageBox( wxT( "FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}
