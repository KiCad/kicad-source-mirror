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

#include "mdw_git_library_fixture.h"

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_lifecycle.pb.h>
#include <api/mdw/backend_locking.pb.h>
#include <api/mdw/backend_revisions.pb.h>
#include <api/mdw/backend_shared_libs.pb.h>
#include <diff_merge/kicad_diff_types.h>
#include <gestfich.h>
#include <git/git_library_members.h>
#include <git/kigit_symbol_split.h>
#include <mdw/client/mdw_op.h>
#include <widgets/mdw_part_controller.h>

#include <json_common.h>

#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>

namespace fs = std::filesystem;
namespace mdw = kiapi::mdw;


namespace
{

const std::string ICS = "ics.kicad_sym";


std::map<std::string, mdw::LibraryMember> listMembers( MDW_BACKEND& aBackend )
{
    mdw::ListMembersRequest request;
    mdw::ListMembersReply   reply;
    request.set_library_id( LIBRARY );

    MDW_RESULT result = aBackend.CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::map<std::string, mdw::LibraryMember> members;

    for( const mdw::LibraryMember& member : reply.members() )
        members[member.member_id()] = member;

    BOOST_REQUIRE_EQUAL( members.size(), 2u );
    return members;
}


/// aText with every line ending as aEnd, CRLF as a save on Windows writes it or LF as the blob holds it
std::string lineEnds( const std::string& aText, const std::string& aEnd )
{
    std::string out;

    for( size_t ii = 0; ii < aText.size(); ++ii )
    {
        if( aText[ii] == '\r' && ii + 1 < aText.size() && aText[ii + 1] == '\n' )
            continue;

        if( aText[ii] == '\n' )
            out += aEnd;
        else
            out += aText[ii];
    }

    return out;
}


void checkIn( const MDW_GIT_LIBRARIAN& aLibrarian, const std::string& aId, const std::string& aIdempotency )
{
    const kiapi::workspace::WorkspaceError error = tryCheckIn( aLibrarian, aId, aIdempotency );
    BOOST_REQUIRE_MESSAGE( error.code() == kiapi::workspace::WE_OK, error.message() );
}


/// How aLibrarian's copy lists member aId among its local changes, if at all
std::optional<mdw::ChangeType> listedChange( const MDW_GIT_LIBRARIAN& aLibrarian, const std::string& aId )
{
    mdw::ListLocalChangesRequest request;
    mdw::ListLocalChangesReply   reply;
    request.set_local_root( aLibrarian.Copy().string() );

    MDW_RESULT result = aLibrarian.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    for( const mdw::LocalChange& change : reply.changes() )
    {
        if( change.member().item_id() == aId )
            return change.change();
    }

    return std::nullopt;
}


/// How many references of aRepo match aGlob
size_t refCount( git_repository* aRepo, const char* aGlob = "refs/kicad/kept/*" )
{
    size_t count = 0;

    git_reference_foreach_glob(
            aRepo, aGlob,
            []( const char*, void* aCount )
            {
                ++*static_cast<size_t*>( aCount );
                return 0;
            },
            &count );

    return count;
}


/// Alice and Bob, each with a copy of the passives library
class TWO_LIBRARIANS : public ONE_LIBRARIAN
{
public:
    TWO_LIBRARIANS()
    {
        m_bob = Connect( wxS( "bob" ) );

        m_bob.SyncOk();

        m_bob.SetAuthor( "bob" );
    }

    /// Alice releases L, edits it again while Bob checks in its keywords, and keeps her version as she gets his
    void keepMine()
    {
        const fs::path aliceFile = m_alice.Copy() / LIBRARY;

        mdw::ReleaseRevisionReply released;

        edit( aliceFile, "Inductor", "Inductor, 10uH" );
        MDW_RESULT result = release( *m_alice.backend, aliceFile, L, "", "v1:inductor-10u", released );
        BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

        MDW_LIBRARY_SYNC_RESULT updated = m_bob.SyncOk();

        edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
        claim( m_bob, L );
        checkIn( m_bob, L, "bob-keywords" );

        edit( aliceFile, "Inductor, 10uH", "Inductor, 22uH" );
        updated = m_alice.SyncOk();
        BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );

        const MDW_UPDATE_CONFLICT& conflict = updated.conflicts.front();

        updated = m_alice.Sync( { { conflict.item.item_id, conflict.theirs_revision_id, std::nullopt,
                                    conflict.ours_token } } );
        BOOST_REQUIRE_MESSAGE( updated.result.IsOk(), updated.result.message );
        BOOST_REQUIRE( updated.conflicts.empty() );
    }

    MDW_GIT_LIBRARIAN m_bob;
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitRelease )


