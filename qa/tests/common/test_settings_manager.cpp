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

/**
 * @file test_settings_manager.cpp
 * Tests for SETTINGS_MANAGER lifecycle behavior.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>

#include <json_common.h>
#include <kiplatform/io.h>
#include <kiway.h>
#include <lockfile.h>
#include <project.h>

#include <wx/filename.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;


// Backed by a caller-controlled absolute path (SETTINGS_LOC::NONE) so the tests persist and
// reload from a scratch directory without touching the shared config corpus.
class FLUSH_TEST_SETTINGS : public JSON_SETTINGS
{
public:
    FLUSH_TEST_SETTINGS( const wxString& aFullPath ) :
            JSON_SETTINGS( aFullPath, SETTINGS_LOC::NONE, 1 ),
            m_value( 0 )
    {
        m_params.emplace_back( new PARAM<int>( "test.value", &m_value, 0 ) );
    }

    int m_value;
};


struct SETTINGS_MANAGER_FIXTURE
{
    SETTINGS_MANAGER_FIXTURE() :
            m_tempDir( fs::temp_directory_path() / "kicad_settings_manager_test" )
    {
        std::error_code ec;
        fs::remove_all( m_tempDir, ec );

        // Throwing overload so an unusable scratch directory fails setup loudly
        fs::create_directories( m_tempDir );
    }

    ~SETTINGS_MANAGER_FIXTURE()
    {
        std::error_code ec;
        fs::remove_all( m_tempDir, ec );
    }

    wxString Path( const std::string& aName ) const
    {
        return wxString( ( m_tempDir / aName ).string() );
    }

    fs::path m_tempDir;
};


BOOST_FIXTURE_TEST_SUITE( SettingsManager, SETTINGS_MANAGER_FIXTURE )


// Load() may run after a settings object was edited in memory but not yet written; the pending
// edit must be flushed before reloading or the stale on-disk copy silently discards it.
BOOST_AUTO_TEST_CASE( LoadFlushesDirtySettings )
{
    SETTINGS_MANAGER mgr;

    // Drop the auto-registered common settings so Load() only touches the scratch object
    mgr.FlushAndRelease( mgr.GetCommonSettings(), false );

    FLUSH_TEST_SETTINGS* cfg =
            mgr.RegisterSettings( new FLUSH_TEST_SETTINGS( Path( "dirty" ) ), false );

    cfg->SaveToFile();
    cfg->m_value = 42;

    mgr.Load();

    BOOST_CHECK_EQUAL( cfg->m_value, 42 );

    // The dirty value must have reached disk, not merely survived in memory
    FLUSH_TEST_SETTINGS fresh( Path( "dirty" ) );
    fresh.LoadFromFile();
    BOOST_CHECK_EQUAL( fresh.m_value, 42 );
}


// A registered object that has never been synchronized with its file must not be flushed by
// Load(); flushing would overwrite the file with construction state before it is ever read.
BOOST_AUTO_TEST_CASE( LoadDoesNotFlushNeverSyncedSettings )
{
    {
        FLUSH_TEST_SETTINGS seed( Path( "cold" ) );
        seed.m_value = 7;
        seed.SaveToFile();
    }

    SETTINGS_MANAGER mgr;
    mgr.FlushAndRelease( mgr.GetCommonSettings(), false );

    FLUSH_TEST_SETTINGS* cfg =
            mgr.RegisterSettings( new FLUSH_TEST_SETTINGS( Path( "cold" ) ), false );

    mgr.Load();

    BOOST_CHECK_EQUAL( cfg->m_value, 7 );

    FLUSH_TEST_SETTINGS fresh( Path( "cold" ) );
    fresh.LoadFromFile();
    BOOST_CHECK_EQUAL( fresh.m_value, 7 );
}


// An incomplete color theme (missing keys added by a newer build) must not be rewritten merely
// to inject default colors when the user made no change, mirroring the .kicad_pro guarantee.
//
// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24402
BOOST_AUTO_TEST_CASE( ColorThemeNotRewrittenWhenUnchanged )
{
    // Canonical theme written with KiCad's own writer so the reload round-trip is clean.
    {
        COLOR_SETTINGS seed( Path( "theme" ), true );
        seed.SaveToFile( wxEmptyString, true );
    }

    fs::path themePath = m_tempDir / "theme.json";

    auto readFile = []( const fs::path& aPath )
    {
        std::ifstream in( aPath );
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    };

    // Drop a whole colored section so the file mimics a theme saved before those colors existed.
    // Their in-memory values load as defaults, so a no-op load must not resurrect them.
    {
        nlohmann::json js = nlohmann::json::parse( readFile( themePath ) );
        BOOST_REQUIRE( js.contains( "gerbview" ) );
        js.erase( "gerbview" );

        std::ofstream out( themePath );
        out << std::setw( 2 ) << js << std::endl;
        out.close();
    }

    std::string before = readFile( themePath );

    COLOR_SETTINGS cfg( Path( "theme" ), true );
    cfg.LoadFromFile();

    BOOST_CHECK( !cfg.SaveToFile( wxEmptyString ) );
    BOOST_CHECK_EQUAL( before, readFile( themePath ) );
}


// Writes a loadable project plus a lock file owned by aOwner, and returns the project path.
static wxString seedLockedProject( const fs::path& aDir, const std::string& aName,
                                   const nlohmann::json& aOwner )
{
    fs::path pro = aDir / ( aName + ".kicad_pro" );

    {
        std::ofstream out( pro.string() );
        out << R"({"meta": {"filename": ")" << aName << R"(.kicad_pro", "version": 3}})";
    }

    std::ofstream lck( LOCKFILE::LockPathFor( wxString( pro.string() ) ).ToStdString() );
    lck << aOwner.dump();

    return wxString( pro.string() );
}


static nlohmann::json selfOwnerRecord()
{
    nlohmann::json owner;
    owner["username"] = std::string( wxGetUserId().mb_str() );
    owner["hostname"] = std::string( wxGetHostName().mb_str() );
    owner["token"] = "0123456789abcdef0123456789abcdef";
    return owner;
}


// Issue #11458 - a crash-orphaned self lock must be reclaimed, not leave the project read-only
BOOST_AUTO_TEST_CASE( StaleOwnProjectLockIsReclaimedOnLoad )
{
    // No OS lock is held, the state a crash leaves behind
    wxString projectPath = seedLockedProject( m_tempDir, "stale", selfOwnerRecord() );

    SETTINGS_MANAGER mgr;
    BOOST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_REQUIRE( project );

    BOOST_CHECK( !project->IsReadOnly() );
    BOOST_CHECK( project->GetProjectLock() != nullptr );
    BOOST_CHECK( wxFileName::FileExists( LOCKFILE::LockPathFor( projectPath ) ) );
}


// Foreign lock is the negative control - the project must still open read-only, untouched
BOOST_AUTO_TEST_CASE( ForeignProjectLockOpensReadOnlyAndIsNotStolen )
{
    nlohmann::json owner;
    owner["username"] = "someone-else";
    owner["hostname"] = "another-host";

    wxString projectPath = seedLockedProject( m_tempDir, "locked", owner );

    SETTINGS_MANAGER mgr;
    BOOST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_REQUIRE( project );

    // A lock we cannot take degrades to read-only, never to refusing the project
    BOOST_CHECK( project->IsReadOnly() );

    BOOST_REQUIRE( wxFileName::FileExists( LOCKFILE::LockPathFor( projectPath ) ) );

    LOCKFILE reread( projectPath );
    BOOST_CHECK_EQUAL( reread.GetUsername(), wxString( "someone-else" ) );
    BOOST_CHECK_EQUAL( reread.GetHostname(), wxString( "another-host" ) );
}


// A live same-user lock held by another KiCad process must never be taken
BOOST_AUTO_TEST_CASE( LiveProjectLockNotStolenFromAnotherExecutable )
{
    wxString projectPath = seedLockedProject( m_tempDir, "live", selfOwnerRecord() );

    KIPLATFORM::IO::FILE_LOCK owner;
    bool                      created = false;

    BOOST_REQUIRE( owner.Acquire( LOCKFILE::LockPathFor( projectPath ), created )
                   == KIPLATFORM::IO::FILE_LOCK::STATE::HELD );

    SETTINGS_MANAGER mgr;
    BOOST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_REQUIRE( project );

    BOOST_CHECK( project->IsReadOnly() );

    BOOST_REQUIRE( wxFileName::FileExists( LOCKFILE::LockPathFor( projectPath ) ) );

    std::ifstream in( LOCKFILE::LockPathFor( projectPath ).ToStdString() );
    BOOST_CHECK_EQUAL( nlohmann::json::parse( in ).value( "token", std::string() ),
                       std::string( "0123456789abcdef0123456789abcdef" ) );
}


class TEST_KIWAY : public KIWAY
{
public:
    TEST_KIWAY( SETTINGS_MANAGER& aManager ) :
            KIWAY( KFCTL_STANDALONE ),
            m_manager( aManager )
    {
    }

    void ProjectChanged() override
    {
        m_notified = true;
        m_lockHeldWhenNotified = m_manager.Prj().GetProjectLock() != nullptr;
    }

    bool Notified() const { return m_notified; }
    bool LockHeldWhenNotified() const { return m_lockHeldWhenNotified; }

private:
    SETTINGS_MANAGER& m_manager;
    bool              m_notified = false;
    bool              m_lockHeldWhenNotified = false;
};


BOOST_AUTO_TEST_CASE( ProjectOwnsItsLockBeforeTheChangeIsAnnounced )
{
    fs::path pro = m_tempDir / "unversioned.kicad_pro";

    {
        std::ofstream out( pro.string() );
        out << "{}";
    }

    wxString projectPath = wxString( pro.string() );

    SETTINGS_MANAGER mgr;
    TEST_KIWAY       kiway( mgr );

    mgr.SetKiway( &kiway );
    mgr.LoadProject( projectPath );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_REQUIRE( project );
    BOOST_REQUIRE( kiway.Notified() );

    BOOST_CHECK_MESSAGE( kiway.LockHeldWhenNotified(), "The project must own its lock before the change is announced" );

    BOOST_CHECK( project->GetProjectLock() != nullptr );
    BOOST_CHECK( wxFileName::FileExists( LOCKFILE::LockPathFor( projectPath ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
