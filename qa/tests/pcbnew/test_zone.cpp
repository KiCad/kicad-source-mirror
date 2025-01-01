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
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <zone.h>


struct ZONE_TEST_FIXTURE
{
    BOARD m_board;
};


BOOST_FIXTURE_TEST_SUITE( Zone, ZONE_TEST_FIXTURE )

BOOST_AUTO_TEST_CASE( SingleLayer )
{
    ZONE zone( &m_board );

    zone.SetLayer( F_Cu );

    BOOST_TEST( zone.GetLayer() == F_Cu );
    BOOST_TEST( zone.GetLayer() == zone.GetFirstLayer() );

    BOOST_TEST( zone.IsOnCopperLayer() == true );
}

BOOST_AUTO_TEST_CASE( MultipleLayers )
{
    ZONE zone( &m_board );

    zone.SetLayerSet( { F_Cu, B_Cu } );

    // There is no "the" layer in a multi-layer zone
    BOOST_TEST( zone.GetLayer() == UNDEFINED_LAYER );
    // ... but there is a first layer
    BOOST_TEST( zone.GetFirstLayer() == F_Cu );

    BOOST_TEST( zone.IsOnCopperLayer() == true );
}

/**
 * During zone loading, the layer is set to Rescue if the layer is not found.
 * This is not a UI-visible layer, so make sure it can still be retreived.
 *
 * https://gitlab.com/kicad/code/kicad/-/issues/18553
 */
BOOST_AUTO_TEST_CASE( RescuedLayers )
{
    ZONE zone( &m_board );

    zone.SetLayer( Rescue );

    BOOST_TEST( zone.GetLayer() == Rescue );
    BOOST_TEST( zone.GetLayer() == zone.GetFirstLayer() );

    BOOST_TEST( zone.IsOnCopperLayer() == false );
}

BOOST_AUTO_TEST_SUITE_END()
