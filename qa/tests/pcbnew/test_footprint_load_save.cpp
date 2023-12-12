/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <filesystem>

#include <board.h>
#include <kiid.h>
#include <footprint.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/settings_manager.h>

namespace
{

struct FOOTPRINT_LOAD_TEST_FIXTURE
{
    FOOTPRINT_LOAD_TEST_FIXTURE() {}
};


struct FOOTPRINT_LOAD_TEST_CASE
{
    // Which footprint this is
    KIID     m_footprintUuid;
    bool     m_expectedLocked;
    VECTOR2I m_expectedPos;
};


struct FOOTPRINT_LOAD_BOARD_TEST_CASE
{
    // The board to load
    wxString m_boardFileRelativePath;
    // List of images to check
    std::vector<FOOTPRINT_LOAD_TEST_CASE> m_imageCases;
};

} // namespace

BOOST_FIXTURE_TEST_CASE( FootprintLoadSave, FOOTPRINT_LOAD_TEST_FIXTURE )
{
    const std::vector<FOOTPRINT_LOAD_BOARD_TEST_CASE> testCases{
        {
                "footprints_load_save",
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

    for( const FOOTPRINT_LOAD_BOARD_TEST_CASE& testCase : testCases )
    {
        const auto doBoardTest = [&]( const BOARD& aBoard )
        {
            for( const FOOTPRINT_LOAD_TEST_CASE& fooprintTestCase : testCase.m_imageCases )
            {
                BOOST_TEST_MESSAGE( "Checking for footprint with UUID: "
                                    << fooprintTestCase.m_footprintUuid.AsString() );

                const auto& image = static_cast<FOOTPRINT&>( KI_TEST::RequireBoardItemWithTypeAndId(
                        aBoard, PCB_FOOTPRINT_T, fooprintTestCase.m_footprintUuid ) );

                BOOST_CHECK_EQUAL( image.IsLocked(), fooprintTestCase.m_expectedLocked );
                BOOST_CHECK_EQUAL( image.GetPosition(), fooprintTestCase.m_expectedPos * 1000000 );
            }
        };

        KI_TEST::LoadAndTestBoardFile( testCase.m_boardFileRelativePath, true, doBoardTest );
    }
}