// The listing names a member's release, never its last commit, so an unreleased part reads unreleased
BOOST_FIXTURE_TEST_CASE( ListingNamesOnlyReleases, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    MDW_BACKEND&                              backend = *librarian.backend;
    const fs::path                            file = librarian.Copy() / LIBRARY;
    std::map<std::string, mdw::LibraryMember> before = listMembers( backend );

    for( const auto& [id, member] : before )
    {
        BOOST_TEST_CONTEXT( id )
        {
            BOOST_CHECK_EQUAL( member.revision_id(), "" );
            BOOST_CHECK_EQUAL( member.revision_label(), "" );
            BOOST_CHECK_EQUAL( member.release_digest(), "" );
            BOOST_CHECK_EQUAL( member.releases_size(), 0 );
            BOOST_CHECK( !member.version().empty() );
        }
    }

    mdw::ReleaseRevisionReply released;
    MDW_RESULT                result = release( backend, file, C, "", "v1:capacitor", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::map<std::string, mdw::LibraryMember> after = listMembers( backend );

    BOOST_CHECK_EQUAL( after[C].revision_id(), released.revision().revision_id() );
    BOOST_CHECK_EQUAL( after[C].revision_label(), "1" );
    BOOST_CHECK_EQUAL( after[C].release_digest(), "v1:capacitor" );
    BOOST_CHECK_EQUAL( after[C].version(), before[C].version() );
    BOOST_CHECK_EQUAL( after[L].revision_id(), "" );
    BOOST_CHECK_EQUAL( after[L].version(), before[L].version() );

    // An edit released from the listing names the release it started from, none for L
    edit( file, "Inductor", "Inductor, 10uH" );
    result = release( backend, file, L, after[L].revision_id(), "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::map<std::string, mdw::LibraryMember> edited = listMembers( backend );

    BOOST_CHECK_EQUAL( edited[L].revision_id(), released.revision().revision_id() );
    BOOST_CHECK_EQUAL( edited[L].version(), released.revision().revision_id() );
    BOOST_CHECK_NE( edited[L].version(), before[L].version() );

    edit( file, "Inductor, 10uH", "Inductor, 22uH" );
    result = release( backend, file, L, edited[L].revision_id(), "v1:inductor-22u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    // Placed parts are weighed against every release, so an outdated one is not read as unreleased
    std::map<std::string, mdw::LibraryMember> twice = listMembers( backend );

    BOOST_REQUIRE_EQUAL( twice[L].releases_size(), 2 );
    BOOST_CHECK_EQUAL( twice[L].releases( 0 ).label(), "1" );
    BOOST_CHECK_EQUAL( twice[L].releases( 0 ).content_digest(), "v1:inductor-10u" );
    BOOST_CHECK_EQUAL( twice[L].releases( 1 ).label(), "2" );
    BOOST_CHECK_EQUAL( twice[L].releases( 1 ).content_digest(), "v1:inductor-22u" );
    BOOST_CHECK_EQUAL( twice[C].releases_size(), 1 );

    // Rev 1 is superseded, so an edit from it would publish over Rev 2 unseen
    edit( file, "Inductor, 22uH", "Inductor, 47uH" );
    result = release( backend, file, L, edited[L].revision_id(), "v1:inductor-47u", released );
    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
}


// A teammate's release of C moves the branch, which must not refuse a release of L in the same file
BOOST_FIXTURE_TEST_CASE( ReleaseOfOneSymbolSurvivesTeammateReleaseOfAnotherInSameFile, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;
    const fs::path bobFile = m_bob.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( aliceFile, C_FILTERS, "C? Capacitor*" );
    MDW_RESULT result = release( *m_alice.backend, aliceFile, C, "", "v1:capacitor-filters", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    edit( bobFile, "Inductor", "Inductor, 10uH" );
    result = release( *m_bob.backend, bobFile, L, "", "v1:inductor-10u", released );
    BOOST_CHECK_MESSAGE( result.error == MDW_ERROR_CODE::COPY_BEHIND, result.message );

    MDW_LIBRARY_SYNC_RESULT updated = m_bob.SyncOk();
    BOOST_REQUIRE( updated.conflicts.empty() );

    result = release( *m_bob.backend, bobFile, L, "", "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK_EQUAL( released.revision().label(), "1" );

    const std::string shared = Upstream( LIBRARY );
    BOOST_CHECK( shared.find( "\"C? Capacitor*\"" ) != std::string::npos );
    BOOST_CHECK( shared.find( "\"Inductor, 10uH\"" ) != std::string::npos );
}


// A check-in since the release is what the copy holds, so an edit made after syncing it releases
BOOST_FIXTURE_TEST_CASE( ReleaseAfterSyncUsesTheCopyAsBase, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;
    const fs::path bobFile = m_bob.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( aliceFile, "Inductor", "Inductor, 10uH" );
    MDW_RESULT result = release( *m_alice.backend, aliceFile, L, "", "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    MDW_LIBRARY_SYNC_RESULT updated = m_bob.SyncOk();

    edit( bobFile, L_KEYWORDS, "inductor choke" );
    claim( m_bob, L );
    checkIn( m_bob, L, "bob-keywords" );

    updated = m_alice.SyncOk();
    BOOST_REQUIRE( updated.conflicts.empty() );

    edit( aliceFile, "Inductor, 10uH", "Inductor, 22uH" );
    std::map<std::string, mdw::LibraryMember> listed = listMembers( *m_alice.backend );

    result = release( *m_alice.backend, aliceFile, L, listed[L].revision_id(), "v1:inductor-22u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK_EQUAL( released.revision().label(), "2" );

    const std::string shared = Upstream( LIBRARY );
    BOOST_CHECK( shared.find( "\"Inductor, 22uH\"" ) != std::string::npos );
    BOOST_CHECK( shared.find( "\"inductor choke\"" ) != std::string::npos );
}


// A copy behind would be updated and released again, publishing over the first librarian's edit
BOOST_FIXTURE_TEST_CASE( SameMemberEditedByTwoLibrariansIsAConflict, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;
    const fs::path bobFile = m_bob.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( aliceFile, "Inductor", "Inductor, 10uH" );
    edit( bobFile, "Inductor", "Inductor, 22uH" );

    MDW_RESULT result = release( *m_alice.backend, aliceFile, L, "", "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    result = release( *m_bob.backend, bobFile, L, "", "v1:inductor-22u", released );
    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( result.message.Contains( wxS( "alice" ) ), result.message );

    // Naming Alice's release, as Bob's listing does once it heard of it, still leaves his edit begun before it
    std::map<std::string, mdw::LibraryMember> listed = listMembers( *m_bob.backend );
    BOOST_REQUIRE( !listed[L].revision_id().empty() );

    result = release( *m_bob.backend, bobFile, L, listed[L].revision_id(), "v1:inductor-22u", released );
    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( result.message.Contains( wxS( "alice" ) ), result.message );
    BOOST_CHECK( Upstream( LIBRARY ).find( "\"Inductor, 10uH\"" ) != std::string::npos );
}


// Content edited before a check-in that changed it would publish over that check-in unseen
BOOST_FIXTURE_TEST_CASE( StaleEditAfterPostReleaseCheckInIsRefused, TWO_LIBRARIANS )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( file, "Inductor", "Inductor, 10uH" );
    MDW_RESULT result = release( *m_alice.backend, file, L, "", "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    // An editor still holding Rev 1 edits it while Bob checks in L's keywords
    edit( file, "Inductor, 10uH", "Inductor, 47uH" );
    const std::string stale = ReadFile( file );

    edit( file, "Inductor, 47uH", "Inductor, 10uH" );
    BOOST_REQUIRE( m_bob.Sync().result.IsOk() );
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    claim( m_bob, L );
    checkIn( m_bob, L, "bob-keywords" );
    BOOST_REQUIRE( m_alice.Sync().result.IsOk() );

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << stale;
    std::map<std::string, mdw::LibraryMember> listed = listMembers( *m_alice.backend );

    result = release( *m_alice.backend, file, L, listed[L].revision_id(), "v1:inductor-47u", released );
    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( result.message.Contains( wxS( "bob" ) ), result.message );

    const std::string shared = Upstream( LIBRARY );
    BOOST_CHECK( shared.find( "\"inductor choke\"" ) != std::string::npos );
    BOOST_CHECK( shared.find( "\"Inductor, 47uH\"" ) == std::string::npos );
}


// A member never released has no release to judge from, yet a stale edit must not publish over a check-in
BOOST_FIXTURE_TEST_CASE( StaleEditOfUnreleasedMemberIsRefused, TWO_LIBRARIANS )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( file, "Inductor", "Inductor, 47uH" );
    const std::string stale = ReadFile( file );

    edit( file, "Inductor, 47uH", "Inductor" );
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );
    BOOST_REQUIRE( m_alice.Sync().result.IsOk() );

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << stale;

    MDW_RESULT result = release( *m_alice.backend, file, L, "", "v1:inductor-47u", released );
    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( result.message.Contains( wxS( "bob" ) ), result.message );

    const std::string shared = Upstream( LIBRARY );
    BOOST_CHECK( shared.find( "\"inductor choke\"" ) != std::string::npos );
    BOOST_CHECK( shared.find( "\"Inductor, 47uH\"" ) == std::string::npos );
}


// Keeping my version over a teammate's check-in must not leave the release refused while the copy holds theirs
BOOST_FIXTURE_TEST_CASE( KeepMineSyncThenReleaseSucceeds, TWO_LIBRARIANS )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    keepMine();

    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, m_alice.Copy().string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr   repoPtr( repo );
    mdw::ReleaseRevisionReply released;

    BOOST_REQUIRE_EQUAL( refCount( repo ), 1u );

    std::map<std::string, mdw::LibraryMember> listed = listMembers( *m_alice.backend );
    MDW_RESULT result = release( *m_alice.backend, file, L, listed[L].revision_id(), "v1:inductor-22u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK_EQUAL( released.revision().label(), "2" );

    const std::string shared = Upstream( LIBRARY );
    BOOST_CHECK( shared.find( "\"Inductor, 22uH\"" ) != std::string::npos );
    BOOST_CHECK( shared.find( "\"inductor choke\"" ) == std::string::npos );

    // The answer is this copy's alone and spent by the release
    BOOST_CHECK_EQUAL( refCount( repo ), 0u );
    BOOST_CHECK_EQUAL( refCount( m_upstreamRepo.get() ), 0u );
}


// A kept answer covers the version it was given against, not a check-in made after it
BOOST_FIXTURE_TEST_CASE( KeptAnswerDoesNotCoverALaterCheckIn, TWO_LIBRARIANS )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    keepMine();

    const std::string mine = ReadFile( file );
    const std::string theirs = Upstream( LIBRARY );

    edit( m_bob.Copy() / LIBRARY, "Choke_* *Coil* Inductor_* L_*", "L_*" );
    claim( m_bob, L );
    checkIn( m_bob, L, "bob-filters" );

    mdw::ReleaseRevisionReply                 released;
    std::map<std::string, mdw::LibraryMember> listed = listMembers( *m_alice.backend );
    MDW_RESULT result = release( *m_alice.backend, file, L, listed[L].revision_id(), "v1:inductor-22u", released );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( result.message.Contains( wxS( "bob" ) ), result.message );

    // Her edit dropped, the update brings Bob's check-in without a conflict, and a stale editor saves hers again
    std::ofstream( file, std::ios::binary | std::ios::trunc ) << theirs;
    BOOST_REQUIRE( m_alice.Sync().result.IsOk() );
    std::ofstream( file, std::ios::binary | std::ios::trunc ) << mine;

    listed = listMembers( *m_alice.backend );
    result = release( *m_alice.backend, file, L, listed[L].revision_id(), "v1:inductor-22u", released );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( result.message.Contains( wxS( "bob" ) ), result.message );
    BOOST_CHECK( Upstream( LIBRARY ).find( "\"Inductor, 22uH\"" ) == std::string::npos );
}


// Behind the team in the same file, a member nobody else touched is behind rather than in conflict
BOOST_FIXTURE_TEST_CASE( StaleCheckInIsCopyBehind, TWO_LIBRARIANS )
{
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );

    edit( m_alice.Copy() / LIBRARY, C_FILTERS, "C_*" );

    const std::string head = m_alice.Head();

    BOOST_CHECK_EQUAL( tryCheckIn( m_alice, C, "alice-filters" ).code(), kiapi::workspace::WE_COPY_BEHIND );
    BOOST_CHECK_EQUAL( m_alice.Head(), head );
    BOOST_CHECK( listedChange( m_alice, C ) == mdw::CT_MODIFIED );
}


// A part only saved back here takes a teammate's check-in without a conflict
BOOST_FIXTURE_TEST_CASE( ResavedPartTakesTeammatesCheckIn, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;
    std::string    resaved = ReadFile( aliceFile );

    for( size_t at = resaved.find( '\t' ); at != std::string::npos; at = resaved.find( '\t', at + 2 ) )
        resaved.replace( at, 1, "  " );

    std::ofstream( aliceFile, std::ios::binary | std::ios::trunc ) << resaved;

    edit( m_bob.Copy() / LIBRARY, C_FILTERS, "C_*" );
    checkIn( m_bob, C, "bob-filters" );

    MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
    BOOST_REQUIRE( updated.conflicts.empty() );
    BOOST_CHECK( ReadFile( aliceFile ).find( "\"C_*\"" ) != std::string::npos );
}


// Uncommenting a line only moves a line break, and it is still an edit
BOOST_FIXTURE_TEST_CASE( UncommentedLineConflictsWithTeammatesCheckIn, TWO_LIBRARIANS )
{
    const std::string jumpers = "(duplicate_pin_numbers_are_jumpers yes)";

    auto replace = []( const fs::path& aFile, const std::string& aFrom, const std::string& aTo )
    {
        std::string  text = ReadFile( aFile );
        const size_t at = text.find( aFrom );

        BOOST_REQUIRE( at != std::string::npos );
        text.replace( at, aFrom.size(), aTo );
        std::ofstream( aFile, std::ios::binary | std::ios::trunc ) << text;
    };

    replace( Source() / LIBRARY, "(duplicate_pin_numbers_are_jumpers no)", "# " + jumpers );
    Publish( wxS( "Comment out C's jumpers" ) );
    m_alice.SyncOk();
    m_bob.SyncOk();

    replace( m_alice.Copy() / LIBRARY, "# " + jumpers, "#\n\t\t\t\t" + jumpers );
    edit( m_bob.Copy() / LIBRARY, C_FILTERS, "C_*" );
    checkIn( m_bob, C, "bob-filters" );

    BOOST_CHECK_EQUAL( m_alice.SyncOk().conflicts.size(), 1u );
}


/// The text of top-level symbol aName in the .kicad_sym text aText, as the client reads a member
std::string memberOf( const std::string& aText, const std::string& aName )
{
    SCOPED_TEMP_PATH folder;
    BOOST_REQUIRE( folder.MakeDirectory( wxS( "mdw-member" ) ) );

    const fs::path      file = fs::path( folder.Path().utf8_string() ) / LIBRARY;
    MDW_UPDATE_CONFLICT member;

    member.name = wxString::FromUTF8( aName );
    member.relative_path = wxString::FromUTF8( LIBRARY );
    std::ofstream( file, std::ios::binary ) << aText;

    std::optional<std::string> text = MDW_SETUP::ReadMember( wxString::FromUTF8( file.string() ), member );
    BOOST_REQUIRE( text );
    return *text;
}


/// aText with its top-level symbol aName replaced by the one aFrom holds
std::string withSymbolOf( std::string aText, const std::string& aName, const std::string& aFrom )
{
    const std::string old = memberOf( aText, aName );
    const size_t      at = aText.find( old );

    BOOST_REQUIRE( at != std::string::npos );
    aText.replace( at, old.size(), memberOf( aFrom, aName ) );
    return aText;
}


// A part the answer changed here is counted as an update, though the copy had its own edit of it
BOOST_FIXTURE_TEST_CASE( TakenTeamVersionCountsAsAChangedPart, TWO_LIBRARIANS )
{
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 22uH" );
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );

    MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
    BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );

    // The answer file holds only the member, as the conflict prompt writes it
    const std::string bobs = ReadFile( m_bob.Copy() / LIBRARY );
    const size_t      start = bobs.find( "\t(symbol \"L\"\n" );
    const size_t      next = bobs.find( "\n\t(symbol ", start + 1 );
    const size_t      end = next == std::string::npos ? bobs.rfind( ')' ) : next + 1;

    BOOST_REQUIRE( start != std::string::npos && end > start );

    const std::string answer = bobs.substr( 0, bobs.find( "\t(symbol " ) ) + bobs.substr( start, end - start ) + ")\n";
    const MDW_UPDATE_CONFLICT& conflict = updated.conflicts.front();

    updated = m_alice.SyncOk( { { conflict.item.item_id, conflict.theirs_revision_id, answer, conflict.ours_token } } );
    BOOST_REQUIRE( updated.conflicts.empty() );
    BOOST_CHECK_EQUAL( updated.changes.size(), 1u );
}


// A part answered under another format's header would read differently spliced into the copy's file
BOOST_FIXTURE_TEST_CASE( AnswerInAnotherFormatIsRefused, TWO_LIBRARIANS )
{
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 22uH" );
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );

    MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
    BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );

    const std::string upgraded = ReadFile( Seed().parent_path() / "acme-library-upgraded" / LIBRARY );
    const std::string answer = upgraded.substr( 0, upgraded.find( "\t(symbol " ) )
                               + memberOf( ReadFile( m_bob.Copy() / LIBRARY ), "L" ) + "\n)\n";
    const MDW_UPDATE_CONFLICT& conflict = updated.conflicts.front();
    const std::string          before = ReadFile( m_alice.Copy() / LIBRARY );

    updated = m_alice.Sync( { { conflict.item.item_id, conflict.theirs_revision_id, answer, conflict.ours_token } } );
    BOOST_CHECK( updated.result.error == MDW_ERROR_CODE::INVALID_PARAMETER );
    BOOST_CHECK( ReadFile( m_alice.Copy() / LIBRARY ) == before );
}


