/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "zone_manager_preference.h"
#include <kiplatform/ui.h>


//NOTE - Is recording in the setting file needed ?
static bool s_refillOnClose = false;


bool ZONE_MANAGER_PREFERENCE::GetRepourOnClose()
{
    return s_refillOnClose;
}


wxColour ZONE_MANAGER_PREFERENCE::GetCanvasBackgroundColor()
{
    if( KIPLATFORM::UI::IsDarkTheme() )
        return wxColour( 0, 0, 0, 30 );

    return wxColour( 238, 243, 243 );
}


wxColour ZONE_MANAGER_PREFERENCE::GetBoundBoundingFillColor()
{
    if( KIPLATFORM::UI::IsDarkTheme() )
        return wxColour( 238, 243, 243, 60 );

    return wxColour( 84, 84, 84, 40 );
}


void ZONE_MANAGER_PREFERENCE::SetRefillOnClose( bool aRepour )
{
    s_refillOnClose = aRepour;
}
