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


static bool resolvesToNetclass( NET_SETTINGS& aSettings, const wxString& aNetName, const wxString& aExpected )
{
    std::shared_ptr<NETCLASS> resolved = aSettings.GetEffectiveNetClass( aNetName );

    if( !resolved )
        return false;

    if( resolved->GetName() == aExpected )
        return true;

    for( NETCLASS* constituent : resolved->GetConstituentNetclasses() )
    {
        if( constituent && constituent->GetName() == aExpected )
            return true;
    }

    return false;
}


// A net can pick up one class from a pattern that names it and another from a wildcard. The
// two are found by different means, so both have to end up on the net.
BOOST_AUTO_TEST_CASE( PlainAndWildcardPatternsBothApply )
{
    NET_SETTINGS settings( nullptr, "" );

    std::shared_ptr<NETCLASS>                     power = std::make_shared<NETCLASS>( wxS( "Power" ), false );
    std::shared_ptr<NETCLASS>                     highSpeed = std::make_shared<NETCLASS>( wxS( "HighSpeed" ), false );
    std::map<wxString, std::shared_ptr<NETCLASS>> classes;
    classes[wxS( "Power" )] = power;
    classes[wxS( "HighSpeed" )] = highSpeed;
    settings.SetNetclasses( classes );

    settings.SetNetclassPatternAssignment( wxS( "VCC_3V3" ), wxS( "Power" ) );
    settings.SetNetclassPatternAssignment( wxS( "VCC_*" ), wxS( "HighSpeed" ) );

    BOOST_CHECK( resolvesToNetclass( settings, wxS( "VCC_3V3" ), wxS( "Power" ) ) );
    BOOST_CHECK( resolvesToNetclass( settings, wxS( "VCC_3V3" ), wxS( "HighSpeed" ) ) );

    // A wildcard still has to reach the nets no pattern names outright.
    BOOST_CHECK( resolvesToNetclass( settings, wxS( "VCC_5V" ), wxS( "HighSpeed" ) ) );
    BOOST_CHECK( !resolvesToNetclass( settings, wxS( "VCC_5V" ), wxS( "Power" ) ) );
}


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
