/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <kiway.h>
#include <sch_edit_frame.h>
#include "sim_plot_frame.h"

void SCH_EDIT_FRAME::OnSimulate( wxCommandEvent& event )
{
    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, true );
    simFrame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();
}