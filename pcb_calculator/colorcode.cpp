/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2011-2021 Kicad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/app.h>
#include "bitmaps/color_code_value_and_name.xpm"
#include "bitmaps/color_code_value.xpm"
#include "bitmaps/color_code_multiplier.xpm"
#include "bitmaps/color_code_tolerance.xpm"
#include "pcb_calculator_frame.h"


void PCB_CALCULATOR_FRAME::initColorCodePanel()
{
    m_ccValueNamesBitmap = new wxBitmap( color_code_value_and_name_xpm );
    m_ccValuesBitmap = new wxBitmap( color_code_value_xpm );
    m_ccMultipliersBitmap = new wxBitmap( color_code_multiplier_xpm );
    m_ccTolerancesBitmap = new wxBitmap( color_code_tolerance_xpm );
}


void PCB_CALCULATOR_FRAME::OnToleranceSelection( wxCommandEvent& event )
{
    ToleranceSelection( event.GetSelection() );
}


void PCB_CALCULATOR_FRAME::ToleranceSelection( int aSelection )
{
    /* For tolerance = 5 or 10 %, there are 3 bands for the value
     * but for tolerance < 5 %, there are 4 bands
     */
    bool show4thBand = aSelection != 0;

    m_Band4bitmap->Show(show4thBand);
    m_Band4Label->Show(show4thBand);

    // m_Band4Label visibility has changed:
    // The new size must be taken in account
    m_panelColorCode->GetSizer()->Layout();

    // All this shouldn't be necessary but if you want the bitmaps to show up on OSX it is.
    m_Band1bitmap->SetBitmap( *m_ccValueNamesBitmap );
   	m_Band2bitmap->SetBitmap( *m_ccValuesBitmap );
   	m_Band3bitmap->SetBitmap( *m_ccValuesBitmap );
   	m_Band4bitmap->SetBitmap( *m_ccValuesBitmap );
   	m_Band_mult_bitmap->SetBitmap( *m_ccMultipliersBitmap );
   	m_Band_tol_bitmap->SetBitmap( *m_ccTolerancesBitmap );

    m_panelColorCode->Refresh();
}