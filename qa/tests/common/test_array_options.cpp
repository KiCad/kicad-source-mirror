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
 * @file test_array_options.cpp
 * Test suite for #ARRAY_OPTIONS
 */

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <trigo.h>

#include <array_options.h>

/**
 * Define a stream function for logging this type.
 */
std::ostream& boost_test_print_type( std::ostream& os, const ARRAY_OPTIONS::TRANSFORM& aObj )
{
    os << "TRANSFORM[ " << aObj.m_offset << " r " << aObj.m_rotation.AsDegrees() << "deg"
       << " ]";
    return os;
}


/**
 * Predicate to see if a #ARRAY_OPTIONS::TRANSFORM is equal or nearly equal
 */
bool TransformIsClose( const ARRAY_OPTIONS::TRANSFORM& aL, const ARRAY_OPTIONS::TRANSFORM& aR )
{
    return KI_TEST::IsVecWithinTol<VECTOR2I>( aL.m_offset, aR.m_offset, 1 )
               && KI_TEST::IsWithin<double>( aL.m_rotation.AsDegrees(),
                                             aR.m_rotation.AsDegrees(), 0.001 );
}


/**
 * Generate all array transforms for an array descriptor and compare
 * against a list of expected transforms
 * @param aOpts the array descriptor
 * @param aPos  the position of the reference item
 * @param aExp  expected transform list
 */
void CheckArrayTransforms( const ARRAY_OPTIONS& aOpts, const VECTOR2I& aPos,
        const std::vector<ARRAY_OPTIONS::TRANSFORM>& aExp )
{
    std::vector<ARRAY_OPTIONS::TRANSFORM> transforms;

    for( int i = 0; i < aOpts.GetArraySize(); ++i )
    {
        transforms.push_back( aOpts.GetTransform( i, aPos ) );
    }

    BOOST_CHECK_EQUAL( transforms.size(), aExp.size() );

    for( unsigned i = 0; i < std::min( transforms.size(), aExp.size() ); ++i )
    {
        BOOST_TEST_CONTEXT( "Index " << i )
        {
            BOOST_CHECK_PREDICATE( TransformIsClose, ( transforms[i] )( aExp[i] ) );
        }
    }
}


/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( ArrayOptions )


struct GRID_ARRAY_GEOM_PARAMS
{
    int      m_nx;
    int      m_ny;
    VECTOR2I m_delta;
    VECTOR2I m_offset;
    int      m_stagger;
    bool     m_stagger_by_row;
    bool     m_alternate_numbers;
    bool     m_h_then_v;
};

struct GRID_ARRAY_TEST_CASE
{
    std::string                           m_case_name;
    GRID_ARRAY_GEOM_PARAMS                m_geom;
    VECTOR2I                              m_item_pos;
    std::vector<ARRAY_OPTIONS::TRANSFORM> m_exp_transforms;
};


