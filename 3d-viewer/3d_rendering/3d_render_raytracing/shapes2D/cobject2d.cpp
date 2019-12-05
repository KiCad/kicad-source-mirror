/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  cobject2d.cpp
 * @brief
 */

#include "cobject2d.h"
#include <cstdio>


COBJECT2D_STATS *COBJECT2D_STATS::s_instance = 0;


COBJECT2D::COBJECT2D( OBJECT2D_TYPE aObjType, const BOARD_ITEM &aBoardItem )
    : m_boardItem(aBoardItem)
{
    m_obj_type = aObjType;
    COBJECT2D_STATS::Instance().AddOne( aObjType );
}


static const char *OBJECT2D_STR[OBJ2D_MAX] =
{
    "OBJ2D_FILLED_CIRCLE",
    "OBJ2D_CSG",
    "OBJ2D_POLYGON",
    "OBJ2D_DUMMYBLOCK",
    "OBJ2D_POLYGON4PT",
    "OBJ2D_RING",
    "OBJ2D_ROUNDSEG",
    "OBJ2D_TRIANGLE",
    "OBJ2D_CONTAINER",
    "OBJ2D_BVHCONTAINER"
};


void COBJECT2D_STATS::PrintStats()
{
    printf( "OBJ2D Statistics:\n" );

    for( unsigned int i = 0; i < OBJ2D_MAX; ++i )
    {
        printf( "  %20s  %u\n", OBJECT2D_STR[i], m_counter[i] );
    }
}
