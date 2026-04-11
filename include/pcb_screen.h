/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef PCB_SCREEN_H
#define PCB_SCREEN_H


#include <base_screen.h>
#include <board_item.h>


/* Handle info to display a board */
class PCB_SCREEN : public BASE_SCREEN
{
public:
    /**
     * @param aPageSizeIU is the size of the initial paper page in internal units.
     */
    PCB_SCREEN( const VECTOR2I& aPageSizeIU );

    PCB_LAYER_ID m_Active_Layer;
    PCB_LAYER_ID m_Route_Layer_TOP;
    PCB_LAYER_ID m_Route_Layer_BOTTOM;
};

#endif  // PCB_SCREEN_H
