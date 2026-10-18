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

#include "math/vector2d.h"
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pad_utils.h>
#include <padstack.h>
#include <units_provider.h>

#include <algorithm>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE( PadCheck )

namespace
{

/**
 * The check code and corresponding message for a single pad check error.
 */
using PAD_CHECK_MESSAGE = std::pair<PAD_UTILS::PAD_CHECK_CODE, wxString>;

bool hasCheckResult( const std::vector<PAD_CHECK_MESSAGE>& aMessages,
                     PAD_UTILS::PAD_CHECK_CODE aCode )
{
    const auto checkTypeIsFunc =
            [&]( const PAD_CHECK_MESSAGE& aMessage )
            {
                return aMessage.first == aCode;
            };

    return std::any_of( aMessages.begin(), aMessages.end(), checkTypeIsFunc );
}

} // namespace


/**
 * Fixture for pad check tests: This sets up a board, footprint, and pad,
 * and provides utility functions to run pad checks on the pad.
 */
struct PAD_CHECK_FIXTURE
{
    BOARD     m_Board;
    FOOTPRINT m_Footprint;
    PAD       m_Pad;

    PAD_CHECK_FIXTURE() :
            m_Footprint( &m_Board ),
            m_Pad( &m_Footprint )
    {
        m_Board.SetBoardUse( BOARD_USE::FPHOLDER );

        m_Pad.SetAttribute( PAD_ATTRIB::SMD );
        m_Pad.SetPadstackMode( PADSTACK::MODE::NORMAL );
        m_Pad.SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

        const VECTOR2I padSize{ pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) };
        m_Pad.SetSize( PADSTACK::ALL_LAYERS, padSize );
    }

    void setLayers( const PAD_ATTRIB& aAttrib, const LSET& aLayers )
    {
        m_Pad.SetAttribute( aAttrib );
        m_Pad.SetLayerSet( aLayers );
    }

    std::vector<PAD_CHECK_MESSAGE> runPadCheckOnPad()
    {
        UNITS_PROVIDER                 units( pcbIUScale, EDA_UNITS::MM );
        std::vector<PAD_CHECK_MESSAGE> messages;

        const auto errorHandler =
                [&]( const PAD_UTILS::PAD_CHECK_RESULT& aResult )
                {
                    messages.emplace_back( aResult.code, aResult.message );
                };

        PAD_UTILS::CheckPad( m_Pad, units, false, errorHandler );

        return messages;
    }
};


BOOST_FIXTURE_TEST_CASE( InnerOnlySmdPadIsReported, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { In1_Cu, In2_Cu } ) );

    const auto messages = runPadCheckOnPad();

    BOOST_CHECK_EQUAL( messages.size(), 1u );
    BOOST_CHECK( hasCheckResult( messages, PAD_UTILS::PAD_CHECK_CODE::PAD_NO_OUTER_COPPER ) );
}


BOOST_FIXTURE_TEST_CASE( InnerOnlyConnPadIsReported, PAD_CHECK_FIXTURE )
{
    m_Pad.SetAttribute( PAD_ATTRIB::CONN );
    setLayers( PAD_ATTRIB::CONN, LSET( { In1_Cu } ) );

    const auto messages = runPadCheckOnPad();

    BOOST_CHECK_EQUAL( messages.size(), 1u );
    BOOST_CHECK( hasCheckResult( messages, PAD_UTILS::PAD_CHECK_CODE::PAD_NO_OUTER_COPPER ) );
}


BOOST_FIXTURE_TEST_CASE( InnerOnlySmdPadWithMaskIsReported, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { In1_Cu, F_Mask } ) );

    const auto messages = runPadCheckOnPad();

    // The current checker reports the missing outer copper, not the mask issue itself.
    BOOST_CHECK_EQUAL( messages.size(), 1u );
    BOOST_CHECK( hasCheckResult( messages, PAD_UTILS::PAD_CHECK_CODE::PAD_NO_OUTER_COPPER ) );
}


BOOST_FIXTURE_TEST_CASE( InnerOnlySmdPadWithPasteIsReported, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { In2_Cu, B_Paste } ) );

    const auto messages = runPadCheckOnPad();

    // The current checker reports the missing outer copper, not the paste issue itself.
    BOOST_CHECK_EQUAL( messages.size(), 1u );
    BOOST_CHECK( hasCheckResult( messages, PAD_UTILS::PAD_CHECK_CODE::PAD_NO_OUTER_COPPER ) );
}


BOOST_FIXTURE_TEST_CASE( AperturePadWithPasteIsAllowed, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { F_Paste } ) );

    const auto messages = runPadCheckOnPad();
    BOOST_CHECK( messages.empty() );
}


BOOST_FIXTURE_TEST_CASE( OuterAndInnerSmdPadIsAllowed, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { F_Cu, In1_Cu, F_Mask, F_Paste } ) );

    const auto messages = runPadCheckOnPad();
    BOOST_CHECK( messages.empty() );
}


BOOST_FIXTURE_TEST_CASE( BothOuterCopperSidesAreReported, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { F_Cu, B_Cu } ) );

    const auto messages = runPadCheckOnPad();

    BOOST_CHECK_EQUAL( messages.size(), 1u );
    BOOST_CHECK( hasCheckResult( messages, PAD_UTILS::PAD_CHECK_CODE::SMD_BOTH_OUTER_SIDES ) );
}


BOOST_FIXTURE_TEST_CASE( WrongSideMaskIsReported, PAD_CHECK_FIXTURE )
{
    setLayers( PAD_ATTRIB::SMD, LSET( { B_Cu, F_Mask } ) );

    const auto messages = runPadCheckOnPad();

    BOOST_CHECK_EQUAL( messages.size(), 1u );
    BOOST_CHECK( hasCheckResult( messages, PAD_UTILS::PAD_CHECK_CODE::SMD_MASK_WRONG_SIDE ) );
}

BOOST_AUTO_TEST_SUITE_END()
