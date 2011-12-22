/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"

#include "pcbnew_id.h"

#include "protos.h"


void FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        m_DisplayPadFill = !m_optionsToolBar->GetToolToggled( id );
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        m_DisplayViaFill = !m_optionsToolBar->GetToolToggled( id );
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        m_DisplayModText = m_optionsToolBar->GetToolToggled( id ) ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        m_DisplayModEdge = m_optionsToolBar->GetToolToggled( id ) ? SKETCH : FILLED;
        m_canvas->Refresh( );
        break;

    default:
        DisplayError( this,
                      wxT( "FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}
