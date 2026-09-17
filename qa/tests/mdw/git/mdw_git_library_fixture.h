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

#ifndef MDW_GIT_LIBRARY_FIXTURE_H
#define MDW_GIT_LIBRARY_FIXTURE_H

#include <boost/test/unit_test.hpp>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_lifecycle.pb.h>
#include <api/mdw/backend_locking.pb.h>
#include <api/mdw/backend_revisions.pb.h>
#include <api/mdw/backend_shared_libs.pb.h>

#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_backend.h>
#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_session.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <qa_utils/env_var_utils.h>
#include <qa_utils/git_test_fixture.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>


/// One librarian's library source, with its own copy of the remote and its own kicad-mdw-git
struct MDW_GIT_LIBRARIAN
{
    std::unique_ptr<MDW_LIBRARY_SOURCES> sources;
    MDW_LIBRARY_SOURCE_CONFIG            config;
    std::shared_ptr<MDW_SESSION>         session;
    std::shared_ptr<MDW_BACKEND>         backend; ///< The session's, for calls the client does not make
    std::shared_ptr<MDW_SOURCE>          source;

    std::filesystem::path Copy() const
    {
        return std::filesystem::path( sources->LocalRoot( config ).utf8_string() );
    }

    MDW_LIBRARY_SYNC_RESULT Sync( const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions = {} )
    {
        return MDW_SETUP::SyncLibrarySource( *sources, source, config, {}, aResolutions );
    }

    /// Sync() that the test requires to succeed
    MDW_LIBRARY_SYNC_RESULT SyncOk( const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions = {} )
    {
        MDW_LIBRARY_SYNC_RESULT synced = Sync( aResolutions );

        BOOST_REQUIRE_MESSAGE( synced.result.IsOk(), synced.result.message );
        return synced;
    }

    /// The commit the copy's HEAD names
    std::string Head() const
    {
        git_repository* repo = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &repo, Copy().string().c_str() ), GIT_OK );
        KIGIT::GitRepositoryPtr repoPtr( repo );

        git_oid oid;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &oid, repo, "HEAD" ), GIT_OK );
        return git_oid_tostr_s( &oid );
    }

    /// Sign this librarian's commits and releases aName
    void SetAuthor( const std::string& aName ) const
    {
        git_repository* repo = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &repo, Copy().string().c_str() ), GIT_OK );
        KIGIT::GitRepositoryPtr repoPtr( repo );

        git_config* config = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &config, repo ), GIT_OK );
        KIGIT::GitConfigPtr configPtr( config );

        BOOST_REQUIRE_EQUAL( git_config_set_string( config, "user.name", aName.c_str() ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_config_set_string( config, "user.email", ( aName + "@example.com" ).c_str() ),
                             GIT_OK );
    }
};


/**
 * A git library source as Set Up adds one. Its remote is a bare clone of the repository at Source(), and each
 * librarian Connect() adds has kicad-mdw-git serve it in that librarian's library session.
 */
class MDW_GIT_LIBRARY_FIXTURE
{
public:
    MDW_GIT_LIBRARY_FIXTURE() :
            m_temp( wxS( "mdw-git-library" ) ),
            m_dataDir( wxS( "KICAD_MDW_DATA_DIR" ), m_temp.CreateChildDirStr( wxS( "data" ) ) )
    {
        std::filesystem::create_directories( Source() );
    }

    /// The acme-library fixture
    static std::filesystem::path Seed()
    {
        return std::filesystem::path( KI_TEST::GetTestDataRootDir() ) / "common/git/acme-library";
    }

    static std::string ReadFile( const std::filesystem::path& aPath )
    {
        std::ifstream      in( aPath, std::ios::binary );
        std::ostringstream text;

        text << in.rdbuf();
        return text.str();
    }

