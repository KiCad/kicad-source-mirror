/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see change_log.txt for contributors.
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

#ifndef TOOLS_MICROWAVE_TOOL_H
#define TOOLS_MICROWAVE_TOOL_H

#include <tools/pcb_tool.h>

#include <tool/tool_menu.h>


/**
 * Class MICROWAVE_TOOL
 *
 * Tool responsible for adding microwave features to PCBs
 */
class MICROWAVE_TOOL : public PCB_TOOL
{
public:
    MICROWAVE_TOOL();
    ~MICROWAVE_TOOL();

    ///> React to model/view changes
    void Reset( RESET_REASON aReason ) override;

    ///> Basic initalization
    bool Init() override;

    ///> Bind handlers to corresponding TOOL_ACTIONs
    void SetTransitions() override;

private:

    ///> Main interactive tool
    int addMicrowaveFootprint( const TOOL_EVENT& aEvent );

    ///> Create an inductor between the two points
    void createInductorBetween( const VECTOR2I& aStart, const VECTOR2I& aEnd );

    ///> Draw a microwave inductor interactively
    int drawMicrowaveInductor( const TOOL_EVENT& aEvent );

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;
};


#endif // TOOLS_MICROWAVE_TOOL_H