/// Send aLibrarian's update answering each of aConflicts with aWhole, their whole file made from aBuiltFrom
mdw::UpdateSharedLibrariesReply answerWhole( const MDW_GIT_LIBRARIAN&                aLibrarian,
                                             const std::vector<MDW_UPDATE_CONFLICT>& aConflicts,
                                             const std::string& aWhole, const std::string& aBuiltFrom )
{
    mdw::UpdateSharedLibrariesRequest request;
    mdw::UpdateSharedLibrariesReply   reply;

    request.set_local_root( aLibrarian.Copy().string() );
    request.set_idempotency_id( "whole-" + KIGIT::BlobDigest( aWhole ) );

    for( const MDW_UPDATE_CONFLICT& conflict : aConflicts )
    {
        mdw::ConflictResolution* resolution = request.add_resolutions();

        resolution->set_item_id( conflict.item.item_id.utf8_string() );
        resolution->set_theirs_revision_id( conflict.theirs_revision_id.utf8_string() );
        resolution->set_merged( aWhole );
        resolution->set_ours_token( conflict.ours_token.utf8_string() );
        resolution->set_whole_file_of( KIGIT::BlobDigest( aBuiltFrom ) );
    }

    MDW_RESULT result = aLibrarian.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    return reply;
}


/// Alice's L conflicting with Bob's, which he checked in with an edit of C, and the merge of both L edits
class BOB_EDITED_C_TOO : public TWO_LIBRARIANS
{
public:
    BOB_EDITED_C_TOO()
    {
        edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 22uH" );
        edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
        edit( m_bob.Copy() / LIBRARY, C_FILTERS, "C_*" );
        checkIn( m_bob, L, "bob-keywords" );
        checkIn( m_bob, C, "bob-filters" );

        MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
        BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );

        m_conflict = updated.conflicts.front();
        m_alices = ReadFile( m_alice.Copy() / LIBRARY );
        m_mergedL = m_temp.Path() / LIBRARY;

        fs::copy_file( m_bob.Copy() / LIBRARY, m_mergedL );
        edit( m_mergedL, "Inductor", "Inductor, 22uH" );
    }

    MDW_UPDATE_CONFLICT m_conflict;
    std::string         m_alices;
    fs::path            m_mergedL; ///< Bob's file with Alice's L edit merged into his
};


// A teammate's edit elsewhere in the file of a whole-file answer is placed into it, at the same format
BOOST_FIXTURE_TEST_CASE( WholeFileAnswerKeepsTeammatesOtherEdit, BOB_EDITED_C_TOO )
{
    const std::string answer = withSymbolOf( m_alices, "L", ReadFile( m_mergedL ) );

    BOOST_CHECK_EQUAL( answerWhole( m_alice, { m_conflict }, answer, m_alices ).conflicts_size(), 0 );

    const std::string text = ReadFile( m_alice.Copy() / LIBRARY );

    BOOST_CHECK( text.find( "\"C_*\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "\"Inductor, 22uH\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "\"inductor choke\"" ) != std::string::npos );
}


// Under the newer format of a whole-file answer, a teammate's older-format edit is asked for, never dropped
BOOST_FIXTURE_TEST_CASE( WholeFileAnswerAsksForTeammatesOlderEdit, BOB_EDITED_C_TOO )
{
    const std::string upgraded = ReadFile( Seed().parent_path() / "acme-library-upgraded" / LIBRARY );
    const std::string answer = withSymbolOf( upgraded, "L", ReadFile( m_mergedL ) );

    mdw::UpdateSharedLibrariesReply reply = answerWhole( m_alice, { m_conflict }, answer, m_alices );

    BOOST_REQUIRE_EQUAL( reply.conflicts_size(), 1 );
    BOOST_CHECK_EQUAL( reply.conflicts( 0 ).item().item_id(), C );
    BOOST_CHECK( ReadFile( m_alice.Copy() / LIBRARY ) == m_alices );
}


/// Alice's C and L each conflicting with Bob's, and his file with both parts merged with hers
class BOTH_PARTS_CONFLICT : public TWO_LIBRARIANS
{
public:
    BOTH_PARTS_CONFLICT()
    {
        edit( m_alice.Copy() / LIBRARY, C_FILTERS, "C_0402*" );
        edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 22uH" );
        edit( m_bob.Copy() / LIBRARY, C_FILTERS, "C_*" );
        edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
        checkIn( m_bob, C, "bob-filters" );
        checkIn( m_bob, L, "bob-keywords" );

        MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
        BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 2u );

        m_conflicts = updated.conflicts;
        m_alices = ReadFile( m_alice.Copy() / LIBRARY );
        m_merged = m_temp.Path() / LIBRARY;

        fs::copy_file( m_bob.Copy() / LIBRARY, m_merged );
        edit( m_merged, "C_*", "C_0402* C_*" );
        edit( m_merged, "Inductor", "Inductor, 22uH" );
    }

    std::vector<MDW_UPDATE_CONFLICT> m_conflicts; ///< C's, then L's
    std::string                      m_alices;
    fs::path                         m_merged;
};


// Taking one part of a whole-file answer must not make its file's other answer look saved since
BOOST_FIXTURE_TEST_CASE( WholeFileAnswersInOneFileAllTakeEffect, BOTH_PARTS_CONFLICT )
{
    BOOST_CHECK_EQUAL( answerWhole( m_alice, m_conflicts, ReadFile( m_merged ), m_alices ).conflicts_size(), 0 );

    const std::string text = ReadFile( m_alice.Copy() / LIBRARY );

    BOOST_CHECK( text.find( "\"C_0402* C_*\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "\"Inductor, 22uH\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "\"inductor choke\"" ) != std::string::npos );
}


// A whole file answering C leaves out L, which the same request answers deleted
BOOST_FIXTURE_TEST_CASE( WholeFileAnswerLeavesOutAPartAnsweredDeleted, BOTH_PARTS_CONFLICT )
{
    std::string       answer = ReadFile( m_merged );
    const std::string l = "\n\t" + memberOf( answer, "L" );

    BOOST_REQUIRE( answer.find( l ) != std::string::npos );
    answer.erase( answer.find( l ), l.size() );

    BOOST_CHECK_EQUAL( answerWhole( m_alice, m_conflicts, answer, m_alices ).conflicts_size(), 0 );

    const std::string text = ReadFile( m_alice.Copy() / LIBRARY );

    BOOST_CHECK( text.find( "\"C_0402* C_*\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "(symbol \"L\"" ) == std::string::npos );
}


// A whole-file answer speaks for every part in its file, so a save made after it was built makes it stale
BOOST_FIXTURE_TEST_CASE( WholeFileAnswerMadeBeforeASaveIsStale, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;

    edit( aliceFile, "Inductor", "Inductor, 22uH" );
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );

    MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
    BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );

    const std::string alices = ReadFile( aliceFile );
    const std::string answer = withSymbolOf( alices, "L", ReadFile( m_bob.Copy() / LIBRARY ) );

    edit( aliceFile, C_FILTERS, "C_*" );

    BOOST_CHECK_EQUAL( answerWhole( m_alice, { updated.conflicts.front() }, answer, alices ).conflicts_size(), 1 );
    BOOST_CHECK( ReadFile( aliceFile ).find( "\"C_*\"" ) != std::string::npos );
}


// A teammate's part spliced under this copy's newer header would read differently, so the client converts it
BOOST_FIXTURE_TEST_CASE( OlderFormatPartIsAConflictInANewerFile, TWO_LIBRARIANS )
{
    const fs::path upgraded = Seed().parent_path() / "acme-library-upgraded" / LIBRARY;
    const fs::path bobFile = m_bob.Copy() / LIBRARY;

    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 22uH" );
    checkIn( m_alice, L, "alice-inductor" );

    fs::copy_file( upgraded, bobFile, fs::copy_options::overwrite_existing );

    MDW_LIBRARY_SYNC_RESULT updated = m_bob.SyncOk();

    BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );
    BOOST_CHECK( updated.conflicts.front().item.item_id == wxString::FromUTF8( L ) );
    BOOST_CHECK( ReadFile( bobFile ) == ReadFile( upgraded ) );
}


// A footprint check-in behind the team commits nothing, so the edit stays listed
BOOST_FIXTURE_TEST_CASE( StaleFootprintCheckInIsCopyBehind, TWO_LIBRARIANS )
{
    const std::string name = "R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal";
    const std::string file = "fp_resistors.pretty/" + name + ".kicad_mod";
    const std::string axial = "fp_resistors.pretty#" + name;
    const fs::path    pads = fs::path( KI_TEST::GetTestDataRootDir() ) / "pcbnew/exchange_pad_changes.pretty";

    fs::create_directories( ( Source() / file ).parent_path() );
    fs::copy_file( pads / ( name + ".kicad_mod" ), Source() / file );
    Publish( wxS( "Add the axial footprint" ) );
    m_alice.SyncOk();
    m_bob.SyncOk();

    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );

    const fs::path footprint = m_alice.Copy() / file;
    std::string    text = ReadFile( footprint );
    const size_t   at = text.find( "(at 10.16 0)" );

    BOOST_REQUIRE( at != std::string::npos );
    text.replace( at, 12, "(at 10.16 1.27)" );
    std::ofstream( footprint, std::ios::binary | std::ios::trunc ) << text;

    const std::string head = m_alice.Head();

    BOOST_CHECK_EQUAL( tryCheckIn( m_alice, axial, "alice-pad" ).code(), kiapi::workspace::WE_COPY_BEHIND );
    BOOST_CHECK_EQUAL( m_alice.Head(), head );
    BOOST_CHECK( listedChange( m_alice, axial ) == mdw::CT_MODIFIED );
}


// Two edits of one member conflict by naming the teammate, and Alice keeps no commit
BOOST_FIXTURE_TEST_CASE( CheckInOverTeammateEditNamesTheTeammate, TWO_LIBRARIANS )
{
    edit( m_bob.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );
    checkIn( m_bob, L, "bob-keywords" );

    edit( m_alice.Copy() / LIBRARY, L_KEYWORDS, "inductor coil" );

    const std::string                      head = m_alice.Head();
    const kiapi::workspace::WorkspaceError refused = tryCheckIn( m_alice, L, "alice-keywords" );

    BOOST_CHECK_EQUAL( refused.code(), kiapi::workspace::WE_VERSION_CONFLICT );
    BOOST_CHECK_MESSAGE( refused.message().find( "bob" ) != std::string::npos, refused.message() );
    BOOST_CHECK_EQUAL( m_alice.Head(), head );
    BOOST_CHECK( Upstream( LIBRARY ).find( "\"inductor coil\"" ) == std::string::npos );
}


