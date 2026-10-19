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

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <padstack.h>


BOOST_AUTO_TEST_SUITE( FootprintPadFixup )

struct FOOTPRINT_PAD_FIXUP_FIXTURE
{
    BOARD     m_Board;
    FOOTPRINT m_Footprint;
    PAD*      m_Pad;

    FOOTPRINT_PAD_FIXUP_FIXTURE() :
            m_Footprint( &m_Board ),
            m_Pad( new PAD( &m_Footprint ) )
    {
        m_Board.SetBoardUse( BOARD_USE::FPHOLDER );
        m_Board.SetCopperLayerCount( 4 );
        m_Footprint.SetStackupMode( FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS );

        m_Pad->SetAttribute( PAD_ATTRIB::SMD );
        m_Pad->SetPadstackMode( PADSTACK::MODE::NORMAL );
        m_Pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

        const VECTOR2I pad_size = VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) );
        m_Pad->SetSize( PADSTACK::ALL_LAYERS, pad_size );
        m_Footprint.Add( m_Pad );
    }
};


BOOST_FIXTURE_TEST_CASE( PthPadExpandsToAllCopperLayers, FOOTPRINT_PAD_FIXUP_FIXTURE )
{
    m_Pad->SetAttribute( PAD_ATTRIB::PTH );
    m_Pad->SetLayerSet( LSET( { F_Cu, B_Cu } ) );

    m_Footprint.FixUpPadsForBoard( &m_Board );

    const LSET layers = m_Pad->GetLayerSet();

    BOOST_CHECK_EQUAL( layers.count(), 4u );

    BOOST_CHECK( layers.test( F_Cu ) );
    BOOST_CHECK( layers.test( In1_Cu ) );
    BOOST_CHECK( layers.test( In2_Cu ) );
    BOOST_CHECK( layers.test( B_Cu ) );
}


BOOST_AUTO_TEST_SUITE_END()
