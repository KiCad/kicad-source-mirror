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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <widgets/mdw_member_index.h>
#include <widgets/mdw_part_controller.h>


BOOST_AUTO_TEST_SUITE( MdwListedFactsTests )


namespace
{

// Content digests of two releases of inductor L
const wxString REV1 = wxS( "v1:inductor-10u" );
const wxString REV2 = wxS( "v1:inductor-22u" );

MDW_MEMBER_RELEASE release( const wxString& aLabel, const wxString& aDigest,
                            MDW_LIFECYCLE_CATEGORY aState = MDW_LIFECYCLE_CATEGORY::RELEASED, bool aPublished = true )
{
    MDW_MEMBER_RELEASE rel;
    rel.revision_id = wxS( "rev-" ) + aLabel;
    rel.label = aLabel;
    rel.content_digest = aDigest;
    rel.state.category = aState;
    rel.published = aPublished;
    return rel;
}


// A listed library holding the member L with the given releases
struct LISTED_L
{
    explicit LISTED_L( std::vector<MDW_MEMBER_RELEASE> aReleases )
    {
        MDW_MEMBER_ENTRY entry;
        entry.member.name = wxS( "L" );
        entry.member.releases = std::move( aReleases );
        members.emplace( wxS( "L" ), entry );
    }

    MDW_LIBRARY_LISTING Listing() const { return { true, &members }; }

    std::map<wxString, MDW_MEMBER_ENTRY> members;
};

kiapi::mdw::PlacedPartStatus only( const std::optional<MDW_PART_FACTS>& aFacts )
{
    BOOST_REQUIRE( aFacts );
    BOOST_REQUIRE_EQUAL( aFacts->statuses.size(), 1 );
    return aFacts->statuses[0];
}

} // namespace


// A footprint library the session never listed must not count as released
BOOST_AUTO_TEST_CASE( UnlistedFootprintIsUnverifiedNotReleased )
{
    std::optional<MDW_PART_FACTS> facts = MdwListedFacts( { true, nullptr }, wxS( "L" ), REV1 );

    BOOST_CHECK( only( facts ) == kiapi::mdw::PPS_UNVERIFIED );
    BOOST_CHECK( !facts->message.IsEmpty() );
}


BOOST_AUTO_TEST_CASE( NonTeamLibraryHasNoFacts )
{
    BOOST_CHECK( !MdwListedFacts( {}, wxS( "L" ), REV1 ) );
}


// A member a listed library lacks was never checked in, which is not "not listed yet"
BOOST_AUTO_TEST_CASE( MemberMissingFromListedLibraryIsNotCheckedIn )
{
    LISTED_L library( { release( wxS( "1" ), REV1 ) } );

    std::optional<MDW_PART_FACTS> facts = MdwListedFacts( library.Listing(), wxS( "C" ), REV1 );

    BOOST_CHECK( only( facts ) == kiapi::mdw::PPS_UNKNOWN );
    BOOST_CHECK_EQUAL( facts->message, wxS( "Not checked in" ) );
}


BOOST_AUTO_TEST_CASE( ListedWithoutReleaseIsNeverReleased )
{
    LISTED_L library( {} );

    BOOST_CHECK( only( MdwListedFacts( library.Listing(), wxS( "L" ), REV1 ) ) == kiapi::mdw::PPS_NEVER_RELEASED );
}


// A part placed from Rev 1 after Rev 2 exists is outdated, as Release Design says, and not unreleased
BOOST_AUTO_TEST_CASE( OlderReleaseIsOutdatedAndKnowsItsRevision )
{
    LISTED_L library( { release( wxS( "1" ), REV1 ), release( wxS( "2" ), REV2 ) } );

    std::optional<MDW_PART_FACTS> facts = MdwListedFacts( library.Listing(), wxS( "L" ), REV1 );

    BOOST_CHECK( only( facts ) == kiapi::mdw::PPS_OUTDATED );
    BOOST_CHECK_EQUAL( facts->matched_revision_label, wxS( "1" ) );
    BOOST_CHECK_EQUAL( facts->latest_revision_label, wxS( "2" ) );

    BOOST_CHECK( only( MdwListedFacts( library.Listing(), wxS( "L" ), REV2 ) ) == kiapi::mdw::PPS_RELEASED );
    BOOST_CHECK( only( MdwListedFacts( library.Listing(), wxS( "L" ), wxS( "v1:other" ) ) )
                 == kiapi::mdw::PPS_DIFFERS );
}


