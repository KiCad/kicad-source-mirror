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

#include <board.h>
#include <board_design_settings.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_test_utils.h>


namespace
{
struct BDS_TEST_FIXTURE
{
    BDS_TEST_FIXTURE() :
            m_board( new BOARD() )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};
} // namespace


BOOST_FIXTURE_TEST_SUITE( BoardDesignSettings, BDS_TEST_FIXTURE )


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23327
 *
 * Negative silk clearance values must survive a save/load round-trip through
 * the project settings JSON. Previously the PARAM_SCALED lower bound was 0,
 * causing negative values to be reset to the default on load.
 */
BOOST_AUTO_TEST_CASE( NegativeSilkClearanceRoundTrip )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    int negativeValue = pcbIUScale.mmToIU( -0.1 );
    bds.m_SilkClearance = negativeValue;

    // Store all params into the JSON backing store
    bds.Store();

    // Reset to default
    bds.m_SilkClearance = 0;

    // Load back from JSON
    bds.Load();

    BOOST_CHECK_EQUAL( bds.m_SilkClearance, negativeValue );
}


BOOST_AUTO_TEST_SUITE_END()
