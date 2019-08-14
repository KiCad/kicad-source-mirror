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

#ifndef KICAD_MANAGER_CONTROL_H
#define KICAD_MANAGER_CONTROL_H

#include <tool/tool_interactive.h>


class KICAD_MANAGER_FRAME;


/**
 * Class KICAD_MANAGER_CONTROL
 *
 * Handles actions in the kicad manager frame.
 */

class KICAD_MANAGER_CONTROL : public TOOL_INTERACTIVE
{
public:
    KICAD_MANAGER_CONTROL();
    ~KICAD_MANAGER_CONTROL() override { }

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int NewProject( const TOOL_EVENT& aEvent );
    int NewFromTemplate( const TOOL_EVENT& aEvent );
    int OpenProject( const TOOL_EVENT& aEvent );

    int Refresh( const TOOL_EVENT& aEvent );
    int UpdateMenu( const TOOL_EVENT& aEvent );

    int ShowPlayer( const TOOL_EVENT& aEvent );
    int Execute( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    ///> Pointer to the currently used edit/draw frame.
    KICAD_MANAGER_FRAME* m_frame;
};

#endif
