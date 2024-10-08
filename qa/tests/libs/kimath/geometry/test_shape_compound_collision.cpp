/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <geometry/shape_compound.h>
#include <geometry/shape_circle.h>

#include "fixtures_geometry.h"

/**
 * Fixture for the Collision test suite. It contains an instance of the common data and two
 * vectors containing colliding and non-colliding points.
 */
struct ShapeCompoundCollisionFixture
{
    // Structure to store the common data.
    struct KI_TEST::CommonTestData common;

    // Vectors containing colliding and non-colliding points
    std::vector<SHAPE*> shapesA, shapesB, shapesC;

    SHAPE_COMPOUND* compoundA;
    SHAPE_COMPOUND* compoundB;
    SHAPE_COMPOUND* compoundC;

    /**
    * Constructor
    */
    ShapeCompoundCollisionFixture()
    {
        shapesA.push_back( new SHAPE_CIRCLE( VECTOR2I( 0, 0 ), 100 ) );
        shapesA.push_back( new SHAPE_CIRCLE( VECTOR2I( 80, 0 ), 100 ) );

        shapesB.push_back( new SHAPE_CIRCLE( VECTOR2I( 0, 80 ), 100 ) );
        shapesB.push_back( new SHAPE_CIRCLE( VECTOR2I( 80, 80 ), 100 ) );

        shapesC.push_back( new SHAPE_CIRCLE( VECTOR2I( 0, 280 ), 100 ) );
        shapesC.push_back( new SHAPE_CIRCLE( VECTOR2I( 80, 280 ), 100 ) );

        compoundA = new SHAPE_COMPOUND( shapesA );
        compoundB = new SHAPE_COMPOUND( shapesB );
        compoundC = new SHAPE_COMPOUND( shapesC );
    }

    ~ShapeCompoundCollisionFixture()
    {
        delete compoundA;
        delete compoundB;
        delete compoundC;
    }
};


/**
 * Declares the CollisionFixture as the boost test suite fixture.
 */
BOOST_FIXTURE_TEST_SUITE( SCompoundCollision, ShapeCompoundCollisionFixture )


/**
 * This test checks basic behaviour of PointOnEdge, testing if points on corners, outline edges
 * and hole edges are detected as colliding.
 */
BOOST_AUTO_TEST_CASE( ShapeCompoundCollide )
{
    int actual;
    // Check points on corners
    BOOST_CHECK( compoundA->Collide( compoundB, 0, &actual ) );
    BOOST_TEST( actual == 0 );

    BOOST_CHECK( !compoundA->Collide( compoundC, 0, &actual ) );
    BOOST_TEST( actual == 0 );

    BOOST_CHECK( compoundA->Collide( compoundC, 100, &actual ) );
    BOOST_TEST( actual == 80 );

    BOOST_CHECK( shapesA[0]->Collide( compoundB, 0 ) );
    BOOST_CHECK( shapesA[1]->Collide( compoundB, 0 ) );
    BOOST_CHECK( compoundB->Collide( shapesA[0], 0 ) );
    BOOST_CHECK( compoundB->Collide( shapesA[1], 0 ) );

    BOOST_CHECK( shapesB[0]->Collide( compoundA, 0 ) );
    BOOST_CHECK( shapesB[1]->Collide( compoundA, 0 ) );
    BOOST_CHECK( compoundA->Collide( shapesB[0], 0 ) );
    BOOST_CHECK( compoundA->Collide( shapesB[1], 0 ) );

    BOOST_CHECK( ! shapesC[0]->Collide( compoundA, 0 ) );
    BOOST_CHECK( ! shapesC[1]->Collide( compoundA, 0 ) );
    BOOST_CHECK( ! compoundA->Collide( shapesC[0], 0 ) );
    BOOST_CHECK( ! compoundA->Collide( shapesC[1], 0 ) );

    BOOST_CHECK( ! shapesA[0]->Collide( compoundC, 0 ) );
    BOOST_CHECK( ! shapesA[1]->Collide( compoundC, 0 ) );
    BOOST_CHECK( ! compoundC->Collide( shapesA[0], 0 ) );
    BOOST_CHECK( ! compoundC->Collide( shapesA[1], 0 ) );

    BOOST_CHECK( shapesC[0]->Collide( compoundA, 100, &actual ) );
    BOOST_TEST( actual == 80 );
    BOOST_CHECK( shapesC[1]->Collide( compoundA, 100, &actual ) );
    BOOST_TEST( actual == 80 );
    BOOST_CHECK( compoundA->Collide( shapesC[0], 100, &actual ) );
    BOOST_TEST( actual == 80 );
    BOOST_CHECK( compoundA->Collide( shapesC[1], 100, &actual ) );
    BOOST_TEST( actual == 80 );
}


BOOST_AUTO_TEST_SUITE_END()
