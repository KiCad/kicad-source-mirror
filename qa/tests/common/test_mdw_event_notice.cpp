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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <mdw/client/mdw_event_notice.h>


namespace
{

const wxString PROJECT = wxS( "git:/srv/mdw/amp-board.git" );


MDW_EVENT eventOf( MDW_EVENT_TYPE aType, const wxString& aItem, const wxString& aPayload = wxString() )
{
    MDW_EVENT event;
    event.type = aType;
    event.source = PROJECT;
    event.item_id = aItem;
    event.path_hint = wxS( "amp-board.kicad_pro" );
    event.payload = aPayload;
    return event;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MdwEventNotices )


// A claim is state the pill and tree show live, so only losing my own claim is news
BOOST_AUTO_TEST_CASE( ClaimEventsReachTheBellOnlyWhenMyClaimWasReleased )
{
    // Another user's claim is not actionable
    MDW_EVENT locked = eventOf( MDW_EVENT_TYPE::ITEM_LOCKED, wxS( "amp-board.kicad_pro" ),
                                wxS( R"({"owner_id":"dave@example.com","owner_display_name":"Dave",)"
                                     R"("locked_by_me":false,"advisory":true})" ) );
    locked.client_identity = wxS( "Dave" );
    BOOST_CHECK( !MdwEventNotice( locked, PROJECT ) );

    // Neither is a release of someone else's claim
    MDW_EVENT theirs = eventOf( MDW_EVENT_TYPE::ITEM_UNLOCKED, wxS( "amp-board.kicad_pro" ),
                                wxS( R"({"owner_display_name":"Dave","locked_by_me":false,)"
                                     R"("released_by":"Dave","released_by_me":false})" ) );
    BOOST_CHECK( !MdwEventNotice( theirs, PROJECT ) );

    // Bob breaking my claim is
    MDW_EVENT mine = eventOf( MDW_EVENT_TYPE::ITEM_UNLOCKED, wxS( "amp-board.kicad_pro" ),
                              wxS( R"({"owner_display_name":"Alice Adams","locked_by_me":true,"released_by":"Bob",)"
                                   R"("released_by_me":false,"reason":"needs the board for the rev B respin"})" ) );

    std::optional<MDW_NOTICE> notice = MdwEventNotice( mine, PROJECT );

    BOOST_REQUIRE( notice );
    BOOST_CHECK( notice->message.Contains( wxS( "Bob" ) ) );
    BOOST_CHECK( notice->message.Contains( wxS( "rev B respin" ) ) );

    // Releasing my own claim from my other client is not
    MDW_EVENT myOwn = eventOf( MDW_EVENT_TYPE::ITEM_UNLOCKED, wxS( "amp-board.kicad_pro" ),
                               wxS( R"({"owner_display_name":"Alice Adams","locked_by_me":true,)"
                                    R"("released_by":"Alice Adams","released_by_me":true})" ) );
    BOOST_CHECK( !MdwEventNotice( myOwn, PROJECT ) );

    // An Odoo check out that vanished names nobody, since Odoo cannot say who cleared it
    MDW_EVENT gone = eventOf( MDW_EVENT_TYPE::ITEM_UNLOCKED, wxS( "amp-board.kicad_sch" ),
                              wxS( R"({"owner_display_name":"Bob","locked_by_me":true,"advisory":false,)"
                                   R"("released_by":"","released_by_me":false,"reason":""})" ) );

    std::optional<MDW_NOTICE> lost = MdwEventNotice( gone, PROJECT );

    BOOST_REQUIRE( lost );
    BOOST_CHECK( lost->message.Contains( wxS( "no longer held" ) ) );
}


// One item holds one notice, so a newer event replaces the older one instead of piling up beside it
BOOST_AUTO_TEST_CASE( OneItemSharesOneNoticeKey )
{
    MDW_EVENT checkedIn = eventOf( MDW_EVENT_TYPE::REVISION_CHECKED_IN, wxS( "amp-board.kicad_pro" ) );
    MDW_EVENT released = eventOf( MDW_EVENT_TYPE::ITEM_UNLOCKED, wxS( "amp-board.kicad_pro" ),
                                  wxS( R"({"owner_display_name":"Alice Adams","locked_by_me":true,)"
                                       R"("released_by":"Bob","released_by_me":false})" ) );
    MDW_EVENT lifecycle = eventOf( MDW_EVENT_TYPE::LIFECYCLE_CATEGORY_CHANGED, wxS( "amp-board.kicad_pro" ) );
    MDW_EVENT other = eventOf( MDW_EVENT_TYPE::REVISION_CHECKED_IN, wxS( "psu.kicad_pro" ) );

    std::optional<MDW_NOTICE> a = MdwEventNotice( checkedIn, PROJECT );
    std::optional<MDW_NOTICE> b = MdwEventNotice( released, PROJECT );
    std::optional<MDW_NOTICE> c = MdwEventNotice( lifecycle, PROJECT );
    std::optional<MDW_NOTICE> d = MdwEventNotice( other, PROJECT );

    BOOST_REQUIRE( a && b && c && d );
    BOOST_CHECK( a->key.StartsWith( wxS( "mdw-" ) ) );
    BOOST_CHECK_EQUAL( a->key, b->key );
    BOOST_CHECK_EQUAL( a->key, c->key );
    BOOST_CHECK( a->key != d->key );

    checkedIn.from_this_client = true;
    BOOST_CHECK( !MdwEventNotice( checkedIn, PROJECT ) );
}


// A team library is never the project's source, yet its holder must still hear that a claim was released
BOOST_AUTO_TEST_CASE( LibraryEventsReachTheBellOnlyForMyReleasedClaim )
{
    MDW_EVENT released = eventOf( MDW_EVENT_TYPE::ITEM_UNLOCKED, wxS( "passives.kicad_sym#R_TEST" ),
                                  wxS( R"({"owner_display_name":"Alice","locked_by_me":true,"released_by":"Bob",)"
                                       R"("released_by_me":false,"reason":"needs R_TEST for the board review"})" ) );
    released.source = wxS( "git:/srv/mdw/acme-library.git" );
    released.path_hint = wxS( "passives.kicad_sym#R_TEST" );

    MDW_EVENT checkedIn = released;
    checkedIn.type = MDW_EVENT_TYPE::REVISION_CHECKED_IN;

    for( const wxString& project : { PROJECT, wxString() } )
    {
        std::optional<MDW_NOTICE> notice = MdwEventNotice( released, project );

        BOOST_REQUIRE( notice );
        BOOST_CHECK( notice->message.Contains( wxS( "board review" ) ) );
        BOOST_CHECK( !MdwEventNotice( checkedIn, project ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
