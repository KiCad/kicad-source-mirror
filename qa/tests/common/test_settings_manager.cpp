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

#include <settings/common_settings.h>
#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>

#include <filesystem>
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


BOOST_AUTO_TEST_SUITE_END()