// A release pushes only its tag, so a copy that has not fetched it must still refuse an unclaimed edit
BOOST_FIXTURE_TEST_CASE( CheckInOfAMemberReleasedSinceIsDenied, TWO_LIBRARIANS )
{
    mdw::ReleaseRevisionReply released;
    MDW_RESULT result = release( *m_bob.backend, m_bob.Copy() / LIBRARY, L, "", "v1:inductor", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    edit( m_alice.Copy() / LIBRARY, L_KEYWORDS, "inductor choke" );

    const std::string head = m_alice.Head();

    BOOST_CHECK_EQUAL( tryCheckIn( m_alice, L, "alice-keywords" ).code(), kiapi::workspace::WE_PERMISSION_DENIED );
    BOOST_CHECK_EQUAL( m_alice.Head(), head );
    BOOST_CHECK( listedChange( m_alice, L ) == mdw::CT_MODIFIED );
}


// A state tag makes a released part obsolete, and a teammate reads it back
BOOST_FIXTURE_TEST_CASE( ObsoleteStateTagIsReadBack, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( aliceFile, C_FILTERS, "C? Capacitor*" );
    MDW_RESULT result = release( *m_alice.backend, aliceFile, C, "", "v1:capacitor-filters", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    mdw::TransitionLifecycleRequest request;
    mdw::TransitionLifecycleReply   reply;
    request.set_item_id( C );
    request.set_target_state_id( "obsolete" );
    request.set_comment( "Replaced by C_Small" );
    request.set_idempotency_id( "alice-obsoletes-c" );

    result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK( reply.state().category() == kiapi::workspace::LC_RESTRICTED );

    m_bob.SyncOk();

    std::map<std::string, mdw::LibraryMember> members = listMembers( *m_bob.backend );

    BOOST_CHECK_EQUAL( members[C].state().state_id(), "obsolete" );
    BOOST_CHECK( members[C].state().category() == kiapi::workspace::LC_RESTRICTED );
    BOOST_CHECK_EQUAL( members[C].revision_label(), "1" );
    BOOST_CHECK( members[L].state().category() == kiapi::workspace::LC_EDITABLE );

    mdw::GetAvailableTransitionsRequest available;
    mdw::GetAvailableTransitionsReply   transitions;
    available.set_item_id( C );

    result = m_bob.backend->CallSync( available, transitions, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::set<std::string> targets;

    for( const kiapi::workspace::LifecycleTransition& transition : transitions.transitions() )
        targets.insert( transition.target_state_id() );

    BOOST_CHECK( targets == std::set<std::string>( { "released", "not_for_new_designs" } ) );
}


/// Move aLibrarian's newest release of C to aState under the retry key aOperation
MDW_RESULT transitionC( const MDW_GIT_LIBRARIAN& aLibrarian, const std::string& aState, const std::string& aOperation )
{
    mdw::TransitionLifecycleRequest request;
    mdw::TransitionLifecycleReply   reply;
    request.set_item_id( C );
    request.set_target_state_id( aState );
    request.set_idempotency_id( aOperation );

    return aLibrarian.backend->CallSync( request, reply, 30000 );
}


// The design check reads a release's state, so an obsolete part is not usable
BOOST_FIXTURE_TEST_CASE( ObsoletePartIsNotUsableInDesignCheck, TWO_LIBRARIANS )
{
    mdw::ReleaseRevisionReply released;

    edit( m_alice.Copy() / LIBRARY, C_FILTERS, "C? Capacitor*" );
    MDW_RESULT result = release( *m_alice.backend, m_alice.Copy() / LIBRARY, C, "", "v1:capacitor-filters", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    result = transitionC( m_alice, "obsolete", "alice-obsoletes-c" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    mdw::CheckPlacedPartsRequest check;
    mdw::CheckPlacedPartsReply   facts;
    mdw::PlacedPart*             placed = check.add_parts();
    placed->mutable_item()->set_item_id( C );
    placed->set_content_digest( "v1:capacitor-filters" );

    result = m_bob.backend->CallSync( check, facts, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_REQUIRE_EQUAL( facts.facts_size(), 1 );
    BOOST_CHECK_EQUAL( facts.facts( 0 ).matched_state().state_id(), "obsolete" );

    const auto& statuses = facts.facts( 0 ).statuses();
    BOOST_CHECK( std::find( statuses.begin(), statuses.end(), mdw::PPS_NOT_USABLE ) != statuses.end() );
}


// Not for New Designs stays usable, so the design check warns instead of passing it as released
BOOST_FIXTURE_TEST_CASE( NotForNewDesignsPartIsFlaggedInDesignCheck, TWO_LIBRARIANS )
{
    mdw::ReleaseRevisionReply released;

    edit( m_alice.Copy() / LIBRARY, C_FILTERS, "C? Capacitor*" );
    MDW_RESULT result = release( *m_alice.backend, m_alice.Copy() / LIBRARY, C, "", "v1:capacitor-filters", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    result = transitionC( m_alice, "not_for_new_designs", "alice-nrnd-c" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    mdw::CheckPlacedPartsRequest check;
    mdw::CheckPlacedPartsReply   facts;
    mdw::PlacedPart*             placed = check.add_parts();
    placed->mutable_item()->set_item_id( C );
    placed->set_content_digest( "v1:capacitor-filters" );

    result = m_bob.backend->CallSync( check, facts, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_REQUIRE_EQUAL( facts.facts_size(), 1 );

    const auto& statuses = facts.facts( 0 ).statuses();
    BOOST_CHECK( std::vector<int>( statuses.begin(), statuses.end() )
                 == std::vector<int>{ mdw::PPS_NOT_FOR_NEW_DESIGNS } );
}


// Two librarians moving one release each take their own sequence
BOOST_FIXTURE_TEST_CASE( ConcurrentStateChangesTakeTheNextSequence, TWO_LIBRARIANS )
{
    mdw::ReleaseRevisionReply released;

    edit( m_alice.Copy() / LIBRARY, C_FILTERS, "C? Capacitor*" );
    MDW_RESULT result = release( *m_alice.backend, m_alice.Copy() / LIBRARY, C, "", "v1:capacitor-filters", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    result = transitionC( m_alice, "obsolete", "alice-obsoletes-c" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    // Bob's copy has never seen Alice's tag, so his own numbering would reuse hers
    result = transitionC( m_bob, "not_for_new_designs", "bob-nrnd-c" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK_EQUAL( refCount( m_upstreamRepo.get(), "refs/tags/kicad/state/*" ), 2u );

    m_alice.SyncOk();
    BOOST_CHECK_EQUAL( listMembers( *m_alice.backend )[C].state().state_id(), "not_for_new_designs" );
}


BOOST_FIXTURE_TEST_CASE( TeammateClaimShowsInItemInfo, TWO_LIBRARIANS )
{
    claim( m_alice, L );

    // Bob restarts, so his backend asks the remote instead of answering from the claims his set-up read
    Reconnect( m_bob );

    mdw::GetItemInfoRequest request;
    mdw::GetItemInfoReply   reply;
    request.set_item_id( L );

    MDW_RESULT result = m_bob.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_CHECK( reply.item().lock().is_locked() );
    BOOST_CHECK( !reply.item().lock().locked_by_me() );
}


/// The text of aFile at the copy's HEAD
std::string headText( const fs::path& aCopy, const std::string& aFile )
{
    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, aCopy.string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_object* blob = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &blob, repo, ( "HEAD:" + aFile ).c_str() ), GIT_OK );
    KIGIT::GitObjectPtr blobPtr( blob );

    return std::string( static_cast<const char*>( git_blob_rawcontent( reinterpret_cast<git_blob*>( blob ) ) ),
                        git_blob_rawsize( reinterpret_cast<git_blob*>( blob ) ) );
}


/// Whether aFile at the copy's HEAD is what the copy stores for aOnDisk, line endings filtered as git does
bool headHolds( const fs::path& aCopy, const std::string& aFile, const fs::path& aOnDisk )
{
    return KIGIT::BlobDigest( headText( aCopy, aFile ) )
           == MDW_GIT_LIBRARY_FIXTURE::StoredDigest( aCopy, aFile, aOnDisk );
}


/// Files with a modified entry in the copy's worktree
size_t modifiedFiles( const fs::path& aCopy )
{
    git_repository* repo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &repo, aCopy.string().c_str() ), GIT_OK );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_status_list* status = nullptr;
    BOOST_REQUIRE_EQUAL( git_status_list_new( &status, repo, nullptr ), GIT_OK );

    size_t count = git_status_list_entrycount( status );
    git_status_list_free( status );
    return count;
}


/// Alice with a copy of ics.kicad_sym and passives.kicad_sym, and the ics file as a save on another machine writes
/// it, with the same symbols at a different indent
class REFORMATTED_ICS : public MDW_GIT_LIBRARY_FIXTURE
{
public:
    REFORMATTED_ICS()
    {
        fs::copy_file( Seed() / ICS, Source() / ICS );
        fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
        Publish( wxS( "Import acme-library" ) );
        m_alice = Connect( wxS( "alice" ) );

        m_alice.SyncOk();

        m_alice.SetAuthor( "alice" );

        std::string text = ReadFile( Seed() / ICS );

        for( size_t at = text.find( '\t' ); at != std::string::npos; at = text.find( '\t', at + 2 ) )
            text.replace( at, 1, "  " );

        m_reformatted = m_temp.Path() / ICS;
        std::ofstream( m_reformatted, std::ios::binary ) << text;
    }

    /// Save the reformatted file over alice's copy of it
    void reformat() const
    {
        fs::copy_file( m_reformatted, m_alice.Copy() / ICS, fs::copy_options::overwrite_existing );
    }

    MDW_GIT_LIBRARIAN m_alice;
    fs::path          m_reformatted;
};


/// A check-in naming aWhole, each file with the digest of the bytes on disk unless aDigests overrides it
mdw::CheckInSharedLibraryRequest checkInWhole( const MDW_GIT_LIBRARIAN& aLibrarian, const std::set<std::string>& aWhole,
                                               const std::vector<std::string>& aItems = {} )
{
    mdw::CheckInSharedLibraryRequest request;
    request.set_local_root( aLibrarian.Copy().string() );
    request.set_comment( "Adopt re-saved files" );
    request.set_idempotency_id( "whole-" + std::to_string( aWhole.size() ) + std::to_string( aItems.size() ) );

    for( const std::string& id : aItems )
        request.add_item_ids( id );

    for( const std::string& file : aWhole )
        ( *request.mutable_whole_files() )[file] =
                KIGIT::BlobDigest( MDW_GIT_LIBRARY_FIXTURE::ReadFile( aLibrarian.Copy() / file ) );

    return request;
}


// A re-save on another OS rewrites the file but no symbol, so the check-in adopts the file as it is
BOOST_FIXTURE_TEST_CASE( CheckInAdoptsReformattedFile, REFORMATTED_ICS )
{
    reformat();

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { ICS } );
    mdw::CheckInSharedLibraryReply   reply;

    MDW_RESULT result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, m_reformatted ) );
    BOOST_CHECK_EQUAL( modifiedFiles( m_alice.Copy() ), 0u );
}


// Another file's edit and an adopted file go in one commit, each as its own rules say
BOOST_FIXTURE_TEST_CASE( WholeFileRidesWithAnotherFilesEdit, REFORMATTED_ICS )
{
    reformat();
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { ICS }, { L } );
    mdw::CheckInSharedLibraryReply   reply;

    MDW_RESULT result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, m_reformatted ) );
    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ).find( "Inductor, 10uH" ) != std::string::npos );
    BOOST_CHECK_EQUAL( modifiedFiles( m_alice.Copy() ), 0u );
}


