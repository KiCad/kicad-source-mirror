/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL_H_
#define CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL_H_

#include <display_footprints_frame.h>

#include <tool/action_menu.h>
#include <tool/selection.h>
#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>


/**
 * Class CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL
 *
 * Selection tool for the footprint viewer in cvpcb.
 */
class CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL();
    ~CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL() {}

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * Function Main()
     *
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Selections aren't currently supported in the footprint viewer.
     */
    SELECTION& GetSelection()
    {
        return m_selection;
    }

    void clearSelection() {}

    ///> Launches a tool to measure between points
    int MeasureTool( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    /// Pointer to the parent frame.
    DISPLAY_FOOTPRINTS_FRAME* m_frame;

    /// Current state of selection (not really used: no selection in display footprints frame).
    SELECTION m_selection;
};

#endif
