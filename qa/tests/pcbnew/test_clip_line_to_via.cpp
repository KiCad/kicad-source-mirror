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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_arc.h>
#include <length_delay_calculation/length_delay_calculation.h>


struct CLIP_LINE_TO_VIA_FIXTURE
{
    CLIP_LINE_TO_VIA_FIXTURE() :
            m_board()
    {
    }

    // A circular via at aCenter, big enough that a centre endpoint lands well
    // inside the pad.
    std::unique_ptr<PCB_VIA> MakeVia( const VECTOR2I& aCenter )
    {
        auto via = std::make_unique<PCB_VIA>( &m_board );
        via->SetPosition( aCenter );
        via->SetLayerPair( F_Cu, B_Cu );
        via->SetDrill( 400000 );  // 0.4 mm
        via->SetWidth( 1270000 ); // 1.27 mm -> radius 0.635 mm
        return via;
    }

    // A circular pad the same size as the via. OptimiseTraceInPad is arc-aware,
    // so it is the reference.
    std::unique_ptr<PAD> MakeCircularPad( const VECTOR2I& aCenter )
    {
        auto pad = std::make_unique<PAD>( &m_footprint );
        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 1270000, 1270000 ) );
        pad->SetPosition( aCenter );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        return pad;
    }

    BOARD     m_board;
    FOOTPRINT m_footprint{ &m_board };
};


BOOST_FIXTURE_TEST_SUITE( ClipLineToVia, CLIP_LINE_TO_VIA_FIXTURE )


// Control with no arc. Clipping a straight trace forwards and backwards must
// give the same length. Proves the harness is sound.
BOOST_AUTO_TEST_CASE( StraightTrace_ForwardBackwardAgree )
{
    const VECTOR2I viaCenter( 189190000, 46760000 );
    auto           via = MakeVia( viaCenter );

    SHAPE_LINE_CHAIN chain;
    chain.Append( viaCenter );                                 // inside via
    chain.Append( VECTOR2I( 189190000 + 5000000, 46760000 ) ); // 5 mm away

    SHAPE_LINE_CHAIN chainForward = chain;
    SHAPE_LINE_CHAIN chainBackward = chain;
    chainBackward.Reverse();

    BOOST_REQUIRE_EQUAL( chainForward.Length(), chainBackward.Length() );

    LENGTH_DELAY_CALCULATION::OptimiseTraceInVia( chainForward, via.get(), F_Cu );
    LENGTH_DELAY_CALCULATION::OptimiseTraceInVia( chainBackward, via.get(), F_Cu );

    BOOST_CHECK_CLOSE( static_cast<double>( chainForward.Length() ), static_cast<double>( chainBackward.Length() ),
                       0.5 );
}


// An arc starts at the via centre and exits the pad. The via clip and the
// arc-aware pad clip use the same circle, so they must report the same length.
// A gap means the via clip mishandles the arc.
BOOST_AUTO_TEST_CASE( ArcAtViaCenter_ViaClipMatchesPadClip )
{
    const VECTOR2I arcStart( 189190000, 46760000 );
    const VECTOR2I arcMid( 190382763, 46815467 );
    const VECTOR2I arcEnd( 189758445, 47833301 );

    SHAPE_LINE_CHAIN baseChain;
    baseChain.Append( arcStart );
    SHAPE_ARC arc( arcStart, arcMid, arcEnd, 0 );
    baseChain.Append( arc );

    const double fullArcLen = static_cast<double>( baseChain.Length() );

    SHAPE_LINE_CHAIN viaChain = baseChain;
    SHAPE_LINE_CHAIN padChain = baseChain;

    auto via = MakeVia( arcStart );
    auto pad = MakeCircularPad( arcStart );

    LENGTH_DELAY_CALCULATION::OptimiseTraceInVia( viaChain, via.get(), F_Cu );
    LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( padChain, pad.get(), F_Cu );

    const double viaLen = static_cast<double>( viaChain.Length() );
    const double padLen = static_cast<double>( padChain.Length() );

    BOOST_TEST_MESSAGE( "full arc length (no clip) (mm): " << fullArcLen / 1000000.0 );
    BOOST_TEST_MESSAGE( "via-clip length          (mm): " << viaLen / 1000000.0 );
    BOOST_TEST_MESSAGE( "pad-clip length (arc-aware)(mm): " << padLen / 1000000.0 );
    BOOST_TEST_MESSAGE( "via vs pad gap           (um): " << ( viaLen - padLen ) / 1000.0 );

    // Via clip should agree with the arc-aware pad clip on identical geometry.
    BOOST_CHECK_CLOSE( viaLen, padLen, 0.5 );
}


// Merge a straight line joined to an arc and check the total length is kept.
// Guards the arc-aware merge. An arc that bends into a segment must join one
// chain, not leave the segment ending at the bend.
BOOST_AUTO_TEST_CASE( Merge_DoesNotDropArcLength )
{
    const VECTOR2I a( 0, 0 );
    const VECTOR2I b( 5000000, 0 );
    const VECTOR2I arcMid( 6000000, 1000000 );
    const VECTOR2I arcEnd( 7000000, 0 );

    SHAPE_LINE_CHAIN straight;
    straight.Append( a );
    straight.Append( b );

    SHAPE_LINE_CHAIN arcChain;
    arcChain.Append( b );
    SHAPE_ARC arc( b, arcMid, arcEnd, 0 );
    arcChain.Append( arc );

    const double expected = static_cast<double>( straight.Length() + arcChain.Length() );

    auto makeItems = [&]()
    {
        std::vector<LENGTH_DELAY_CALCULATION_ITEM> items;

        LENGTH_DELAY_CALCULATION_ITEM s;
        s.SetLine( straight );
        s.SetLayers( F_Cu );
        s.SetWidth( 200000 );

        LENGTH_DELAY_CALCULATION_ITEM c;
        c.SetLine( arcChain );
        c.SetLayers( F_Cu );
        c.SetWidth( 200000 );

        items.push_back( s );
        items.push_back( c );
        return items;
    };

    LENGTH_DELAY_CALCULATION calc( &m_board );

    constexpr PATH_OPTIMISATIONS noMerge = {
        .OptimiseVias = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = false
    };
    constexpr PATH_OPTIMISATIONS merge = {
        .OptimiseVias = false, .MergeTracks = true, .OptimiseTracesInPads = false, .InferViaInPad = false
    };

    auto i1 = makeItems();
    auto i2 = makeItems();

    const double lenNoMerge = static_cast<double>( calc.CalculateLength( i1, noMerge, nullptr, nullptr ) );
    const double lenMerge = static_cast<double>( calc.CalculateLength( i2, merge, nullptr, nullptr ) );

    BOOST_TEST_MESSAGE( "expected  (mm): " << expected / 1000000.0 );
    BOOST_TEST_MESSAGE( "no-merge  (mm): " << lenNoMerge / 1000000.0 );
    BOOST_TEST_MESSAGE( "merge     (mm): " << lenMerge / 1000000.0 );

    BOOST_CHECK_CLOSE( lenNoMerge, expected, 0.01 );
    BOOST_CHECK_CLOSE( lenMerge, lenNoMerge, 0.01 );
}


BOOST_AUTO_TEST_SUITE_END()
