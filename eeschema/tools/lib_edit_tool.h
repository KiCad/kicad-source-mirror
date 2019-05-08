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

#ifndef KICAD_LIB_EDIT_TOOL_H
#define KICAD_LIB_EDIT_TOOL_H

#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>
#include <sch_base_frame.h>


class LIB_EDIT_FRAME;
class SCH_SELECTION_TOOL;


class LIB_EDIT_TOOL : public TOOL_INTERACTIVE
{
public:
    LIB_EDIT_TOOL();
    ~LIB_EDIT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///> Get the SCH_DRAWING_TOOL top-level context menu
    inline TOOL_MENU& GetToolMenu() { return m_menu; }

    int Rotate( const TOOL_EVENT& aEvent );
    int Mirror( const TOOL_EVENT& aEvent );

    int Duplicate( const TOOL_EVENT& aEvent );

    int Properties( const TOOL_EVENT& aEvent );

    int Cut( const TOOL_EVENT& aEvent );
    int Copy( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );

    /**
     * Function DoDelete()
     *
     * Deletes the selected items, or the item under the cursor.
     */
    int DoDelete( const TOOL_EVENT& aEvent );

    ///> Runs the deletion tool.
    int DeleteItemCursor( const TOOL_EVENT& aEvent );

private:
    void editGraphicProperties( LIB_ITEM* aItem );
    void editTextProperties( LIB_ITEM* aItem );
    void editFieldProperties( LIB_FIELD* aField );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    SCH_SELECTION_TOOL*   m_selectionTool;
    LIB_EDIT_FRAME*       m_frame;

    /// Menu model displayed by the tool.
    TOOL_MENU             m_menu;
};

#endif //KICAD_LIB_EDIT_TOOL_H