// Ticking only a formatting row must not check in the edits left unticked beside it
BOOST_FIXTURE_TEST_CASE( WholeFileAloneLeavesUntickedEditsAlone, REFORMATTED_ICS )
{
    reformat();
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { ICS } );
    mdw::CheckInSharedLibraryReply   reply;

    MDW_RESULT result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, m_reformatted ) );
    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ).find( "Inductor, 10uH" ) == std::string::npos );
    BOOST_CHECK_EQUAL( modifiedFiles( m_alice.Copy() ), 1u );
}


// A file edited after the dialog listed it is not the file the user saw
BOOST_FIXTURE_TEST_CASE( WholeFileRefusedWhenItChangedSinceListed, REFORMATTED_ICS )
{
    reformat();

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { ICS } );
    std::ofstream( m_alice.Copy() / ICS, std::ios::binary | std::ios::app ) << "\n";

    mdw::CheckInSharedLibraryReply reply;
    MDW_RESULT                     result = m_alice.backend->CallSync( request, reply, 30000 );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, Seed() / ICS ) );
}


// A refused push of files adopted whole is an error, or the adoption would read as done
BOOST_FIXTURE_TEST_CASE( WholeFileRefusedPushIsAnError, REFORMATTED_ICS )
{
    edit( Source() / LIBRARY, L_KEYWORDS, "inductor choke" );
    Publish( wxS( "L: fewer keywords" ) );
    reformat();

    const std::string              head = m_alice.Head();
    mdw::CheckInSharedLibraryReply reply;
    MDW_RESULT result = m_alice.backend->CallSync( checkInWhole( m_alice, { ICS } ), reply, 30000 );

    BOOST_CHECK( !result.IsOk() );
    BOOST_CHECK_EQUAL( m_alice.Head(), head );
    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, Seed() / ICS ) );
}


// An edit to a released member takes the release rules, which a re-save cannot skip by being named whole
BOOST_FIXTURE_TEST_CASE( WholeFileWithAReleasedMemberIsNotAdopted, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;

    mdw::ReleaseRevisionReply released;

    edit( aliceFile, "Inductor", "Inductor, 10uH" );
    MDW_RESULT result = release( *m_alice.backend, aliceFile, L, "", "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    const std::string before = headText( m_alice.Copy(), LIBRARY );

    edit( aliceFile, "Inductor, 10uH", "Inductor, 22uH" );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { LIBRARY }, { L } );
    mdw::CheckInSharedLibraryReply   reply;

    result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ) == before );
    BOOST_REQUIRE_EQUAL( reply.skipped_files_size(), 1 );
    BOOST_CHECK_EQUAL( reply.skipped_files( 0 ).path(), LIBRARY );
    BOOST_CHECK( !reply.skipped_files( 0 ).reason().empty() );
}


/// A differ that reads a symbol as edited when its text holds EDITED, standing in for the symbol differ on a
/// re-saved file
KIGIT::LIBRARY_DIFF editedWordDiff()
{
    return []( const std::string&, const std::string& aAfter, KICAD_DIFF::DOCUMENT_DIFF* aDiff )
    {
        if( aAfter.find( "EDITED" ) != std::string::npos )
            aDiff->changes.emplace_back();

        return true;
    };
}


/// What the check-in dialog lists for aLibrarian's copy
MDW_LIBRARY_CHANGES_RESULT listChanges( const MDW_GIT_LIBRARIAN& aLibrarian, const wxString& aMarker )
{
    aLibrarian.session->Probe( 30000 );
    BOOST_REQUIRE( aLibrarian.source->Info().connection.phase == MDW_CONNECTION::ONLINE );

    return MDW_SETUP::LibraryChangesFor( aLibrarian.source, *aLibrarian.sources->Copy( aMarker ), editedWordDiff() );
}


// A file with only reformatted symbols lists as one row that adopts it, and a check-in of that row alone commits it
BOOST_FIXTURE_TEST_CASE( LibraryListsReformattedFileAsOneRow, REFORMATTED_ICS )
{
    reformat();

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );
    BOOST_REQUIRE_EQUAL( listed.changes.size(), 1u );
    BOOST_CHECK_EQUAL( listed.changes[0].relative_path, wxString::FromUTF8( ICS ) );
    BOOST_CHECK_EQUAL( listed.changes[0].format_digest,
                       wxString::FromUTF8( KIGIT::BlobDigest( ReadFile( m_reformatted ) ) ) );

    MDW_LIBRARY_CHECKIN_RESULT checkedIn = MDW_SETUP::CheckInLibraryCopy(
            *m_alice.sources, m_alice.source, *m_alice.sources->Copy( marker ), listed.changes, wxS( "Adopt" ) );

    BOOST_REQUIRE_MESSAGE( checkedIn.result.IsOk(), checkedIn.result.message );
    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, m_reformatted ) );
    BOOST_CHECK_EQUAL( modifiedFiles( m_alice.Copy() ), 0u );
    BOOST_CHECK( m_alice.sources->Copy( marker )->snapshot.token == checkedIn.checkin.copy_snapshot.token );
}


// A file with a real edit lists through its members, never as a file adopted whole
BOOST_FIXTURE_TEST_CASE( LibraryDoesNotListAnEditedFileAsFormatOnly, REFORMATTED_ICS )
{
    std::string  text = ReadFile( m_reformatted );
    const size_t at = text.find( "\"24C16\"" );

    BOOST_REQUIRE( at != std::string::npos );
    text.replace( at, 7, "\"24C16 EDITED\"" );
    std::ofstream( m_alice.Copy() / ICS, std::ios::binary | std::ios::trunc ) << text;

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );

    bool edited = false;

    for( const MDW_LOCAL_CHANGE& change : listed.changes )
    {
        BOOST_CHECK( change.format_digest.IsEmpty() );
        edited |= change.name == wxS( "24C16" );
    }

    BOOST_CHECK( edited );
}


/// The reformatted ics file as saved after adding R_TEST, a copy of 24C16
std::string withNewPart( const fs::path& aReformatted )
{
    std::string  text = MDW_GIT_LIBRARY_FIXTURE::ReadFile( aReformatted );
    const size_t from = text.find( "(symbol \"24C16\"" );
    const size_t end = text.rfind( ')' );

    BOOST_REQUIRE( from != std::string::npos );

    std::string part = text.substr( from, end - from );

    for( size_t at = part.find( "\"24C16" ); at != std::string::npos; at = part.find( "\"24C16", at ) )
        part.replace( at + 1, 5, "R_TEST" );

    return text.insert( end, "  " + part );
}


// The first save rewrote the whole file, so a new part's check-in takes the re-save along and leaves nothing behind
BOOST_FIXTURE_TEST_CASE( LibraryCheckInTakesTheFilesResaveAlong, REFORMATTED_ICS )
{
    std::ofstream( m_alice.Copy() / ICS, std::ios::binary | std::ios::trunc ) << withNewPart( m_reformatted );

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );
    BOOST_REQUIRE_EQUAL( listed.changes.size(), 1u );
    BOOST_CHECK_EQUAL( listed.changes[0].name, wxS( "R_TEST" ) );

    MDW_LIBRARY_CHECKIN_RESULT checkedIn = MDW_SETUP::CheckInLibraryCopy(
            *m_alice.sources, m_alice.source, *m_alice.sources->Copy( marker ), listed.changes, wxS( "Add R_TEST" ) );

    BOOST_REQUIRE_MESSAGE( checkedIn.result.IsOk(), checkedIn.result.message );
    BOOST_REQUIRE_EQUAL( checkedIn.checkin.results.size(), 1u );
    BOOST_CHECK( checkedIn.checkin.results[0].status.IsOk() );
    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, m_alice.Copy() / ICS ) );
    BOOST_CHECK( listChanges( m_alice, marker ).changes.empty() );
}


// An edit left unticked keeps its file from going whole, or it would go in with the ticked part
BOOST_FIXTURE_TEST_CASE( LibraryCheckInLeavesAnUntickedEditInTheFile, REFORMATTED_ICS )
{
    std::string  text = withNewPart( m_reformatted );
    const size_t at = text.find( "\"Value\" \"24C16\"" );

    BOOST_REQUIRE( at != std::string::npos );
    text.insert( at + 14, " EDITED" );
    std::ofstream( m_alice.Copy() / ICS, std::ios::binary | std::ios::trunc ) << text;

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );

    std::erase_if( listed.changes,
                   []( const MDW_LOCAL_CHANGE& aChange )
                   {
                       return aChange.name != wxS( "R_TEST" );
                   } );

    BOOST_REQUIRE_EQUAL( listed.changes.size(), 1u );

    MDW_LIBRARY_CHECKIN_RESULT checkedIn = MDW_SETUP::CheckInLibraryCopy(
            *m_alice.sources, m_alice.source, *m_alice.sources->Copy( marker ), listed.changes, wxS( "Add R_TEST" ) );

    BOOST_REQUIRE_MESSAGE( checkedIn.result.IsOk(), checkedIn.result.message );

    const std::string head = headText( m_alice.Copy(), ICS );

    BOOST_CHECK( head.find( "R_TEST" ) != std::string::npos );
    BOOST_CHECK( head.find( "EDITED" ) == std::string::npos );
}


/// The format the upgraded passives fixture names
const long UPGRADED_FORMAT = 20261106;


/// An upgrade of aLibrarian's passives file, read as it is now, to aAfter
MDW_FORMAT_UPGRADE passivesUpgrade( const MDW_GIT_LIBRARIAN& aLibrarian, const std::string& aAfter )
{
    const fs::path     file = aLibrarian.Copy() / LIBRARY;
    const wxString     path = wxString::FromUTF8( file.string() );
    MDW_FORMAT_UPGRADE upgrade;

    upgrade.library = path;
    upgrade.before[path] = MDW_GIT_LIBRARY_FIXTURE::ReadFile( file );
    upgrade.after[path] = aAfter;
    return upgrade;
}


