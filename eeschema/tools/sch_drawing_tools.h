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

#ifndef SCH_DRAWING_TOOLS_H
#define SCH_DRAWING_TOOLS_H

#include <tools/ee_tool_base.h>
#include <sch_base_frame.h>
#include <status_popup.h>

class SCH_COMPONENT;
class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCHLIB_FILTER;
class SCH_EDIT_FRAME;
class EE_SELECTION_TOOL;


/**
 * Class SCH_DRAWING_TOOLS
 *
 * Tool responsible for drawing/placing items (symbols, wires, busses, labels, etc.)
 */

class SCH_DRAWING_TOOLS : public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_DRAWING_TOOLS();
    ~SCH_DRAWING_TOOLS() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int PlaceComponent( const TOOL_EVENT& aEvent );
    int SingleClickPlace( const TOOL_EVENT& aEvent );
    int TwoClickPlace( const TOOL_EVENT& aEvent );
    int DrawSheet( const TOOL_EVENT& aEvent );
    int PlaceImage( const TOOL_EVENT& aEvent );

private:

    void sizeSheet( SCH_SHEET* aSheet, VECTOR2I aPos );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    std::unique_ptr<STATUS_TEXT_POPUP> m_statusPopup;
};

#endif /* SCH_DRAWING_TOOLS_H */
