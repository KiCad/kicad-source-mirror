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

#include <git/git_config_handler.h>
#include <git/kicad_git_compat.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_known_hosts.h>
#include <git/kigit_platform.h>
#include <git/kigit_remote_url.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/mdw_framework.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>

#include <qa_utils/env_var_utils.h>
#include <qa_utils/scoped_git_backend.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "mdw_git_test_utils.h"

#include <json_common.h>

#include <fstream>
#include <map>
#include <utility>
#include <vector>

namespace
{

/// A keychain and account map in place of the platform's, restored when it goes
struct FAKE_VAULT
{
    FAKE_VAULT() :
            saved( KIGIT::PlatformServices() )
    {
        KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();

        services.getSecretNoPrompt = [this]( const wxString& aService, const wxString& aKey, wxString& aSecret )
        {
            auto it = secrets.find( { aService, aKey } );

            if( it == secrets.end() )
                return false;

            aSecret = it->second;
            return true;
        };

        services.storeSecret = [this]( const wxString& aService, const wxString& aKey, const wxString& aSecret )
        {
            secrets[{ aService, aKey }] = aSecret;
            return true;
        };

        services.storeSecretAsync = [this]( const wxString& aService, const wxString& aKey, const wxString& aSecret,
                                            std::function<void( bool )> aDone )
        {
            secrets[{ aService, aKey }] = aSecret;

            if( aDone )
                aDone( true );
        };

        services.deleteSecret = [this]( const wxString& aService, const wxString& aKey )
        {
            return secrets.erase( { aService, aKey } ) > 0;
        };

        services.getAccount = [this]( const wxString& aRealm )
        {
            return accounts.contains( aRealm ) ? accounts.at( aRealm ) : wxString();
        };

        services.setAccount = [this]( const wxString& aRealm, const wxString& aUsername )
        {
            accounts[aRealm] = aUsername;
        };
    }

    ~FAKE_VAULT() { KIGIT::PlatformServices() = saved; }

    KIGIT::PLATFORM_SERVICES                          saved;
    std::map<std::pair<wxString, wxString>, wxString> secrets;
    std::map<wxString, wxString>                      accounts;
};


/// A session on the project aName whose origin is aUrl, reading its credential as the framework has it read
std::unique_ptr<MDW_SESSION> projectSession( const MDW_FRAMEWORK& aFramework, const SCOPED_DATA_DIR& aData,
                                             const std::string& aName, const std::string& aUrl,
                                             const wxString& aWorkspaceId )
{
    std::unique_ptr<MDW_SESSION> session = projectGitSession( aData, aUrl, aName, aWorkspaceId );

    session->SetCredentialSource(
            [&aFramework, held = session.get()]()
            {
                return aFramework.StoredCredentials( *held );
            } );

    return session;
}


/// A session bound to aLocator on a backend whose challenge names aRealm, reading its credential as the framework does
std::unique_ptr<MDW_SESSION> realmSession( const MDW_FRAMEWORK& aFramework, const wxString& aRealm,
                                           const wxString& aLocator )
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR realm( wxS( "QA_MDW_REALM" ), aRealm );
    MDW_BACKEND_MANIFEST            manifest;
    MDW_BACKEND_CONTEXT             context;

    manifest.backend_id = wxS( "org.kicad.mdw.qa.realm" );
    manifest.executable_path = wxString::FromUTF8( MDW_REALM_EXECUTABLE );
    context.workspaceId = wxS( "realm" );
    context.locator = aLocator;

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( manifest, context );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    auto session = std::make_unique<MDW_SESSION>(
            MDW_PROJECT_BINDING{ manifest.backend_id, context.workspaceId, aLocator }, std::move( supervisor ) );

    session->SetCredentialSource(
            [&aFramework, held = session.get()]()
            {
                return aFramework.StoredCredentials( *held );
            } );

    return session;
}

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitCredentials )


