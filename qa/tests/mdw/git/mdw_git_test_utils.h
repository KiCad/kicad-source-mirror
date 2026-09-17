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

#ifndef MDW_GIT_TEST_UTILS_H
#define MDW_GIT_TEST_UTILS_H

#include <boost/test/unit_test.hpp>

#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_session.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <git/kigit_platform.h>
#include <json_common.h>
#include <git2.h>

#include <qa_utils/env_var_utils.h>
#include <qa_utils/file_utils.h>

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/utils.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;


/// A test program that hosts the MDW framework unless it stands in for a standalone editor
struct SCOPED_PGM : public PGM_BASE
{
    explicit SCOPED_PGM( bool aHost = true )
    {
        SetPgm( this );
        m_settings_manager = std::make_unique<SETTINGS_MANAGER>();

        if( aHost )
            HostMdwFramework();
    }

    // The framework goes with the program, since stopping it as an exiting program does would stop the
    // client workers for every later test
    ~SCOPED_PGM() { SetPgm( nullptr ); }

    void MacOpenFile( const wxString& ) override {}
};


/// A temporary data directory that KICAD_MDW_DATA_DIR names until the test ends
struct SCOPED_DATA_DIR
{
    KI_TEST::SCOPED_TEMP_DIR        temp{ wxS( "mdw-qa-data" ) };
    KI_TEST::SCOPED_PROCESS_ENV_VAR env{ wxS( "KICAD_MDW_DATA_DIR" ), temp.PathStr() };

    const fs::path& Path() const { return temp.Path(); }
    wxString        PathStr() const { return temp.PathStr(); }
};


/**
 * Keep the host's git config and proxy out of every backend this binary launches, and out of its own libgit2.
 * A proxy or a url.insteadOf rewrite in the developer's settings would send the tests' loopback traffic elsewhere.
 */
struct GIT_HOST_ISOLATION
{
    KI_TEST::SCOPED_TEMP_DIR                                      home{ wxS( "mdw-qa-home" ) };
    std::vector<std::unique_ptr<KI_TEST::SCOPED_PROCESS_ENV_VAR>> env;

    GIT_HOST_ISOLATION()
    {
        for( const wxChar* name : { wxS( "HOME" ), wxS( "XDG_CONFIG_HOME" ), wxS( "USERPROFILE" ) } )
            env.push_back( std::make_unique<KI_TEST::SCOPED_PROCESS_ENV_VAR>( name, home.PathStr() ) );

        for( const wxChar* name : { wxS( "http_proxy" ), wxS( "HTTP_PROXY" ), wxS( "https_proxy" ),
                                    wxS( "HTTPS_PROXY" ), wxS( "all_proxy" ), wxS( "ALL_PROXY" ) } )
        {
            env.push_back( std::make_unique<KI_TEST::SCOPED_PROCESS_ENV_VAR>( name, std::nullopt ) );
        }

        for( const wxChar* name : { wxS( "no_proxy" ), wxS( "NO_PROXY" ) } )
            env.push_back( std::make_unique<KI_TEST::SCOPED_PROCESS_ENV_VAR>( name, wxS( "127.0.0.1,localhost" ) ) );
    }
};


/// Overrides the author preference until the test ends
struct SCOPED_AUTHOR_PREFERENCE
{
    explicit SCOPED_AUTHOR_PREFERENCE( const COMMON_SETTINGS::GIT& aPrefs ) :
            m_saved( KIGIT::PlatformServices().getAuthorPreference )
    {
        KIGIT::PlatformServices().getAuthorPreference = [aPrefs]
        {
            return aPrefs;
        };
    }

    ~SCOPED_AUTHOR_PREFERENCE() { KIGIT::PlatformServices().getAuthorPreference = m_saved; }

    std::function<COMMON_SETTINGS::GIT()> m_saved;
};


/**
 * No event loop runs here, so completions the framework posts to the UI stay queued past the case that made them.
 * Each case's leftovers are named and dropped as it ends, so none runs in, or passes for, a later case.
 */
