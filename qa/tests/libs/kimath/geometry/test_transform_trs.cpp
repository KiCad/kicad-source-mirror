/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/transform_trs.h>


BOOST_AUTO_TEST_SUITE( TransformTrs )


// 1 unit slack for float-to-int rounding.
static constexpr int IUNIT_TOL = 1;


static void CHECK_VEC_NEAR( const VECTOR2I& aActual, const VECTOR2I& aExpected, int aTol )
{
    BOOST_CHECK_MESSAGE( std::abs( aActual.x - aExpected.x ) <= aTol
                        && std::abs( aActual.y - aExpected.y ) <= aTol,
                        "actual ( " << aActual.x << ", " << aActual.y << " ) "
                        "expected ( " << aExpected.x << ", " << aExpected.y << " ) "
                        "tol " << aTol );
}


BOOST_AUTO_TEST_CASE( IdentityApply )
{
    TRANSFORM_TRS t;
    BOOST_CHECK( t.IsIdentity() );
    BOOST_CHECK( t.IsUniformScale() );

    VECTOR2I p( 12345, -6789 );
    CHECK_VEC_NEAR( t.Apply( p ), p, 0 );
    CHECK_VEC_NEAR( t.InverseApply( p ), p, 0 );
}


BOOST_AUTO_TEST_CASE( TranslateOnly )
{
    TRANSFORM_TRS t( VECTOR2I( 100, 200 ), ANGLE_0, 1.0, 1.0 );
    BOOST_CHECK( !t.IsIdentity() );

    CHECK_VEC_NEAR( t.Apply( VECTOR2I( 0, 0 ) ), VECTOR2I( 100, 200 ), 0 );
    CHECK_VEC_NEAR( t.Apply( VECTOR2I( 50, 50 ) ), VECTOR2I( 150, 250 ), 0 );

    CHECK_VEC_NEAR( t.InverseApply( VECTOR2I( 100, 200 ) ), VECTOR2I( 0, 0 ), 0 );
    CHECK_VEC_NEAR( t.InverseApply( VECTOR2I( 150, 250 ) ), VECTOR2I( 50, 50 ), 0 );
}


BOOST_AUTO_TEST_CASE( RotateOnly )
{
    // Expected values match trigo.h::RotatePoint conventions.
    struct CASE { EDA_ANGLE angle; VECTOR2I p; VECTOR2I expected; };

    const std::vector<CASE> cases = {
        { ANGLE_0,   VECTOR2I( 100, 0 ), VECTOR2I( 100, 0 ) },
        { ANGLE_90,  VECTOR2I( 100, 0 ), VECTOR2I( 0, -100 ) },
        { ANGLE_180, VECTOR2I( 100, 0 ), VECTOR2I( -100, 0 ) },
        { ANGLE_270, VECTOR2I( 100, 0 ), VECTOR2I( 0, 100 ) },
    };

    for( const CASE& c : cases )
    {
        TRANSFORM_TRS t( VECTOR2I( 0, 0 ), c.angle, 1.0, 1.0 );
        CHECK_VEC_NEAR( t.Apply( c.p ), c.expected, IUNIT_TOL );
        CHECK_VEC_NEAR( t.InverseApply( c.expected ), c.p, IUNIT_TOL );
    }
}


BOOST_AUTO_TEST_CASE( ScaleOnly )
{
    TRANSFORM_TRS uniform( VECTOR2I( 0, 0 ), ANGLE_0, 2.0, 2.0 );
    BOOST_CHECK( uniform.IsUniformScale() );
    CHECK_VEC_NEAR( uniform.Apply( VECTOR2I( 100, 50 ) ), VECTOR2I( 200, 100 ), 0 );
    CHECK_VEC_NEAR( uniform.InverseApply( VECTOR2I( 200, 100 ) ), VECTOR2I( 100, 50 ), 0 );

    TRANSFORM_TRS half( VECTOR2I( 0, 0 ), ANGLE_0, 0.5, 0.5 );
    CHECK_VEC_NEAR( half.Apply( VECTOR2I( 100, 50 ) ), VECTOR2I( 50, 25 ), 0 );

    TRANSFORM_TRS nonuniform( VECTOR2I( 0, 0 ), ANGLE_0, 2.0, 0.5 );
    BOOST_CHECK( !nonuniform.IsUniformScale() );
    CHECK_VEC_NEAR( nonuniform.Apply( VECTOR2I( 100, 100 ) ), VECTOR2I( 200, 50 ), 0 );
    CHECK_VEC_NEAR( nonuniform.InverseApply( VECTOR2I( 200, 50 ) ), VECTOR2I( 100, 100 ), 0 );
}


