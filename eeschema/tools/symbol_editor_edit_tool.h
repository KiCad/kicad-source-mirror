/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <tools/sch_tool_base.h>


class SCH_PIN;
class SCH_SHAPE;
class SYMBOL_EDIT_FRAME;


class SYMBOL_EDITOR_EDIT_TOOL : public SCH_TOOL_BASE<SYMBOL_EDIT_FRAME>
{
public:
    SYMBOL_EDITOR_EDIT_TOOL();
    ~SYMBOL_EDITOR_EDIT_TOOL() = default;

    static const std::vector<KICAD_T> SwappableItems;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int Rotate( const TOOL_EVENT& aEvent );
    int Mirror( const TOOL_EVENT& aEvent );
    int Swap( const TOOL_EVENT& aEvent );

    int Duplicate( const TOOL_EVENT& aEvent );

    int Properties( const TOOL_EVENT& aEvent );

    /// Open the symbol properties dialog directly on its Pin Map page (issue #2282).
    int EditSymbolPinMaps( const TOOL_EVENT& aEvent );

    int PinTable( const TOOL_EVENT& aEvent );
    int ConvertStackedPins( const TOOL_EVENT& aEvent );
    int ExplodeStackedPin( const TOOL_EVENT& aEvent );
    int UpdateSymbolFields( const TOOL_EVENT& aEvent );

    int Undo( const TOOL_EVENT& aEvent );
    int Redo( const TOOL_EVENT& aEvent );
    int Cut( const TOOL_EVENT& aEvent );
    int Copy( const TOOL_EVENT& aEvent );
    int CopyAsText( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );

    /**
     * Delete the selected items, or the item under the cursor.
     */
    int DoDelete( const TOOL_EVENT& aEvent );

private:
    void editShapeProperties( SCH_SHAPE* aShape );
    void editTextProperties( SCH_ITEM* aItem );
    void editTextBoxProperties( SCH_ITEM* aItem );
    void editFieldProperties( SCH_FIELD* aField );
    void editSymbolProperties();
    void editSymbolPropertiesFromLibrary( const LIB_ID& aLibId );

    ///< Set up handlers for various events.
    void setTransitions() override;
};
