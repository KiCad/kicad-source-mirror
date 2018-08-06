/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CVPCB_SELECTION_TOOL_H
#define CVPCB_SELECTION_TOOL_H


#include <tool/tool_interactive.h>
#include <tool/context_menu.h>
#include <tool/selection.h>
#include <tool/selection_conditions.h>
#include <tool/tool_menu.h>

#include <display_footprints_frame.h>

class SELECTION_AREA;
class GERBER_COLLECTOR;

namespace KIGFX
{
    class GAL;
}


/**
 * Class CVPCB_SELECTION_TOOL
 *
 * Selection tool for GerbView, based on the one in PcbNew
 */
class CVPCB_SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    CVPCB_SELECTION_TOOL();
    ~CVPCB_SELECTION_TOOL();

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

    /** Returns the set of currently selected items.
     */
    SELECTION& GetSelection();

    inline TOOL_MENU& GetToolMenu()
    {
        return m_menu;
    }

    /** Clears the current selection.
     */
    void clearSelection() {};

    ///> Launches a tool to measure between points
    int MeasureTool( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void setTransitions() override;

    ///> Zooms the screen to center and fit the current selection.
    void zoomFitSelection( void );

private:

    /// Pointer to the parent frame.
    DISPLAY_FOOTPRINTS_FRAME* m_frame;

    /// Current state of selection (not really used: no selection in display footprints frame).
    SELECTION m_selection;

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;
};

#endif
