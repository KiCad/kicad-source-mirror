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
#include <pcb_group.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

namespace
{
struct GROUP_LOAD_TEST_CASE
{
    // Which one to look at in the file?
    KIID m_searchUuid;
    // Expected values
    bool     m_expectedLocked;
    wxString m_expectedName;
    unsigned m_expectedMemberCount;
};


struct GROUP_LOAD_BOARD_TEST_CASE: public KI_TEST::BOARD_LOAD_TEST_CASE
{
    // List of items to check on this board
    std::vector<GROUP_LOAD_TEST_CASE> m_groupCases;
};


const std::vector<GROUP_LOAD_BOARD_TEST_CASE> GroupsLoadSave_testCases{
    {
            "groups_load_save",
            20231231,
            {
                    // From top to bottom in the board file
                    {
                            "a78cc65c-451e-451e-9147-4460cc669685",
                            true,
                            "GroupName",
                            2,
                    },
            },
    },
    {
            // Before 20231231, 'id' was used in group s-exprs
            // and we need to continue to load it for compatibility
            "groups_load_save_v20231212",
            20231212,
            {
                    // From top to bottom in the board file
                    {
                            "a78cc65c-451e-451e-9147-4460cc669685",
                            true,
                            "GroupName",
                            2,
                    },
            },
    },
};

} // namespace

/**
 * @brief This is similar to group_saveload.cpp's HealthyGroups,
 * but runs the other way around: it loads a fixed board file and checks that the
 * groups are loaded correctly.
 *
 * This makes it a good place to check parsing of the board file format, as well
 * as stability of the format even if it's saved at a newer format version and re-read.
 *
 * But it's less good at programmatic generation of interesting groups in the first place.
 */
BOOST_DATA_TEST_CASE( GroupsLoadSave, boost::unit_test::data::make( GroupsLoadSave_testCases ),
                      testCase )
{
    const auto doBoardTest = [&]( const BOARD& aBoard )
    {
        for( const GROUP_LOAD_TEST_CASE& groupTestCase : testCase.m_groupCases )
        {
            BOOST_TEST_MESSAGE( "Checking for group with UUID: "
                                << groupTestCase.m_searchUuid.AsString()
                                << " and name: " << groupTestCase.m_expectedName );

            const auto& group = static_cast<PCB_GROUP&>( KI_TEST::RequireBoardItemWithTypeAndId(
                    aBoard, PCB_GROUP_T, groupTestCase.m_searchUuid ) );

            BOOST_CHECK_EQUAL( group.IsLocked(), groupTestCase.m_expectedLocked );
            BOOST_CHECK_EQUAL( group.GetName(), groupTestCase.m_expectedName );
            BOOST_CHECK_EQUAL( group.GetItems().size(), groupTestCase.m_expectedMemberCount );
        }
    };

    KI_TEST::LoadAndTestBoardFile( testCase.m_BoardFileRelativePath, true, doBoardTest,
                                   testCase.m_ExpectedBoardVersion );
}