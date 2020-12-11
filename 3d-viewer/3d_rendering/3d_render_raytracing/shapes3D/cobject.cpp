/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cobject.cpp
 * @brief
 */

#include "cobject.h"
#include <cstdio>
#include <map>


COBJECT3D_STATS *COBJECT3D_STATS::s_instance = 0;

static const CBLINN_PHONG_MATERIAL s_defaultMaterial = CBLINN_PHONG_MATERIAL();


COBJECT::COBJECT( OBJECT3D_TYPE aObjType )
{
    m_obj_type = aObjType;
    COBJECT3D_STATS::Instance().AddOne( aObjType );
    m_material = &s_defaultMaterial;
    m_modelTransparency = 0.0f;
    m_boardItem = nullptr;
}


/*
 * Lookup table for OBJECT2D_TYPE printed names
 */
// clang-format off
const std::map<OBJECT3D_TYPE, const char*> objectTypeNames
{
    { OBJECT3D_TYPE::CYLINDER,   "OBJECT3D_TYPE::CYLINDER" },
    { OBJECT3D_TYPE::DUMMYBLOCK, "OBJECT2D_TYPE::DUMMYBLOCK" },
    { OBJECT3D_TYPE::LAYERITEM,  "OBJECT2D_TYPE::LAYERITEM" },
    { OBJECT3D_TYPE::XYPLANE,    "OBJECT2D_TYPE::XYPLANE" },
    { OBJECT3D_TYPE::ROUNDSEG,   "OBJECT2D_TYPE::ROUNDSEG" },
    { OBJECT3D_TYPE::TRIANGLE,   "OBJECT2D_TYPE::TRIANGLE" }
};
// clang-format on


void COBJECT3D_STATS::PrintStats()
{
    wxLogDebug( "OBJ3D Statistics:\n" );

    for( auto& objectType : objectTypeNames )
    {
        wxLogDebug( "  %20s  %u\n", objectType.second,
                    m_counter[static_cast<int>( objectType.first )] );
    }
}
