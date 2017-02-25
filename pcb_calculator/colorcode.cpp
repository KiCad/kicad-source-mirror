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

#include <wx/app.h>
#include <wx/config.h>
#include <wx/msgdlg.h>

#include <pcb_calculator_frame_base.h>
#include <pcb_calculator.h>

void PCB_CALCULATOR_FRAME::OnToleranceSelection( wxCommandEvent& event )
{
    ToleranceSelection( event.GetSelection() );
}

void PCB_CALCULATOR_FRAME::ToleranceSelection( int aSelection )
{
    /* For tolerance = 5 or 10 %, there are 3 bands for the value
     * but for tolerance < 5 %, there are 4 bands
     */
    bool show4thBand;
    switch( aSelection )
    {
        case 0: // 5 or 10 %
            show4thBand = false;
            break;
        case 1: // < 5 %
            show4thBand = true;
            break;
        default: // Show 4th band if something went wrong
            show4thBand = true;
            break;
    }
    bool oldstate = m_Band4Label->IsShown();
    if( oldstate != show4thBand )
    {
        m_Band4bitmap->Show(show4thBand);
        m_Band4Label->Show(show4thBand);
        // m_Band4Label visibility has changed:
        // The new size must be taken in account
        m_panelColorCode->GetSizer()->Layout();
        m_panelColorCode->Refresh();
    }
}

