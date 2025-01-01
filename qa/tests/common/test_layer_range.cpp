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


#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <layer_range.h>
#include <vector>

BOOST_AUTO_TEST_SUITE(LayerRangeTests)

BOOST_AUTO_TEST_CASE( ForwardIterationTwoLayers )
{
    LAYER_RANGE               range( F_Cu, B_Cu, 2 );
    std::vector<PCB_LAYER_ID> expected = { F_Cu, B_Cu };
    std::vector<PCB_LAYER_ID> result;

    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_CASE( ForwardIterationFourLayers )
{
    LAYER_RANGE               range( F_Cu, B_Cu, 4 );
    std::vector<PCB_LAYER_ID> expected = { F_Cu, In1_Cu, In2_Cu, B_Cu };
    std::vector<PCB_LAYER_ID> result;

    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_CASE( ReverseIterationFourLayers )
{
    LAYER_RANGE               range( B_Cu, F_Cu, 4 );
    std::vector<PCB_LAYER_ID> expected = { B_Cu, In2_Cu, In1_Cu, F_Cu };
    std::vector<PCB_LAYER_ID> result;

    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_CASE( PartialRangeForward )
{
    LAYER_RANGE               range( In1_Cu, B_Cu, 6 );
    std::vector<PCB_LAYER_ID> expected = { In1_Cu, In2_Cu, In3_Cu, In4_Cu, B_Cu };
    std::vector<PCB_LAYER_ID> result;

    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_CASE( PartialRangeReverse )
{
    LAYER_RANGE               range( In3_Cu, F_Cu, 6 );
    std::vector<PCB_LAYER_ID> expected = { In3_Cu, In2_Cu, In1_Cu, F_Cu };
    std::vector<PCB_LAYER_ID> result;

    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_CASE( InvalidLayerThrowsException )
{
    BOOST_CHECK_THROW( LAYER_RANGE( F_Mask, B_Cu, 4 ), std::invalid_argument );
    BOOST_CHECK_THROW( LAYER_RANGE( F_Cu, B_Mask, 4 ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE( SingleLayerRange )
{
    LAYER_RANGE               range( In2_Cu, In2_Cu, 6 );
    std::vector<PCB_LAYER_ID> expected = { In2_Cu };
    std::vector<PCB_LAYER_ID> result;

    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_CASE( MaxLayerCount )
{
    LAYER_RANGE               range( F_Cu, B_Cu, PCB_LAYER_ID_COUNT );
    std::vector<PCB_LAYER_ID> expected = { F_Cu };


    for( int i = In1_Cu; i < 2 * PCB_LAYER_ID_COUNT; i += 2 )
    {
        expected.push_back( static_cast<PCB_LAYER_ID>( i ) );
    }
    expected.push_back( B_Cu );

    std::vector<PCB_LAYER_ID> result;
    for( auto layer : range )
    {
        result.push_back( layer );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( result.begin(), result.end(), expected.begin(), expected.end() );
}

BOOST_AUTO_TEST_SUITE_END()
