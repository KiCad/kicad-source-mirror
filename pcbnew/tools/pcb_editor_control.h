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

#ifndef PCB_EDITOR_CONTROL_H
#define PCB_EDITOR_CONTROL_H

#include <tool/tool_interactive.h>

namespace KIGFX {
    class ORIGIN_VIEWITEM;
}
class PCB_EDIT_FRAME;

/**
 * Class PCB_EDITOR_CONTROL
 *
 * Handles actions specific to the board editor in pcbnew.
 */
class PCB_EDITOR_CONTROL : public TOOL_INTERACTIVE
{
public:
    PCB_EDITOR_CONTROL();
    ~PCB_EDITOR_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason );

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    // Track & via size control
    int TrackWidthInc( const TOOL_EVENT& aEvent );
    int TrackWidthDec( const TOOL_EVENT& aEvent );
    int ViaSizeInc( const TOOL_EVENT& aEvent );
    int ViaSizeDec( const TOOL_EVENT& aEvent );

    // Zone actions
    int ZoneFill( const TOOL_EVENT& aEvent );
    int ZoneFillAll( const TOOL_EVENT& aEvent );
    int ZoneUnfill( const TOOL_EVENT& aEvent );
    int ZoneUnfillAll( const TOOL_EVENT& aEvent );
    int ZoneMerge( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceTarget()
     * Allows user to place a layer alignment target.
     */
    int PlaceTarget( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceModule()
     * Displays a dialog to select a module to be added and allows the user to set its position.
     */
    int PlaceModule( const TOOL_EVENT& aEvent );

    ///> Notifies eeschema about the selected item.
    int SelectionCrossProbe( const TOOL_EVENT& aEvent );

    ///> Places the origin point for drill and pick-and-place files.
    int DrillOrigin( const TOOL_EVENT& aEvent );

    ///> Highlights net belonging to the item under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///> Launches a tool to pick the item whose net is going to be highlighted.
    int HighlightNetCursor( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void SetTransitions();

private:
    ///> Pointer to the currently used edit frame.
    PCB_EDIT_FRAME* m_frame;

    ///> Place & drill origin marker.
    KIGFX::ORIGIN_VIEWITEM* m_placeOrigin;

    // How does line width change after one -/+ key press.
    static const int WIDTH_STEP;
};

#endif
