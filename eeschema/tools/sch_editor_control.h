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
class SCH_COMPONENT;
class SCHLIB_FILTER;

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

    /**
     * Function PlaceComponent()
     * Displays a dialog to select a module to be added and allows the user to set its position.
     */
    int PlaceComponent( const TOOL_EVENT& aEvent );

    ///> Toggles 'lock' property for selected items.
    int ToggleLockSelected( const TOOL_EVENT& aEvent );

    ///> Locks selected items.
    int LockSelected( const TOOL_EVENT& aEvent );

    ///> Unlocks selected items.
    int UnlockSelected( const TOOL_EVENT& aEvent );

    ///> Reacts to selection change in pcbnew.
    int CrossProbePcbToSch( const TOOL_EVENT& aEvent );

    ///> Notifies pcbnew about the selected item.
    int CrossProbeSchToPcb( const TOOL_EVENT& aEvent );

    ///> Highlights net belonging to the item under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///> Highlight all items which match the frame's SelectedNetName.
    int HighlightNetSelection( const TOOL_EVENT& aEvent );

    ///> Launches a tool to pick the item whose net is going to be highlighted.
    int HighlightNetCursor( const TOOL_EVENT& aEvent );

    int PlaceSymbol( const TOOL_EVENT& aEvent );
    int PlacePower( const TOOL_EVENT& aEvent );

private:

    int placeComponent( SCH_COMPONENT* aComponent, SCHLIB_FILTER* aFilter,
                        SCH_BASE_FRAME::HISTORY_LIST aHistoryList );

    ///> Sets up handlers for various events.
    void setTransitions() override;

    ///> Pointer to the currently used edit frame.
    SCH_EDIT_FRAME* m_frame;

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;

};


#endif // SCH_EDITOR_CONTROL_H