// Signing in again as someone else moves a binding that names no one, and is recorded on a source that does
BOOST_AUTO_TEST_CASE( SignInAsAnotherUserIsRemembered )
{
    FAKE_VAULT               vault;
    SCOPED_PGM               program;
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-accounts" ) );
    const wxString           realm = wxS( "http://127.0.0.1:18073" );
    const fs::path           projectFile = temp.Path() / "ctrl-board.kicad_pro";

    vault.accounts[realm] = wxS( "alice" );
    fs::copy_file( fs::path( KI_TEST::GetTestDataRootDir() ) / "common/git/mdw_ctrl_board/ctrl-board.kicad_pro",
                   projectFile );

    SETTINGS_MANAGER manager;
    const wxString   path = wxString::FromUTF8( projectFile.string() );

    BOOST_REQUIRE( manager.LoadProject( path, false ) );

    PROJECT& project = *manager.GetProject( path );

    MDW_SETUP::RememberProjectAccount(
            project,
            { wxS( "org.kicad.mdw.odoo" ), wxEmptyString,
              realm + wxS( "/?db=kicad&root=ACME%20X11&transport=jsonrpc&project=PCBA-AMP-001" ) },
            wxS( "bob" ) );

    BOOST_CHECK( vault.accounts[realm] == wxS( "bob" ) );
    BOOST_CHECK( project.GetLocalSettings().m_GitRepoUsername.IsEmpty() );

    const wxString      userFile = temp.ChildPathStr( wxS( "sources.json" ) );
    const wxString      siteFile = temp.ChildPathStr( wxS( "site.json" ) );
    MDW_LIBRARY_SOURCES sources( userFile, siteFile, temp.CreateChildDirStr( wxS( "copies" ) ) );

    MDW_LIBRARY_SOURCE_CONFIG acme;
    acme.backend_id = wxS( "org.kicad.mdw.git" );
    acme.locator = wxS( "http://192.0.2.26:3301/acme/acme-library.git" );
    acme.name = wxS( "acme-library" );
    acme.account = wxS( "carol" );
    BOOST_REQUIRE( sources.Add( acme ).IsOk() );
    BOOST_REQUIRE( sources.SetAccount( acme.id, wxS( "bob" ) ).IsOk() );

    std::optional<MDW_LIBRARY_SOURCE_CONFIG> reread =
            MDW_LIBRARY_SOURCES( userFile, siteFile, wxEmptyString ).Find( acme.id );

    BOOST_REQUIRE( reread );
    BOOST_CHECK( reread->account == wxS( "bob" ) );
}


// A signed-out library session with nothing stored is not reopened, which would start another backend
BOOST_AUTO_TEST_CASE( OnlyAStoredLoginIsWorthAFreshSession )
{
    SCOPED_DATA_DIR   dataDir;
    FAKE_VAULT        vault;
    SCOPED_PGM program( false );
    MDW_FRAMEWORK     framework;
    const std::string host = "http://127.0.0.1:9";

    std::unique_ptr<MDW_SESSION> session =
            projectSession( framework, dataDir, "board", host + "/acme/board.git", wxEmptyString );

    BOOST_REQUIRE( session->Status().realm == wxString::FromUTF8( host ) );
    BOOST_CHECK( !framework.HasUnrefusedStoredLogin( *session ) );

    BOOST_REQUIRE( framework.Credentials().Save( session->Status().realm, carol() ) );
    BOOST_CHECK( framework.HasUnrefusedStoredLogin( *session ) );
}


// A sign-in saved after the pane missed the keychain serves its next push, as the account it named
BOOST_AUTO_TEST_CASE( WorkspaceSignInServesTheNextPanePush )
{
    FAKE_VAULT     vault;
    SCOPED_PGM program( false );
    MDW_FRAMEWORK  framework;
    const wxString realm = wxS( "http://127.0.0.1:18074" );
    auto           owner = std::make_shared<bool>( true );
    auto           common = std::make_shared<KIGIT_COMMON>( nullptr );

    KIGIT::PlatformServices().getSecretAsync =
            []( const wxString&, const wxString&, std::function<void( bool, const wxString& )> aDone )
    {
        aDone( false, wxEmptyString );
    };

    common->SyncRemoteUrl( realm + wxS( "/acme/ctrl-board.git" ) );
    BOOST_REQUIRE( common->GetPassword().IsEmpty() );
    bool retried = false;

    BOOST_REQUIRE( KIGIT_COMMON::DeferForSecret( common, owner,
                                                 [&]()
                                                 {
                                                     retried = true;
                                                 } ) );
    BOOST_REQUIRE( retried && common->SecretRead() );

    // A binding tested as someone else is not the server's account, so the pane does not push as carol
    vault.accounts[realm] = wxS( "bob" );
    BOOST_REQUIRE( framework.Credentials().Save( realm, carol() ) );
    BOOST_CHECK( common->GetCredentialIdentity().username.IsEmpty() );

    MDW_TYPED_CREDENTIALS bob;
    bob.fields = { { wxS( "username" ), wxS( "bob" ) }, { wxS( "password" ), wxS( "bob-pw" ) } };
    BOOST_REQUIRE( framework.Credentials().Save( realm, bob ) );

    BOOST_CHECK( common->GetCredentialIdentity().username == wxS( "bob" ) );
    BOOST_CHECK( common->GetPassword() == wxS( "bob-pw" ) );
}


