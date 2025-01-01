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
#include <geometry/ellipse.h>
#include <bezier_curves.h>

#include <fmt.h>

BOOST_AUTO_TEST_SUITE( EllipseToBezier )


/// Allows for rounding in the testcases
const double MAX_ERROR = 0.01;


struct ELLIPSE_TO_BEZIER_CASE
{
    std::string name;
    ELLIPSE<double> input;
    std::vector<BEZIER<double>> expected;
};


BOOST_AUTO_TEST_CASE( EllipseToBezier )
{
    // This data mut be created "on the fly" to avoid a CTOR odres issue, because it uses
    // static data like ANGLE_0 and it must be created **after** ANGLE_0 CTOR is called
    // clang-format off
    static const std::vector<ELLIPSE_TO_BEZIER_CASE> cases = {
        {
            "full circle",
            { { 0, 0 }, { 100, 0 }, 1.0, ANGLE_0, FULL_CIRCLE },
            { { { 100, 0 }, { 100, -55.22847498307934 }, { 55.228474983079344, -100 }, { 0, -100 } },
              { { 0, -100 }, { -55.22847498307934, -100 }, { -100, -55.228474983079344 }, { -100, 0 } },
              { { -100, 0 }, { -100, 55.22847498307934 }, { -55.228474983079344, 100 }, { 0, 100 } },
              { { 0, 100 }, { 55.22847498307934, 100 }, { 100, 55.228474983079344 }, { 100, 0 } }
            }
        },
        {
            "ellipse",
            { { 0, 0 }, { -100, 0 }, 0.5, ANGLE_0, FULL_CIRCLE },
            { { { -100, 0 }, { -100, 27.61423749153967 }, { -55.228474983079344, 50 }, { 0, 50 } },
              { { 0, 50 }, { 55.22847498307934, 50 }, { 100, 27.614237491539672 }, { 100, 0 } },
              { { 100, 0 }, { 100, -27.61423749153967 }, { 55.228474983079344, -50 }, { 0, -50 } },
              { { 0, -50 }, { -55.22847498307934, -50 }, { -100, -27.614237491539672 }, { -100, 0 } }
            }
        },
        {
            "arc1",
            { { 0, 0 }, { 100, 0 }, 0.5, ANGLE_180, FULL_CIRCLE },
            { { { -100, 0 }, { -100, 27.61423749153967 }, { -55.228474983079344, 50 }, { 0, 50 } },
              { { 0, 50 }, { 55.22847498307934, 50 }, { 100, 27.614237491539672 }, { 100, 0 } }
            }
        },
        {
            "arc2",
            { { 223, 165 }, { 372, 634 }, 0.96, EDA_ANGLE( 4.437, RADIANS_T ), EDA_ANGLE( 0.401, RADIANS_T ) },
            { { { -463.86, 336.27 }, { -389.81, 608.33 }, { -170.75, 818.49 }, { 99.52, 876.73 } },
              { { 99.52, 876.73 }, { 369.80, 934.98 }, { 643.36, 831.0 }, { 803.07, 609.31 } }
            }
        },
        {
            "arc3",
            { { 112.75, 490.24 }, { 304.54, 129.16 }, 7.14, EDA_ANGLE( 1.90, RADIANS_T ), EDA_ANGLE( 1.09, RADIANS_T ) },
            { { { 886.98, -1609.17 }, { 608.61, -1333.45 }, { 110.16, -394.92 }, { -305.88, 636.89 } },
              { { -305.88, 636.89 }, { -721.93, 1668.69 }, { -940.88, 2509.32 }, { -829.87, 2648.63 } },
              { { -829.87, 2648.63 }, { -718.85, 2787.95 }, { -308.47, 2187.55 }, { 152.23, 1211.78 } },
              { { 152.23, 1211.78 }, { 612.93, 236.01 }, { 996.95, -846.11 }, { 1071.24, -1377.92 } }
            }
        },
    };
    // clang-format on

    for( const ELLIPSE_TO_BEZIER_CASE& c : cases )
    {
        BOOST_TEST_CONTEXT( c.name )
        {
            std::vector<BEZIER<double>> out;
            TransformEllipseToBeziers<double>( c.input, out );

            BOOST_CHECK_EQUAL( c.expected.size(), out.size() );

#if 0
            for( BEZIER<double>& b : out )
            {
                BOOST_TEST_MESSAGE( fmt::format( "{{ {{ {}, {} }}, {{ {}, {} }}, {{ {}, {} }}, {{ {}, {} }} }}",
                                    b.Start.x, b.Start.y, b.C1.x, b.C1.y,
                                    b.C2.x, b.C2.y, b.End.x, b.End.y ) );
            }
#endif

            for( size_t i = 0; i < out.size(); i++ )
            {
                BOOST_CHECK_LE( ( c.expected[i].Start - out[i].Start ).EuclideanNorm(),
                                MAX_ERROR );
                BOOST_CHECK_LE( ( c.expected[i].C1 - out[i].C1 ).EuclideanNorm(),
                                MAX_ERROR );
                BOOST_CHECK_LE( ( c.expected[i].C2 - out[i].C2 ).EuclideanNorm(),
                                MAX_ERROR );
                BOOST_CHECK_LE( ( c.expected[i].End - out[i].End ).EuclideanNorm(),
                                MAX_ERROR );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
