/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Brian Piccioni <brian@documenteddesigns.com>
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

#ifndef PCBREANNOTATETOOL_H_
#define PCBREANNOTATETOOL_H_

#include <dialogs/dialog_board_reannotate.h>
#include <pcb_edit_frame.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>


class BOARD_REANNOTATE_TOOL : public wxEvtHandler, public PCB_TOOL_BASE
{
public:
    BOARD_REANNOTATE_TOOL();

    bool Init() override;
    void Reset( RESET_REASON aReason ) override;
    int  ShowReannotateDialog( const TOOL_EVENT& aEvent );

    int  ReannotateDuplicatesInSelection();

    int  ReannotateDuplicates( const PCB_SELECTION& aSelectionToReannotate,
                               const std::vector<EDA_ITEM*>& aAdditionalFootprints );

private:
    void setTransitions() override; //> Bind handlers to corresponding TOOL_ACTIONs

private:
    PCB_SELECTION_TOOL* m_selectionTool;
    PCB_EDIT_FRAME*     m_frame; // Pointer to the currently used edit frame.
};

#endif /* PCBREANNOTATETOOL_H_ */
