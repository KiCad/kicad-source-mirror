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

#ifndef KICAD_SCH_EDIT_TOOL_H
#define KICAD_SCH_EDIT_TOOL_H

#include <tools/ee_tool_base.h>
#include <sch_base_frame.h>


class SCH_EDIT_FRAME;
class EE_SELECTION_TOOL;


class SCH_EDIT_TOOL : public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_EDIT_TOOL();
    ~SCH_EDIT_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int Rotate( const TOOL_EVENT& aEvent );
    int Mirror( const TOOL_EVENT& aEvent );

    int Duplicate( const TOOL_EVENT& aEvent );
    int RepeatDrawItem( const TOOL_EVENT& aEvent );

    int Properties( const TOOL_EVENT& aEvent );
    int EditField( const TOOL_EVENT& aEvent );
    int AutoplaceFields( const TOOL_EVENT& aEvent );
    int UpdateFields( const TOOL_EVENT& aEvent );
    int ConvertDeMorgan( const TOOL_EVENT& aEvent );

    int ChangeShape( const TOOL_EVENT& aEvent );
    int ChangeTextType( const TOOL_EVENT& aEvent );

    int BreakWire( const TOOL_EVENT& aEvent );

    int CleanupSheetPins( const TOOL_EVENT& aEvent );
    int GlobalEdit( const TOOL_EVENT& aEvent );

    ///> Deletes the selected items, or the item under the cursor.
    int DoDelete( const TOOL_EVENT& aEvent );

    ///> Runs the deletion tool.
    int DeleteItemCursor( const TOOL_EVENT& aEvent );

private:
    void editComponentFieldText( SCH_FIELD* aField );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    EDA_ITEM* m_pickerItem;
};

#endif //KICAD_SCH_EDIT_TOOL_H
