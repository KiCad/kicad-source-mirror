/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <view/zoom_controller.h>


// All these tests are of a class in KIGFX
using namespace KIGFX;


BOOST_AUTO_TEST_SUITE( ZoomController )


struct CONST_ZOOM_CASE
{
    double scale_factor;
    int    scroll_amount;
    double exp_zoom_in;
    double exp_zoom_out;
};


/*
 * Some "sane" examples for steps, scale factors and results.
 * These should be actual examples that could be encountered
 *
 * TODO: Add more cases for, eg, Mac
 */
static const std::vector<CONST_ZOOM_CASE> const_zoom_cases = {
    // A single scroll step on a GTK3 Linux system
    // 120 is the standard wheel delta, so it's what you might expect
    // from a single scroll wheel detent.
    { CONSTANT_ZOOM_CONTROLLER::GTK3_SCALE, 120, 1.1, 1 / 1.1 },
};

/**
 * Check basic setting and getting of values
 */
BOOST_AUTO_TEST_CASE( ConstController )
{
    // How close we need to be (not very, this is a subjective thing anyway)
    const double tol_percent = 10;

    double scale_for_step;

    for( const auto& c : const_zoom_cases )
    {
        CONSTANT_ZOOM_CONTROLLER zoom_ctrl( c.scale_factor );

        scale_for_step = zoom_ctrl.GetScaleForRotation( c.scroll_amount );
        BOOST_CHECK_CLOSE( scale_for_step, c.exp_zoom_in, tol_percent );

        scale_for_step = zoom_ctrl.GetScaleForRotation( -c.scroll_amount );
        BOOST_CHECK_CLOSE( scale_for_step, c.exp_zoom_out, tol_percent );
    }
}

/**
 * Timestamper that returns predefined values from a vector
 */
class PREDEF_TIMESTAMPER : public ACCELERATING_ZOOM_CONTROLLER::TIMESTAMP_PROVIDER
{
public:
    using STAMP_LIST = std::vector<int>;

    PREDEF_TIMESTAMPER( const STAMP_LIST& aStamps )
            : m_stamps( aStamps ), m_iter( m_stamps.begin() )
    {
    }

    /**
     * @return the next time point in the predefined sequence
     */
    ACCELERATING_ZOOM_CONTROLLER::TIME_PT GetTimestamp() override
    {
        // Don't ask for more samples than given
        BOOST_REQUIRE( m_iter != m_stamps.end() );

        return ACCELERATING_ZOOM_CONTROLLER::TIME_PT( std::chrono::milliseconds( *m_iter++ ) );
    }

    const STAMP_LIST           m_stamps;
    STAMP_LIST::const_iterator m_iter;
};


struct ACCEL_ZOOM_CASE
{
    int                 timeout;
    std::vector<int>    stamps; // NB includes the initial stamp!
    std::vector<int>    scrolls;
    std::vector<double> zooms;
};

static const std::vector<ACCEL_ZOOM_CASE> accel_cases = {
    // Scrolls widely spaced, just go up and down by a constant factor
    { 500, { 0, 1000, 2000, 3000, 4000 }, { 120, 120, -120, -120 }, { 1.05, 1.05, 1 / 1.05, 1 / 1.05 } },
    // Close scrolls - acceleration, apart from when changing direction
    { 500, { 0, 1000, 1100, 1200, 1300, 1400 }, { 120, 120, -120, -120, 120 }, { 1.05, 2.05, 1 / 1.05, 1 / 2.05, 1.05 } },
};


/**
 * Check basic setting and getting of values
 */
BOOST_AUTO_TEST_CASE( AccelController )
{
    const double tol_percent = 10.0;

    for( const auto& c : accel_cases )
    {
        PREDEF_TIMESTAMPER timestamper( c.stamps );

        ACCELERATING_ZOOM_CONTROLLER zoom_ctrl(
                ACCELERATING_ZOOM_CONTROLLER::DEFAULT_ACCELERATION_SCALE,
                std::chrono::milliseconds( c.timeout ), &timestamper );

        for( unsigned i = 0; i < c.scrolls.size(); i++ )
        {
            const auto zoom_scale = zoom_ctrl.GetScaleForRotation( c.scrolls[i] );

            BOOST_CHECK_CLOSE( zoom_scale, c.zooms[i], tol_percent );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