/// Check in aUpgrade as a KiCad whose current format is aVersion does
MDW_LIBRARY_CHECKIN_RESULT checkInUpgrade( const MDW_GIT_LIBRARIAN& aLibrarian, const MDW_FORMAT_UPGRADE& aUpgrade,
                                           long aVersion = UPGRADED_FORMAT )
{
    aLibrarian.session->Probe( 30000 );

    return MDW_SETUP::CheckInFormatUpgrade(
            *aLibrarian.sources, aLibrarian.source,
            *aLibrarian.sources->Copy( MDW_LIBRARY_SOURCES::Marker( aLibrarian.config ) ), aUpgrade, aVersion,
            wxS( "Upgrade passives.kicad_sym" ) );
}


/// The passives file as the format upgrade saves it
std::string upgradedPassives()
{
    return MDW_GIT_LIBRARY_FIXTURE::ReadFile( fs::path( KI_TEST::GetTestDataRootDir() )
                                              / "common/git/acme-library-upgraded" / LIBRARY );
}


// A format upgrade rewrites every symbol but revises none, so a teammate takes it cleanly and releases stand
BOOST_FIXTURE_TEST_CASE( FormatUpgradeKeepsRevisionsAndReleases, TWO_LIBRARIANS )
{
    MDW_RESULT result = release( *m_alice.backend, m_alice.Copy() / LIBRARY, C, "v1:capacitor" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    m_bob.SyncOk();

    std::map<std::string, mdw::LibraryMember> before = listMembers( *m_bob.backend );
    MDW_LIBRARY_CHECKIN_RESULT upgrade = checkInUpgrade( m_alice, passivesUpgrade( m_alice, upgradedPassives() ) );

    BOOST_REQUIRE_MESSAGE( upgrade.result.IsOk(), upgrade.result.message );
    BOOST_REQUIRE( headText( m_alice.Copy(), LIBRARY ) == upgradedPassives() );
    BOOST_CHECK( m_bob.SyncOk().conflicts.empty() );

    std::map<std::string, mdw::LibraryMember> after = listMembers( *m_bob.backend );

    BOOST_CHECK_EQUAL( after[C].version(), before[C].version() );
    BOOST_CHECK_EQUAL( after[L].version(), before[L].version() );
    BOOST_CHECK_EQUAL( after[C].revision_label(), "1" );
    BOOST_CHECK( after[C].state().category() == kiapi::workspace::LC_RELEASED );

    edit( m_bob.Copy() / LIBRARY, "Inductor", "Inductor EDITED" );

    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_bob, MDW_LIBRARY_SOURCES::Marker( m_bob.config ) );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );
    BOOST_REQUIRE_EQUAL( listed.changes.size(), 1u );
    BOOST_CHECK( listed.changes[0].member.item_id == wxString::FromUTF8( L ) );
}


// A save made while the upgrade was staged is not written over, and nothing is checked in
BOOST_FIXTURE_TEST_CASE( FormatUpgradeKeepsAnEditSavedMeanwhile, ONE_LIBRARIAN )
{
    const MDW_FORMAT_UPGRADE upgrade = passivesUpgrade( m_alice, upgradedPassives() );
    const std::string        head = m_alice.Head();

    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 10uH" );

    const std::string saved = ReadFile( m_alice.Copy() / LIBRARY );

    BOOST_CHECK( checkInUpgrade( m_alice, upgrade ).result.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK( ReadFile( m_alice.Copy() / LIBRARY ) == saved );
    BOOST_CHECK_EQUAL( m_alice.Head(), head );
}


// An older KiCad cannot pass its own format off as an upgrade of a team file already in a newer one
BOOST_FIXTURE_TEST_CASE( FormatUpgradeCannotLowerTheFormat, ONE_LIBRARIAN )
{
    MDW_LIBRARY_CHECKIN_RESULT upgrade = checkInUpgrade( m_alice, passivesUpgrade( m_alice, upgradedPassives() ) );
    BOOST_REQUIRE_MESSAGE( upgrade.result.IsOk(), upgrade.result.message );

    const std::string seed = ReadFile( Seed() / LIBRARY );

    upgrade = checkInUpgrade( m_alice, passivesUpgrade( m_alice, seed ), 20241209 );

    BOOST_CHECK( upgrade.result.message.Contains( wxS( "newer file format than this KiCad writes" ) ) );
    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ) == upgradedPassives() );
}


// An upgrade the team library refused is taken back off the disk, or it would sit there as an edit
BOOST_FIXTURE_TEST_CASE( FormatUpgradeRefusedIsPutBack, ONE_LIBRARIAN )
{
    edit( Source() / LIBRARY, L_KEYWORDS, "inductor choke" );
    Publish( wxS( "L: fewer keywords" ) );

    const std::string original = ReadFile( m_alice.Copy() / LIBRARY );

    BOOST_CHECK( !checkInUpgrade( m_alice, passivesUpgrade( m_alice, upgradedPassives() ) ).result.IsOk() );
    BOOST_CHECK( ReadFile( m_alice.Copy() / LIBRARY ) == original );
}


// A file the save left in the old format, as in a folder whose last file was current, is no upgrade
BOOST_FIXTURE_TEST_CASE( FormatUpgradeNeedsEveryFileCurrent, ONE_LIBRARIAN )
{
    const std::string original = ReadFile( m_alice.Copy() / LIBRARY );

    BOOST_CHECK( !checkInUpgrade( m_alice, passivesUpgrade( m_alice, original ) ).result.IsOk() );
}


// A comment cannot pass an edit off as a format upgrade, which history would not date the edit by
BOOST_FIXTURE_TEST_CASE( CommentCannotNameAFormatUpgrade, ONE_LIBRARIAN )
{
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryRequest request;
    mdw::CheckInSharedLibraryReply   reply;
    request.set_local_root( m_alice.Copy().string() );
    request.add_item_ids( L );
    request.set_comment( "Edit description\nKiCad-Format-Upgrade: yes\nMore text" );
    request.set_idempotency_id( "spoofed-upgrade" );

    BOOST_CHECK( m_alice.backend->CallSync( request, reply, 30000 ).error == MDW_ERROR_CODE::INVALID_PARAMETER );
}


// A comment cannot name an edited member as only reformatted, which history would then not date by the edit
BOOST_FIXTURE_TEST_CASE( CommentCannotNameAReformattedMember, ONE_LIBRARIAN )
{
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryRequest request;
    mdw::CheckInSharedLibraryReply   reply;
    request.set_local_root( m_alice.Copy().string() );
    request.add_item_ids( L );
    request.set_comment( "Edit description\n\nKiCad-Reformatted: " + L );
    request.set_idempotency_id( "spoofed-reformat" );

    BOOST_CHECK( m_alice.backend->CallSync( request, reply, 30000 ).error == MDW_ERROR_CODE::INVALID_PARAMETER );
}


// History trusts a format upgrade to revise no symbol, so one that adds a symbol is refused
BOOST_FIXTURE_TEST_CASE( FormatUpgradeRefusesAnAddedSymbol, REFORMATTED_ICS )
{
    std::ofstream( m_alice.Copy() / ICS, std::ios::binary | std::ios::trunc ) << withNewPart( m_reformatted );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { ICS } );
    mdw::CheckInSharedLibraryReply   reply;
    request.set_format_upgrade( true );

    MDW_RESULT result = m_alice.backend->CallSync( request, reply, 30000 );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::INVALID_PARAMETER );
    BOOST_CHECK( headHolds( m_alice.Copy(), ICS, Seed() / ICS ) );
}


// A save in a newer format that edited every symbol of an older file leaves no symbol only reformatted, and the
// file still goes in whole with its edits
BOOST_FIXTURE_TEST_CASE( LibraryCheckInTakesANewerFormatWithEverySymbolEdited, ONE_LIBRARIAN )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << upgradedPassives();
    edit( file, C_FILTERS, "C_* EDITED" );
    edit( file, L_KEYWORDS, "inductor EDITED" );

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );
    BOOST_REQUIRE_EQUAL( listed.changes.size(), 2u );

    MDW_LIBRARY_CHECKIN_RESULT checkedIn = MDW_SETUP::CheckInLibraryCopy(
            *m_alice.sources, m_alice.source, *m_alice.sources->Copy( marker ), listed.changes, wxS( "Edit C and L" ) );

    BOOST_REQUIRE_MESSAGE( checkedIn.result.IsOk(), checkedIn.result.message );

    for( const MDW_MEMBER_RESULT& result : checkedIn.checkin.results )
        BOOST_CHECK_MESSAGE( result.status.IsOk(), result.status.message );

    BOOST_CHECK( headHolds( m_alice.Copy(), LIBRARY, file ) );
}


