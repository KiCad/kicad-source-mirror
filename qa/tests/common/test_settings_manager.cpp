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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;


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


BOOST_AUTO_TEST_SUITE_END()
