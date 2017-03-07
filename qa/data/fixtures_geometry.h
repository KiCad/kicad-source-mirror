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

#ifndef __FIXTURES_H
#define __FIXTURES_H

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <polygon/PolyLine.h>

/**
 * Common data for the tests:
 *      1. holeyPolySet: A polyset containing one single squared outline with two holes: a
 *      non-convex pentagon and a triangle.
 *      2.solidPolySet: A polyset with three empty outlines and no holes.
 *      3. uniqueVertexPolySet: A polyset with one single outline that contains just one vertex.
 *      4. emptyPolySet: A polyset with no outlines.
 */
struct CommonTestData
{
    // Polygon sets common for all the tests
    SHAPE_POLY_SET emptyPolySet;
    SHAPE_POLY_SET uniqueVertexPolySet;
    SHAPE_POLY_SET solidPolySet;
    SHAPE_POLY_SET holeyPolySet;

    // Vectors containing the information with which the polygons are populated.
    std::vector<VECTOR2I> uniquePoints;
    std::vector<VECTOR2I> holeyPoints;
    std::vector<SEG> holeySegments;

    /**
     * Constructor.
     */
    CommonTestData()
    {
        // UniqueVertexPolySet shall have a unique vertex
        uniquePoints.push_back( VECTOR2I( 100, 50 ) );

        // Populate the holey polygon set points with 12 points

        // Square
        holeyPoints.push_back( VECTOR2I( 100,100 ) );
        holeyPoints.push_back( VECTOR2I( 0,100 ) );
        holeyPoints.push_back( VECTOR2I( 0,0 ) );
        holeyPoints.push_back( VECTOR2I( 100,0 ) );

        // Pentagon
        holeyPoints.push_back( VECTOR2I( 10,10 ) );
        holeyPoints.push_back( VECTOR2I( 10,20 ) );
        holeyPoints.push_back( VECTOR2I( 15,15 ) );
        holeyPoints.push_back( VECTOR2I( 20,20 ) );
        holeyPoints.push_back( VECTOR2I( 20,10 ) );

        // Triangle
        holeyPoints.push_back( VECTOR2I( 40,10 ) );
        holeyPoints.push_back( VECTOR2I( 40,20 ) );
        holeyPoints.push_back( VECTOR2I( 60,10 ) );

        // Save the segments of the holeyPolySet.
        holeySegments.push_back( SEG( holeyPoints[0], holeyPoints[1] ) );
        holeySegments.push_back( SEG( holeyPoints[1], holeyPoints[2] ) );
        holeySegments.push_back( SEG( holeyPoints[2], holeyPoints[3] ) );
        holeySegments.push_back( SEG( holeyPoints[3], holeyPoints[0] ) );

        // Pentagon segments
        holeySegments.push_back( SEG( holeyPoints[4], holeyPoints[5] ) );
        holeySegments.push_back( SEG( holeyPoints[5], holeyPoints[6] ) );
        holeySegments.push_back( SEG( holeyPoints[6], holeyPoints[7] ) );
        holeySegments.push_back( SEG( holeyPoints[7], holeyPoints[8] ) );
        holeySegments.push_back( SEG( holeyPoints[8], holeyPoints[4] ) );

        // Triangle segments
        holeySegments.push_back( SEG( holeyPoints[ 9], holeyPoints[10] ) );
        holeySegments.push_back( SEG( holeyPoints[10], holeyPoints[11] ) );
        holeySegments.push_back( SEG( holeyPoints[11], holeyPoints[9] ) );

        // Auxiliary variables to store the contours that will be added to the polygons
        SHAPE_LINE_CHAIN polyLine, hole;

        // Create a polygon set with a unique vertex
        polyLine.Append( uniquePoints[0] );
        polyLine.SetClosed( true );
        uniqueVertexPolySet.AddOutline(polyLine);

        // Create a polygon set without holes
        solidPolySet.NewOutline();
        solidPolySet.NewOutline();
        solidPolySet.NewOutline();

        // Create a polygon set with holes

        // Adds a new squared outline
        polyLine.Clear();

        for( int i = 0; i < 4; i++ )
            polyLine.Append( holeyPoints[i] );

        polyLine.SetClosed( true );

        holeyPolySet.AddOutline(polyLine);

        // Adds a new hole (a pentagon)
        for( int i = 4; i < 9; i++ )
            hole.Append( holeyPoints[i] );

        hole.SetClosed( true );
        holeyPolySet.AddHole( hole );


        // Adds a new hole (a triangle)
        hole.Clear();
        for( int i = 9; i < 12; i++ )
            hole.Append( holeyPoints[i] );

        hole.SetClosed( true );
        holeyPolySet.AddHole( hole );
    }

    ~CommonTestData(){}
};

/**
 * Fixture for the ChamferFillet test suite. It contains an instance of the common data and the
 * holeyPolySet replicated as a CPolyLine, in order to test behaviour of old and new Chamfer and
 * Fillet methods.
 */
