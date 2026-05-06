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
 */

#include <boost/test/unit_test.hpp>

#include <limits>

#include <base_units.h>
#include <core/minoptmax.h>
#include <net_chain_bridging.h>


static void applyBridging( MINOPTMAX<int>& aTarget, long long aBridgingIU )
{
    if( aTarget.HasMin() )
        aTarget.SetMin( SubtractBridgingClamped( aTarget.Min(), aBridgingIU ) );

    if( aTarget.HasOpt() )
        aTarget.SetOpt( SubtractBridgingClamped( aTarget.Opt(), aBridgingIU ) );

    if( aTarget.HasMax() )
        aTarget.SetMax( SubtractBridgingClamped( aTarget.Max(), aBridgingIU ) );
}


BOOST_AUTO_TEST_SUITE( TuningBridgingClamp )


BOOST_AUTO_TEST_CASE( TimeDomainAttosecondsClampsToZero )
{
    constexpr long long bridging = 3'000LL * static_cast<long long>( pcbIUScale.IU_PER_PS );

    BOOST_CHECK_GT( bridging, static_cast<long long>( std::numeric_limits<int>::max() ) );

    MINOPTMAX<int> target;
    target.SetMin( 1'000'000'000 );
    target.SetMax( 1'200'000'000 );

    applyBridging( target, bridging );

    BOOST_CHECK_EQUAL( target.Min(), 0 );
    BOOST_CHECK_EQUAL( target.Max(), 0 );
}


BOOST_AUTO_TEST_CASE( LengthModeUnaffectedWithinIntRange )
{
    MINOPTMAX<int> target;
    target.SetMin( 100'000'000 );
    target.SetMax( 120'000'000 );

    applyBridging( target, 5'000'000 );

    BOOST_CHECK_EQUAL( target.Min(), 95'000'000 );
    BOOST_CHECK_EQUAL( target.Max(), 115'000'000 );
}


BOOST_AUTO_TEST_CASE( BridgingExceedsTargetFloorsAtZero )
{
    MINOPTMAX<int> target;
    target.SetMin( 100 );
    target.SetMax( 200 );

    applyBridging( target, 1'000'000'000LL );

    BOOST_CHECK_EQUAL( target.Min(), 0 );
    BOOST_CHECK_EQUAL( target.Max(), 0 );
}


// Pre-fix code truncated bridgingDelayIU to int, wrapping the value before subtraction;
// verify the long long path keeps the magnitude intact.
BOOST_AUTO_TEST_CASE( WideArithmeticPreservesMagnitude )
{
    constexpr long long bridging = 500'000'000'000'000LL;

    long long truncated = static_cast<long long>( static_cast<int>( bridging ) );

    BOOST_CHECK_NE( truncated, bridging );

    MINOPTMAX<int> target;
    target.SetMin( 2'000'000'000 );

    applyBridging( target, bridging );

    BOOST_CHECK_EQUAL( target.Min(), 0 );
}


BOOST_AUTO_TEST_CASE( OptStaysWithinMinMaxAfterBridging )
{
    MINOPTMAX<int> target;
    target.SetMin( 100'000'000 );
    target.SetOpt( 110'000'000 );
    target.SetMax( 120'000'000 );

    applyBridging( target, 5'000'000 );

    BOOST_CHECK_EQUAL( target.Min(), 95'000'000 );
    BOOST_CHECK_EQUAL( target.Opt(), 105'000'000 );
    BOOST_CHECK_EQUAL( target.Max(), 115'000'000 );

    BOOST_CHECK_LE( target.Min(), target.Opt() );
    BOOST_CHECK_LE( target.Opt(), target.Max() );
}


BOOST_AUTO_TEST_CASE( OptClampsToZeroWhenBridgingExceedsTarget )
{
    MINOPTMAX<int> target;
    target.SetMin( 1'000'000'000 );
    target.SetOpt( 1'100'000'000 );
    target.SetMax( 1'200'000'000 );

    constexpr long long bridging = 3'000LL * static_cast<long long>( pcbIUScale.IU_PER_PS );

    applyBridging( target, bridging );

    BOOST_CHECK_EQUAL( target.Min(), 0 );
    BOOST_CHECK_EQUAL( target.Opt(), 0 );
    BOOST_CHECK_EQUAL( target.Max(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
