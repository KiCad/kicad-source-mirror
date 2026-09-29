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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cmath>
#include <iomanip>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/eda_angle.h>


BOOST_AUTO_TEST_SUITE( EdaAngle )


struct EDA_ANGLE_NORMALISE_CASE
{
    double m_Angle;

    // Expected:
    double m_ExpNormalized;
    double m_ExpNormalizedNegative;
    double m_ExpNormalized90;
    double m_ExpNormalized180;
    double m_ExpNormalized720;
};


static const std::vector<EDA_ANGLE_NORMALISE_CASE> normalize_cases =
{
    //@todo: should we unify the ranges of Normalize180, Normalize720 to be the same
    // as Normalize90 (i.e. inclusive of both sides of the range)?

    //                 [0,360)   (-360,0]    [-90,90]   (-180,180]   [-360,360)
    // Original     Normalized    NormNeg      Norm90      Norm180      Norm720
    {       90.0,        90.0,    -270.0,       90.0,        90.0,        90.0 },
    {      -90.0,       270.0,     -90.0,      -90.0,       -90.0,       -90.0 },
    {      135.0,       135.0,    -225.0,      -45.0,       135.0,       135.0 },
    {     -135.0,       225.0,    -135.0,       45.0,      -135.0,      -135.0 },
    {      180.0,       180.0,    -180.0,        0.0,       180.0,       180.0 },
    {     -180.0,       180.0,    -180.0,        0.0,       180.0,      -180.0 },
    {      360.0,         0.0,       0.0,        0.0,         0.0,         0.0 },
    {     -360.0,         0.0,       0.0,        0.0,         0.0,      -360.0 },
    {      390.0,        30.0,    -330.0,       30.0,        30.0,        30.0 },
    {     -390.0,       330.0,     -30.0,      -30.0,       -30.0,       -30.0 },
    {      720.0,         0.0,       0.0,        0.0,         0.0,         0.0 },
    {     -720.0,         0.0,       0.0,        0.0,         0.0,      -360.0 },
};


BOOST_AUTO_TEST_CASE( Normalize )
{
    for( const auto& c : normalize_cases )
    {
        BOOST_TEST_INFO_SCOPE( "Original angle: " << c.m_Angle << " degrees" );

        EDA_ANGLE normalized( c.m_Angle, DEGREES_T );
        normalized.Normalize();

        EDA_ANGLE normalizedNegative( c.m_Angle, DEGREES_T );
        normalizedNegative.NormalizeNegative();

        EDA_ANGLE normalized90( c.m_Angle, DEGREES_T );
        normalized90.Normalize90();

        EDA_ANGLE normalized180( c.m_Angle, DEGREES_T );
        normalized180.Normalize180();

        EDA_ANGLE normalized720( c.m_Angle, DEGREES_T );
        normalized720.Normalize720();

        BOOST_CHECK_EQUAL( normalized.AsDegrees(), c.m_ExpNormalized );
        BOOST_CHECK_EQUAL( normalizedNegative.AsDegrees(), c.m_ExpNormalizedNegative );
        BOOST_CHECK_EQUAL( normalized90.AsDegrees(), c.m_ExpNormalized90 );
        BOOST_CHECK_EQUAL( normalized180.AsDegrees(), c.m_ExpNormalized180 );
        BOOST_CHECK_EQUAL( normalized720.AsDegrees(), c.m_ExpNormalized720 );
    }
}


BOOST_AUTO_TEST_CASE( NormalizeHugeAndNonFinite )
{
    // The old loops leave NaN alone and never finish on inf or 1e19, so fail here before reaching those
    BOOST_REQUIRE_EQUAL( EDA_ANGLE( NAN, DEGREES_T ).Normalize180().AsDegrees(), 0.0 );

    // 1e19 is exact and 1e19 mod 360 = 280
    //                 [0,360)   (-360,0]    [-90,90]   (-180,180]   [-360,360)
    const std::vector<EDA_ANGLE_NORMALISE_CASE> cases = {
        {       1e19,       280.0,     -80.0,      -80.0,       -80.0,       280.0 },
        {      -1e19,        80.0,    -280.0,       80.0,        80.0,      -280.0 },
        {   INFINITY,         0.0,       0.0,        0.0,         0.0,         0.0 },
        {  -INFINITY,         0.0,       0.0,        0.0,         0.0,         0.0 },
        {        NAN,         0.0,       0.0,        0.0,         0.0,         0.0 },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_INFO_SCOPE( "Original angle: " << c.m_Angle << " degrees" );

        BOOST_CHECK_EQUAL( EDA_ANGLE( c.m_Angle, DEGREES_T ).Normalize().AsDegrees(), c.m_ExpNormalized );
        BOOST_CHECK_EQUAL( EDA_ANGLE( c.m_Angle, DEGREES_T ).NormalizeNegative().AsDegrees(),
                           c.m_ExpNormalizedNegative );
        BOOST_CHECK_EQUAL( EDA_ANGLE( c.m_Angle, DEGREES_T ).Normalize90().AsDegrees(), c.m_ExpNormalized90 );
        BOOST_CHECK_EQUAL( EDA_ANGLE( c.m_Angle, DEGREES_T ).Normalize180().AsDegrees(), c.m_ExpNormalized180 );
        BOOST_CHECK_EQUAL( EDA_ANGLE( c.m_Angle, DEGREES_T ).Normalize720().AsDegrees(), c.m_ExpNormalized720 );
    }

    // 9e18 is an exact multiple of 180
    BOOST_CHECK( EDA_ANGLE( 9e18, DEGREES_T ).IsCardinal() );
    BOOST_CHECK( !EDA_ANGLE( 9e18, DEGREES_T ).IsCardinal90() );

    for( double bad : { INFINITY, -INFINITY } )
    {
        BOOST_CHECK( !EDA_ANGLE( bad, DEGREES_T ).IsCardinal() );
        BOOST_CHECK( !EDA_ANGLE( bad, DEGREES_T ).IsCardinal90() );
    }
}