    /**
     * The blob id aCopy would store for aFile at aPath. A checkout under core.autocrlf writes CRLF, so this
     * compares contents the way git does rather than byte for byte.
     */
    static std::string StoredDigest( const std::filesystem::path& aCopy, const std::filesystem::path& aPath,
                                     const std::filesystem::path& aFile )
    {
        git_repository* repo = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &repo, aCopy.string().c_str() ), GIT_OK );
        KIGIT::GitRepositoryPtr repoPtr( repo );

        git_oid oid;
        BOOST_REQUIRE_EQUAL( git_repository_hashfile( &oid, repo, aFile.string().c_str(), GIT_OBJECT_BLOB,
                                                      aPath.generic_string().c_str() ),
                             GIT_OK );
        return git_oid_tostr_s( &oid );
    }

    std::filesystem::path Source() const { return m_temp.Path() / "source"; }

    /// Commit everything in Source() and bring it to the remote, as a check-in reaching the server does
    void Publish( const wxString& aMessage )
    {
        const std::filesystem::path upstream = m_temp.Path() / "acme-library.git";

        if( !m_sourceRepo )
            m_sourceRepo = KI_TEST::InitGitRepository( wxString::FromUTF8( Source().string() ) );

        KI_TEST::CommitAllGitFiles( m_sourceRepo.get(), aMessage );

        if( !m_upstreamRepo )
        {
            git_repository*   bare = nullptr;
            git_clone_options options = GIT_CLONE_OPTIONS_INIT;
            options.bare = 1;

            BOOST_REQUIRE_EQUAL( git_clone( &bare, Source().string().c_str(), upstream.string().c_str(), &options ),
                                 GIT_OK );
            m_upstreamRepo.reset( bare, git_repository_free );
            return;
        }

        git_remote* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_remote_create_anonymous( &remote, m_upstreamRepo.get(), Source().string().c_str() ),
                             GIT_OK );
        KIGIT::GitRemotePtr remotePtr( remote );

        char         refspec[] = "+refs/heads/main:refs/heads/main";
        char*        refspecs[] = { refspec };
        git_strarray specs = { refspecs, 1 };

        BOOST_REQUIRE_EQUAL( git_remote_fetch( remote, &specs, nullptr, nullptr ), GIT_OK );
    }

    /// aPath as the remote's main branch holds it now
    std::string Upstream( const std::string& aPath ) const
    {
        git_object* object = nullptr;
        BOOST_REQUIRE_EQUAL( git_revparse_single( &object, m_upstreamRepo.get(), ( "main:" + aPath ).c_str() ),
                             GIT_OK );
        KIGIT::GitObjectPtr objectPtr( object );

        const git_blob* blob = reinterpret_cast<const git_blob*>( object );

        return std::string( static_cast<const char*>( git_blob_rawcontent( blob ) ), git_blob_rawsize( blob ) );
    }

    /// Add a source for a librarian whose copies live under aName, and start its backend in the session its key
    /// names, so members are read from that copy
    MDW_GIT_LIBRARIAN Connect( const wxString& aName = wxS( "copies" ), const wxString& aPrefix = wxString() )
    {
        MDW_GIT_LIBRARIAN librarian;

        librarian.sources = std::make_unique<MDW_LIBRARY_SOURCES>(
                m_temp.ChildPathStr( aName + wxS( "-sources.json" ) ), m_temp.ChildPathStr( wxS( "site.json" ) ),
                m_temp.CreateChildDirStr( aName ) );

        MDW_LIBRARY_SOURCE_CONFIG& config = librarian.config;
        config.backend_id = wxS( "org.kicad.mdw.git" );
        config.locator = wxString::FromUTF8( ( m_temp.Path() / "acme-library.git" ).string() );
        config.name = wxS( "acme-library" );
        config.prefix = aPrefix;

        BOOST_REQUIRE( librarian.sources->Add( config ).IsOk() );

        start( librarian );
        return librarian;
    }

    /// Stop aLibrarian's backend and start another on the same copy, as a restart does
    void Reconnect( MDW_GIT_LIBRARIAN& aLibrarian )
    {
        aLibrarian.source.reset();
        aLibrarian.backend.reset();
        aLibrarian.session.reset();
        start( aLibrarian );
    }

private:
    void start( MDW_GIT_LIBRARIAN& aLibrarian )
    {
        const MDW_LIBRARY_SOURCE_CONFIG& config = aLibrarian.config;

        MDW_BACKEND_MANIFEST manifest;
        manifest.backend_id = config.backend_id;
        manifest.executable_path = wxString::FromUTF8( MDW_GIT_EXECUTABLE );

        MDW_BACKEND_CONTEXT library;
        library.projectPath = aLibrarian.sources->SessionKey( config );
        library.workspaceId = config.name;
        library.locator = config.locator;

        auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, library );
        BOOST_REQUIRE( supervisor->Launch( 8000 ) );

        aLibrarian.backend = supervisor->Backend();
        BOOST_REQUIRE( aLibrarian.backend );

        aLibrarian.session = std::make_shared<MDW_SESSION>(
                MDW_PROJECT_BINDING{ config.backend_id, config.name, config.locator }, std::move( supervisor ) );
        BOOST_REQUIRE( aLibrarian.session->StartEventIngress() );
        aLibrarian.source = std::make_shared<MDW_SOURCE>( aLibrarian.session );
    }

