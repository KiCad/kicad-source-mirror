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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <vector>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <footprint_utils.h>
#include <layer_ids.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_shape.h>


BOOST_AUTO_TEST_SUITE( FootprintNetTieCheck )

namespace
{
/**
 * Build footprints that have various combinations of pads and copper segments
 * on various layers for net-tie testing.
 *
 * This allows to test a footprint's internal net-tie detection logic, which underpins
 * the wider DRC-level net-tie checks, but is also used independently for footprint validation.
 */
struct FP_LEVEL_NET_TIE_FIXTURE
{
    BOARD     m_board;
    FOOTPRINT m_footprint;

    FP_LEVEL_NET_TIE_FIXTURE() :
            m_footprint( &m_board )
    {
        m_board.SetBoardUse( BOARD_USE::FPHOLDER );

        // Start with the default stackup mode, which expands inner layers automatically.
        m_footprint.SetStackupMode( FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS );
    }


    void setCustomStackup( int numCuLayers )
    {
        m_footprint.SetStackupMode( FOOTPRINT_STACKUP::CUSTOM_LAYERS );
        m_footprint.SetStackupLayers( LSET::AllCuMask( numCuLayers ) );
    }

    /**
     * Add a single non-component pad to the footprint at the specified position and layer.
     *
     * Non-component pads are the most common type of pad used in net-ties.
     */
    void addNonComponentPad( const wxString& aNumber, const VECTOR2I& aPos, PCB_LAYER_ID aLayer )
    {
        const VECTOR2I padSize( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) );

        PAD* pad = new PAD( &m_footprint );

        pad->SetNumber( aNumber );
        pad->SetAttribute( PAD_ATTRIB::NON_COMPONENT );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, padSize );
        pad->SetLayerSet( LSET( { aLayer } ) );
        pad->SetPosition( aPos );

        m_footprint.Add( pad, ADD_MODE::APPEND );
    }

    void addCopperLine( const VECTOR2I& aStart, const VECTOR2I& aEnd, PCB_LAYER_ID aLayer )
    {
        PCB_SHAPE* line = new PCB_SHAPE( &m_footprint, SHAPE_T::SEGMENT );

        line->SetStart( aStart );
        line->SetEnd( aEnd );
        line->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
        line->SetLayer( aLayer );

        m_footprint.Add( line, ADD_MODE::APPEND );
    }

    /**
     * Add a two-pad net-tie with a connecting copper segment on @p aLayer, with each pad in its
     * own allowed-shorting group, which means they must not share copper.
     */
    void buildTwoPadTie( PCB_LAYER_ID aLayer )
    {
        const VECTOR2I padAPos( pcbIUScale.mmToIU( 90.0 ), pcbIUScale.mmToIU( 0.0 ) );
        const VECTOR2I padBPos( pcbIUScale.mmToIU( 110.0 ), pcbIUScale.mmToIU( 0.0 ) );

        addNonComponentPad( wxS( "1" ), padAPos, aLayer );
        addNonComponentPad( wxS( "2" ), padBPos, aLayer );
        addCopperLine( padAPos, padBPos, aLayer );
    }

    struct SHORT_INFO
    {
        const BOARD_ITEM* padA;
        const BOARD_ITEM* padB;
        const BOARD_ITEM* copper;
        VECTOR2I          pos;
    };

    /**
     * @return the number of shorted items reported by the footprint-level net-tie check.
     */
    std::vector<SHORT_INFO> getShortedItems()
    {
        std::vector<SHORT_INFO> shorts;

        const auto onShort =
                [&]( const BOARD_ITEM* padA, const BOARD_ITEM* padB, const BOARD_ITEM* copper, const VECTOR2I& pos )
                {
                    shorts.emplace_back( SHORT_INFO{ padA, padB, copper, pos } );
                };

        FOOTPRINT_UTILS::CheckNetTies( m_footprint, onShort );

        return shorts;
    }

    size_t countShortedItems()
    {
        return getShortedItems().size();
    }

    const PAD& expectPadWithNumber( const wxString& aNumber )
    {
        for( const PAD* pad : m_footprint.Pads() )
        {
            if( pad->GetNumber() == aNumber )
                return *pad;
        }
        throw std::runtime_error( "Pad with number " + std::string( aNumber.mb_str() ) + " not found" );
    }

    const PCB_SHAPE& expectCopperLine()
    {
        for( const BOARD_ITEM* shape : m_footprint.GraphicalItems() )
        {
            if( shape->Type() == PCB_SHAPE_T )
                return *static_cast<const PCB_SHAPE*>( shape );
        }
        throw std::runtime_error( "Copper line not found" );
    }
};

} // namespace


/**
 * Two pads in different groups, joined by copper on an outer layer.
 */