struct ChamferFilletFixture {
    // Structure to store the common data.
    struct CommonTestData common;

    // CPolyLine representing the same polygon in polySet.
    CPolyLine legacyPolyLine;

    /**
     * Constructor.
     */
    ChamferFilletFixture()
    {
        // Replicate the vertices in the polySet outline
        legacyPolyLine.Start( 0, common.holeyPoints[0].x, common.holeyPoints[0].y,
                              CPolyLine::NO_HATCH );

        for( int i = 1; i < 4; i++ )
        {
            VECTOR2I point = common.holeyPoints[i];
            legacyPolyLine.AppendCorner( point.x, point.y );
        }

        legacyPolyLine.CloseLastContour();

        // Add the non-convex pentagon hole
        legacyPolyLine.Start( 0, common.holeyPoints[4].x, common.holeyPoints[4].y,
                              CPolyLine::NO_HATCH );

        for( int i = 5; i < 9; i++ )
        {
            VECTOR2I point = common.holeyPoints[i];
            legacyPolyLine.AppendCorner( point.x, point.y );
        }

        legacyPolyLine.CloseLastContour();

        // Add the triangle hole
        legacyPolyLine.Start( 0, common.holeyPoints[9].x, common.holeyPoints[9].y,
                              CPolyLine::NO_HATCH );

        for( int i = 10; i < 12; i++ )
        {
            VECTOR2I point = common.holeyPoints[i];
            legacyPolyLine.AppendCorner( point.x, point.y );
        }

        legacyPolyLine.CloseLastContour();
    }

    ~ChamferFilletFixture(){}
};

/**
 * Fixture for the Collision test suite. It contains an instance of the common data and two
 * vectors containing colliding and non-colliding points.
 */
struct CollisionFixture {
    // Structure to store the common data.
    struct CommonTestData common;

    // Vectors containing colliding and non-colliding points
    std::vector<VECTOR2I> collidingPoints, nonCollidingPoints;

    /**
    * Constructor
    */
    CollisionFixture()
    {
        // Create points colliding with the poly set.

        // Inside the polygon
        collidingPoints.push_back( VECTOR2I( 10,90 ) );

        // Inside the polygon, but on a re-entrant angle of a hole
        collidingPoints.push_back( VECTOR2I( 15,16 ) );

        // On a hole edge => inside the polygon
        collidingPoints.push_back( VECTOR2I( 40,25 ) );

        // On the outline edge => inside the polygon
        collidingPoints.push_back( VECTOR2I( 0,10 ) );

        // Create points not colliding with the poly set.

        // Completely outside of the polygon
        nonCollidingPoints.push_back( VECTOR2I( 200,200 ) );

        // Inside the outline and inside a hole => outside the polygon
        nonCollidingPoints.push_back( VECTOR2I( 15,12 ) );
    }

    ~CollisionFixture(){}
};

/**
* Fixture for the Iterator test suite. It contains an instance of the common data, three polysets with null segments and a vector containing their points.
*/
struct IteratorFixture {
    // Structure to store the common data.
    struct CommonTestData common;

    // Polygons to test whether the RemoveNullSegments method works
    SHAPE_POLY_SET lastNullSegmentPolySet;
    SHAPE_POLY_SET firstNullSegmentPolySet;
    SHAPE_POLY_SET insideNullSegmentPolySet;

    // Null segments points
    std::vector<VECTOR2I> nullPoints;

    IteratorFixture()
    {
        nullPoints.push_back( VECTOR2I( 100,100 ) );
        nullPoints.push_back( VECTOR2I(   0,100 ) );
        nullPoints.push_back( VECTOR2I(   0,  0 ) );

        // Create a polygon with its last segment null
        SHAPE_LINE_CHAIN polyLine;
        polyLine.Append( nullPoints[0] );
        polyLine.Append( nullPoints[1] );
        polyLine.Append( nullPoints[2] );
        polyLine.Append( nullPoints[2], true );
        polyLine.SetClosed( true );

        lastNullSegmentPolySet.AddOutline(polyLine);

        // Create a polygon with its first segment null
        polyLine.Clear();
        polyLine.Append( nullPoints[0] );
        polyLine.Append( nullPoints[0], true );
        polyLine.Append( nullPoints[1] );
        polyLine.Append( nullPoints[2] );
        polyLine.SetClosed( true );

        firstNullSegmentPolySet.AddOutline(polyLine);

        // Create a polygon with an inside segment null
        polyLine.Clear();
        polyLine.Append( nullPoints[0] );
        polyLine.Append( nullPoints[1] );
        polyLine.Append( nullPoints[1], true );
        polyLine.Append( nullPoints[2] );
        polyLine.SetClosed( true );

        insideNullSegmentPolySet.AddOutline(polyLine);
    }

    ~IteratorFixture(){}
};

#endif //__FIXTURES_H