// A placed part with no digest knows nothing of itself, so it cannot pass as the newest release
BOOST_AUTO_TEST_CASE( PlacedPartWithoutDigestIsUnknown )
{
    LISTED_L library( { release( wxS( "1" ), REV1 ), release( wxS( "2" ), REV2 ) } );

    std::optional<MDW_PART_FACTS> facts = MdwListedFacts( library.Listing(), wxS( "L" ), wxEmptyString );

    BOOST_CHECK( only( facts ) == kiapi::mdw::PPS_UNKNOWN );
    BOOST_CHECK_EQUAL( facts->message, wxS( "The placed part carries no content digest" ) );
}


// A tampered newest release has no digest, and the message must not blame the part
BOOST_AUTO_TEST_CASE( TamperedNewestReleaseIsNotBlamedOnThePart )
{
    LISTED_L library( { release( wxS( "1" ), REV1 ), release( wxS( "2" ), wxEmptyString ) } );

    std::optional<MDW_PART_FACTS> facts = MdwListedLatestFacts( library.Listing(), wxS( "L" ) );

    BOOST_CHECK( only( facts ) == kiapi::mdw::PPS_UNKNOWN );
    BOOST_CHECK_EQUAL( facts->message, wxS( "The newest release carries no content digest" ) );
}


// A part only chosen stands for the newest release
BOOST_AUTO_TEST_CASE( ChosenPartIsWeighedAsTheNewestRelease )
{
    LISTED_L library( { release( wxS( "1" ), REV1 ), release( wxS( "2" ), REV2 ) } );

    std::optional<MDW_PART_FACTS> facts = MdwListedLatestFacts( library.Listing(), wxS( "L" ) );

    BOOST_CHECK( only( facts ) == kiapi::mdw::PPS_RELEASED );
    BOOST_CHECK_EQUAL( facts->matched_revision_label, wxS( "2" ) );
}


BOOST_AUTO_TEST_CASE( StateAndPublicationOfTheMatchedReleaseCount )
{
    LISTED_L obsolete( { release( wxS( "1" ), REV1, MDW_LIFECYCLE_CATEGORY::RESTRICTED ) } );
    BOOST_CHECK( only( MdwListedFacts( obsolete.Listing(), wxS( "L" ), REV1 ) ) == kiapi::mdw::PPS_NOT_USABLE );

    LISTED_L local( { release( wxS( "1" ), REV1, MDW_LIFECYCLE_CATEGORY::RELEASED, false ) } );
    BOOST_CHECK( only( MdwListedFacts( local.Listing(), wxS( "L" ), REV1 ) ) == kiapi::mdw::PPS_UNPUBLISHED );
}


// A symbol release asks before pointing at a footprint nobody released or could check, and only notes the rest
BOOST_AUTO_TEST_CASE( ReleaseAsksOnlyForNeverReleasedOrUncheckedFootprints )
{
    auto asks = [&]( const MDW_LIBRARY_LISTING& aListing, const wxString& aName )
    {
        std::optional<MDW_PART_FACTS> facts = MdwListedLatestFacts( aListing, aName );

        BOOST_REQUIRE( facts );
        return MdwReleaseNeedsConfirmation( *facts );
    };

    LISTED_L never( {} );
    LISTED_L current( { release( wxS( "1" ), REV1 ), release( wxS( "2" ), REV2 ) } );
    LISTED_L obsolete( { release( wxS( "1" ), REV1, MDW_LIFECYCLE_CATEGORY::RESTRICTED ) } );

    BOOST_CHECK( asks( { true, nullptr }, wxS( "L" ) ) );
    BOOST_CHECK( asks( never.Listing(), wxS( "L" ) ) );
    BOOST_CHECK( asks( current.Listing(), wxS( "C" ) ) );
    BOOST_CHECK( !asks( current.Listing(), wxS( "L" ) ) );
    BOOST_CHECK( !asks( obsolete.Listing(), wxS( "L" ) ) );
}


// A footprint the listed library lacks, with no file standing in for it
BOOST_AUTO_TEST_CASE( MissingFootprintIsNamedMissing )
{
    LISTED_L library( { release( wxS( "1" ), REV1 ) } );

    std::optional<MDW_PART_FACTS> facts = MdwListedFacts( library.Listing(), wxS( "TO-92" ), wxEmptyString );

    BOOST_REQUIRE( facts );
    BOOST_CHECK( MdwReleaseNeedsConfirmation( *facts ) );
    BOOST_CHECK_EQUAL( MdwFootprintFactText( *facts, false ), wxS( "which does not exist" ) );
    BOOST_CHECK( MdwFootprintFactText( *facts, true ).Contains( wxS( "Not checked in" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
