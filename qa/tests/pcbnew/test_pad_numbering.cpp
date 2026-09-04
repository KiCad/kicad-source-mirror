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

struct PAD_FIXTURE
{
    PAD_FIXTURE() :
            m_board(),
            m_footprint( &m_board )
    {
    }

    PAD* MakePTH( const wxString& aNumber )
    {
        PAD* pad = new PAD( &m_footprint );

        pad->SetNumber( aNumber );
        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetLayerSet( PAD::UnplatedHoleMask() );

        m_footprint.Add( pad );
        return pad;
    }

    PAD* MakeNPTH()
    {
        PAD* pad = new PAD( &m_footprint );

        pad->SetAttribute( PAD_ATTRIB::NPTH );
        pad->SetLayerSet( PAD::UnplatedHoleMask() );

        m_footprint.Add( pad );
        return pad;
    }

    PAD* MakeAperture()
    {
        PAD* pad = new PAD( &m_footprint );

        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetLayerSet( PAD::ApertureMask() );

        m_footprint.Add( pad );
        return pad;
    }

    PAD* MakeSmd( const wxString& aNumber = wxEmptyString )
    {
        PAD* pad = new PAD( &m_footprint );

        pad->SetNumber( aNumber );
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( PAD::SMDMask() );

        m_footprint.Add( pad );
        return pad;
    }

    BOARD     m_board;
    FOOTPRINT m_footprint;
};


BOOST_FIXTURE_TEST_SUITE( PadNumbering, PAD_FIXTURE )

/**
 * Check what gets names and what doesn't
 */
BOOST_AUTO_TEST_CASE( CanNumber )
{
    PAD* npth = MakeNPTH();
    BOOST_CHECK_EQUAL( false, npth->CanHaveNumber() );

    PAD* aperture = MakeAperture();
    BOOST_CHECK_EQUAL( false, aperture->CanHaveNumber() );

    PAD* smd = MakeSmd();
    BOOST_CHECK_EQUAL( true, smd->CanHaveNumber() );
}


BOOST_AUTO_TEST_CASE( TestPadCounting )
{
    PAD* p1  = MakeSmd( "1" );
    PAD* p2  = MakeSmd( "2" );
    PAD* p3  = MakeSmd( "33" );
    PAD* p4  = MakeSmd( "AA12" );
    PAD* p5  = MakeSmd( "BC35" );
    PAD* pXa = MakeSmd( "AAA1" );
    PAD* pXb = MakePTH( "MP" );
    PAD* pXc = MakeSmd( "MP1a" );

    BOOST_CHECK_EQUAL( m_footprint.GetNumberedPadCount(), 5 );
}


BOOST_AUTO_TEST_CASE( TestUSBcPadCounting )
{
    PAD* p1_2   = MakeSmd( "A1_B12" );
    PAD* p3_4   = MakeSmd( "A4_B9" );
    PAD* p5     = MakeSmd( "B8" );
    PAD* p6     = MakeSmd( "A5" );
    PAD* p7     = MakeSmd( "B7" );
    PAD* p8     = MakeSmd( "A6" );
    PAD* p9     = MakeSmd( "A7" );
    PAD* p10    = MakeSmd( "B6" );
    PAD* p11    = MakeSmd( "A8" );
    PAD* p12    = MakeSmd( "B5" );
    PAD* p13_14 = MakeSmd( "B4_A9" );
    PAD* p15_16 = MakeSmd( "B1_A12" );
    PAD* p17a   = MakeSmd( "S1" );
    PAD* p17b   = MakeSmd( "S1" );
    PAD* p17c   = MakeSmd( "S1" );

    BOOST_CHECK_EQUAL( m_footprint.GetNumberedPadCount(), 17 );
}


BOOST_AUTO_TEST_SUITE_END()