// A whole-file check-in only reformats a released part beside another part's edit, which leaves it released
BOOST_FIXTURE_TEST_CASE( WholeFileCheckInKeepsAReformattedRelease, ONE_LIBRARIAN )
{
    const fs::path file = m_alice.Copy() / LIBRARY;
    MDW_RESULT     result = release( *m_alice.backend, file, C, "v1:capacitor" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::string text = ReadFile( file );

    for( size_t at = text.find( '\t' ); at != std::string::npos; at = text.find( '\t', at + 2 ) )
        text.replace( at, 1, "  " );

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << text;
    edit( file, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryReply reply;
    result = m_alice.backend->CallSync( checkInWhole( m_alice, { LIBRARY }, { L } ), reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    BOOST_CHECK_EQUAL( reply.skipped_files_size(), 0 );
    BOOST_CHECK( headHolds( m_alice.Copy(), LIBRARY, file ) );
    BOOST_CHECK( listMembers( *m_alice.backend )[C].state().category() == kiapi::workspace::LC_RELEASED );
}


// A file saved in an older format than the team's upgraded one never goes in whole, even with every edit ticked
BOOST_FIXTURE_TEST_CASE( WholeFileInAnOlderFormatIsNotAdopted, ONE_LIBRARIAN )
{
    MDW_LIBRARY_CHECKIN_RESULT upgrade = checkInUpgrade( m_alice, passivesUpgrade( m_alice, upgradedPassives() ) );
    BOOST_REQUIRE_MESSAGE( upgrade.result.IsOk(), upgrade.result.message );

    const fs::path file = m_alice.Copy() / LIBRARY;
    fs::copy_file( Seed() / LIBRARY, file, fs::copy_options::overwrite_existing );
    edit( file, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryReply reply;
    MDW_RESULT result = m_alice.backend->CallSync( checkInWhole( m_alice, { LIBRARY }, { L } ), reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_REQUIRE_EQUAL( reply.results_size(), 1 );

    BOOST_CHECK( reply.results( 0 ).error().message().find( "older file format" ) != std::string::npos );
    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ) == upgradedPassives() );
}


// The refusal beside a released part must be followable whether that part was edited or only reformatted
BOOST_FIXTURE_TEST_CASE( RefusalBesideAReleasedPartAsksForARevise, ONE_LIBRARIAN )
{
    const fs::path file = m_alice.Copy() / LIBRARY;
    MDW_RESULT     result = release( *m_alice.backend, file, C, "v1:capacitor" );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << upgradedPassives();
    edit( file, "Inductor", "Inductor, 10uH" );

    const std::string why = tryCheckIn( m_alice, L, "beside-release" ).message();

    BOOST_CHECK( why.find( "Revise C, then" ) != std::string::npos );
}


// A release from a file newer than the team's points to checking that file in, as Upgrade is not offered for it
BOOST_FIXTURE_TEST_CASE( ReleaseFromANewerFileAsksForItsCheckIn, ONE_LIBRARIAN )
{
    const fs::path file = m_alice.Copy() / LIBRARY;
    std::ofstream( file, std::ios::binary | std::ios::trunc ) << upgradedPassives();

    const MDW_RESULT result = release( *m_alice.backend, file, C, "v1:capacitor" );

    BOOST_REQUIRE( !result.IsOk() );
    BOOST_CHECK( result.message.find( "Check in the changes to " + LIBRARY + " first." ) != std::string::npos );
}


/// The upgraded passives file with C renamed CAP and L renamed IND, which lists as two removed and two added rows
std::string renamedPassives()
{
    std::string text = upgradedPassives();

    // A symbol's units carry its name, so they are renamed with it
    auto rename = [&]( const std::string& aFrom, const std::string& aTo )
    {
        const std::string from = "(symbol \"" + aFrom;

        for( size_t at = text.find( from ); at != std::string::npos; at = text.find( from, at + 1 ) )
            text.replace( at, from.size(), "(symbol \"" + aTo );
    };

    rename( "C", "CAP" );
    rename( "L", "IND" );
    return text;
}


// Renaming every symbol leaves only removed and added rows, and the file's newer format still sends it whole
BOOST_FIXTURE_TEST_CASE( LibraryCheckInTakesANewerFormatWithEverySymbolRenamed, ONE_LIBRARIAN )
{
    const fs::path file = m_alice.Copy() / LIBRARY;

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << renamedPassives();

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );
    BOOST_REQUIRE_EQUAL( listed.changes.size(), 4u );

    MDW_LIBRARY_CHECKIN_RESULT checkedIn = MDW_SETUP::CheckInLibraryCopy(
            *m_alice.sources, m_alice.source, *m_alice.sources->Copy( marker ), listed.changes, wxS( "Rename" ) );

    BOOST_REQUIRE_MESSAGE( checkedIn.result.IsOk(), checkedIn.result.message );

    for( const MDW_MEMBER_RESULT& result : checkedIn.checkin.results )
        BOOST_CHECK_MESSAGE( result.status.IsOk(), result.status.message );

    BOOST_CHECK( headHolds( m_alice.Copy(), LIBRARY, file ) );
}


// The team upgraded past the save while its symbols kept their older revisions, so only the team's file can say the
// save is older and must not replace it
BOOST_FIXTURE_TEST_CASE( LibraryCheckInKeepsAFileTheTeamUpgradedPastTheSave, ONE_LIBRARIAN )
{
    MDW_LIBRARY_CHECKIN_RESULT upgrade = checkInUpgrade( m_alice, passivesUpgrade( m_alice, upgradedPassives() ) );
    BOOST_REQUIRE_MESSAGE( upgrade.result.IsOk(), upgrade.result.message );

    // A format between the one the symbols' revisions were saved in and the team's
    std::string  text = renamedPassives();
    const size_t at = text.find( "(version 20261106)" );

    BOOST_REQUIRE( at != std::string::npos );
    text.replace( at, 18, "(version 20250114)" );
    std::ofstream( m_alice.Copy() / LIBRARY, std::ios::binary | std::ios::trunc ) << text;

    const wxString             marker = MDW_LIBRARY_SOURCES::Marker( m_alice.config );
    MDW_LIBRARY_CHANGES_RESULT listed = listChanges( m_alice, marker );
    BOOST_REQUIRE_MESSAGE( listed.result.IsOk(), listed.result.message );
    BOOST_REQUIRE_EQUAL( listed.changes.size(), 4u );

    for( const MDW_LOCAL_CHANGE& change : listed.changes )
        BOOST_CHECK( change.file_digest.IsEmpty() );

    MDW_SETUP::CheckInLibraryCopy( *m_alice.sources, m_alice.source, *m_alice.sources->Copy( marker ), listed.changes,
                                   wxS( "Rename" ) );

    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ) == upgradedPassives() );
}


// Teammates on other KiCad builds each re-save the file's writer fields, which must not stop an update
BOOST_FIXTURE_TEST_CASE( ResavesByOtherBuildsKeepTheEdit, TWO_LIBRARIANS )
{
    const fs::path aliceFile = m_alice.Copy() / LIBRARY;
    const fs::path bobFile = m_bob.Copy() / LIBRARY;

    edit( aliceFile, "10.99", "10.1" );
    edit( aliceFile, C_FILTERS, "C_*" );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { LIBRARY }, { C } );
    mdw::CheckInSharedLibraryReply   reply;
    MDW_RESULT                       result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    edit( bobFile, "10.99", "10.0" );
    edit( bobFile, "Inductor", "Inductor, 22uH" );

    BOOST_REQUIRE( m_bob.SyncOk().conflicts.empty() );

    const std::string text = ReadFile( bobFile );

    BOOST_CHECK( text.find( "\"Inductor, 22uH\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "\"C_*\"" ) != std::string::npos );
    BOOST_CHECK( text.find( "(generator_version \"10.1\")" ) != std::string::npos );
}


// An answer holding upstream's bytes still reads by the copy's newer header, so upstream's older one cannot take it
BOOST_FIXTURE_TEST_CASE( AnswerWithUpstreamsBytesKeepsItsFormat, TWO_LIBRARIANS )
{
    const fs::path    aliceFile = m_alice.Copy() / LIBRARY;
    const fs::path    bobFile = m_bob.Copy() / LIBRARY;

    // KIGIT::FormatVersion lives in common, which this test does not link
    auto versionOf = []( const std::string& aText )
    {
        const size_t from = aText.find( "(version " ) + 9;
        return aText.substr( from, aText.find( ')', from ) - from );
    };

    const std::string newer = versionOf( ReadFile( Seed().parent_path() / "acme-library-upgraded" / LIBRARY ) );

    edit( bobFile, "10.99", "10.1" );
    edit( bobFile, L_KEYWORDS, "inductor choke" );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_bob, { LIBRARY }, { L } );
    mdw::CheckInSharedLibraryReply   reply;
    MDW_RESULT                       result = m_bob.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    // As a newer build re-saves it, with no symbol written differently
    std::string       alices = ReadFile( aliceFile );
    const std::string older = "(version " + versionOf( alices ) + ")";

    alices.replace( alices.find( older ), older.size(), "(version " + newer + ")" );
    std::ofstream( aliceFile, std::ios::binary | std::ios::trunc ) << alices;
    edit( aliceFile, "Inductor", "Inductor, 22uH" );

    MDW_LIBRARY_SYNC_RESULT updated = m_alice.SyncOk();
    BOOST_REQUIRE_EQUAL( updated.conflicts.size(), 1u );

    const std::string before = ReadFile( aliceFile );
    const std::string answer =
            before.substr( 0, before.find( "\t(symbol " ) ) + memberOf( ReadFile( bobFile ), "L" ) + "\n)\n";
    const MDW_UPDATE_CONFLICT& conflict = updated.conflicts.front();

    updated = m_alice.Sync( { { conflict.item.item_id, conflict.theirs_revision_id, answer, conflict.ours_token } } );
    BOOST_CHECK( updated.result.error == MDW_ERROR_CODE::INVALID_PARAMETER );
    BOOST_CHECK( ReadFile( aliceFile ) == before );
}


// A part checked in alone under the team's older header would read differently there
BOOST_FIXTURE_TEST_CASE( CheckInRefusesAPartInANewerFormat, ONE_LIBRARIAN )
{
    fs::copy_file( Seed().parent_path() / "acme-library-upgraded" / LIBRARY, m_alice.Copy() / LIBRARY,
                   fs::copy_options::overwrite_existing );
    edit( m_alice.Copy() / LIBRARY, "Inductor", "Inductor, 10uH" );

    BOOST_CHECK( tryCheckIn( m_alice, L, "newer-format" ).code() == kiapi::workspace::WE_NOT_SUPPORTED );
    BOOST_CHECK( headHolds( m_alice.Copy(), LIBRARY, Seed() / LIBRARY ) );
}


// A part saved on Windows goes in with the LF of the file it joins, and CRLF alone is no local change
BOOST_FIXTURE_TEST_CASE( CrLfSavedPartChecksInAsLf, ONE_LIBRARIAN )
{
    const fs::path    file = m_alice.Copy() / LIBRARY;
    const std::string crlf = lineEnds( ReadFile( file ), "\r\n" );

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << crlf;
    BOOST_CHECK( !listedChange( m_alice, C ) );

    edit( file, "Inductor", "Inductor, 10uH" );
    checkIn( m_alice, L, "crlf-part" );

    const std::string committed = headText( m_alice.Copy(), LIBRARY );

    BOOST_CHECK( committed.find( '\r' ) == std::string::npos );
    BOOST_CHECK( committed.find( "\"Inductor, 10uH\"" ) != std::string::npos );
}


// A whole file saved on Windows goes in with the LF its blob holds
BOOST_FIXTURE_TEST_CASE( CrLfWholeFileChecksInAsLf, ONE_LIBRARIAN )
{
    const fs::path    file = m_alice.Copy() / LIBRARY;
    const std::string crlf = lineEnds( ReadFile( file ), "\r\n" );

    std::ofstream( file, std::ios::binary | std::ios::trunc ) << crlf;
    edit( file, "Inductor", "Inductor, 10uH" );

    mdw::CheckInSharedLibraryRequest request = checkInWhole( m_alice, { LIBRARY }, { L } );
    mdw::CheckInSharedLibraryReply   reply;
    MDW_RESULT                       result = m_alice.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    BOOST_CHECK( headText( m_alice.Copy(), LIBRARY ) == lineEnds( ReadFile( file ), "\n" ) );
}


// A part saved back on Windows is CRLF on disk and still matches the team library's LF blob of it
BOOST_FIXTURE_TEST_CASE( SavedBackPartMatchesAcrossLineEndings, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
    Publish( wxS( "Import acme-library" ) );
    MDW_GIT_LIBRARIAN librarian = Connect();

    librarian.SyncOk();

    const mdw::LibraryMember member = listMembers( *librarian.backend ).at( L );
    const fs::path           saved = librarian.Copy() / LIBRARY;
    MDW_CONFLICT_SIDES       side;

    side.conflict.item.item_id = wxString::FromUTF8( L );
    side.conflict.name = wxString::FromUTF8( member.name() );
    side.conflict.relative_path = wxString::FromUTF8( LIBRARY );
    side.oursFile = wxString::FromUTF8( saved.string() );
    side.theirsFile = m_temp.ChildPathStr( wxS( "theirs.kicad_sym" ) );

    auto [downloaded, revision] = MdwSync<MDW_REVISION>(
            [&]( MDW_DONE<MDW_REVISION> aDone )
            {
                return librarian.source->Download( side.conflict.item, wxString::FromUTF8( member.version() ),
                                                   side.theirsFile, std::move( aDone ) );
            } );
    BOOST_REQUIRE_MESSAGE( downloaded.IsOk(), downloaded.message );

    const std::string crlf = lineEnds( ReadFile( saved ), "\r\n" );

    std::ofstream( saved, std::ios::binary | std::ios::trunc ) << crlf;

    BOOST_CHECK( MDW_SETUP::SidesMatch( side ) );
}


