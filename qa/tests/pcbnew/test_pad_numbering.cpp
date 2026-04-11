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

    PAD MakeNPTH()
    {
        PAD pad( &m_footprint );

        pad.SetAttribute( PAD_ATTRIB::NPTH );
        pad.SetLayerSet( PAD::UnplatedHoleMask() );

        return pad;
    }

    PAD MakeAperture()
    {
        PAD pad( &m_footprint );

        pad.SetAttribute( PAD_ATTRIB::PTH );
        pad.SetLayerSet( PAD::ApertureMask() );

        return pad;
    }

    PAD MakeSmd()
    {
        PAD pad( &m_footprint );

        pad.SetAttribute( PAD_ATTRIB::SMD );
        pad.SetLayerSet( PAD::SMDMask() );

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
    auto npth = MakeNPTH();
    BOOST_CHECK_EQUAL( false, npth.CanHaveNumber() );

    auto aperture = MakeAperture();
    BOOST_CHECK_EQUAL( false, aperture.CanHaveNumber() );

    auto smd = MakeSmd();
    BOOST_CHECK_EQUAL( true, smd.CanHaveNumber() );
}


BOOST_AUTO_TEST_SUITE_END()