struct UI_EVENT_ISOLATION
{
    struct OBSERVER : boost::unit_test::test_observer
    {
        void test_unit_finish( const boost::unit_test::test_unit& aUnit, unsigned long ) override
        {
            if( !wxTheApp || !wxTheApp->HasPendingEvents() )
                return;

            BOOST_TEST_MESSAGE( aUnit.full_name() << " left UI events queued" );
            wxTheApp->DeletePendingEvents();
        }
    };

    // Never deregistered, since the framework tears global fixtures down while it walks its observers
    UI_EVENT_ISOLATION()
    {
        static OBSERVER observer;
        boost::unit_test::framework::register_observer( observer );
    }
};


/// A project tree whose origin is aUrl, as a project bound to a remote repository has
inline fs::path makeProject( const fs::path& aDir, const std::string& aUrl )
{
    git_repository* repo = nullptr;
    git_remote*     remote = nullptr;

    git_libgit2_init();
    BOOST_REQUIRE_EQUAL( git_repository_init( &repo, aDir.string().c_str(), 0 ), 0 );
    BOOST_REQUIRE_EQUAL( git_remote_create( &remote, repo, "origin", aUrl.c_str() ), 0 );
    git_remote_free( remote );
    git_repository_free( repo );
    git_libgit2_shutdown();

    return aDir / "board.kicad_pro";
}


inline MDW_BACKEND_MANIFEST gitManifest()
{
    MDW_BACKEND_MANIFEST manifest;
    manifest.backend_id = wxS( "org.kicad.mdw.git" );
    manifest.executable_path = wxString::FromUTF8( MDW_GIT_EXECUTABLE );

    return manifest;
}


/// A git session over the tree or locator of aContext, as the framework opens one
inline std::unique_ptr<MDW_SESSION> gitSession( const MDW_BACKEND_CONTEXT& aContext )
{
    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( gitManifest(), aContext );
    BOOST_REQUIRE( supervisor->Launch( 8000 ) );

    MDW_PROJECT_BINDING binding{ wxS( "org.kicad.mdw.git" ), aContext.workspaceId, aContext.locator };

    return std::make_unique<MDW_SESSION>( binding, std::move( supervisor ) );
}


/// A git session on the project tree named aName under aData, whose origin is aUrl
inline std::unique_ptr<MDW_SESSION> projectGitSession( const SCOPED_DATA_DIR& aData, const std::string& aUrl,
                                                       const std::string& aName = "project",
                                                       const wxString&    aWorkspaceId = wxS( "project" ) )
{
    MDW_BACKEND_CONTEXT project;
    project.workspaceId = aWorkspaceId;
    project.projectPath = wxString::FromUTF8( makeProject( aData.Path() / aName, aUrl ).string() );

    return gitSession( project );
}


/// Offer aFramework the backend aId run from aExecutable, as an installed manifest would
inline void offerBackend( MDW_FRAMEWORK& aFramework, const SCOPED_DATA_DIR& aData, const std::string& aId,
                          const std::string& aExecutable, nlohmann::json aManifest = nlohmann::json::object() )
{
    const fs::path dir = aData.Path() / "backends";
    fs::create_directories( dir );

    aManifest["id"] = aId;
    aManifest["executable"] = aExecutable;
    std::ofstream( dir / ( aId + ".kicad-mdw.json" ) ) << aManifest.dump();

    aFramework.Registry().Discover( { wxString::FromUTF8( dir.string() ) } );
    BOOST_REQUIRE( aFramework.Registry().Find( wxString::FromUTF8( aId ) ) );
}


/// Offer aFramework the stand-in backend whose behavior the QA_MDW_* variables set
inline void offerRealmBackend( MDW_FRAMEWORK& aFramework, const SCOPED_DATA_DIR& aData )
{
    offerBackend( aFramework, aData, "org.kicad.mdw.qa.realm", MDW_REALM_EXECUTABLE );
}


inline MDW_PROJECT_BINDING realmBinding()
{
    return { wxS( "org.kicad.mdw.qa.realm" ), wxS( "realm" ), wxEmptyString };
}


inline MDW_TYPED_CREDENTIALS carol()
{
    MDW_TYPED_CREDENTIALS credentials;
    credentials.fields[wxS( "username" )] = wxS( "carol" );
    credentials.fields[wxS( "password" )] = wxS( "carol-pw" );

    return credentials;
}

#endif // MDW_GIT_TEST_UTILS_H
