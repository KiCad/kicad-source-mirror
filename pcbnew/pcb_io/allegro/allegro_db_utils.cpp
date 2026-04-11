/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "allegro_db_utils.h"


using namespace ALLEGRO;


uint32_t ALLEGRO::GetPrimaryNext( const BLOCK_BASE& aBlock )
{
    return aBlock.GetNext();
}


std::optional<FIELD_VALUE> ALLEGRO::GetFirstFieldOfType( const BRD_DB& aDb, uint32_t aFieldsPtr, uint32_t aEndKey,
                                                         uint16_t aFieldCode )
{
    LL_WALKER fieldWalker{ aFieldsPtr, aEndKey, aDb };

    for( const BLOCK_BASE* block : fieldWalker )
    {
        if( block->GetBlockType() != 0x03 )
            continue;

        const auto& field = BlockDataAs<BLK_0x03_FIELD>( *block );

        if( field.m_Hdr1 != aFieldCode )
            continue;

        switch( field.m_SubType )
        {
        case 0x68:
        {
            const std::string* str = std::get_if<std::string>( &field.m_Substruct );

            if( str )
                return wxString( *str );

            break;
        }
        case 0x66:
        {
            const uint32_t* val = std::get_if<uint32_t>( &field.m_Substruct );

            if( val )
                return *val;

            break;
        }
        }
    }

    return std::nullopt;
}


std::optional<int> ALLEGRO::GetFirstFieldOfTypeInt( const BRD_DB& aDb, uint32_t aFieldsPtr, uint32_t aEndKey,
                                                    uint16_t aFieldCode )
{
    std::optional<FIELD_VALUE> result = GetFirstFieldOfType( aDb, aFieldsPtr, aEndKey, aFieldCode );

    if( !result.has_value() )
        return std::nullopt;

    if( uint32_t* val = std::get_if<uint32_t>( &result.value() ) )
        return static_cast<int>( *val );

    return std::nullopt;
}
