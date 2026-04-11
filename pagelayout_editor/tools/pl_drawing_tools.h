/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef PL_DRAWING_TOOLS_H
#define PL_DRAWING_TOOLS_H

#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>
#include <view/view.h>

class PL_EDITOR_FRAME;
class PL_SELECTION_TOOL;


/**
 * Tool responsible for drawing/placing items (lines, rectangles, text, etc.)
 */

class PL_DRAWING_TOOLS : public TOOL_INTERACTIVE
{
public:
    PL_DRAWING_TOOLS();
    ~PL_DRAWING_TOOLS() {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int DrawShape( const TOOL_EVENT& aEvent );
    int PlaceItem( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    PL_EDITOR_FRAME*   m_frame;
    PL_SELECTION_TOOL* m_selectionTool;
};

#endif /* PL_DRAWING_TOOLS_H */
