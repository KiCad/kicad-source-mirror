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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GERBVIEW_INSPECTION_TOOL_H
#define GERBVIEW_INSPECTION_TOOL_H

#include <tool/tool_interactive.h>
#include <gerbview_frame.h>

class TOOL_EVENT;

class GERBVIEW_INSPECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    GERBVIEW_INSPECTION_TOOL();
    ~GERBVIEW_INSPECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///< Launch a tool to measure between points
    int MeasureTool( const TOOL_EVENT& aEvent );

    ///< Show a list of the DCodes
    int ShowDCodes( const TOOL_EVENT& aEvent );

    ///< Show the source for the gerber file
    int ShowSource( const TOOL_EVENT& aEvent );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    GERBVIEW_FRAME* m_frame;        // Pointer to the parent frame.
};

#endif