BOOST_AUTO_TEST_CASE( CompoundSrtOracle )
{
    // p (10, 0): scale 2x to (20, 0); rotate 90 to (0, -20); translate to (1000, 480).
    TRANSFORM_TRS t( VECTOR2I( 1000, 500 ), ANGLE_90, 2.0, 2.0 );

    CHECK_VEC_NEAR( t.Apply( VECTOR2I( 10, 0 ) ), VECTOR2I( 1000, 480 ), IUNIT_TOL );
    CHECK_VEC_NEAR( t.InverseApply( t.Apply( VECTOR2I( 10, 0 ) ) ), VECTOR2I( 10, 0 ), IUNIT_TOL );
}


BOOST_AUTO_TEST_CASE( InverseApplyRoundTrip )
{
    const std::vector<TRANSFORM_TRS> ts = {
        TRANSFORM_TRS( VECTOR2I( 100, 200 ), EDA_ANGLE( 37.5, DEGREES_T ), 1.0, 1.0 ),
        TRANSFORM_TRS( VECTOR2I( -50, 75 ), EDA_ANGLE( 123.4, DEGREES_T ), 1.5, 1.5 ),
        TRANSFORM_TRS( VECTOR2I( 0, 0 ), EDA_ANGLE( 60.0, DEGREES_T ), 2.0, 0.5 ),
        TRANSFORM_TRS( VECTOR2I( 1234, -567 ), EDA_ANGLE( 89.0, DEGREES_T ), 0.75, 1.25 ),
    };

    const std::vector<VECTOR2I> probes = {
        { 0, 0 }, { 100, 0 }, { 0, 100 }, { 250, -125 }, { -300, 400 },
    };

    for( const TRANSFORM_TRS& t : ts )
    {
        for( const VECTOR2I& p : probes )
        {
            VECTOR2I round = t.InverseApply( t.Apply( p ) );
            CHECK_VEC_NEAR( round, p, IUNIT_TOL );
        }
    }
}


BOOST_AUTO_TEST_CASE( InvertExactWhenUniformOrAxisAligned )
{
    // Invert is exact only for uniform scale or zero rotation.
    const std::vector<TRANSFORM_TRS> exactlyInvertible = {
        TRANSFORM_TRS( VECTOR2I( 100, 200 ), EDA_ANGLE( 30.0, DEGREES_T ), 2.0, 2.0 ),
        TRANSFORM_TRS( VECTOR2I( -50, 75 ), ANGLE_0, 2.0, 0.5 ),
        TRANSFORM_TRS( VECTOR2I( 0, 0 ), EDA_ANGLE( 45.0, DEGREES_T ), 1.0, 1.0 ),
    };

    const VECTOR2I probe( 250, -125 );

    for( const TRANSFORM_TRS& t : exactlyInvertible )
    {
        TRANSFORM_TRS inv = t.Invert();
        VECTOR2I round = inv.Apply( t.Apply( probe ) );
        CHECK_VEC_NEAR( round, probe, IUNIT_TOL );
    }
}


BOOST_AUTO_TEST_CASE( ComposeWithTranslateOnlyOuter )
{
    TRANSFORM_TRS inner( VECTOR2I( 100, 50 ), EDA_ANGLE( 30.0, DEGREES_T ), 2.0, 0.5 );
    TRANSFORM_TRS outer( VECTOR2I( 1000, 1000 ), ANGLE_0, 1.0, 1.0 );

    TRANSFORM_TRS composed = inner.Compose( outer );

    const VECTOR2I probe( 25, 75 );
    CHECK_VEC_NEAR( composed.Apply( probe ), outer.Apply( inner.Apply( probe ) ), IUNIT_TOL );
}


BOOST_AUTO_TEST_CASE( ComposeWithUniformScaleOuter )
{
    TRANSFORM_TRS inner( VECTOR2I( 100, 50 ), EDA_ANGLE( 30.0, DEGREES_T ), 1.0, 1.0 );
    TRANSFORM_TRS outer( VECTOR2I( 200, -100 ), EDA_ANGLE( 45.0, DEGREES_T ), 2.0, 2.0 );

    TRANSFORM_TRS composed = inner.Compose( outer );

    const VECTOR2I probe( 30, 40 );
    CHECK_VEC_NEAR( composed.Apply( probe ), outer.Apply( inner.Apply( probe ) ), 2 );
}


BOOST_AUTO_TEST_CASE( RescaleAroundFixedPointInvariant )
{
    TRANSFORM_TRS t( VECTOR2I( 1000, 500 ), ANGLE_0, 1.0, 1.0 );
    const VECTOR2I libPoint( 25, 25 );
    const VECTOR2I fixedPoint = t.Apply( libPoint );

    TRANSFORM_TRS rescaled = t.RescaleAround( fixedPoint, 3.0, 3.0 );

    CHECK_VEC_NEAR( rescaled.Apply( libPoint ), fixedPoint, IUNIT_TOL );
}


