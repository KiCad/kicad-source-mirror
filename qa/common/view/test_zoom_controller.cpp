/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <view/zoom_controller.h>


// All these tests are of a class in KIGFX
using namespace KIGFX;


/**
 * Declares a struct as the Boost test fixture.
 */
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

/*
 * Testing the accelerated version without making a very slow test is a little
 * tricky and would need a mock timestamping interface, which complicates the
 * real interface a bit and does not really seem worth the effort.
 */

BOOST_AUTO_TEST_SUITE_END()
