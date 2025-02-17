/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program;
 * if not, you may find one at http://www.gnu.org/licenses/
 */

#ifndef LIBRARY_EDITOR_CONTROL_H
#define LIBRARY_EDITOR_CONTROL_H

#include <tool/tool_interactive.h>
#include <project.h>


class EDA_DRAW_FRAME;

/**
 * Module editor specific tools.
 */
class LIBRARY_EDITOR_CONTROL : public TOOL_INTERACTIVE
{
public:
    LIBRARY_EDITOR_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    void AddContextMenuItems( CONDITIONAL_MENU* aMenu );

    int PinLibrary( const TOOL_EVENT& aEvent );
    int UnpinLibrary( const TOOL_EVENT& aEvent );
    int ToggleLibraryTree( const TOOL_EVENT& aEvent );
    int LibraryTreeSearch( const TOOL_EVENT& aEvent );

    bool RenameLibrary( const wxString& aTitle, const wxString& aName,
                        std::function<bool( const wxString& aNewName )> aValidator );

private:
    void changeSelectedPinStatus( const bool aPin );

    /// Set up handlers for various events.
    void setTransitions() override;

    void regenerateLibraryTree();

private:
    EDA_DRAW_FRAME* m_frame;
};

#endif  // LIBRARY_EDITOR_CONTROL_H
