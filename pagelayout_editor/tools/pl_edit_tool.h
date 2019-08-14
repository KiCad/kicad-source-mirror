/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef PL_EDIT_TOOL_H
#define PL_EDIT_TOOL_H

#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>


class PL_EDITOR_FRAME;
class PL_SELECTION_TOOL;


class PL_EDIT_TOOL : public TOOL_INTERACTIVE
{
public:
    PL_EDIT_TOOL();
    ~PL_EDIT_TOOL() {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// The "move" event loop
    int Main( const TOOL_EVENT& aEvent );

    int Undo( const TOOL_EVENT& aEvent );
    int Redo( const TOOL_EVENT& aEvent );

    int Cut( const TOOL_EVENT& aEvent );
    int Copy( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );

    int ImportWorksheetContent( const TOOL_EVENT& aEvent );

    /**
     * Function DoDelete()
     *
     * Deletes the selected items, or the item under the cursor.
     */
    int DoDelete( const TOOL_EVENT& aEvent );

    ///> Runs the deletion tool.
    int DeleteItemCursor( const TOOL_EVENT& aEvent );

private:
    void moveItem( EDA_ITEM* aItem, VECTOR2I aDelta );

    ///> Returns the right modification point (e.g. for rotation), depending on the number of
    ///> selected items.
    bool updateModificationPoint( PL_SELECTION& aSelection );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    PL_EDITOR_FRAME*   m_frame;
    PL_SELECTION_TOOL* m_selectionTool;

    ///> Flag determining if anything is being dragged right now
    bool               m_moveInProgress;

    ///> Used for chaining commands
    VECTOR2I           m_moveOffset;

    ///> Last cursor position (needed for getModificationPoint() to avoid changes
    ///> of edit reference point).
    VECTOR2I           m_cursor;

    EDA_ITEM*          m_pickerItem;
};

#endif //PL_EDIT_TOOL_H
