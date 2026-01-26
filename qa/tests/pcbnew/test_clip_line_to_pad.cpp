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
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_arc.h>
#include <length_delay_calculation/length_delay_calculation.h>

#include <iostream>


struct CLIP_LINE_TO_PAD_FIXTURE
{
    CLIP_LINE_TO_PAD_FIXTURE() :
            m_board(),
            m_footprint( &m_board )
    {
    }

    std::unique_ptr<PAD> MakePad()
    {
        auto pad = std::make_unique<PAD>( &m_footprint );
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 2540000, 1270000 ) );
        pad->SetPosition( VECTOR2I( 189190000, 46760000 ) );
        pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, 0.15 );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        return pad;
    }

    BOARD     m_board;
    FOOTPRINT m_footprint;
};


BOOST_FIXTURE_TEST_SUITE( ClipLineToPad, CLIP_LINE_TO_PAD_FIXTURE )


BOOST_AUTO_TEST_CASE( Issue22612_ArcStartsAtPadCenter )
{
    auto pad = MakePad();

    const VECTOR2I arcStart( 189190000, 46760000 );
    const VECTOR2I arcMid( 190382763, 46815467 );
    const VECTOR2I arcEnd( 189758445, 47833301 );

    SHAPE_LINE_CHAIN chain;
    chain.Append( arcStart );

    SHAPE_ARC arc( arcStart, arcMid, arcEnd, 0 );
    chain.Append( arc );

    SHAPE_LINE_CHAIN chainForward = chain;
    SHAPE_LINE_CHAIN chainBackward = chain;
    chainBackward.Reverse();

    BOOST_REQUIRE_EQUAL( chainForward.Length(), chainBackward.Length() );

    LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( chainForward, pad.get(), F_Cu );
    LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( chainBackward, pad.get(), F_Cu );

    int64_t lengthForward = chainForward.Length();
    int64_t lengthBackward = chainBackward.Length();

    BOOST_CHECK_CLOSE( static_cast<double>( lengthForward ), static_cast<double>( lengthBackward ), 0.5 );

    BOOST_CHECK_CLOSE( lengthForward / 1000000.0, 2.4778, 2.0 );
}

BOOST_AUTO_TEST_CASE( Issue22612_TwoArcsFromPadCenter )
{
    auto pad = MakePad();

    // Arc 1: starts at pad center
    const VECTOR2I arc1Start( 189190000, 46760000 );
    const VECTOR2I arc1Mid( 189810000, 46140000 );
    const VECTOR2I arc1End( 190430000, 46760000 );

    // Arc 2: connects to arc1 end, defined in chain direction
    const VECTOR2I arc2Start( 190430000, 46760000 );
    const VECTOR2I arc2Mid( 191310000, 47640000 );
    const VECTOR2I arc2End( 192190000, 46760000 );

    SHAPE_LINE_CHAIN chain;
    chain.Append( arc1Start );

    SHAPE_ARC arc1( arc1Start, arc1Mid, arc1End, 0 );
    chain.Append( arc1 );

    SHAPE_ARC arc2( arc2Start, arc2Mid, arc2End, 0 );
    chain.Append( arc2 );

    SHAPE_LINE_CHAIN chainForward = chain;
    SHAPE_LINE_CHAIN chainBackward = chain;
    chainBackward.Reverse();

    BOOST_REQUIRE_EQUAL( chainForward.Length(), chainBackward.Length() );

    LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( chainForward, pad.get(), F_Cu );
    LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( chainBackward, pad.get(), F_Cu );

    int64_t lengthForward = chainForward.Length();
    int64_t lengthBackward = chainBackward.Length();

    BOOST_CHECK_CLOSE( static_cast<double>( lengthForward ), static_cast<double>( lengthBackward ), 0.5 );

    BOOST_CHECK_CLOSE( lengthForward / 1000000.0, 3.8246, 2.0 );
}


BOOST_AUTO_TEST_SUITE_END()