BOOST_FIXTURE_TEST_CASE( PadsInDifferentGroupsOnOuterLayerExpandingStackup, FP_LEVEL_NET_TIE_FIXTURE )
{
    buildTwoPadTie( F_Cu );

    const std::vector<FP_LEVEL_NET_TIE_FIXTURE::SHORT_INFO> shorts = getShortedItems();

    BOOST_REQUIRE_EQUAL( shorts.size(), 1 );

    // Check that the short involves two distinct pads and a copper line.
    BOOST_CHECK_NE( shorts[0].padA, nullptr );
    BOOST_CHECK_NE( shorts[0].padB, nullptr );
    BOOST_CHECK_NE( shorts[0].padA, shorts[0].padB );

    BOOST_CHECK_NE( shorts[0].copper, &expectCopperLine() );

    BOOST_CHECK( m_footprint.GetBoundingBox().Contains( shorts[0].pos ) );
}


/**
 * The same short on an inner layer must be found too.  Inner copper is used by buried graphics
 * and by non-component pads, and a check that only walks the outer layers misses it.
 */
BOOST_FIXTURE_TEST_CASE( PadsInDifferentGroupsOnInnerLayerExpandingStackup, FP_LEVEL_NET_TIE_FIXTURE )
{
    // Expanding stackups can handle In1 (only)
    buildTwoPadTie( In1_Cu );

    const std::vector<FP_LEVEL_NET_TIE_FIXTURE::SHORT_INFO> shorts = getShortedItems();

    BOOST_REQUIRE_EQUAL( shorts.size(), 1 );

    // Check that the short involves two distinct pads and a copper line.
    BOOST_CHECK_NE( shorts[0].padA, nullptr );
    BOOST_CHECK_NE( shorts[0].padB, nullptr );
    BOOST_CHECK_NE( shorts[0].padA, shorts[0].padB );

    BOOST_CHECK_NE( shorts[0].copper, &expectCopperLine() );

    BOOST_CHECK( m_footprint.GetBoundingBox().Contains( shorts[0].pos ) );
}


/**
 * The same short on an inner layer must be found too.  Inner copper is used by buried graphics
 * and by non-component pads, and a check that only walks the outer layers misses it.
 */
BOOST_FIXTURE_TEST_CASE( PadsInDifferentGroupsOnInnerLayerCustomStackup, FP_LEVEL_NET_TIE_FIXTURE )
{
    setCustomStackup( 4 );
    buildTwoPadTie( In2_Cu );

    const std::vector<FP_LEVEL_NET_TIE_FIXTURE::SHORT_INFO> shorts = getShortedItems();

    BOOST_REQUIRE_EQUAL( shorts.size(), 1 );

    // Check that the short involves two distinct pads and a copper line.
    BOOST_CHECK_NE( shorts[0].padA, nullptr );
    BOOST_CHECK_NE( shorts[0].padB, nullptr );
    BOOST_CHECK_NE( shorts[0].padA, shorts[0].padB );

    BOOST_CHECK_NE( shorts[0].copper, &expectCopperLine() );

    BOOST_CHECK( m_footprint.GetBoundingBox().Contains( shorts[0].pos ) );
}


/**
 * Pads in the same group are an allowed short, wherever the copper is.
 *
 * In this case, the two pads are connected by copper ion In2_Cu but
 * belong to the same net-tie pad group, so the short is allowed.
 */
BOOST_FIXTURE_TEST_CASE( PadsInSameGroupAreAllowed, FP_LEVEL_NET_TIE_FIXTURE )
{
    setCustomStackup( 4 );
    buildTwoPadTie( In2_Cu );

    m_footprint.AddNetTiePadGroup( wxS( "1, 2" ) );

    BOOST_CHECK_EQUAL( countShortedItems(), 0 );
}


/**
 * A pad which is not on the layer carrying the copper must not be treated as part of that
 * layer's copper connectivity group.
 *
 * In this case one pad is on a different layer to the other pad and copper graphic.
 */
BOOST_FIXTURE_TEST_CASE( PadOnOtherLayerIsNotPartOfConnection, FP_LEVEL_NET_TIE_FIXTURE )
{
    setCustomStackup( 4 );

    const VECTOR2I padAPos( pcbIUScale.mmToIU( 90.0 ), pcbIUScale.mmToIU( 0.0 ) );
    const VECTOR2I padBPos( pcbIUScale.mmToIU( 100.0 ), pcbIUScale.mmToIU( 0.0 ) );

    // Both pads sit at the ends of the line, but only pad 1 carries copper on In2.Cu.
    addNonComponentPad( wxS( "1" ), padAPos, In2_Cu );
    addNonComponentPad( wxS( "2" ), padBPos, In1_Cu );
    addCopperLine( padAPos, padBPos, In2_Cu );

    BOOST_CHECK_EQUAL( countShortedItems(), 0 );
}

BOOST_AUTO_TEST_SUITE_END()
