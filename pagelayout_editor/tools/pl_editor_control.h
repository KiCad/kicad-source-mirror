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


#ifndef PL_EDITOR_CONTROL_H
#define PL_EDITOR_CONTROL_H

#include <tool/tool_interactive.h>

class PL_EDITOR_FRAME;


/**
 * Class PL_EDITOR_CONTROL
 *
 * Handles actions specific to the schematic editor in eeschema.
 */
class PL_EDITOR_CONTROL : public wxEvtHandler, public TOOL_INTERACTIVE
{
public:
    PL_EDITOR_CONTROL()  :
            TOOL_INTERACTIVE( "plEditor.EditorControl" ),
            m_frame( nullptr )
    { }

    ~PL_EDITOR_CONTROL() { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int New( const TOOL_EVENT& aEvent );
    int Open( const TOOL_EVENT& aEvent );
    int Save( const TOOL_EVENT& aEvent );
    int SaveAs( const TOOL_EVENT& aEvent );
    int PageSetup( const TOOL_EVENT& aEvent );
    int Print( const TOOL_EVENT& aEvent );
    int Plot( const TOOL_EVENT& aEvent );

    int ToggleBackgroundColor( const TOOL_EVENT& aEvent );
    int ShowInspector( const TOOL_EVENT& aEvent );

    /**
     * Update the message panel *and* the Properties frame, after change
     * (selection, move, edit ...) of a wks item
     */
    int UpdateMessagePanel( const TOOL_EVENT& aEvent );

private:
    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    PL_EDITOR_FRAME*   m_frame;

};


#endif // PL_EDITOR_CONTROL_H
