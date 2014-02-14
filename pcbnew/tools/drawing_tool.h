/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __DRAWING_TOOL_H
#define __DRAWING_TOOL_H

#include <tool/tool_interactive.h>

namespace KIGFX
{
    class VIEW;
    class VIEW_CONTROLS;
}
class BOARD;
class PCB_EDIT_FRAME;

/**
 * Class DRAWING_TOOL
 *
 * Tool responsible for drawing graphical elements like lines, arcs, circles, etc.
 */

class DRAWING_TOOL : public TOOL_INTERACTIVE
{
public:
    DRAWING_TOOL();
    ~DRAWING_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason );

    /**
     * Function DrawLine()
     * Starts interactively drawing a line. After invoking the function it expects the user
     * to click at least twice to determine the origin and the end for a line. If there are
     * more clicks, the line is drawn as a continous polyline.
     */
    int DrawLine( TOOL_EVENT& aEvent );

    int DrawCircle( TOOL_EVENT& aEvent );

    int DrawArc( TOOL_EVENT& aEvent );

    int DrawText( TOOL_EVENT& aEvent );

    int DrawDimension( TOOL_EVENT& aEvent );

    int DrawZone( TOOL_EVENT& aEvent );

    int DrawKeepout( TOOL_EVENT& aEvent );

    int PlaceTarget( TOOL_EVENT& aEvent );

    int PlaceModule( TOOL_EVENT& aEvent );

private:
    ///> Starts drawing a selected shape.
    int draw( int aShape );

    ///> Sets up handlers for various events.
    void setTransitions();

    ///> Should drawing be stopped after drawing one object or should it continue with another one.
    bool m_continous;

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    BOARD* m_board;
    PCB_EDIT_FRAME* m_frame;

    // How does line width change after one -/+ key press.
    static const int WIDTH_STEP = 100000;
};

#endif /* __DRAWING_TOOL_H */
