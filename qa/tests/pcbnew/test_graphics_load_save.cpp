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
#include <pcb_shape.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

namespace
{

struct GRAPHICS_LOAD_TEST_CASE
{
    // Which one to look at in the file?
    KIID m_searchUuid;
    // Expected values
    bool m_expectedFilled;
};


struct GRAPHICS_LOAD_BOARD_TEST_CASE: public KI_TEST::BOARD_LOAD_TEST_CASE
{
    // List of images to check
    std::vector<GRAPHICS_LOAD_TEST_CASE> m_generatorCases;
};

const std::vector<GRAPHICS_LOAD_BOARD_TEST_CASE> GraphicsLoad_testCases{
    {
            // Before 20241129, 'fill' would be solid or none
            // in PCB shapes
            "graphics_load_save_v20240108",
            20240108,
            {
                    // Filled poly
                    {
                            "65596b4f-7b03-48e9-8be7-5824316ea7fd",
                            true,
                    },
                    // Unfilled poly
                    {
                            "cf265305-49c9-43d8-bb2a-ad34997b22d6",
                            false,
                    },
                    // Filled rect
                    {
                            "d0669ae2-442f-427f-af0f-bc3008af779a",
                            true,
                    },
                    // Unfilled rect
                    {
                            "fd1649a3-9a92-4dd3-96b4-88469cb257ba",
                            false,
                    },
            },
    },
};

} // namespace

BOOST_DATA_TEST_CASE( GraphicsLoad, boost::unit_test::data::make( GraphicsLoad_testCases ),
                      testCase )
{
    const auto doBoardTest =
            [&]( const BOARD& aBoard )
            {
                for( const GRAPHICS_LOAD_TEST_CASE& testCase : testCase.m_generatorCases )
                {
                    BOOST_TEST_MESSAGE( "Checking for graphic with UUID: "
                                            << testCase.m_searchUuid.AsString() );

                    const auto& graphic =
                            static_cast<PCB_SHAPE&>( KI_TEST::RequireBoardItemWithTypeAndId(
                                    aBoard, PCB_SHAPE_T, testCase.m_searchUuid ) );

                    BOOST_CHECK_EQUAL( graphic.IsSolidFill(), testCase.m_expectedFilled );
                }
            };

    KI_TEST::LoadAndTestBoardFile( testCase.m_BoardFileRelativePath, true, doBoardTest,
                                   testCase.m_ExpectedBoardVersion );
}