/**
 * Reference copy of the stepping loops that normalization used before exact folding.  They are
 * exact at the magnitudes sampled here, so the folded versions must match them bit for bit,
 * signed zero included.
 */
static double refStep( double aValue, double aLow, bool aLowIncl, double aHigh, bool aHighIncl, double aPeriod )
{
    while( aLowIncl ? aValue < aLow : aValue <= aLow )
        aValue += aPeriod;

    while( aHighIncl ? aValue > aHigh : aValue >= aHigh )
        aValue -= aPeriod;

    return aValue;
}


static bool refIsCardinal( double aValue )
{
    while( aValue < 0.0 )
        aValue += 90.0;

    while( aValue >= 90.0 )
        aValue -= 90.0;

    return aValue == 0.0;
}


static bool refIsCardinal90( double aValue )
{
    aValue = std::abs( aValue );

    while( aValue >= 180.0 )
        aValue -= 180.0;

    return aValue == 90.0;
}


BOOST_AUTO_TEST_CASE( NormalizeMatchesSteppingLoops )
{
    std::vector<double> inputs;

    for( int i = -40; i <= 40; ++i )
    {
        double base = 45.0 * i;
        inputs.insert( inputs.end(), { base, std::nextafter( base, -INFINITY ), std::nextafter( base, INFINITY ),
                                       base - 1e-20, base + 1e-20 } );
    }

    for( int i = -5000; i <= 5000; ++i )
        inputs.push_back( i * 199.87654321 );

    inputs.insert( inputs.end(), { 0.0, -0.0, 1e-300, -1e-300 } );

    auto sameBits = []( double a, double b )
    {
        return a == b && std::signbit( a ) == std::signbit( b );
    };

    for( double x : inputs )
    {
        BOOST_TEST_INFO_SCOPE( "Original angle: " << std::setprecision( 17 ) << x << " degrees" );

        BOOST_CHECK( sameBits( EDA_ANGLE( x, DEGREES_T ).Normalize().AsDegrees(),
                               refStep( x, 0.0, true, 360.0, false, 360.0 ) ) );
        BOOST_CHECK( sameBits( EDA_ANGLE( x, DEGREES_T ).NormalizeNegative().AsDegrees(),
                               refStep( x, -360.0, false, 0.0, true, 360.0 ) ) );
        BOOST_CHECK( sameBits( EDA_ANGLE( x, DEGREES_T ).Normalize90().AsDegrees(),
                               refStep( x, -90.0, true, 90.0, true, 180.0 ) ) );
        BOOST_CHECK( sameBits( EDA_ANGLE( x, DEGREES_T ).Normalize180().AsDegrees(),
                               refStep( x, -180.0, false, 180.0, true, 360.0 ) ) );
        BOOST_CHECK( sameBits( EDA_ANGLE( x, DEGREES_T ).Normalize720().AsDegrees(),
                               refStep( x, -360.0, true, 360.0, false, 360.0 ) ) );
        BOOST_CHECK_EQUAL( EDA_ANGLE( x, DEGREES_T ).IsCardinal(), refIsCardinal( x ) );
        BOOST_CHECK_EQUAL( EDA_ANGLE( x, DEGREES_T ).IsCardinal90(), refIsCardinal90( x ) );
    }
}


