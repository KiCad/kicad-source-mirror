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

#include <board.h>
#include <kiid.h>
#include <footprint.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>


namespace
{

struct FOOTPRINT_LOAD_TEST_CASE
{
    // Which footprint to look at in the file
    KIID     m_footprintUuid;
    // Expected values
    bool     m_expectedLocked;
    VECTOR2I m_expectedPos;
};


struct FOOTPRINT_LOAD_BOARD_TEST_CASE: public KI_TEST::BOARD_LOAD_TEST_CASE
{
    // List of images to check
    std::vector<FOOTPRINT_LOAD_TEST_CASE> m_fpCases;
};


const std::vector<FOOTPRINT_LOAD_BOARD_TEST_CASE> FootprintLoadSave_testCases{
    {
            "footprints_load_save",
            std::nullopt,
            {
                    // From top to bottom in the board file
                    {
                            "898cf321-03c7-40bb-8d78-4bc5e52986c2",
                            false,
                            { 100, 80 },
                    },
                    {
                            "0775cd70-2e84-4592-a160-456c37a8f4f6",
                            true,
                            { 100, 100 },
                    },
            },
    },
};

} // namespace

/**
 * Simple tests cases that load a board file and check expected properties of the footprints.
 * This is not the same as FpLibLoadSave as it loads _board files_, and not footprint
 * files from a library.
 */
BOOST_DATA_TEST_CASE( FootprintLoadSave,
                      boost::unit_test::data::make( FootprintLoadSave_testCases ), testCase )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        for( const FOOTPRINT_LOAD_TEST_CASE& fooprintTestCase : testCase.m_fpCases )
        {
            BOOST_TEST_MESSAGE( "Checking for footprint with UUID: "
                                << fooprintTestCase.m_footprintUuid.AsString() );

            const auto& fp = static_cast<FOOTPRINT&>( KI_TEST::RequireBoardItemWithTypeAndId(
                    aBoard, PCB_FOOTPRINT_T, fooprintTestCase.m_footprintUuid ) );

            BOOST_CHECK_EQUAL( fp.IsLocked(), fooprintTestCase.m_expectedLocked );
            BOOST_CHECK_EQUAL( fp.GetPosition(), fooprintTestCase.m_expectedPos * 1000000 );
        }
    };

    KI_TEST::LoadAndTestBoardFile( testCase.m_BoardFileRelativePath, true, doBoardTest,
                                   testCase.m_ExpectedBoardVersion );
}