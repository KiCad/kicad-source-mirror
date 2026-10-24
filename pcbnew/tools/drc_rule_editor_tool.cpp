/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "drc_rule_editor_tool.h"

#include <pcb_edit_frame.h>
#include <tools/pcb_actions.h>


DRC_RULE_EDITOR_TOOL::DRC_RULE_EDITOR_TOOL() :
        PCB_TOOL_BASE( "pcbnew.DRETool" ),
        m_editFrame( nullptr )
{
}


void DRC_RULE_EDITOR_TOOL::Reset( RESET_REASON aReason )
{
    m_editFrame = getEditFrame<PCB_EDIT_FRAME>();
}


int DRC_RULE_EDITOR_TOOL::ShowDRCRuleEditorDialog( const TOOL_EVENT& aEvent )
{
    m_editFrame->ShowBoardSetupDialog( _( "Clearance" ) );
    return 0;
}


void DRC_RULE_EDITOR_TOOL::setTransitions()
{
    Go( &DRC_RULE_EDITOR_TOOL::ShowDRCRuleEditorDialog, PCB_ACTIONS::drcRuleEditor.MakeEvent() );
}
