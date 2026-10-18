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
#include <boost/test/data/test_case.hpp>

#include <algorithm>
#include <vector>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pad_utils.h>
#include <padstack.h>
#include <units_provider.h>

BOOST_AUTO_TEST_SUITE( PadCheck )

namespace
{
using PAD_CHECK_CODE = PAD_UTILS::PAD_CHECK_CODE;

/**
 * A named test case for the pad check layer/attribute matrix.
 */
struct PAD_CHECK_TEST_CASE : public KI_TEST::NAMED_CASE
{
    PAD_ATTRIB                  m_Attribute;
    LSET                        m_Layers;
    std::vector<PAD_CHECK_CODE> m_ExpectedCodes;
};


const std::vector<PAD_CHECK_TEST_CASE> padCheckCases = {
    {
            "inner-only SMD pad",
            PAD_ATTRIB::SMD,
            LSET( { In1_Cu, In2_Cu } ),
            { PAD_CHECK_CODE::PAD_NO_OUTER_COPPER },
    },
    {
            "inner-only CONN pad",
            PAD_ATTRIB::CONN,
            LSET( { In1_Cu } ),
            { PAD_CHECK_CODE::PAD_NO_OUTER_COPPER },
    },
    // The current checker reports the missing outer copper, not the mask issue itself.
    {
            "inner-only SMD pad with front mask",
            PAD_ATTRIB::SMD,
            LSET( { In1_Cu, F_Mask } ),
            { PAD_CHECK_CODE::PAD_NO_OUTER_COPPER },
    },
    // The current checker reports the missing outer copper, not the paste issue itself.
    {
            "inner-only SMD pad with back paste",
            PAD_ATTRIB::SMD,
            LSET( { In2_Cu, B_Paste } ),
            { PAD_CHECK_CODE::PAD_NO_OUTER_COPPER },
    },
    {
            "aperture pad with paste",
            PAD_ATTRIB::SMD,
            LSET( { F_Paste } ),
            {}, // Not expected to trigger any pad check errors
    },
    {
            "outer and inner SMD pad",
            PAD_ATTRIB::SMD,
            LSET( { F_Cu, In1_Cu, F_Mask, F_Paste } ),
            {},
    },
    {
            "both outer copper sides",
            PAD_ATTRIB::SMD,
            LSET( { F_Cu, B_Cu } ),
            { PAD_CHECK_CODE::SMD_BOTH_OUTER_SIDES },
    },
    {
            "wrong-side mask",
            PAD_ATTRIB::SMD,
            LSET( { B_Cu, F_Mask } ),
            { PAD_CHECK_CODE::SMD_MASK_WRONG_SIDE },
    },
};


void checkPadResults( const std::vector<PAD_UTILS::PAD_CHECK_RESULT>& aResults,
                      const std::vector<PAD_CHECK_CODE>&              aExpectedCodes )
{
    std::vector<int> actualCodes;
    actualCodes.reserve( aResults.size() );

    for( const PAD_UTILS::PAD_CHECK_RESULT& message : aResults )
        actualCodes.push_back( static_cast<int>( message.code ) );

    std::vector<int> expectedCodes;
    expectedCodes.reserve( aExpectedCodes.size() );

    for( PAD_CHECK_CODE code : aExpectedCodes )
        expectedCodes.push_back( static_cast<int>( code ) );

    std::sort( actualCodes.begin(), actualCodes.end() );
    std::sort( expectedCodes.begin(), expectedCodes.end() );

    BOOST_CHECK_EQUAL_COLLECTIONS( actualCodes.begin(), actualCodes.end(), expectedCodes.begin(), expectedCodes.end() );
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

    void setLayers( PAD_ATTRIB aAttrib, const LSET& aLayers )
    {
        m_Pad.SetAttribute( aAttrib );
        m_Pad.SetLayerSet( aLayers );
    }

    /**
     * Runs the pad check on the current pad and returns the results.
     *
     * @return A vector of PAD_CHECK_RESULT containing any issues found with the pad.
     */
    std::vector<PAD_UTILS::PAD_CHECK_RESULT> runPadCheckOnPad()
    {
        UNITS_PROVIDER                           units( pcbIUScale, EDA_UNITS::MM );
        std::vector<PAD_UTILS::PAD_CHECK_RESULT> messages;

        const auto errorHandler =
                [&]( const PAD_UTILS::PAD_CHECK_RESULT& aResult )
                {
                    messages.push_back( aResult );
                };

        PAD_UTILS::CheckPad( m_Pad, units, false, errorHandler );

        return messages;
    }
};


/**
 * Run the simple pad check test cases data-driven tests
 */
BOOST_DATA_TEST_CASE_F( PAD_CHECK_FIXTURE, PadCheckCases, boost::unit_test::data::make( padCheckCases ), testCase )
{
    BOOST_TEST_CONTEXT( testCase.m_CaseName )
    {
        setLayers( testCase.m_Attribute, testCase.m_Layers );

        const auto messages = runPadCheckOnPad();

        checkPadResults( messages, testCase.m_ExpectedCodes );
    }
}

BOOST_AUTO_TEST_SUITE_END()
