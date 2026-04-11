/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file  object_3d.cpp
 */

#include "object_3d.h"
#include <board_item.h>
#include "../hitinfo.h"
#include <map>


OBJECT_3D_STATS* OBJECT_3D_STATS::s_instance = 0;

static const BLINN_PHONG_MATERIAL s_defaultMaterial = BLINN_PHONG_MATERIAL();


OBJECT_3D::OBJECT_3D( OBJECT_3D_TYPE aObjType )
{
    m_obj_type = aObjType;
    OBJECT_3D_STATS::Instance().AddOne( aObjType );
    m_material = &s_defaultMaterial;
    m_modelTransparency = 0.0f;
    m_boardItem = nullptr;
}


/*
 * Lookup table for OBJECT_2D_TYPE printed names
 */
// clang-format off
const std::map<OBJECT_3D_TYPE, const char*> objectTypeNames
{
    { OBJECT_3D_TYPE::CYLINDER,   "OBJECT_3D_TYPE::CYLINDER" },
    { OBJECT_3D_TYPE::DUMMYBLOCK, "OBJECT_3D_TYPE::DUMMY_BLOCK" },
    { OBJECT_3D_TYPE::LAYERITEM,  "OBJECT_3D_TYPE::LAYER_ITEM" },
    { OBJECT_3D_TYPE::XYPLANE,    "OBJECT_3D_TYPE::XY_PLANE" },
    { OBJECT_3D_TYPE::ROUNDSEG,   "OBJECT_3D_TYPE::ROUND_SEG" },
    { OBJECT_3D_TYPE::TRIANGLE,   "OBJECT_3D_TYPE::TRIANGLE" }
};
// clang-format on


// void OBJECT_3D_STATS::PrintStats()
// {
//     wxLogDebug( "OBJECT_3D_STATS:\n" );

//     for( auto& objectType : objectTypeNames )
//     {
//         wxLogDebug( "  %20s  %u\n", objectType.second,
//                     m_counter[static_cast<int>( objectType.first )] );
//     }
// }
