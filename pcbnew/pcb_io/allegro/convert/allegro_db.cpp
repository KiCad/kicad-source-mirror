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

#include <set>

#include <wx/log.h>

#include <ki_exception.h>

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


static void collectSentinelKeys( const FILE_HEADER& aHeader, BRD_DB& aDb )
{
    auto addTail = [&]( const FILE_HEADER::LINKED_LIST& aLL )
    {
        aDb.AddSentinelKey( aLL.m_Tail );
    };

    addTail( aHeader.m_LL_0x04 );
    addTail( aHeader.m_LL_0x06 );
    addTail( aHeader.m_LL_0x0C );
    addTail( aHeader.m_LL_Shapes );
    addTail( aHeader.m_LL_0x14 );
    addTail( aHeader.m_LL_0x1B_Nets );
    addTail( aHeader.m_LL_0x1C );
    addTail( aHeader.m_LL_0x24_0x28 );
    addTail( aHeader.m_LL_Unknown1 );
    addTail( aHeader.m_LL_0x2B );
    addTail( aHeader.m_LL_0x03_0x30 );
    addTail( aHeader.m_LL_0x0A );
    addTail( aHeader.m_LL_0x1D_0x1E_0x1F );
    addTail( aHeader.m_LL_Unknown2 );
    addTail( aHeader.m_LL_0x38 );
    addTail( aHeader.m_LL_0x2C );
    addTail( aHeader.m_LL_0x0C_2 );
    addTail( aHeader.m_LL_Unknown3 );
    addTail( aHeader.m_LL_0x36 );
    addTail( aHeader.GetUnknown5() );
    addTail( aHeader.m_LL_Unknown6 );
    addTail( aHeader.m_LL_0x0A_2 );

    if( aHeader.m_LL_V18_1.has_value() )
    {
        addTail( aHeader.m_LL_V18_1.value() );
        addTail( aHeader.m_LL_V18_2.value() );
        addTail( aHeader.m_LL_V18_3.value() );
        addTail( aHeader.m_LL_V18_4.value() );
        addTail( aHeader.m_LL_V18_5.value() );
        addTail( aHeader.m_LL_V18_6.value() );
    }
}


BRD_DB::BRD_DB() :
        m_FmtVer( FMT_VER::V_UNKNOWN )
{
}


bool BRD_DB::ResolveAndValidate()
{
    if( m_FmtVer >= FMT_VER::V_180 )
        collectSentinelKeys( *m_Header, *this );

    return true;
}
