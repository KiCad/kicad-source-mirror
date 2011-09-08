/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxstruct.h"
#include "confirm.h"
#include "gestfich.h"

#include "wx/wx.h"
#include "wx/config.h"

#include "pcb_calculator_frame_base.h"
#include "pcb_calculator.h"

#include "bitmaps.h"
#include "colors_selection.h"
#include "build_version.h"

// PCB_CALCULATOR_APP

void EDA_APP::MacOpenFile(const wxString &fileName)
{
}

IMPLEMENT_APP( EDA_APP )

///-----------------------------------------------------------------------------
// PCB_CALCULATOR_APP
// main program
//-----------------------------------------------------------------------------

bool EDA_APP::OnInit()
{

    InitEDA_Appl( wxT( "PCBcalc" ) );

    wxFrame* frame = new PCB_CALCULATOR_FRAME( NULL );
    SetTopWindow( frame );
    frame->Show( true );

    return true;
}