BOOST_AUTO_TEST_CASE( ConstantAngles )
{
    BOOST_CHECK_EQUAL( ANGLE_0.AsDegrees(), 0.0 );
    BOOST_CHECK_EQUAL( ANGLE_45.AsDegrees(), 45.0 );
    BOOST_CHECK_EQUAL( ANGLE_90.AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( ANGLE_135.AsDegrees(), 135.0 );
    BOOST_CHECK_EQUAL( ANGLE_180.AsDegrees(), 180.0 );
    BOOST_CHECK_EQUAL( ANGLE_270.AsDegrees(), 270.0 );
    BOOST_CHECK_EQUAL( ANGLE_360.AsDegrees(), 360.0 );

    BOOST_CHECK_EQUAL( ANGLE_HORIZONTAL.AsDegrees(), 0.0 );
    BOOST_CHECK_EQUAL( ANGLE_VERTICAL.AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( FULL_CIRCLE.AsDegrees(), 360.0 );
}


BOOST_AUTO_TEST_CASE( Snapped )
{
    auto deg = []( double d ) { return EDA_ANGLE( d, DEGREES_T ); };

    BOOST_CHECK_EQUAL( deg( 80 ).Snapped( ANGLE_90 ).AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( deg( 30 ).Snapped( ANGLE_90 ).AsDegrees(), 0.0 );
    BOOST_CHECK_EQUAL( deg( 135 ).Snapped( ANGLE_90 ).AsDegrees(), 180.0 );
    BOOST_CHECK_EQUAL( deg( 45 ).Snapped( ANGLE_90 ).AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( deg( -10 ).Snapped( ANGLE_90 ).AsDegrees(), 0.0 );
    BOOST_CHECK_EQUAL( deg( 80 ).Snapped( ANGLE_45 ).AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( deg( 5 ).Snapped( ANGLE_0 ).AsDegrees(), 5.0 ); // step <= 0 guard

    // Minimal equivalent rotation used by grid-aware placement: raw - raw.Snapped(period).
    auto residual = [&]( double raw, const EDA_ANGLE& period )
    {
        EDA_ANGLE a = deg( raw );
        return ( a - a.Snapped( period ) ).AsDegrees();
    };

    BOOST_CHECK_EQUAL( residual( 80, ANGLE_90 ), -10.0 );
    BOOST_CHECK_EQUAL( residual( 30, ANGLE_90 ), 30.0 );
    BOOST_CHECK_EQUAL( residual( 135, ANGLE_90 ), -45.0 );
    BOOST_CHECK_EQUAL( residual( 0, ANGLE_90 ), 0.0 );
}


BOOST_AUTO_TEST_CASE( Orientation )
{
    auto deg = []( double d ) { return EDA_ANGLE( d, DEGREES_T ); };
    auto orientation = [&]( double d ) { return EDA_ORIENTATION( deg( d ) ); };

    // Construction normalizes into [0, 360), and equality compares canonical values.
    BOOST_CHECK_EQUAL( orientation( 370 ).GetAngle().AsDegrees(), 10.0 );
    BOOST_CHECK_EQUAL( orientation( -90 ).GetAngle().AsDegrees(), 270.0 );
    BOOST_CHECK( orientation( 370 ) == orientation( 10 ) );

    // Addition and subtraction wrap in every form.
    EDA_ORIENTATION turn = orientation( 350 );
    BOOST_CHECK_EQUAL( ( turn + deg( 30 ) ).GetAngle().AsDegrees(), 20.0 );
    BOOST_CHECK_EQUAL( ( deg( 30 ) + turn ).GetAngle().AsDegrees(), 20.0 );
    BOOST_CHECK_EQUAL( ( turn += deg( 30 ) ).GetAngle().AsDegrees(), 20.0 );
    BOOST_CHECK_EQUAL( ( turn -= deg( 30 ) ).GetAngle().AsDegrees(), 350.0 );

    // Negation and subtraction
    BOOST_CHECK_EQUAL( ( -orientation( 45 ) ).GetAngle().AsDegrees(), 315.0 );
    // Wrapping subtraction
    BOOST_CHECK_EQUAL( ( ANGLE_180 - orientation( 270 ) ).GetAngle().AsDegrees(), 270.0 );
    // Orientation minus angle, with wrapping
    BOOST_CHECK_EQUAL( ( orientation( 10 ) - deg( 350 ) ).GetAngle().AsDegrees(), 20.0 );

    // A difference of orientations -> signed rotation amount (EDA_ANGLE, not EDA_ORIENTATION)
    EDA_ANGLE diff = orientation( 10 ) - orientation( 350 );
    BOOST_CHECK_EQUAL( diff.AsDegrees(), -340.0 );

    // -0 becomes 0
    BOOST_CHECK( !std::signbit( ( -orientation( 0 ) ).GetAngle().AsDegrees() ) );
}


BOOST_AUTO_TEST_SUITE_END()
