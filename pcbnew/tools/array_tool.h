/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <memory>

#include <wx/event.h>

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_selection.h>

class ARRAY_OPTIONS;
class BOARD_COMMIT;
class DIALOG_CREATE_ARRAY;

/**
 * The array tool.
 *
 * Handles actions and cross-action state for creating arrays.
 *
 * The cross-action persistence is for cases such as hiding a dialog
 * to give the user a chance to choose a point or item manually.
 */

class ARRAY_TOOL : public PCB_TOOL_BASE, public wxEvtHandler
{
public:
    ARRAY_TOOL();
    ~ARRAY_TOOL();

    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /**
     * Invoke a dialog box to allow positioning of the item relative to another by an exact amount.
     */
    int CreateArray( const TOOL_EVENT& aEvent );

    /**
     * Position the m_position_relative_selection selection relative to anchor position using
     * the given translation.
     */
    // int DoCreateArray( const VECTOR2I& anchor, const VECTOR2I& translation );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    void onDialogClosed( wxCloseEvent& aEvent );

    DIALOG_CREATE_ARRAY*           m_dialog;
    std::unique_ptr<ARRAY_OPTIONS> m_array_opts;
    std::unique_ptr<PCB_SELECTION> m_selection;
};
