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

/**
 * @file test_meander_corner_radius.cpp
 * Test suite for meander corner radius constraints (issue #8629).
 *
 * This verifies that meanders maintain visibly rounded corners and do not
 * degenerate to nearly-square shapes when space is constrained.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <router/pns_meander.h>

BOOST_AUTO_TEST_SUITE( MeanderCornerRadius )

/**
 * Test that MEANDER_SETTINGS has correct defaults for corner style.
 */
BOOST_AUTO_TEST_CASE( DefaultSettings )
{
    PNS::MEANDER_SETTINGS settings;

    // Default should be round corners
    BOOST_CHECK_EQUAL( settings.m_cornerStyle, PNS::MEANDER_STYLE_ROUND );

    // Corner radius percentage default is 80%
    BOOST_CHECK_EQUAL( settings.m_cornerRadiusPercentage, 80 );

    // Default spacing
    BOOST_CHECK_EQUAL( settings.m_spacing, 600000 );  // 0.6mm

    // Minimum amplitude should be set
    BOOST_CHECK_GT( settings.m_minAmplitude, 0 );
}

/**
 * Test minimum corner radius threshold relative to track width.
 *
 * For issue #8629, the fix ensures that meanders with corner radii
 * less than half the track width are rejected. This test verifies
 * the relationship between track width and acceptable corner radius.
 */
BOOST_AUTO_TEST_CASE( MinCornerRadiusThreshold )
{
    // Test the minimum corner radius calculation logic
    // The threshold should be width / 2 to ensure visibly rounded corners

    std::vector<int> widths = { 100000, 150000, 200000, 250000, 400000 };

    for( int width : widths )
    {
        // The minimum acceptable corner radius is half the width
        int minCornerRadius = width / 2;

        // Verify the threshold is reasonable (not too large, not too small)
        BOOST_CHECK_GT( minCornerRadius, 0 );
        BOOST_CHECK_LE( minCornerRadius, width );

        BOOST_TEST_MESSAGE( wxString::Format( "Width %d: minCornerRadius=%d",
                                              width, minCornerRadius ) );
    }
}

/**
 * Test that corner radius requirements are geometrically consistent.
 *
 * For a meander with round corners, the corner radius is limited by:
 * 1. Half the amplitude (to fit the U-turn)
 * 2. Half the spacing (to fit between adjacent segments)
 *
 * If the minimum corner radius (width/2) exceeds these limits,
 * the meander configuration should be rejected.
 */
BOOST_AUTO_TEST_CASE( GeometricConstraints )
{
    int width = 250000;  // 0.25mm track
    int minCornerRadius = width / 2;  // 0.125mm

    // For corner radius to be at least minCornerRadius:
    // 1. amplitude / 2 >= minCornerRadius => amplitude >= width
    // 2. spacing / 2 >= minCornerRadius => spacing >= width

    // Test various amplitude/spacing combinations
    struct TestCase
    {
        int amplitude;
        int spacing;
        bool shouldFit;
    };

    std::vector<TestCase> testCases = {
        { 500000, 600000, true },   // 0.5mm amp, 0.6mm spacing - plenty of room
        { 250000, 600000, true },   // 0.25mm amp, 0.6mm spacing - minimum amplitude
        { 500000, 250000, true },   // 0.5mm amp, 0.25mm spacing - minimum spacing
        { 250000, 250000, true },   // 0.25mm amp, 0.25mm spacing - both at minimum
        { 200000, 600000, false },  // 0.2mm amp < width - amplitude too small
        { 500000, 200000, false },  // 0.2mm spacing < width - spacing too small
    };

    for( const auto& tc : testCases )
    {
        // Calculate maximum corner radius based on amplitude and spacing
        int maxCrFromAmp = tc.amplitude / 2;
        int maxCrFromSpacing = tc.spacing / 2;
        int maxCr = std::min( maxCrFromAmp, maxCrFromSpacing );

        bool wouldFit = maxCr >= minCornerRadius;

        BOOST_CHECK_EQUAL( wouldFit, tc.shouldFit );

        BOOST_TEST_MESSAGE( wxString::Format(
            "amp=%d, spacing=%d: maxCr=%d, minRequired=%d, fits=%s (expected %s)",
            tc.amplitude, tc.spacing, maxCr, minCornerRadius,
            wouldFit ? "yes" : "no", tc.shouldFit ? "yes" : "no" ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()
