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

#ifndef KICAD_LIB_PIN_TOOL_H
#define KICAD_LIB_PIN_TOOL_H

#include <tools/ee_tool_base.h>
#include <sch_base_frame.h>


class LIB_EDIT_FRAME;


class LIB_PIN_TOOL : public EE_TOOL_BASE<LIB_EDIT_FRAME>
{
public:
    LIB_PIN_TOOL();
    ~LIB_PIN_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    LIB_PIN* CreatePin( const VECTOR2I& aPosition, LIB_PART* aPart );
    LIB_PIN* RepeatPin( const LIB_PIN* aSourcePin );

    bool PlacePin( LIB_PIN* aPin );
    void CreateImagePins( LIB_PIN* aPin );

    bool EditPinProperties( LIB_PIN* aPin );
    int PushPinProperties( const TOOL_EVENT& aEvent );

private:
    ///> Sets up handlers for various events.
    void setTransitions() override;
};

#endif //KICAD_LIB_PIN_TOOL_H
