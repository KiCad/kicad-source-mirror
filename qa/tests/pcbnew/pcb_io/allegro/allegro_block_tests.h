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

#pragma once

#include <cstdint>
#include <functional>
#include <iomanip>
#include <vector>

#include <convert/allegro_db.h>


namespace KI_TEST
{
/**
 * A header block, along with the expected result of parsing it.
 */
struct HEADER_TEST_INFO
{
    /// Source of the header data to be loaded and tested.
    std::string m_HeaderDataSource;
    /// Whether to skip this test while parsers don't support a certain format
    bool m_Skip;

    friend std::ostream& operator<<( std::ostream& os, const HEADER_TEST_INFO& aTestInfo )
    {
        wxString descr = wxString::Format( "Header: %s", aTestInfo.m_HeaderDataSource );
        os << descr.ToStdString();
        return os;
    }
};

/**
 * A single block of test data, along with the expected result of parsing it.
 */
struct BLK_TEST_INFO
{
    /// The type of the block, as in the first byte
    uint8_t m_BlockType;
    /// The offset within the board file where this block is located (used for error messages)
    size_t m_BlockOffset;
    /// Whether to skip this test while parsers don't support a certain format
    bool m_Skip;
    /// The raw bytes of the block, as copied from the file
    std::string m_DataSource;
    /// An optional function to validate the contents of the parsed block if parsing is expected to succeed
    std::function<void( const ALLEGRO::BLOCK_BASE& )> m_ValidateFunc;

    friend std::ostream& operator<<( std::ostream& os, const BLK_TEST_INFO& aTestInfo )
    {
        wxString msg = wxString::Format( "Block type %#02x at offset %#010zx", aTestInfo.m_BlockType,
                                         aTestInfo.m_BlockOffset );
        os << msg.ToStdString();
        return os;
    }
};


struct BOARD_TEST_DEF
{
    /// The name of the board being tested, used for error messages and test context
    std::string m_BrdName;
    /// The version of the Allegro format that this board is in
    ALLEGRO::FMT_VER m_FormatVersion;
    // If there is a header test for this board, it will be stored here, else nullptr
    std::unique_ptr<HEADER_TEST_INFO> m_HeaderTest;
    // List of block tests for this board
    std::vector<BLK_TEST_INFO> m_BlockTests;
};


/**
 * Look up and run any additional ad-hoc tests for a block.
 *
 * Note that these tests don't have context of the wider board, so they are necessarily
 * limited to quite "static" checks of the block content.
 */
extern void RunAdditionalBlockTest( const std::string& aBoardName, size_t aBlockOffset,
                                    const ALLEGRO::BLOCK_BASE& aBlock );

/**
 * Look up and run any additional ad-hoc tests for a DB_OBJ (parsed and converted block)
 *
 * Since most "useful" functionality probably should come from the resolved and
 * invariant-assured DB_OBJ, these tests are probably more useful than binary-level
 * block tests.
 */
extern void RunAdditionalObjectTest( const std::string& aBoardName, size_t aBlockOffset,
                                     const ALLEGRO::DB_OBJ& aDbObj );


} // namespace KI_TEST