// Only an HTTP(S) server is a realm; an SSH secret unlocks a key, so it keeps the address that names the key's user
BOOST_AUTO_TEST_CASE( CredentialRealmNamesOnlyAnHttpServer )
{
    struct CASE
    {
        wxString url;
        wxString realm;
        wxString server;
    };

    const std::vector<CASE> cases = {
        { wxS( "https://carol:pw@Git.Example.com/acme/board.git" ), wxS( "https://git.example.com" ),
          wxS( "git.example.com" ) },
        { wxS( "HTTPS://git.example.com:8443/acme" ), wxS( "https://git.example.com:8443" ),
          wxS( "git.example.com:8443" ) },
        { wxS( "https://git.example.com:443/acme" ), wxS( "https://git.example.com" ), wxS( "git.example.com" ) },
        { wxS( "http://git.example.com:80/acme" ), wxS( "http://git.example.com" ), wxS( "git.example.com" ) },
        { wxS( "http://[::1]:8080/acme.git" ), wxS( "http://[::1]:8080" ), wxS( "[::1]:8080" ) },
        { wxS( "https://[2001:DB8::1]/acme.git" ), wxS( "https://[2001:db8::1]" ), wxS( "[2001:db8::1]" ) },
        { wxS( "https://git.example.com:99999/acme" ), wxEmptyString, wxEmptyString },
        { wxS( "https://git.example.com:x/acme" ), wxEmptyString, wxEmptyString },
        { wxS( "ssh://git@github.com/acme/board.git" ), wxEmptyString, wxS( "github.com" ) },
        { wxS( "ssh+git://git@github.com:22/acme/board.git" ), wxEmptyString, wxS( "github.com" ) },
        { wxS( "ssh://git@github.com:2222/acme/board.git" ), wxEmptyString, wxS( "github.com:2222" ) },
        { wxS( "git@github.com:acme/board.git" ), wxEmptyString, wxS( "github.com" ) },
        { wxS( "/home/carol/board" ), wxEmptyString, wxEmptyString },
        { wxS( "C:\\carol\\board" ), wxEmptyString, wxEmptyString },
        { wxS( "file:///srv/board.git" ), wxEmptyString, wxEmptyString },
    };

    for( const CASE& test : cases )
    {
        BOOST_TEST_CONTEXT( test.url )
        {
            BOOST_CHECK_EQUAL( KIGIT::CredentialRealm( test.url ), test.realm );
            BOOST_CHECK_EQUAL( KIGIT::CredentialServer( test.url ), test.server );
        }
    }
}


// A per-repository token from an earlier release outranks the server's password until a save for the server
BOOST_AUTO_TEST_CASE( LegacyFullUrlKeyServesUntilServerSave )
{
    FAKE_VAULT     vault;
    const wxString url = wxS( "https://git.example.com:8443/acme/board.git" );

    auto password = [&]()
    {
        KIGIT_COMMON common( nullptr );
        common.SetRemote( url );
        common.SetUsername( wxS( "carol" ) );

        return common.GetPassword();
    };

    vault.secrets[{ url, wxS( "carol" ) }] = wxS( "legacy-pw" );
    vault.secrets[{ wxS( "https://git.example.com:8443" ), wxS( "carol" ) }] = wxS( "panel-pw" );
    BOOST_CHECK( password() == wxS( "legacy-pw" ) );

    KIGIT::SaveCredentialAsync( url, wxS( "carol" ), wxS( "new-pw" ) );
    BOOST_CHECK( password() == wxS( "new-pw" ) );
}


// A backend bound to one server gets none of the stored login of another its challenge names
BOOST_AUTO_TEST_CASE( BackendNamingAnotherServerGetsNoCredential )
{
    SCOPED_DATA_DIR dataDir;
    FAKE_VAULT      vault;
    SCOPED_PGM program( false );
    MDW_FRAMEWORK   framework;

    vault.secrets[{ wxS( "https://github.com" ), wxS( "carol" ) }] = wxS( "github-token" );
    vault.accounts[wxS( "https://github.com" )] = wxS( "carol" );

    std::unique_ptr<MDW_SESSION> session =
            realmSession( framework, wxS( "https://github.com" ), wxS( "https://git.example.com/acme/board.git" ) );

    BOOST_CHECK( framework.StoredCredentials( *session ).fields.empty() );

    BOOST_CHECK_EQUAL( MDW_CREDENTIALS::BoundRealm( wxS( "https://erp.example.com/odoo" ),
                                                    wxS( "odoo://erp.example.com:443/odoo?db=acme" ), wxEmptyString ),
                       wxString( wxS( "https://erp.example.com" ) ) );
}


