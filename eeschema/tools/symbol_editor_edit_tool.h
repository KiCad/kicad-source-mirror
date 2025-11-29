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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