// Cancelling an edited revision puts the release back over the saved edit, keeping the file's other parts as saved.
// A save made while the release downloads is newer than what the user gave up, so it stays
BOOST_FIXTURE_TEST_CASE( RestoredPartIsItsReleaseAmongSavedEdits, ONE_LIBRARIAN )
{
    const fs::path            file = m_alice.Copy() / LIBRARY;
    mdw::ReleaseRevisionReply released;

    edit( file, "Inductor", "Inductor, 10uH" );
    MDW_RESULT result = release( *m_alice.backend, file, L, "", "v1:inductor-10u", released );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    edit( file, "Inductor, 10uH", "Inductor, 22uH" );
    edit( file, C_FILTERS, "C? Capacitor*" );

    MDW_PART_TARGET target;
    target.source = m_alice.source;
    target.item = MDW_ITEM_REF{ wxString::FromUTF8( L ), wxString(), m_alice.source->Info().locator };
    target.localPath = wxString::FromUTF8( file.string() );
    target.listed = MDW_MEMBER_ENTRY();
    target.listed->member.name = wxS( "L" );

    auto restore = [&]( const std::function<bool()>& aMayWrite )
    {
        return MdwSync<MDW_NONE>(
                       [&]( MDW_DONE<MDW_NONE> aDone )
                       {
                           // Both sides share one format, so nothing is upgraded
                           return MdwRestorePart( target, wxString::FromUTF8( released.revision().revision_id() ),
                                                  aMayWrite, nullptr, std::move( aDone ) );
                       } )
                .first;
    };

    MDW_RESULT  savedOver = restore(
            [&]()
            {
                edit( file, "Inductor, 22uH", "Inductor, 47uH" );
                return true;
            } );
    std::string saved = ReadFile( file );

    BOOST_CHECK( savedOver.error == MDW_ERROR_CODE::VERSION_CONFLICT );
    BOOST_CHECK( saved.find( "\"Inductor, 47uH\"" ) != std::string::npos );

    std::string expected = saved;
    expected.replace( expected.find( "\"Inductor, 47uH\"" ), 16, "\"Inductor, 10uH\"" );

    MDW_RESULT restored = restore(
            []()
            {
                return true;
            } );

    BOOST_REQUIRE_MESSAGE( restored.IsOk(), restored.message );
    BOOST_CHECK( ReadFile( file ) == expected );
}


// An LF answer written into a part saved on Windows keeps the whole file CRLF
BOOST_FIXTURE_TEST_CASE( AnsweredPartKeepsLineEndings, MDW_GIT_LIBRARY_FIXTURE )
{
    const fs::path      saved = m_temp.Path() / LIBRARY;
    const fs::path      answered = m_temp.Path() / "answer.kicad_sym";
    MDW_UPDATE_CONFLICT conflict;

    conflict.name = wxS( "L" );
    conflict.relative_path = wxString::FromUTF8( LIBRARY );

    std::ofstream( answered, std::ios::binary ) << lineEnds( ReadFile( Seed() / LIBRARY ), "\n" );
    edit( answered, "Inductor", "Inductor, 10uH" );
    std::ofstream( saved, std::ios::binary ) << lineEnds( ReadFile( Seed() / LIBRARY ), "\r\n" );

    BOOST_REQUIRE( MDW_SETUP::WriteMember( wxString::FromUTF8( saved.string() ), conflict, ReadFile( answered ),
                                           nullptr ) );
    BOOST_CHECK( ReadFile( saved ) == lineEnds( ReadFile( answered ), "\r\n" ) );
}


// A claim git cannot enforce can be broken with a reason, and the holder hears who broke it and why
BOOST_FIXTURE_TEST_CASE( AdvisoryClaimBrokenWithReasonIsAnnouncedToHolder, TWO_LIBRARIANS )
{
    claim( m_alice, L );

    // Bob restarts, so his backend asks the remote instead of answering from the claims his set-up read
    Reconnect( m_bob );

    mdw::GetLockInfoRequest query;
    mdw::GetLockInfoReply   held;
    query.set_item_id( L );

    MDW_RESULT result = m_bob.backend->CallSync( query, held, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_REQUIRE( held.lock().is_locked() );
    BOOST_CHECK( held.lock().advisory() );
    BOOST_CHECK( held.lock().lock_timestamp_unix() != 0 );

    mdw::UnlockRequest release;
    mdw::UnlockReply   released;
    release.set_item_id( L );
    release.set_force( true );
    release.set_reason( "left company" );
    release.set_idempotency_id( "bob-releases-l" );

    result = m_bob.backend->CallSync( release, released, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );

    // Bob breaks her claim to take it, so Alice's next fetch finds his claim where hers was
    claim( m_bob, L );

    // Alice restarts, so her backend asks the remote instead of answering from its last fetch
    Reconnect( m_alice );
    BOOST_REQUIRE( m_alice.session->StartEventIngress() );

    std::mutex               mutex;
    std::condition_variable  arrived;
    std::optional<MDW_EVENT> unlocked;

    MDW_SUBSCRIPTION events = m_alice.session->EventBus().Subscribe(
            [&]( const MDW_EVENT& aEvent )
            {
                std::lock_guard<std::mutex> lock( mutex );

                if( aEvent.type == MDW_EVENT_TYPE::ITEM_UNLOCKED && aEvent.item_id == wxString::FromUTF8( L ) )
                    unlocked = aEvent;

                arrived.notify_all();
            } );

    mdw::ListLocksRequest list;
    mdw::ListLocksReply   locks;

    result = m_alice.backend->CallSync( list, locks, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
    BOOST_REQUIRE_EQUAL( locks.locks_size(), 1 );
    BOOST_CHECK_EQUAL( locks.locks( 0 ).lock().owner_display_name(), "bob" );

    std::unique_lock<std::mutex> lock( mutex );

    BOOST_REQUIRE( arrived.wait_for( lock, std::chrono::seconds( 10 ),
                                     [&]()
                                     {
                                         return unlocked.has_value();
                                     } ) );

    nlohmann::json payload = nlohmann::json::parse( unlocked->payload.utf8_string(), nullptr, false );

    BOOST_REQUIRE( payload.is_object() );
    BOOST_CHECK_EQUAL( payload.value( "owner_display_name", "" ), "alice" );
    BOOST_CHECK_EQUAL( payload.value( "released_by", "" ), "bob" );
    BOOST_CHECK_EQUAL( payload.value( "reason", "" ), "left company" );
    BOOST_CHECK( payload.value( "locked_by_me", false ) );
    BOOST_CHECK( !payload.value( "released_by_me", true ) );
}


// A copy made after its backend started, in a folder another repository holds, gives back the claim Revise takes
BOOST_FIXTURE_TEST_CASE( CopyMadeThisSessionGivesBackItsClaim, MDW_GIT_LIBRARY_FIXTURE )
{
    fs::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
    Publish( wxS( "Import acme-library" ) );

    MDW_GIT_LIBRARIAN librarian = Connect();
    const std::string design = ( m_temp.Path() / "design.git" ).string();
    git_repository*   outer = nullptr;
    git_remote*       remote = nullptr;

    BOOST_REQUIRE_EQUAL( git_repository_init( &outer, librarian.Copy().parent_path().string().c_str(), 0 ), GIT_OK );
    KIGIT::GitRepositoryPtr outerPtr( outer );
    BOOST_REQUIRE_EQUAL( git_remote_create( &remote, outer, "origin", design.c_str() ), GIT_OK );
    git_remote_free( remote );
    fs::create_directories( librarian.Copy() );

    BOOST_REQUIRE( librarian.backend->Has( MDW_CAPABILITY_GROUP::LOCKING ) );
    BOOST_CHECK( librarian.session->ListLocks().error == MDW_ERROR_CODE::ITEM_NOT_FOUND );

    MDW_LOCK_TRACKER& tracker = librarian.session->LockTracker();

    BOOST_REQUIRE( !tracker.Get( wxString::FromUTF8( L ) ).known );

    librarian.SyncOk();

    for( int i = 0; i < 200 && !tracker.Get( wxString::FromUTF8( L ) ).known; ++i )
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

    BOOST_CHECK( tracker.Get( wxString::FromUTF8( L ) ).known );

    claim( librarian, L );

    const MDW_ITEM_REF item{ wxString::FromUTF8( L ), wxString(), librarian.source->Info().locator };

    auto [released, lock] = MdwSync<MDW_LOCK_INFO>(
            [&]( MDW_DONE<MDW_LOCK_INFO> aDone )
            {
                return librarian.source->Unlock( item, false, std::move( aDone ) );
            } );

    BOOST_CHECK_MESSAGE( released.IsOk(), released.message );
}


// The tracker is the only store of claims, so Alice must learn of Bob's without asking, and not read it as free before
BOOST_FIXTURE_TEST_CASE( ClaimsAreReadOnceEventsFlow, TWO_LIBRARIANS )
{
    claim( m_bob, L );
    Reconnect( m_alice );

    const wxString    item = wxString::FromUTF8( L );
    MDW_LOCK_TRACKER& tracker = m_alice.session->LockTracker();

    BOOST_CHECK( !tracker.Get( item ).known );

    BOOST_REQUIRE( m_alice.session->StartEventIngress() );
    BOOST_REQUIRE( m_alice.session->Reconnect( 30000 ).IsOk() );

    for( int i = 0; i < 200 && !tracker.Get( item ).is_locked; ++i )
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

    BOOST_CHECK_EQUAL( tracker.Get( item ).owner_display_name, wxS( "bob" ) );

    // Bob may let go while no events arrive, so his claim must not read as current then
    tracker.Invalidate();
    BOOST_CHECK( !tracker.Get( item ).known );
}


// Undo Check Out names no file, as Odoo then restores none, so the saved edits must still read as unreleased
BOOST_FIXTURE_TEST_CASE( UndoCheckOutKeepsEditsMarked, TWO_LIBRARIANS )
{
    MDW_SOURCE&        source = *m_alice.source;
    MDW_DIRTY_TRACKER& dirty = m_alice.session->DirtyTracker();
    const MDW_ITEM_REF item{ wxString::FromUTF8( LIBRARY ), wxString(), source.Info().locator };

    dirty.SetDirty( item.item_id );

    auto [cancelled, none] = MdwSync<MDW_NONE>(
            [&]( MDW_DONE<MDW_NONE> aDone )
            {
                return source.CancelCheckout( item, wxEmptyString, std::move( aDone ) );
            } );

    BOOST_REQUIRE_MESSAGE( cancelled.IsOk(), cancelled.message );
    BOOST_CHECK( dirty.IsDirty( item.item_id ) );
}

BOOST_AUTO_TEST_SUITE_END()
