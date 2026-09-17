/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_drc_length_item_order.cpp
 *
 * A net's length must not depend on the order its items are handed to the
 * calculation. DRC collects them in pointer order, so an editing session
 * changed the answer and the skew error came and went on an untouched board.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <base_units.h>
#include <netinfo.h>
#include <pcb_track.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>

#include <algorithm>
#include <numeric>
#include <set>


BOOST_AUTO_TEST_SUITE( DrcLengthItemOrder )


static VECTOR2I mmPoint( double aX, double aY )
{
    return VECTOR2I( pcbIUScale.mmToIU( aX ), pcbIUScale.mmToIU( aY ) );
}


static void addTrack( BOARD& aBoard, int aNetCode, PCB_LAYER_ID aLayer, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_TRACK* track = new PCB_TRACK( &aBoard );
    track->SetLayer( aLayer );
    track->SetStart( aStart );
    track->SetEnd( aEnd );
    track->SetWidth( pcbIUScale.mmToIU( 0.115 ) );
    track->SetNetCode( aNetCode );
    aBoard.Add( track );
}


/*
 * A via transition where both layers bend at the same point inside the via pad.
 */
static void buildViaTransition( BOARD& aBoard, int aNetCode )
{
    const VECTOR2I viaPos = mmPoint( 0.0, 0.0 );

    // 0.190919 mm from the centre, inside the 0.2 mm via radius
    const VECTOR2I bend = mmPoint( -0.135, -0.135 );

    PCB_VIA* via = new PCB_VIA( &aBoard );
    via->SetPosition( viaPos );
    via->SetWidth( F_Cu, pcbIUScale.mmToIU( 0.4 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.2 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetNetCode( aNetCode );
    aBoard.Add( via );

    addTrack( aBoard, aNetCode, F_Cu, bend, viaPos );
    addTrack( aBoard, aNetCode, F_Cu, mmPoint( -5.0, -0.135 ), bend );

    addTrack( aBoard, aNetCode, B_Cu, bend, viaPos );
    addTrack( aBoard, aNetCode, B_Cu, mmPoint( -5.0, -0.135 ), bend );
}


BOOST_AUTO_TEST_CASE( LengthIsIndependentOfItemOrder )
{
    BOARD board;

    NETINFO_ITEM* net = new NETINFO_ITEM( &board, wxT( "sig" ), 1 );
    board.Add( net );

    buildViaTransition( board, net->GetNetCode() );

    LENGTH_DELAY_CALCULATION*                  calc = board.GetLengthCalculation();
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> items;

    for( PCB_TRACK* track : board.Tracks() )
        items.emplace_back( calc->GetLengthCalculationItem( track ) );

    BOOST_REQUIRE_EQUAL( items.size(), 5 );

    constexpr PATH_OPTIMISATIONS drcOpts = {
        .OptimiseVias = true, .MergeTracks = true, .OptimiseTracesInPads = true, .InferViaInPad = false
    };

    std::vector<size_t> order( items.size() );
    std::iota( order.begin(), order.end(), 0 );

    std::set<int64_t> lengths;

    do
    {
        // CalculateLength consumes its input, so every order starts from fresh copies
        std::vector<LENGTH_DELAY_CALCULATION_ITEM> ordered;
        ordered.reserve( order.size() );

        for( size_t idx : order )
            ordered.emplace_back( items[idx] );

        lengths.insert( calc->CalculateLength( ordered, drcOpts, nullptr, nullptr ) );
    } while( std::next_permutation( order.begin(), order.end() ) );

    BOOST_CHECK_MESSAGE( lengths.size() == 1, "net length depends on item order: "
                                                      << lengths.size() << " different lengths, spread "
                                                      << ( *lengths.rbegin() - *lengths.begin() ) / 1e6 << " mm" );
}


BOOST_AUTO_TEST_SUITE_END()
