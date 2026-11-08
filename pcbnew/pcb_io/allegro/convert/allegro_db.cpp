/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include "convert/allegro_db.h"

#include <algorithm>

#include <convert/allegro_pcb_structs.h>

using namespace ALLEGRO;


void BRD_DB::ReserveCapacity( size_t aObjectCount, size_t aStringCount, size_t aFileSize )
{
    // The counts come from the file header, so clamp them to what the file could hold. A fixed
    // ceiling starves real designs and costs a full rehash of the key map at every doubling
    constexpr size_t minBlockBytes = 16;
    constexpr size_t minStringBytes = 2;

    aObjectCount = std::min( aObjectCount, aFileSize / minBlockBytes );
    aStringCount = std::min( aStringCount, aFileSize / minStringBytes );

    m_Blocks.reserve( aObjectCount );
    m_ObjectKeyMap.reserve( aObjectCount );
    m_StringTable.reserve( aStringCount );
}


void BRD_DB::InsertBlock( std::unique_ptr<BLOCK_BASE> aBlock )
{
    if( aBlock->GetKey() != 0 )
        m_ObjectKeyMap[aBlock->GetKey()] = aBlock.get();

    m_Blocks.push_back( std::move( aBlock ) );
}


BRD_DB::BRD_DB() :
        m_FmtVer( FMT_VER::V_UNKNOWN )
{
}
