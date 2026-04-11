/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#ifndef BITMAP2CMP_CONTROL_H
#define BITMAP2CMP_CONTROL_H

#include <tool/tool_interactive.h>

class BITMAP2CMP_FRAME;


/**
 * Handle actions for the various symbol editor and viewers.
 */
class BITMAP2CMP_CONTROL : public wxEvtHandler, public TOOL_INTERACTIVE
{
public:
    BITMAP2CMP_CONTROL() :
            TOOL_INTERACTIVE( "bitmap2cmp.Control" ),
            m_frame( nullptr )
    { }

    virtual ~BITMAP2CMP_CONTROL() { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int Open( const TOOL_EVENT& aEvent );
    int Close( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    BITMAP2CMP_FRAME* m_frame;
};


#endif // BITMAP2CMP_CONTROL_H