// clang-format off
static const std::vector<GRID_ARRAY_TEST_CASE> grid_geom_cases = {
    {
        "2x3 rect grid",
        {
            2,
            3,
            { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) },
            { 0, 0 },
            1,
            true,
            false,
            true,
        },
        { 0, 0 },
        {
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
    {
        "2x3 offset grid",
        {
            2,
            3,
            { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) },
            { schIUScale.mmToIU( 0.1 ), schIUScale.mmToIU( 0.2 ) },
            1,
            true,
            false,
            true,
        },
        { 0, 0 },
        {
            // add the offsets for each positions
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 0.2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0.1 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2.1 ), schIUScale.mmToIU( 2.2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0.2 ), schIUScale.mmToIU( 4.0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2.2 ), schIUScale.mmToIU( 4.2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
    {
        "2x3 stagger rows",
        {
            2,
            3,
            { schIUScale.mmToIU( 3 ), schIUScale.mmToIU( 2 ) },
            { 0, 0 },
            3,
            true,
            false,
            true,
        },
        { 0, 0 },
        {
            // add the offsets for each positions
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 3 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 1 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 4 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 5 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
    {
        "2x3 stagger cols",
        {
            2,
            3,
            { schIUScale.mmToIU( 3 ), schIUScale.mmToIU( 2 ) },
            { 0, 0 },
            2,
            false,
            false,
            true,
        },
        { 0, 0 },
        {
            // add the offsets for each positions
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 3 ), schIUScale.mmToIU( 1 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 3 ), schIUScale.mmToIU( 3 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 3 ), schIUScale.mmToIU( 5 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
    {
        "2x3 rect alternate",
        {
            2,
            3,
            { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) },
            { 0, 0 },
            1,
            true,
            true,
            true,
        },
        { 0, 0 },
        {
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
    {
        "2x3 rect v then h",
        {
            2,
            3,
            { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) },
            { 0, 0 },
            1,
            true,
            false,
            false,
        },
        { 0, 0 },
        {
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 4 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
};
// clang-format on


/**
 * Test of grid array geometry
 */
BOOST_AUTO_TEST_CASE( GridGeometry )
{
    for( const auto& c : grid_geom_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            ARRAY_GRID_OPTIONS grid_opts;

            grid_opts.m_nx = c.m_geom.m_nx;
            grid_opts.m_ny = c.m_geom.m_ny;
            grid_opts.m_delta = c.m_geom.m_delta;
            grid_opts.m_offset = c.m_geom.m_offset;
            grid_opts.m_stagger = c.m_geom.m_stagger;
            grid_opts.m_stagger_rows = c.m_geom.m_stagger_by_row;
            grid_opts.m_reverseNumberingAlternate = c.m_geom.m_alternate_numbers;
            grid_opts.m_horizontalThenVertical = c.m_geom.m_h_then_v;

            CheckArrayTransforms( grid_opts, c.m_item_pos, c.m_exp_transforms );
        }
    }
}


struct CIRC_ARRAY_GEOM_PARAMS
{
    int      n;
    double   angle_offset;
    VECTOR2I centre;
    bool     rotate;
};

struct CIRC_ARRAY_TEST_CASE
{
    std::string                           m_case_name;
    CIRC_ARRAY_GEOM_PARAMS                m_geom;
    VECTOR2I                              m_item_pos;
    std::vector<ARRAY_OPTIONS::TRANSFORM> m_exp_transforms;
};


// clang-format off
static const std::vector<CIRC_ARRAY_TEST_CASE> circ_geom_cases = {
    {
        "Quad, no rotate items",
        {
            4,
            0,
            { 0, 0 },
            false,
        },
        { schIUScale.mmToIU( 10 ), 0 },
        {
            // diamond shape
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( -10 ), schIUScale.mmToIU( -10 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( -20 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { {schIUScale.mmToIU( -10 ), schIUScale.mmToIU( 10 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
        },
    },
    {
        "Quad, rotate items",
        {
            4,
            0,
            { 0, 0 },
            true,
        },
        { schIUScale.mmToIU( 10 ), 0 },
        {
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            { { schIUScale.mmToIU( -10 ), schIUScale.mmToIU( -10 ) }, EDA_ANGLE( 90.0, DEGREES_T ) },
            { { schIUScale.mmToIU( -20 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 180.0, DEGREES_T ) },
            { {schIUScale.mmToIU( -10 ), schIUScale.mmToIU( 10 ) }, EDA_ANGLE( 270.0, DEGREES_T ) },
        },
    },
    {
        "Three pts, 90 deg angle",
        {
            3,
            45.0,
            { 0, 0 },
            true,
        },
        { schIUScale.mmToIU( 10 ), 0 },
        {
            { { schIUScale.mmToIU( 0 ), schIUScale.mmToIU( 0 ) }, EDA_ANGLE( 0.0, DEGREES_T ) },
            // 10 * [ 1-sin(45), sin(45) ]
            { { schIUScale.mmToIU( -2.9289321881 ), schIUScale.mmToIU( -7.0710678118 ) }, EDA_ANGLE( 45.0, DEGREES_T ) },
            { { schIUScale.mmToIU( -10 ), schIUScale.mmToIU( -10 ) }, EDA_ANGLE( 90.0, DEGREES_T ) },
        },
    },
};
// clang-format on

/**
 * Test of circular array geometry
 */
BOOST_AUTO_TEST_CASE( CircularGeometry )
{
    for( const auto& c : circ_geom_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            ARRAY_CIRCULAR_OPTIONS grid_opts;

            grid_opts.m_nPts = c.m_geom.n;
            grid_opts.m_angle = EDA_ANGLE( c.m_geom.angle_offset, DEGREES_T );
            grid_opts.m_centre = c.m_geom.centre;
            grid_opts.m_rotateItems = c.m_geom.rotate;

            CheckArrayTransforms( grid_opts, c.m_item_pos, c.m_exp_transforms );
        }
    }
}

/**
 * Generate all array names and check against expected
 * @param aOpts the array descriptor
 * @param aExp  expected name list
 */
void CheckArrayNumbering( const ARRAY_OPTIONS& aOpts, const std::vector<std::string>& aExp )
{
    std::vector<std::string> names;

    for( int i = 0; i < aOpts.GetArraySize(); ++i )
    {
        names.push_back( aOpts.GetItemNumber( i ).ToStdString() );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( names.begin(), names.end(), aExp.begin(), aExp.end() );
}


struct GRID_ARRAY_NAMING_PARAMS
{
    ARRAY_AXIS::NUMBERING_TYPE m_pri_type;
    ARRAY_AXIS::NUMBERING_TYPE m_sec_type;
    std::string                m_start_at_x;
    std::string                m_start_at_y;
    bool                       m_2d_numbering;
    bool                       m_h_then_v;
    int                        m_nx;
    int                        m_ny;
};


struct GRID_ARRAY_NAMING_CASE
{
    std::string              m_case_name;
    GRID_ARRAY_NAMING_PARAMS m_prms;
    std::vector<std::string> m_exp_names;
};


// clang-format off
static const std::vector<GRID_ARRAY_NAMING_CASE> grid_name_cases = {
    {
        "Linear grid",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC, // doesn't matter here
            "1",
            "2",
            false,
            false, // doesn't matter
            2,
            3,
        },
        { "1", "2", "3", "4", "5", "6" },
    },
    {
        // Tests a 2d grid
        "2D grid A1",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "A",
            "1",
            true,
            true,
            2,
            3,
        },
        { "A1", "B1", "A2", "B2", "A3", "B3" },
    },
    {
        // Tests a 2d grid
        "2D grid 11",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            "1",
            "1",
            true,
            false,
            2,
            3,
        },
        // moving down the "long axis" first
        // so the first coordinate has a range of 1-3, the second 1-2
        { "11", "21", "31", "12", "22", "32" },
    },
    {
        // Tests a 2d grid, with different types and offsets (and alphabet wrap)
        "2D grid offsets",
        {
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC,
            ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_ALPHA_FULL,
            "5",
            "Z",
            true,
            true,
            2,
            3,
        },
        { "5Z", "6Z", "5AA", "6AA", "5AB", "6AB" },
    },
};
// clang-format on


/**
 * Test of grid array geometry
 */
BOOST_AUTO_TEST_CASE( GridNaming )
{
    for( const auto& c : grid_name_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            ARRAY_GRID_OPTIONS grid_opts;

            grid_opts.m_nx = c.m_prms.m_nx;
            grid_opts.m_ny = c.m_prms.m_ny;

            grid_opts.m_horizontalThenVertical = c.m_prms.m_h_then_v;

            grid_opts.m_pri_axis.SetAxisType( c.m_prms.m_pri_type );
            grid_opts.m_sec_axis.SetAxisType( c.m_prms.m_sec_type );

            grid_opts.m_pri_axis.SetOffset( c.m_prms.m_start_at_x );
            grid_opts.m_sec_axis.SetOffset( c.m_prms.m_start_at_y );

            grid_opts.m_2dArrayNumbering = c.m_prms.m_2d_numbering;

            // other grid settings (geom) can be defaulted, as they don't affect  numbering

            CheckArrayNumbering( grid_opts, c.m_exp_names );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
