/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <boost/test/unit_test.hpp>

#include <project/net_settings.h>
#include <netclass.h>

#include <memory>


BOOST_AUTO_TEST_SUITE( NetSettingsTests )


// A composite held by an item outlives a netclass edit until resync, so it must keep its
// constituents alive or GetName() reads freed names
BOOST_AUTO_TEST_CASE( CompositeOwnsConstituents )
{
    NET_SETTINGS            settings( nullptr, "" );
    std::weak_ptr<NETCLASS> weakSignal;

    {
        std::shared_ptr<NETCLASS> signal = std::make_shared<NETCLASS>( wxS( "Signal" ), false );
        weakSignal = signal;
        settings.SetNetclass( wxS( "Signal" ), signal );
    }

    settings.SetNetclassLabelAssignment( wxS( "N1" ), { wxS( "Signal" ) } );

    std::shared_ptr<NETCLASS> composite = settings.GetEffectiveNetClass( wxS( "N1" ) );

    BOOST_REQUIRE_EQUAL( composite->GetConstituentNetclasses().size(), 2u );

    settings.ClearNetclasses();

    BOOST_REQUIRE( !weakSignal.expired() );
    BOOST_CHECK_EQUAL( composite->GetName(), wxString( "Signal,Default" ) );

    composite.reset();

    BOOST_CHECK( weakSignal.expired() );
}


BOOST_AUTO_TEST_SUITE_END()
