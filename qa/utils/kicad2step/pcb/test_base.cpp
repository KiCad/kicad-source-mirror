/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file
 * Test suite for PCB "base" sexpr parsing
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <pcb/base.h>

#include <sexpr/sexpr_parser.h>

#include <cmath>


namespace BOOST_TEST_PRINT_NAMESPACE_OPEN
{
template <>
struct print_log_value<DOUBLET>
{
    inline void operator()( std::ostream& os, DOUBLET const& o )
    {
        os << "DOUBLET[ " << o.x << ", " << o.y << " ]";
    }
};
}
BOOST_TEST_PRINT_NAMESPACE_CLOSE


/**
 * Radians from degrees
 */
constexpr double DegToRad( double aDeg )
{
    return aDeg * M_PI / 180.0;
}


class TEST_PCB_BASE_FIXTURE
{
public:
    SEXPR::PARSER m_parser;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( PcbBase, TEST_PCB_BASE_FIXTURE )

struct TEST_2D_POS_ROT
{
    std::string m_sexp;
    bool        m_valid;
    DOUBLET     m_exp_pos;
    double      m_exp_rot;
};

/**
 * Test the Get2DPositionAndRotation function
 */
BOOST_AUTO_TEST_CASE( SexprTo2DPosAndRot )
{
    const std::vector<TEST_2D_POS_ROT> cases = {
        {
            "(at 0 0 0)",
            true,
            { 0.0, 0.0 },
            0.0,
        },
        {
            "(at 3.14 4.12 90.4)",
            true,
            { 3.14, 4.12 },
            DegToRad( 90.4 ),
        },
        {
            "(at 3.14)", // no enough params
            false,
            {},
            {},
        },
        {
            "(att 3.14 4.12 90.4)", // bad symbol name
            false,
            {},
            {},
        },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( c.m_sexp )
        {
            DOUBLET gotPos;
            double gotRot;

            std::unique_ptr<SEXPR::SEXPR> sexpr( m_parser.Parse( c.m_sexp ) );

            const bool ret = Get2DPositionAndRotation( sexpr.get(), gotPos, gotRot );

            BOOST_CHECK_EQUAL( ret, c.m_valid );

            if( !ret )
                continue;

            const double tolPercent = 0.00001;  // seems small enough

            BOOST_CHECK_CLOSE( gotPos.x, c.m_exp_pos.x, tolPercent );
            BOOST_CHECK_CLOSE( gotPos.y, c.m_exp_pos.y, tolPercent );
            BOOST_CHECK_CLOSE( gotRot, c.m_exp_rot, tolPercent );
        }
    }
}

struct TEST_LAYER_NAME
{
    std::string m_sexp;
    bool        m_valid;
    std::string m_layer;
};

/**
 * Test the layer list parser.
 */
BOOST_AUTO_TEST_CASE( TestGetLayerName )
{
    const std::vector<TEST_LAYER_NAME> cases = {
        {
            // Quoted string - OK
            "(layer \"Edge.Cuts\")",
            true,
            "Edge.Cuts",
        },
        {
            // Old KiCads exported without quotes (so, as symbols)
            "(layer Edge.Cuts)",
            true,
            "Edge.Cuts",
        },
        {
            // No atoms
            "(layer)",
            false,
            {},
        },
        {
            // Too many atoms in list
            "(layer \"Edge.Cuts\" 2)",
            false,
            {},
        },
        {
            // Wrong atom type
            "(layer 2)",
            false,
            {},
        },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( c.m_sexp )
        {
            std::unique_ptr<SEXPR::SEXPR> sexpr( m_parser.Parse( c.m_sexp ) );

            const OPT<std::string> ret = GetLayerName( *sexpr );

            BOOST_CHECK_EQUAL( !!ret, c.m_valid );

            if( ret )
            {
                BOOST_CHECK_EQUAL( *ret, c.m_layer );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()