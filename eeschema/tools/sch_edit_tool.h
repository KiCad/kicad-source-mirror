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
#include <sch_base_frame.h>


class SCH_EDIT_TOOL : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_EDIT_TOOL();
    ~SCH_EDIT_TOOL() = default;

    static const std::vector<KICAD_T> RotatableItems;
    static const std::vector<KICAD_T> SwappableItems;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int Rotate( const TOOL_EVENT& aEvent );
    int Mirror( const TOOL_EVENT& aEvent );
    int Swap( const TOOL_EVENT& aEvent );
    int SwapPins( const TOOL_EVENT& aEvent );
    int SwapPinLabels( const TOOL_EVENT& aEvent );
    int SwapUnitLabels( const TOOL_EVENT& aEvent );

    int RepeatDrawItem( const TOOL_EVENT& aEvent );

    int Properties( const TOOL_EVENT& aEvent );
    int EditField( const TOOL_EVENT& aEvent );
    int AutoplaceFields( const TOOL_EVENT& aEvent );
    int ChangeSymbols( const TOOL_EVENT& aEvent );
    int CycleBodyStyle( const TOOL_EVENT& aEvent );
    int EditPageNumber( const TOOL_EVENT& aEvent );

    /**
     * Change a text type to another one.
     *
     * The new text, label, hierarchical label, or global label is created from the old text
     * and the old text object is deleted.
     *
     * A tricky case is when the 'old" text is being edited (i.e. moving) because we must
     * create a new text, and prepare the undo/redo command data for this change and the
     * current move/edit command
     */
    int ChangeTextType( const TOOL_EVENT& aEvent );

    int JustifyText( const TOOL_EVENT& aEvent );

    int CleanupSheetPins( const TOOL_EVENT& aEvent );
    int GlobalEdit( const TOOL_EVENT& aEvent );

    ///< Delete the selected items, or the item under the cursor.
    int DoDelete( const TOOL_EVENT& aEvent );

    /// Drag and drop
    int DdAppendFile( const TOOL_EVENT& aEvent );
    int DdAddImage( const TOOL_EVENT& aEvent );

    /// Modify Attributes (DNP, Exclude, etc.)  All attributes are
    /// set to true unless all symbols already have the attribute set to true.
    int SetAttribute( const TOOL_EVENT& aEvent );

    void EditProperties( EDA_ITEM* aItem );

    wxString FixERCErrorMenuText( const std::shared_ptr<RC_ITEM>& aERCItem );
    void FixERCError( const std::shared_ptr<RC_ITEM>& aERCItem );

private:
    void editFieldText( SCH_FIELD* aField );

    void collectUnits( const SCH_SELECTION& aSelection,
                       std::set<std::pair<SCH_SYMBOL*, SCH_SCREEN*>>& aCollectedUnits );

    ///< Set up handlers for various events.
    void setTransitions() override;
};
