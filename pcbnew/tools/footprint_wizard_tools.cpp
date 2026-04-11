/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "tools/footprint_wizard_tools.h"

#include <tools/pcb_actions.h>

#include <pcbnew_id.h>

bool FOOTPRINT_WIZARD_TOOLS::Init()
{
    return true;
}


void FOOTPRINT_WIZARD_TOOLS::Reset( RESET_REASON aReason )
{
}


int FOOTPRINT_WIZARD_TOOLS::ShowWizards( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    frame()->SelectCurrentWizard( dummy );
    return 0;
}


int FOOTPRINT_WIZARD_TOOLS::ResetWizardPrms( const TOOL_EVENT& aEvent )
{
    frame()->DefaultParameters();
    return 0;
}



int FOOTPRINT_WIZARD_TOOLS::ExportFpToEditor( const TOOL_EVENT& aEvent )
{
    wxPostEvent( frame(), wxCommandEvent( wxEVT_TOOL, ID_FOOTPRINT_WIZARD_DONE ) );
    return 0;
}


void FOOTPRINT_WIZARD_TOOLS::setTransitions()
{
    // clang-format off
    Go( &FOOTPRINT_WIZARD_TOOLS::ShowWizards,  PCB_ACTIONS::showWizards.MakeEvent() );
    Go( &FOOTPRINT_WIZARD_TOOLS::ResetWizardPrms,  PCB_ACTIONS::resetWizardPrms.MakeEvent() );
    Go( &FOOTPRINT_WIZARD_TOOLS::ExportFpToEditor,  PCB_ACTIONS::exportFpToEditor.MakeEvent() );
    // clang-format on
}
