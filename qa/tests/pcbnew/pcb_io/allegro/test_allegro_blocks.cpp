/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <convert/allegro_parser.h>

/**
 * @file test_allegro_blocks.cpp
 *
 * This file contains unit tests for parsing individual Allegro blocks, using the BLOCK_PARSER class.
 *
 * This allows to put regression/validation tests in place for block parsing, without having to
 * load an entire .brd file and parse all of its blocks. Allows to "lock in" known parsable blocks
 * at various file version.
 */

using namespace ALLEGRO;


/**
 * Description of a test board, used to provide context for parser tests, in particular
 * where the test data came from and which version of the Allegro format it uses.
 */
struct TEST_BRD_INFO
{
    wxString m_Name;
    wxString m_Url;
    wxString m_License;
    wxString m_BrdFileName;
    FMT_VER  m_FormatVersion;
};


enum class EXPECT_PARSE_RESULT
{
    SUCCESS,
    FAILURE,
};


/**
 * A single block of test data, along with the expected result of parsing it.
 */
struct BLK_TEST_INFO
{
    ///< The board that this block is from
    const TEST_BRD_INFO& m_BrdInfo;
    ///< The offset within the board file where this block is located (used for error messages)
    size_t m_BlockOffset;
    ///< The raw bytes of the block, as copied from the file
    std::vector<uint8_t> m_BlockData;
    ///< The expected result of parsing this block
    EXPECT_PARSE_RESULT m_ExpectParseResult;
    ///< An optional function to validate the contents of the parsed block if parsing is expected to succeed
    std::function<void( const BLOCK_BASE& )> m_ValidateFunc;

    friend std::ostream& operator<<( std::ostream& os, const BLK_TEST_INFO& aTestInfo )
    {
        const uint8_t blockType = aTestInfo.m_BlockData.empty() ? 0x00 : aTestInfo.m_BlockData[0];
        wxString      descr = wxString::Format( "Block type 0x%02x at offset 0x%010zx from board %s", blockType,
                                                aTestInfo.m_BlockOffset, aTestInfo.m_BrdInfo.m_BrdFileName );
        os << descr.ToStdString();
        return os;
    }
};


struct ALLEGRO_BLK_PARSE_FIXTURE
{
    ALLEGRO_BLK_PARSE_FIXTURE() {}

    void RunBlockTest( const BLK_TEST_INFO& aTestBlock )
    {
        const uint8_t blockType = aTestBlock.m_BlockData[0];

        BOOST_TEST_CONTEXT( wxString::Format( "Parsing block type %#02x at offset %#010zx from board %s", blockType,
                                              aTestBlock.m_BlockOffset, aTestBlock.m_BrdInfo.m_BrdFileName ) );

        FILE_STREAM fileStream( aTestBlock.m_BlockData.data(), aTestBlock.m_BlockData.size() );

        BLOCK_PARSER parser( fileStream, aTestBlock.m_BrdInfo.m_FormatVersion );

        bool                        endOfObjectsMarker = false;
        std::unique_ptr<BLOCK_BASE> block = parser.ParseBlock( endOfObjectsMarker );

        // Compare the result to the expected results
        if( aTestBlock.m_ExpectParseResult == EXPECT_PARSE_RESULT::SUCCESS )
        {
            BOOST_REQUIRE( block != nullptr );

            if( aTestBlock.m_ValidateFunc )
            {
                aTestBlock.m_ValidateFunc( *block );
            }
        }
        else
        {
            BOOST_TEST( block == nullptr );
        }
    }
};


static const TEST_BRD_INFO brd_olympus_15061_1b_info{
    "Project Olympus Motherboard",
    "https://www.opencompute.org/wiki/Server/ProjectOlympus",
    "OWFa 1.0",
    "15061-1b.brd",
    FMT_VER::V_165,
};


/**
 * This is an 0x20 block from the Olympus board
 *
 * It's the only one seen so far.
 */
const BLK_TEST_INFO blk_x20_olympus_0x0131553c{
    brd_olympus_15061_1b_info,
    0x0131553c,
    {
            0x20, 0x00, 0x07, 0x05, 0x98, 0xB4, 0x2A, 0x81, 0xF0, 0xF8, 0x4D, 0x82, 0x20, 0x8C, 0x92, 0x81,
            0xF4, 0x01, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, 0xC8, 0xAF, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0xC0, 0xB4, 0x2A, 0x81,
            0x80, 0xCF, 0xDE, 0x81, 0x58, 0xB0, 0xF7, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x36, 0x39, 0x00, 0x00, 0x79, 0x2D, 0x0F, 0x00, 0xAE, 0x25, 0x00, 0x00, 0x9D, 0x27, 0x0F, 0x00,
    },
    EXPECT_PARSE_RESULT::SUCCESS,
    []( const BLOCK_BASE& block )
    {
        const auto& blk = static_cast<const BLOCK<BLK_0x20_UNKNOWN>&>( block ).GetData();
        BOOST_TEST( blk.m_R == 0x0507 );
        BOOST_TEST( blk.m_Key == 0x812AB498 );
        BOOST_TEST( blk.m_Next == 0x824DF8F0 );
    }
};


BOOST_FIXTURE_TEST_SUITE( AllegroBlkParse, ALLEGRO_BLK_PARSE_FIXTURE )


BOOST_DATA_TEST_CASE( Blk_0x20,
                      boost::unit_test::data::make( {
                              blk_x20_olympus_0x0131553c,
                      } ),
                      aTestCase )
{
    RunBlockTest( aTestCase );
}

BOOST_AUTO_TEST_SUITE_END()