public:
    SCOPED_GIT_BACKEND              m_gitBackend;
    KI_TEST::SCOPED_TEMP_DIR        m_temp;
    KI_TEST::SCOPED_PROCESS_ENV_VAR m_dataDir;
    // Shared, since KIGIT's pointer types have internal linkage and this fixture is in a header
    std::shared_ptr<git_repository> m_sourceRepo;
    std::shared_ptr<git_repository> m_upstreamRepo;
};


/// The passives library every librarian starts from, and the parts of it that tests edit
inline const std::string LIBRARY = "passives.kicad_sym";
inline const std::string C = LIBRARY + "#C";
inline const std::string L = LIBRARY + "#L";
inline const std::string C_FILTERS = "C? C_????_* C_???? SMD*_c Capacitor*";
inline const std::string L_KEYWORDS = "inductor choke coil reactor magnetic";


/// Save an edit by replacing the quoted string aFrom, which only one symbol holds
inline void edit( const std::filesystem::path& aFile, const std::string& aFrom, const std::string& aTo )
{
    std::string  text = MDW_GIT_LIBRARY_FIXTURE::ReadFile( aFile );
    const size_t at = text.find( "\"" + aFrom + "\"" );

    BOOST_REQUIRE( at != std::string::npos );
    text.replace( at, aFrom.size() + 2, "\"" + aTo + "\"" );
    std::ofstream( aFile, std::ios::binary | std::ios::trunc ) << text;
}


/// Release aId as the copy's file holds it, from aBase as the editor takes it from the listing
inline MDW_RESULT release( MDW_BACKEND& aBackend, const std::filesystem::path& aFile, const std::string& aId,
                           const std::string& aBase, const std::string& aDigest,
                           kiapi::mdw::ReleaseRevisionReply& aReply )
{
    kiapi::mdw::ReleaseRevisionRequest request;
    request.set_item_id( aId );
    request.set_base_revision_id( aBase );
    request.set_local_path( aFile.string() );
    request.set_content_digest( aDigest );
    request.set_idempotency_id( aDigest );

    return aBackend.CallSync( request, aReply, 30000 );
}


inline MDW_RESULT release( MDW_BACKEND& aBackend, const std::filesystem::path& aFile, const std::string& aId,
                           const std::string& aDigest )
{
    kiapi::mdw::ReleaseRevisionReply reply;

    return release( aBackend, aFile, aId, "", aDigest, reply );
}


/// Check in aId alone, unreleased, as the library check-in dialog does, and answer its row's error
inline kiapi::workspace::WorkspaceError tryCheckIn( const MDW_GIT_LIBRARIAN& aLibrarian, const std::string& aId,
                                                    const std::string& aIdempotency )
{
    kiapi::mdw::CheckInSharedLibraryRequest request;
    kiapi::mdw::CheckInSharedLibraryReply   reply;
    request.set_local_root( aLibrarian.Copy().string() );
    request.add_item_ids( aId );
    request.set_comment( "Check in " + aId );
    request.set_idempotency_id( aIdempotency );

    MDW_RESULT result = aLibrarian.backend->CallSync( request, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), static_cast<int>( result.error ) << " " << result.message );
    BOOST_REQUIRE_EQUAL( reply.results_size(), 1 );

    // A member that applied carries no error
    return reply.results( 0 ).has_error() ? reply.results( 0 ).error() : MdwOk();
}


/// Claim aId, as revising a released member does before it is checked in again
inline void claim( const MDW_GIT_LIBRARIAN& aLibrarian, const std::string& aId )
{
    kiapi::mdw::CheckoutRequest checkout;
    kiapi::mdw::CheckoutReply   reply;
    checkout.set_item_id( aId );
    checkout.set_lock( true );
    checkout.set_idempotency_id( "claim-" + aId );

    MDW_RESULT result = aLibrarian.backend->CallSync( checkout, reply, 30000 );
    BOOST_REQUIRE_MESSAGE( result.IsOk(), result.message );
}


/// Alice's synced copy of the passives library, ready to edit
class ONE_LIBRARIAN : public MDW_GIT_LIBRARY_FIXTURE
{
public:
    ONE_LIBRARIAN()
    {
        std::filesystem::copy_file( Seed() / LIBRARY, Source() / LIBRARY );
        Publish( wxS( "Import acme-library" ) );

        m_alice = Connect( wxS( "alice" ) );

        m_alice.SyncOk();

        m_alice.SetAuthor( "alice" );
    }

    MDW_GIT_LIBRARIAN m_alice;
};

#endif // MDW_GIT_LIBRARY_FIXTURE_H