BOOST_AUTO_TEST_CASE( RescaleAroundComposesScale )
{
    TRANSFORM_TRS t( VECTOR2I( 1000, 500 ), ANGLE_0, 1.5, 1.5 );

    TRANSFORM_TRS rescaled = t.RescaleAround( VECTOR2I( 0, 0 ), 2.0, 2.0 );

    BOOST_CHECK_CLOSE( rescaled.GetScaleX(), 3.0, 1e-6 );
    BOOST_CHECK_CLOSE( rescaled.GetScaleY(), 3.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( RescaleAroundAnchorFollowsCenter )
{
    // 2x rescale around (0, 0): anchor (1000, 500) goes to (2000, 1000).
    TRANSFORM_TRS t( VECTOR2I( 1000, 500 ), ANGLE_0, 1.0, 1.0 );
    TRANSFORM_TRS r = t.RescaleAround( VECTOR2I( 0, 0 ), 2.0, 2.0 );

    CHECK_VEC_NEAR( r.GetTranslate(), VECTOR2I( 2000, 1000 ), 0 );
}


BOOST_AUTO_TEST_CASE( RescaleAroundRotatedNonUniformScalesLocalAxes )
{
    // The scale factors act in the footprint's own frame, so a non-uniform
    // rescale must apply directly (Sx *= aSx, Sy *= aSy) and never swap axes
    // with the rotation.
    TRANSFORM_TRS t( VECTOR2I( 1000, 500 ), EDA_ANGLE( 90.0, DEGREES_T ), 1.0, 1.0 );

    TRANSFORM_TRS r = t.RescaleAround( VECTOR2I( 0, 0 ), 2.0, 3.0 );

    BOOST_CHECK_CLOSE( r.GetScaleX(), 2.0, 1e-6 );
    BOOST_CHECK_CLOSE( r.GetScaleY(), 3.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( RescaleAroundRotatedFixedPointInvariant )
{
    // The fixed point stays put even with rotation and a non-uniform scale.
    TRANSFORM_TRS  t( VECTOR2I( 1000, 500 ), EDA_ANGLE( 90.0, DEGREES_T ), 1.0, 1.0 );
    const VECTOR2I libPoint( 25, 40 );
    const VECTOR2I fixedPoint = t.Apply( libPoint );

    TRANSFORM_TRS r = t.RescaleAround( fixedPoint, 2.0, 3.0 );

    CHECK_VEC_NEAR( r.Apply( libPoint ), fixedPoint, IUNIT_TOL );
}


BOOST_AUTO_TEST_CASE( ComposeAssociativity )
{
    // Translate-only outers keep the math exact.
    TRANSFORM_TRS A( VECTOR2I( 10, 20 ), EDA_ANGLE( 30.0, DEGREES_T ), 1.0, 1.0 );
    TRANSFORM_TRS B( VECTOR2I( 50, -50 ), ANGLE_0, 1.0, 1.0 );
    TRANSFORM_TRS C( VECTOR2I( 100, 0 ), ANGLE_0, 1.0, 1.0 );

    TRANSFORM_TRS leftAssoc = A.Compose( B ).Compose( C );
    TRANSFORM_TRS rightAssoc = A.Compose( B.Compose( C ) );

    const VECTOR2I probe( 7, 13 );
    CHECK_VEC_NEAR( leftAssoc.Apply( probe ), rightAssoc.Apply( probe ), IUNIT_TOL );
}


BOOST_AUTO_TEST_CASE( EqualityAndInequality )
{
    TRANSFORM_TRS a( VECTOR2I( 100, 200 ), ANGLE_45, 1.5, 0.5 );
    TRANSFORM_TRS b( VECTOR2I( 100, 200 ), ANGLE_45, 1.5, 0.5 );
    TRANSFORM_TRS c( VECTOR2I( 100, 200 ), ANGLE_45, 1.5, 0.6 );

    BOOST_CHECK( a == b );
    BOOST_CHECK( !( a == c ) );
    BOOST_CHECK( a != c );
}


BOOST_AUTO_TEST_CASE( ApplyLinearScaleArithmeticMean )
{
    TRANSFORM_TRS t( VECTOR2I( 0, 0 ), ANGLE_0, 2.0, 1.0 );
    BOOST_CHECK_CLOSE( t.ApplyLinearScale( 100.0 ), 150.0, 1e-9 );

    TRANSFORM_TRS uniform( VECTOR2I( 0, 0 ), ANGLE_0, 0.5, 0.5 );
    BOOST_CHECK_CLOSE( uniform.ApplyLinearScale( 100.0 ), 50.0, 1e-9 );
}


BOOST_AUTO_TEST_SUITE_END()
