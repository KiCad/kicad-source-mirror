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
#include <pcb_generator.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

namespace
{
struct GENERATOR_LOAD_TEST_CASE
{
    // Which one to look at in the file?
    KIID m_searchUuid;
    // Expected values
    bool     m_expectedLocked;
    wxString m_expectedName;
    unsigned m_expectedMemberCount;
};


struct GENERATOR_LOAD_BOARD_TEST_CASE: public KI_TEST::BOARD_LOAD_TEST_CASE
{
    // List of images to check
    std::vector<GENERATOR_LOAD_TEST_CASE> m_generatorCases;
};

const std::vector<GENERATOR_LOAD_BOARD_TEST_CASE> GeneratorLoading_testCases{
    {
            "tuning_generators_load_save",
            20231231,
            {
                    // From top to bottom in the board file
                    {
                            "4f22a815-3048-42b3-86fa-eb71720d35ae",
                            false,
                            "Tuning Pattern",
                            47,
                    },
            },
    },
    {
            // Before 20231231, 'id' was used in generator s-exprs
            // and we need to continue to load it for compatibility
            "tuning_generators_load_save_v20231212",
            20231212,
            {
                    // From top to bottom in the board file
                    {
                            "4f22a815-3048-42b3-86fa-eb71720d35ae",
                            false,
                            "Tuning Pattern",
                            47,
                    },
            },
    },
};

} // namespace

BOOST_DATA_TEST_CASE( GeneratorLoading, boost::unit_test::data::make( GeneratorLoading_testCases ),
                      testCase )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        for( const GENERATOR_LOAD_TEST_CASE& testCase : testCase.m_generatorCases )
        {
            BOOST_TEST_MESSAGE(
                    "Checking for generator with UUID: " << testCase.m_searchUuid.AsString() );

            const auto& generator =
                    static_cast<PCB_GENERATOR&>( KI_TEST::RequireBoardItemWithTypeAndId(
                            aBoard, PCB_GENERATOR_T, testCase.m_searchUuid ) );

            BOOST_CHECK_EQUAL( generator.IsLocked(), testCase.m_expectedLocked );
            BOOST_CHECK_EQUAL( generator.GetName(), testCase.m_expectedName );
            BOOST_CHECK_EQUAL( generator.GetItems().size(), testCase.m_expectedMemberCount );
        }
    };

    KI_TEST::LoadAndTestBoardFile( testCase.m_BoardFileRelativePath, true, doBoardTest,
                                    testCase.m_ExpectedBoardVersion );
}