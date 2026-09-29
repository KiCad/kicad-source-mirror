/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SYMBOL_EDITOR_CONTEXT_H
#define KICAD_SYMBOL_EDITOR_CONTEXT_H

#include <memory>

#include <lib_id.h>
#include <wx/string.h>

class KIWAY;
class LIB_SYMBOL;
class PROJECT;
class SCH_SCREEN;
class SYMBOL_EDIT_FRAME;
class TOOL_MANAGER;


class SYMBOL_EDITOR_CONTEXT
{
public:
    virtual ~SYMBOL_EDITOR_CONTEXT() = default;

    virtual LIB_SYMBOL* GetCurSymbol() const = 0;

    virtual SCH_SCREEN* GetScreen() const = 0;

    virtual LIB_ID GetLoadedLibId() const = 0;

    virtual bool IsContentModified() const = 0;

    virtual void SetContentModified( bool aModified = true ) = 0;

    virtual PROJECT& Prj() const = 0;

    virtual TOOL_MANAGER* GetToolManager() const = 0;

    virtual KIWAY* GetKiway() const = 0;

    virtual bool CanAcceptApiCommands() const = 0;

    /**
     * Save the current symbol to its library, without any user interaction.
     * @return false if there is no symbol, the library is read-only, or the save failed
     */
    virtual bool SaveSymbol() = 0;

    /// Save a copy of the current symbol to the given library file, without user interaction
    virtual bool SaveSymbolCopy( const wxString& aFileName, bool aOverwrite ) = 0;

    /// Reload the current symbol from its library, discarding unsaved changes
    virtual bool RevertSymbol() = 0;

    /// Load a symbol into the editor (activating it for editing); false if not found
    virtual bool LoadSymbol( const LIB_ID& aLibId ) = 0;
};


std::shared_ptr<SYMBOL_EDITOR_CONTEXT> CreateSymbolEditorFrameContext( SYMBOL_EDIT_FRAME* aFrame );

/**
 * Shared implementation of SaveSymbolCopy for all contexts; writes the given symbol to a new
 * library file, creating it or overwriting it as requested.
 */
bool SaveSymbolCopyToFile( const LIB_SYMBOL& aSymbol, const wxString& aFileName, bool aOverwrite );

#endif // KICAD_SYMBOL_EDITOR_CONTEXT_H
