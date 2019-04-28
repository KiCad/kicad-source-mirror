/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef SCH_EDITOR_CONTROL_H
#define SCH_EDITOR_CONTROL_H

#include <sch_base_frame.h>
#include <tool/tool_interactive.h>
#include <tool/tool_event.h>
#include <tool/tool_menu.h>

class SCH_EDIT_FRAME;

/**
 * Class SCH_EDITOR_CONTROL
 *
 * Handles actions specific to the schematic editor in eeschema.
 */
class SCH_EDITOR_CONTROL : public wxEvtHandler, public TOOL_INTERACTIVE
{
public:
    SCH_EDITOR_CONTROL();
    ~SCH_EDITOR_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int ToggleLockSelected( const TOOL_EVENT& aEvent );
    int LockSelected( const TOOL_EVENT& aEvent );
    int UnlockSelected( const TOOL_EVENT& aEvent );

    ///> Reacts to selection change in pcbnew.
    int CrossProbePcbToSch( const TOOL_EVENT& aEvent );

    ///> Notifies pcbnew about the selected item.
    int CrossProbeSchToPcb( const TOOL_EVENT& aEvent );

#ifdef KICAD_SPICE
    int SimProbe( const TOOL_EVENT& aEvent );
    int SimTune( const TOOL_EVENT& aEvent );
#endif /* KICAD_SPICE */

    ///> Highlights net under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///> Removes any net highlighting
    int ClearHighlight( const TOOL_EVENT& aEvent );

    ///> Highlights frame's SelectedNetName.
    int HighlightNetSelection( const TOOL_EVENT& aEvent );

    ///> Launches a tool to highlight nets.
    int HighlightNetCursor( const TOOL_EVENT& aEvent );

    ///> Clipboard support.
    int Cut( const TOOL_EVENT& aEvent );
    int Copy( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );

private:
    ///> copy selection to clipboard
    bool doCopy();

    ///> Sets up handlers for various events.
    void setTransitions() override;

    SCH_EDIT_FRAME* m_frame;    ///> Pointer to the currently used edit frame

    bool m_probingSchToPcb;     ///> Recursion guard when cross-probing to PCBNew

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;
};


#endif // SCH_EDITOR_CONTROL_H
