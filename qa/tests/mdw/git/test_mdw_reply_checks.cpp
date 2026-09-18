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

#include <boost/test/unit_test.hpp>

#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_call.h>
#include <mdw/mdw_proto_convert.h>


BOOST_AUTO_TEST_SUITE( MdwReplyChecks )


// A category from a newer protocol may be one that forbids editing
BOOST_AUTO_TEST_CASE( UnknownLifecycleCategoryIsRestricted )
{
    kiapi::workspace::CustomState state;
    state.set_category( static_cast<kiapi::workspace::LifecycleCategory>( 99 ) );

    MdwSanitize( state );

    BOOST_CHECK_EQUAL( state.category(), kiapi::workspace::LC_RESTRICTED );
}


BOOST_AUTO_TEST_CASE( UnsetLifecycleCategoryIsRestricted )
{
    BOOST_CHECK( MdwToStruct( kiapi::workspace::LifecycleState() ).category == MDW_LIFECYCLE_CATEGORY::RESTRICTED );
}


// Listings and design checks reach the client through this conversion, which must keep the warning
BOOST_AUTO_TEST_CASE( NotForNewDesignsSurvivesConversion )
{
    kiapi::workspace::LifecycleState state;
    state.set_not_for_new_designs( true );

    MDW_LIFECYCLE_STATE converted = MdwToStruct( state );
    BOOST_CHECK( converted.not_for_new_designs );

    kiapi::workspace::LifecycleState back;
    MdwToProto( &back, converted );
    BOOST_CHECK( back.not_for_new_designs() );
}


BOOST_AUTO_TEST_CASE( UnknownChangeOutcomeIsNotApplied )
{
    auto future = static_cast<kiapi::workspace::ChangeOutcome>( 99 );

    BOOST_CHECK( MdwFromProto( future ) == MDW_CHANGE_OUTCOME::PENDING );
}


BOOST_AUTO_TEST_CASE( UnsetErrorCodeIsNotSuccess )
{
    BOOST_CHECK( !MdwToResult( kiapi::workspace::WorkspaceError() ).IsOk() );
}


// A backend that leaves out a part's severity or sends no hold we know has not cleared the release
BOOST_AUTO_TEST_CASE( UnclearBaselineVerdictIsMalformed )
{
    auto reply = []()
    {
        kiapi::mdw::CheckBaselineReply checked;
        checked.add_parts()->set_severity( kiapi::mdw::SEV_NONE );
        checked.set_hold( kiapi::mdw::HOLD_NONE );
        return checked;
    };

    MDW_BASELINE_VERDICT           verdict;
    kiapi::mdw::CheckBaselineReply noSeverity = reply();
    kiapi::mdw::CheckBaselineReply noHold = reply();
    kiapi::mdw::CheckBaselineReply newerHold = reply();

    noSeverity.mutable_parts( 0 )->clear_severity();
    noHold.clear_hold();
    newerHold.set_hold( static_cast<kiapi::mdw::BaselineHold>( 99 ) );

    BOOST_CHECK( MdwReadBaselineVerdict( reply(), 1, verdict ) );
    BOOST_CHECK( !MdwReadBaselineVerdict( noSeverity, 1, verdict ) );
    BOOST_CHECK( !MdwReadBaselineVerdict( noHold, 1, verdict ) );
    BOOST_CHECK( !MdwReadBaselineVerdict( newerHold, 1, verdict ) );
}


BOOST_AUTO_TEST_CASE( MissingSourceTakesTheAnsweringLocator )
{
    kiapi::workspace::ItemRef own;
    kiapi::workspace::ItemRef foreign;
    foreign.set_source( "git:other" );

    MdwStamp( own, wxS( "git:here" ) );
    MdwStamp( foreign, wxS( "git:here" ) );

    BOOST_CHECK_EQUAL( own.source(), "git:here" );
    BOOST_CHECK_EQUAL( foreign.source(), "git:other" );
}


BOOST_AUTO_TEST_SUITE_END()
