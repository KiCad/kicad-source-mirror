/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
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

#include "object_2d.h"
#include <map>

OBJECT_2D_STATS *OBJECT_2D_STATS::s_instance = nullptr;


OBJECT_2D::OBJECT_2D( OBJECT_2D_TYPE aObjType, const BOARD_ITEM& aBoardItem ) :
        m_boardItem( aBoardItem )
{
    m_obj_type = aObjType;
    OBJECT_2D_STATS::Instance().AddOne( aObjType );
}


/*
 * Lookup table for OBJECT_2D_TYPE printed names
 */
// clang-format off
const std::map<OBJECT_2D_TYPE, const char*> objectTypeNames
{
    { OBJECT_2D_TYPE::FILLED_CIRCLE, "OBJECT_2D_TYPE::FILLED_CIRCLE" },
    { OBJECT_2D_TYPE::CSG,           "OBJECT_2D_TYPE::CSG" },
    { OBJECT_2D_TYPE::POLYGON,       "OBJECT_2D_TYPE::POLYGON" },
    { OBJECT_2D_TYPE::DUMMYBLOCK,    "OBJECT_2D_TYPE::DUMMYBLOCK" },
    { OBJECT_2D_TYPE::POLYGON4PT,    "OBJECT_2D_TYPE::POLYGON4PT" },
    { OBJECT_2D_TYPE::RING,          "OBJECT_2D_TYPE::RING" },
    { OBJECT_2D_TYPE::ROUNDSEG,      "OBJECT_2D_TYPE::ROUNDSEG" },
    { OBJECT_2D_TYPE::TRIANGLE,      "OBJECT_2D_TYPE::TRIANGLE" },
    { OBJECT_2D_TYPE::CONTAINER,     "OBJECT_2D_TYPE::CONTAINER" },
    { OBJECT_2D_TYPE::BVHCONTAINER,  "OBJECT_2D_TYPE::BVHCONTAINER" },
};
// clang-format on
