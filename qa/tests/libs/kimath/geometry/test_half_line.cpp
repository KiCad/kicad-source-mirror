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
#include <boost/test/data/test_case.hpp>

#include <geometry/half_line.h>
#include <geometry/shape_utils.h>

#include "geom_test_utils.h"


BOOST_AUTO_TEST_SUITE( HalfLine )

struct HalfLineBoxClipCase : public KI_TEST::NAMED_CASE
{
    HALF_LINE          Hl;
    BOX2I              Box;
    std::optional<SEG> ExpectedClippedSeg;
};

struct HalfLineHalfLineIntersectionCase : public KI_TEST::NAMED_CASE
{
    HALF_LINE               HlA;
    HALF_LINE               HlB;
    std::optional<VECTOR2I> ExpectedIntersection;
};

struct HalfLineContainsPointCase : public KI_TEST::NAMED_CASE
{
    HALF_LINE   Hl;
    VECTOR2I    Point;
    bool        ExpectedContains;
};

const std::vector<HalfLineContainsPointCase> Contains_cases{
    {
            "Point on the ray",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            VECTOR2I( 50, 50 ),
            true,
    },
    {
            "Point on the ray start",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            VECTOR2I( 0, 0 ),
            true,
    },
    {
            "Point on the ray end",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            VECTOR2I( 100, 100 ),
            true,
    },
    {
            "Point on the ray, past the end",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            VECTOR2I( 150, 150 ),
            true,
    },
    {
            "Point on the infinite line, but on the wrong side",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            VECTOR2I( -50, -50 ),
            false,
    },
    {
            "Point to one side",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            VECTOR2I( 50, 0 ),
            false,
    }
};


BOOST_DATA_TEST_CASE( Contains, boost::unit_test::data::make( Contains_cases ), c )
{
    const bool contains = c.Hl.Contains( c.Point );

    BOOST_TEST( contains == c.ExpectedContains );
}


const std::vector<HalfLineHalfLineIntersectionCase> Intersect_cases{
    {
            "Simple cross",
            HALF_LINE( SEG( VECTOR2I( -100, -100 ), VECTOR2I( 0, 0 ) ) ),
            HALF_LINE( SEG( VECTOR2I( 100, -100 ), VECTOR2I( 0, 0 ) ) ),
            VECTOR2I( 0, 0 ),
    },
    {
            "Parallel, no intersection",
            HALF_LINE( SEG( VECTOR2I( -100, 0 ), VECTOR2I( -100, 100 ) ) ),
            HALF_LINE( SEG( VECTOR2I( 100, 0 ), VECTOR2I( 100, 100 ) ) ),
            std::nullopt,
    }
};


BOOST_DATA_TEST_CASE( Intersect, boost::unit_test::data::make( Intersect_cases ), c )
{
    std::optional<VECTOR2I> intersection = c.HlA.Intersect( c.HlB );

    BOOST_REQUIRE( intersection.has_value() == c.ExpectedIntersection.has_value() );

    if( intersection )
    {
        BOOST_TEST( *intersection == *c.ExpectedIntersection );
    }
}


const std::vector<HalfLineBoxClipCase> ClipToBox_cases{
    {
            "Center to right edge",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ) ) ),
            BOX2I{ VECTOR2{ -1000, -1000 }, VECTOR2{ 2000, 2000 } },
            SEG( VECTOR2I( 0, 0 ), VECTOR2I( 1000, 0 ) ),
    },
    {
            "Centre to corner",
            HALF_LINE( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ) ),
            BOX2I{ VECTOR2{ -1000, -1000 }, VECTOR2{ 2000, 2000 } },
            SEG( VECTOR2I( 0, 0 ), VECTOR2I( 1000, 1000 ) ),
    },
    {
            "Ray not in the box",
            HALF_LINE( SEG( VECTOR2I( 1500, 0 ), VECTOR2I( 1600, 0 ) ) ),
            BOX2I{ VECTOR2{ -1000, -1000 }, VECTOR2{ 2000, 2000 } },
            std::nullopt,
    },
    {
            "Ray starts outside but crosses box",
            HALF_LINE( SEG( VECTOR2I( -1500, 0 ), VECTOR2I( 0, 0 ) ) ),
            BOX2I{ VECTOR2{ -1000, -1000 }, VECTOR2{ 2000, 2000 } },
            SEG( VECTOR2I( -1000, 0 ), VECTOR2I( 1000, 0 ) ),
    },
};


BOOST_DATA_TEST_CASE( ClipToBox, boost::unit_test::data::make( ClipToBox_cases ), c )
{
    std::optional<SEG> clipped = KIGEOM::ClipHalfLineToBox( c.Hl, c.Box );

    BOOST_REQUIRE( clipped.has_value() == c.ExpectedClippedSeg.has_value() );

    if( clipped )
    {
        BOOST_CHECK_PREDICATE( GEOM_TEST::SegmentsHaveSameEndPoints,
                               ( *clipped )( *c.ExpectedClippedSeg ) );
    }
}

BOOST_AUTO_TEST_SUITE_END();