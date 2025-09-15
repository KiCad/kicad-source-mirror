/*
 * Copyright (C) 2025 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2007 Ecma International (original Java source)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * License info found here:
 * https://www.loc.gov/preservation/digital/formats/fdd/fdd000491.shtml
 */

#include "data_block.h"

using namespace U3D;


DATA_BLOCK::DATA_BLOCK()
{
    m_dataSize = 0;
    m_metaDataSize = 0;
    m_blockType = 0;
    m_priority = 0;
}


void DATA_BLOCK::SetDataSize( uint32_t aSize )
{
    m_dataSize = aSize;
    if( ( m_dataSize & 0x3 ) == 0 )
    {
        m_data.resize( aSize >> 2 );
    }
    else
    {
        m_data.resize( ( aSize >> 2 ) + 1 );
    }
}


void DATA_BLOCK::SetMetaDataSize( uint32_t aSize )
{
    m_metaDataSize = aSize;
    if( ( m_metaDataSize & 0x3 ) == 0 )
    {
        m_metaData.resize( aSize >> 2 );
    }
    else
    {
        m_metaData.resize( ( aSize >> 2 ) + 1 );
    }
}


void DATA_BLOCK::SetMetaData( const std::vector<uint32_t>& aValue )
{
    if( aValue.size() == m_metaData.size() )
    {
        std::copy( aValue.begin(), aValue.end(), m_metaData.begin() );
    }
}