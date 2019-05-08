/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef SCH_INSPECTION_TOOL_H
#define SCH_INSPECTION_TOOL_H

#include <boost/optional/optional.hpp>
#include <tool/tool_interactive.h>
#include <sch_base_frame.h>


class SCH_SELECTION_TOOL;
class SCH_EDIT_FRAME;


class INSPECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    INSPECTION_TOOL();
    ~INSPECTION_TOOL() {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    ///> @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int ShowDatasheet( const TOOL_EVENT& aEvent );
    int ShowMarkerInfo( const TOOL_EVENT& aEvent );

    int UpdateMessagePanel( const TOOL_EVENT& aEvent );

private:
    ///> @copydoc TOOL_INTERACTIVE::setTransitions();
    void setTransitions() override;

private:
    SCH_SELECTION_TOOL*   m_selectionTool;
    KIGFX::SCH_VIEW*      m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    SCH_BASE_FRAME*       m_frame;
};

#endif /* SCH_INSPECTION_TOOL_H */