// An SSH secret unlocks a key and stays with stock git, so a workspace sign-in over SSH saves nothing
BOOST_AUTO_TEST_CASE( SshWorkspaceSignInSavesNothing )
{
    SCOPED_DATA_DIR dataDir;
    FAKE_VAULT      vault;
    SCOPED_PGM program( false );
    MDW_FRAMEWORK   framework;
    const wxString  url = wxS( "ssh://git@git.example.com/acme/board.git" );

    std::unique_ptr<MDW_SESSION> session = realmSession( framework, url, url );

    BOOST_REQUIRE( framework.Credentials().SignIn( *session, carol(), 8000 ).IsOk() );
    BOOST_CHECK( vault.secrets.empty() );
    BOOST_CHECK( vault.accounts.empty() );
}


// The key a project picked goes with every login its backend's form has room for, even one with nothing stored
BOOST_AUTO_TEST_CASE( KeyFileGoesWithEveryLogin )
{
    SCOPED_DATA_DIR                 dataDir;
    FAKE_VAULT                      vault;
    SCOPED_PGM                      program( false );
    MDW_FRAMEWORK                   framework;
    const wxString                  url = wxS( "ssh://git@git.example.com/acme/board.git" );
    KI_TEST::SCOPED_PROCESS_ENV_VAR keyFile( wxS( "QA_MDW_KEY_FILE" ), wxS( "/keys/id_team" ) );

    std::unique_ptr<MDW_SESSION> session = realmSession( framework, url, url );

    BOOST_CHECK( !session->Reconnect( 8000 ).IsOk() );

    session->SetKeyFile( wxS( "/keys/id_team" ) );
    BOOST_CHECK( session->Reconnect( 8000 ).IsOk() );
}


// A host holding no git backend still names the global author, which libgit2 finds only once initialized
BOOST_AUTO_TEST_CASE( HelloNamesGlobalAuthorWithoutGitBackend )
{
    SCOPED_DATA_DIR dataDir;
    SCOPED_PGM program( false );
    MDW_FRAMEWORK   framework;
    const fs::path  home = dataDir.Path() / "home";

    fs::create_directories( home );
    std::ofstream( home / ".gitconfig" ) << "[user]\n\tname = Host User\n\temail = host@example.com\n";

    KI_TEST::SCOPED_PROCESS_ENV_VAR homeEnv( wxS( "HOME" ), wxString::FromUTF8( home.string() ) );
    KI_TEST::SCOPED_PROCESS_ENV_VAR xdgEnv( wxS( "XDG_CONFIG_HOME" ), wxString::FromUTF8( home.string() ) );

    std::unique_ptr<MDW_SESSION> session = realmSession( framework, wxS( "https://git.example.com" ),
                                                         wxS( "https://git.example.com/acme/board.git" ) );

    BOOST_REQUIRE( session->Authenticate( carol(), 8000 ).IsOk() );
    BOOST_CHECK_EQUAL( session->Status().account.display_name, wxString( wxS( "Host User" ) ) );
}


#ifdef __WXMAC__
// libgit2 used uninitialized writes through the thread key it released, which the keychain reuses on macOS
BOOST_AUTO_TEST_CASE( AuthorReadWithoutGitBackendSparesKeychain )
{
    const KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();
    GitUserConfig                   local;
    GitUserConfig                   global;
    wxString                        secret;

    // Only a count of zero releases the thread key, and a held one would let this pass without the guard
    const int count = git_libgit2_init();
    git_libgit2_shutdown();
    BOOST_REQUIRE_EQUAL( count, 1 );

    // A lookup that misses raises a keychain error without writing to the user's keychain
    services.getSecretNoPrompt( wxS( "org.kicad.qa.absent" ), wxS( "nobody" ), secret );
    KIGIT::ReadAuthorConfig( nullptr, local, global );

    BOOST_CHECK( !services.getSecretNoPrompt( wxS( "org.kicad.qa.absent" ), wxS( "nobody" ), secret ) );
}
#endif


#if KIGIT_HAS_HOMEDIR_OPT
// Uninitialized libgit2 has no home folder, so every SSH host would read as unknown
BOOST_AUTO_TEST_CASE( KnownHostsPathWithoutGitBackend )
{
    SCOPED_DATA_DIR dataDir;
    const fs::path  home = dataDir.Path() / "home";

    fs::create_directories( home / ".ssh" );

    KI_TEST::SCOPED_PROCESS_ENV_VAR homeEnv( wxS( "HOME" ), wxString::FromUTF8( home.string() ) );

    // libgit2 reads HOME only when its count leaves zero
    const int count = git_libgit2_init();
    git_libgit2_shutdown();
    BOOST_REQUIRE_EQUAL( count, 1 );

    BOOST_CHECK_EQUAL( KIGIT::KnownHostsPath(), wxString::FromUTF8( ( home / ".ssh" / "known_hosts" ).string() ) );
}
#endif


BOOST_AUTO_TEST_SUITE_END()
