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


BOOST_AUTO_TEST_SUITE_END()
