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

#ifndef SYMBOL_EDITOR_PIN_TOOL_H
#define SYMBOL_EDITOR_PIN_TOOL_H

#include <tools/sch_tool_base.h>
#include <sch_base_frame.h>


class SYMBOL_EDIT_FRAME;


class SYMBOL_EDITOR_PIN_TOOL : public SCH_TOOL_BASE<SYMBOL_EDIT_FRAME>
{
public:
    SYMBOL_EDITOR_PIN_TOOL();
    ~SYMBOL_EDITOR_PIN_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    SCH_PIN* CreatePin( const VECTOR2I& aPosition, LIB_SYMBOL* aSymbol );
    SCH_PIN* RepeatPin( const SCH_PIN* aSourcePin );

    bool PlacePin( SCH_PIN* aPin );
    void CreateImagePins( SCH_PIN* aPin );

    bool EditPinProperties( SCH_PIN* aPin, bool aFocusPinNumber );
    int PushPinProperties( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;
};

#endif // SYMBOL_EDITOR_PIN_TOOL_H
