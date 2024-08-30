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


#ifndef SYMBOL_EDITOR_CONTROL_H
#define SYMBOL_EDITOR_CONTROL_H

#include <sch_base_frame.h>
#include <tools/sch_tool_base.h>

class SYMBOL_EDIT_FRAME;

/**
 * Handle actions for the various symbol editor and viewers.
 */
class SYMBOL_EDITOR_CONTROL : public wxEvtHandler, public SCH_TOOL_BASE<SCH_BASE_FRAME>
{
public:
    SYMBOL_EDITOR_CONTROL() :
            SCH_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.SymbolLibraryControl" )
    { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int AddLibrary( const TOOL_EVENT& aEvent );
    int AddSymbol( const TOOL_EVENT& aEvent );
    int EditSymbol( const TOOL_EVENT& aEvent );
    int EditLibrarySymbol( const TOOL_EVENT& aEvent );

    int Save( const TOOL_EVENT& aEvt );
    int Revert( const TOOL_EVENT& aEvent );

    int CutCopyDelete( const TOOL_EVENT& aEvent );
    int DuplicateSymbol( const TOOL_EVENT& aEvent );
    int RenameSymbol( const TOOL_EVENT& newName );
    int ExportSymbol( const TOOL_EVENT& aEvent );
    int OpenDirectory( const TOOL_EVENT& aEvent );
    int OpenWithTextEditor( const TOOL_EVENT& aEvent );
    int ExportView( const TOOL_EVENT& aEvent );
    int ExportSymbolAsSVG( const TOOL_EVENT& aEvent );
    int AddSymbolToSchematic( const TOOL_EVENT& aEvent );

    int OnDeMorgan( const TOOL_EVENT& aEvent );

    int ShowElectricalTypes( const TOOL_EVENT& aEvent );
    int ShowPinNumbers( const TOOL_EVENT& aEvent );
    int ToggleProperties( const TOOL_EVENT& aEvent );
    int ToggleSyncedPinsMode( const TOOL_EVENT& aEvent );

    int ToggleHiddenPins( const TOOL_EVENT& aEvent );
    int ToggleHiddenFields( const TOOL_EVENT& aEvent );
    int TogglePinAltIcons( const TOOL_EVENT& aEvent );

    int ChangeUnit( const TOOL_EVENT& aEvent );

    int DdAddLibrary( const TOOL_EVENT& aEvent );

    int ShowLibraryTable( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;
};


#endif // SYMBOL_EDITOR_CONTROL_H
