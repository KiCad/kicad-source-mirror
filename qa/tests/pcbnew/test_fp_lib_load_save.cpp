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

struct FPLIB_LOAD_FP_TEST_CASE
{
    // The FP to load
    wxString m_libraryPath;
    wxString m_fpName;

    std::optional<unsigned> m_expectedFootprintVersion;

    // If set, the expected number of pads in the footprint
    std::optional<unsigned> m_expectedPadCount;

    // For printing the test name
    friend std::ostream& operator<<( std::ostream& os, const FPLIB_LOAD_FP_TEST_CASE& aTestCase )
    {
        os << aTestCase.m_fpName;
        return os;
    }
};


const std::vector<FPLIB_LOAD_FP_TEST_CASE> FpLibLoadSave_testCases{
    {
            "plugins/kicad_sexpr/fp.pretty",
            "R_0201_0603Metric",
            20240108U,
            // 2 SMD pads, 2 paste pads
            4U,
    },
};

} // namespace

/**
 * Simple tests cases that run though the given FPs and checks some simple properties.
 *
 * This is not the same as FootprintLoadSave, as that tests the loading and saving of
 * _board files_ that contain footprints. This tests loading and saving of _footprint
 * files_, and includes at least some of the library IO code.
 */
BOOST_DATA_TEST_CASE( FpLibLoadSave, boost::unit_test::data::make( FpLibLoadSave_testCases ),
                      testCase )
{
    const auto doFootprintTest = [&]( const FOOTPRINT& aBoard )
    {
        if( testCase.m_expectedPadCount )
        {
            BOOST_CHECK_EQUAL( aBoard.Pads().size(), *testCase.m_expectedPadCount );
        }
    };

    KI_TEST::LoadAndTestFootprintFile( testCase.m_libraryPath, testCase.m_fpName, true,
                                        doFootprintTest, testCase.m_expectedFootprintVersion );
}