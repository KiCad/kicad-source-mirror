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

#ifndef __BOARD_STATISTICS_TOOL_H
#define __BOARD_STATISTICS_TOOL_H


#include <dialogs/dialog_board_statistics.h>
#include <pcb_edit_frame.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>


/**
 * Class PCB_INSPECTION_TOOL
 *
 * Tool for pcb inspection.
 */
class PCB_INSPECTION_TOOL : public PCB_TOOL_BASE
{
public:
    PCB_INSPECTION_TOOL();

    /**
     * Function ShowStatisticDialog()
     *
     * Shows dialog with board statistics
     */
    int ShowStatisticsDialog( const TOOL_EVENT& aEvent );

    ///> Bind handlers to corresponding TOOL_ACTIONs
    void setTransitions() override;
};

#endif //__BOARD_STATISTICS_TOOL_H
