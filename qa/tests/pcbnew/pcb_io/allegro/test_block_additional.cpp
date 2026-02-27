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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include "allegro_block_tests.h"

#include <hash.h>


/**
 * @file test_block_snippets.cpp
 *
 * This file contains additional tests for blocks (avoids having to have
 * a really complex JSON structure with keys and/or reflection for tons of block
 * types.
 *
 * For many block types, just making sure the parser consumes the right amount of data
 * if good enough.
 *
 * If any of these get really common, they can be built into the JSON test definitions.
 */


using namespace ALLEGRO;
using namespace KI_TEST;


/**
 * Unique key for identifying a block test, used for looking up additional validation functions.
 */
struct BLOCK_TEST_KEY
{
    std::string m_BoardName;
    size_t      m_BlockOffset;

    bool operator==( const BLOCK_TEST_KEY& other ) const
    {
        return m_BoardName == other.m_BoardName &&
               m_BlockOffset == other.m_BlockOffset;
    }
};


namespace std
{
template <>
struct hash<BLOCK_TEST_KEY>
{
    size_t operator()( const BLOCK_TEST_KEY& key ) const
    {
        return hash_val( key.m_BoardName, key.m_BlockOffset );
    }
};
} // namespace std


using BLOCK_TEST_FUNC = std::function<void( const BLOCK_BASE& )>;
using DB_OBJ_TEST_FUNC = std::function<void( const DB_OBJ& )>;


static void TestOlympus0x20( const BLOCK_BASE& aBlock )
{
    BOOST_REQUIRE( aBlock.GetBlockType() == 0x20 );

    const auto& blk = static_cast<const BLOCK<BLK_0x20_UNKNOWN>&>( aBlock ).GetData();
    BOOST_TEST( blk.m_R == 0x0507 );
    BOOST_TEST( blk.m_Key == 0x812AB498 );
    BOOST_TEST( blk.m_Next == 0x824DF8F0 );
}


/**
 * The registry of additional block tests, keyed by board name and block offset.
 *
 * The test function takes the parsed block as an argument, and can perform any additional
 * validation on the block data that isn't easily expressed in the JSON test definitions.
 *
 * You can register any callable here, so it could be a function or function object.
 */
// clang-format off
static const std::unordered_map<BLOCK_TEST_KEY, BLOCK_TEST_FUNC> additionalBlockTests{
    { { "Olympus_15061-1b_v165",      0x0131553c }, TestOlympus0x20 },
};


static const std::unordered_map<BLOCK_TEST_KEY, DB_OBJ_TEST_FUNC> additionalDbObjTests{
    { { "Olympus_15061-1b_v165",      0x0131553c }, []( const DB_OBJ& obj )
        {
            BOOST_REQUIRE( obj.GetType() == BRD_x20 );

            const auto& blk = static_cast<const UNKNOWN_0x20&>( obj );
            BOOST_TEST( blk.m_Next.m_TargetKey == 0x824DF8F0 );
        }
    },
};

// clang-format on


void KI_TEST::RunAdditionalBlockTest( const std::string& aBoardName, size_t aBlockOffset, const BLOCK_BASE& block )
{
    const BLOCK_TEST_KEY key{ aBoardName, aBlockOffset };

    auto it = additionalBlockTests.find( key );
    if( it != additionalBlockTests.end() )
    {
        const auto& testFunc = it->second;
        BOOST_REQUIRE( testFunc );

        testFunc( block );
    }
    else
    {
        BOOST_TEST_MESSAGE( "No additional test defined for this block" );
    }
}


void KI_TEST::RunAdditionalObjectTest( const std::string& aBoardName, size_t aBlockOffset, const DB_OBJ& obj )
{
    const BLOCK_TEST_KEY key{ aBoardName, aBlockOffset };

    auto it = additionalDbObjTests.find( key );
    if( it != additionalDbObjTests.end() )
    {
        const auto& testFunc = it->second;
        BOOST_REQUIRE( testFunc );

        testFunc( obj );
    }
    else
    {
        BOOST_TEST_MESSAGE( "No additional test defined for this DB_OBJ" );
    }
}
