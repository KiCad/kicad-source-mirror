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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_DESIGN_WORKSPACE_H
#define DIALOG_DESIGN_WORKSPACE_H

#include <dialog_shim.h>

class EDA_BASE_FRAME;
class PANEL_GIT_WORKSPACE;


/**
 * The Workspace page in a window of its own, for a project whose design workspace is not reached through git and
 * so has no advanced git window to show it in.
 */
class DIALOG_DESIGN_WORKSPACE : public DIALOG_SHIM
{
public:
    /// Raise aFrame's window, opening it if needed, and select aItemId when not empty
    static void ShowForFrame( EDA_BASE_FRAME* aFrame, const wxString& aItemId = wxEmptyString );

private:
    explicit DIALOG_DESIGN_WORKSPACE( EDA_BASE_FRAME* aFrame );

    PANEL_GIT_WORKSPACE* m_page;
};

#endif // DIALOG_DESIGN_WORKSPACE_H
