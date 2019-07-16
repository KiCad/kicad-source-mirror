/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include "pcb_inspection_tool.h"


PCB_INSPECTION_TOOL::PCB_INSPECTION_TOOL()
        : PCB_TOOL_BASE( "pcbnew.InspectionTool.ShowStatisticsDialog" )
{
}

int PCB_INSPECTION_TOOL::ShowStatisticsDialog( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_EDIT_FRAME>();

    DIALOG_BOARD_STATISTICS dialog( frame );
    dialog.ShowModal();
    return 0;
}

void PCB_INSPECTION_TOOL::setTransitions()
{
    Go( &PCB_INSPECTION_TOOL::ShowStatisticsDialog, PCB_ACTIONS::boardStatistics.MakeEvent() );
}
