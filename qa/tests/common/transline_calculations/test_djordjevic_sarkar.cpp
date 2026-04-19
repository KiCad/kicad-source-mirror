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

#include <cmath>
#include <stdexcept>

#include <transline_calculations/dielectric_djordjevic_sarkar.h>


BOOST_AUTO_TEST_SUITE( DjordjevicSarkar )


// Oracle values come from the scikit-rf djordjevicsvensson reference evaluated
// with (epsR=4.4, tanD=0.02, f_spec=1 GHz, f1=1 kHz, f2=1 THz).
BOOST_AUTO_TEST_CASE( FR4Fit )
{
    DIELECTRIC_DJORDJEVIC_SARKAR model;
    model.Fit( 4.4, 0.02, 1.0e9 );

    BOOST_TEST( !model.IsLossless() );
    BOOST_TEST( model.GetEpsilonInf() == 4.01276, boost::test_tools::tolerance( 1.0e-4 ) );
    BOOST_TEST( model.GetM() == 0.05606, boost::test_tools::tolerance( 1.0e-4 ) );
}


BOOST_AUTO_TEST_CASE( FR4At1MHz )
{
    DIELECTRIC_DJORDJEVIC_SARKAR model;
    model.Fit( 4.4, 0.02, 1.0e9 );

    BOOST_TEST( model.EpsilonRealAt( 1.0e6 ) == 4.7872, boost::test_tools::tolerance( 0.01 ) );
    BOOST_TEST( model.TanDeltaAt( 1.0e6 ) == 0.01838, boost::test_tools::tolerance( 0.0005 ) );
}


BOOST_AUTO_TEST_CASE( FR4At10GHz )
{
    DIELECTRIC_DJORDJEVIC_SARKAR model;
    model.Fit( 4.4, 0.02, 1.0e9 );

    BOOST_TEST( model.EpsilonRealAt( 1.0e10 ) == 4.2709, boost::test_tools::tolerance( 0.01 ) );
    BOOST_TEST( model.TanDeltaAt( 1.0e10 ) == 0.02049, boost::test_tools::tolerance( 0.0005 ) );
}


// Lossless fit must produce a frequency-flat permittivity and exact-zero tan delta
// at any probing frequency, including the numerical extremes.
BOOST_AUTO_TEST_CASE( LosslessInput )
{
    DIELECTRIC_DJORDJEVIC_SARKAR model;
    model.Fit( 4.4, 0.0, 1.0e9 );

    BOOST_TEST( model.IsLossless() );
    BOOST_TEST( model.EpsilonRealAt( 1.0e-6 ) == 4.4 );
    BOOST_TEST( model.EpsilonRealAt( 1.0e15 ) == 4.4 );
    BOOST_TEST( model.TanDeltaAt( 1.0e-6 ) == 0.0 );
    BOOST_TEST( model.TanDeltaAt( 1.0e15 ) == 0.0 );
    BOOST_TEST( model.TanDeltaAt( 0.0 ) == 0.0 );
}


BOOST_AUTO_TEST_CASE( InvalidBandwidth )
{
    DIELECTRIC_DJORDJEVIC_SARKAR model;

    BOOST_CHECK_THROW( model.Fit( 4.4, 0.02, 1.0e9, 0.0, 1.0e12 ), std::invalid_argument );
    BOOST_CHECK_THROW( model.Fit( 4.4, 0.02, 1.0e9, 1.0e12, 1.0e10 ), std::invalid_argument );
    BOOST_CHECK_THROW( model.Fit( 4.4, 0.02, 1.0e20, 1.0e3, 1.0e12 ), std::invalid_argument );
}


// At DC the argument (f2+0j)/(f1+0j) is purely real, so ln is real and the
// complex permittivity collapses to the static limit with zero loss tangent.
BOOST_AUTO_TEST_CASE( DCLimit )
{
    DIELECTRIC_DJORDJEVIC_SARKAR model;
    model.Fit( 4.4, 0.02, 1.0e9 );

    const double epsAtDC  = model.EpsilonRealAt( 0.0 );
    const double expected = model.GetEpsilonInf() + model.GetM() * std::log( 1.0e12 / 1.0e3 );

    BOOST_TEST( std::isfinite( epsAtDC ) );
    BOOST_TEST( epsAtDC == expected, boost::test_tools::tolerance( 1.0e-9 ) );
    BOOST_TEST( model.TanDeltaAt( 0.0 ) == 0.0 );
}


BOOST_AUTO_TEST_SUITE_END()
