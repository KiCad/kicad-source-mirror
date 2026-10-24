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

#ifndef DRC_RULE_EDITOR_TOOL_H
#define DRC_RULE_EDITOR_TOOL_H

#include <tools/pcb_tool_base.h>

class PCB_EDIT_FRAME;


class DRC_RULE_EDITOR_TOOL : public PCB_TOOL_BASE
{
public:
    DRC_RULE_EDITOR_TOOL();

    void Reset( RESET_REASON aReason ) override;

    int ShowDRCRuleEditorDialog( const TOOL_EVENT& aEvent );

private:
    void setTransitions() override;

    PCB_EDIT_FRAME* m_editFrame;
};

#endif // DRC_RULE_EDITOR_TOOL_H
