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

/**
 * @file test_cadstar_archive_parser.cpp
 * Test suite for #CADSTAR_ARCHIVE_PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <common/io/cadstar/cadstar_archive_parser.h>

#include <qa_utils/geometry/geometry.h> // For KI_TEST::IsVecWithinTol
#include <geometry/shape_line_chain.h>
#include <geometry/shape_arc.h> // For SHAPE_ARC::DefaultAccuracyForPCB()


BOOST_AUTO_TEST_SUITE( CadstartArchiveParser )


struct VERTEX_APPEND_CASE
{
    std::string                              m_CaseName;
    CADSTAR_ARCHIVE_PARSER::VERTEX           m_VertexToAppend;
    BOX2I                                    m_ExpBBox;
    int                                      m_ExpBBoxError;
};

    std::function<wxPoint( const wxPoint& )> m_CadstarToKicadPointCallback;

using vt=CADSTAR_ARCHIVE_PARSER::VERTEX_TYPE;


static const std::vector<VERTEX_APPEND_CASE> appendToChainCases
{
    {
        "Append a point on x",
        { vt::VT_POINT, { 500000, 0 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size:*/ { 500000, 0 } },
        0
    },
    {
        "Append a point on y",
        { vt::VT_POINT, { 0, 500000 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size:*/ { 0, 500000 } },
        0
    },
    {
        "Append a Semicircle (clockwise)",
        { vt::CLOCKWISE_SEMICIRCLE, { 500000, 0 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size: */ { 500000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
    {
        "Append a Semicircle (anticlockwise)",
        { vt::ANTICLOCKWISE_SEMICIRCLE, { 500000, 0 } },
        { /* BBOX Position: */ { 0, -250000 }, /* Size: */ { 500000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
    {
        "Append a 90 degree Arc (clockwise)",
        { vt::CLOCKWISE_ARC, { 250000, 250000 }, { 250000, 0 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size: */ { 250000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
    {
        "Append a 90 degree Arc (anticlockwise)",
        { vt::ANTICLOCKWISE_ARC, { 250000, -250000 }, { 250000, 0 } },
        { /* BBOX Position: */ { 0, -250000 }, /* Size: */ { 250000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
};



BOOST_AUTO_TEST_CASE( AppendToChain )
{
    static const std::vector<VECTOR2D> coordinateMultipliers =
    {
      { 0.1,  0.1 },
      { 0.1, -0.1 }, // y inversion
      {  1,    1  },
      {  1,   -1  }, // y inversion
      { 10,   10  },
      { 10,  -10  }  // y inversion
    };


    for( const auto& c : appendToChainCases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_CaseName );

        for( const VECTOR2D& mult : coordinateMultipliers )
        {
            BOOST_TEST_INFO_SCOPE( "Applied scaling x=" << mult.x << " y=" << mult.y );

            SHAPE_LINE_CHAIN chain( { 0, 0 } ); // starting chain contains a point at 0,0

            auto transformCoord =
                [&]( const VECTOR2I& aPt ) -> VECTOR2I
                {
                    int x = double( aPt.x ) * mult.x;
                    int y = double( aPt.y ) * mult.y;
                    return { x, y };
                };

            c.m_VertexToAppend.AppendToChain( &chain, transformCoord, SCH_IU_PER_MM * 0.01 );

            BOX2I expBoxTransformed;
            expBoxTransformed.SetOrigin( transformCoord( c.m_ExpBBox.GetPosition() ) );
            expBoxTransformed.SetSize( transformCoord( c.m_ExpBBox.GetSize() ) );
            expBoxTransformed.Normalize();

            BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                    ( chain.BBox().GetPosition() )( expBoxTransformed.GetPosition() ) ( c.m_ExpBBoxError ) );

            BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                    ( chain.BBox().GetSize() )( expBoxTransformed.GetSize() ) ( c.m_ExpBBoxError ) );
        }
    }

}

BOOST_AUTO_TEST_SUITE_END()

