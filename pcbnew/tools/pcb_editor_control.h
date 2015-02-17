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

    ///> Notifies eeschema about the selected item.
    int SelectionCrossProbe( const TOOL_EVENT& aEvent );

private:
    ///> Sets up handlers for various events.
    void setTransitions();

    ///> Pointer to the currently used edit frame.
    PCB_EDIT_FRAME* m_frame;
};

#endif
