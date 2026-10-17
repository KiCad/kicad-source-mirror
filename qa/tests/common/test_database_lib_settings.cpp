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
#include <database/database_lib_settings.h>
#include <json_common.h>

#include <wx/utils.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;


BOOST_AUTO_TEST_SUITE( DatabaseLibSettings )


/**
 * The settings editor rewrites a user's hand-edited file, so a save must carry the edit and must not
 * drop what the editor does not know about.
 */
BOOST_AUTO_TEST_CASE( UnknownKeysSurviveAnEdit )
{
    fs::path dir = fs::temp_directory_path() / ( "kicad_dblib_settings_" + std::to_string( wxGetProcessId() ) );
    fs::path path = dir / "qa_dblib.kicad_dbl";

    fs::remove_all( dir );
    fs::create_directories( dir );
    fs::copy_file( fs::path( KI_TEST::GetTestDataRootDir() ) / "dblib" / "qa_dblib.kicad_dbl", path );

    auto readJson = [&]()
    {
        std::ifstream     in( path, std::ios::binary );
        std::stringstream buffer;
        buffer << in.rdbuf();
        return nlohmann::json::parse( buffer.str() );
    };

    nlohmann::json json = readJson();

    json["libraries"][0]["x_vendor"] = 1;
    json["libraries"][0]["fields"][0]["x_vendor"] = 2;
    json["libraries"][0]["properties"]["x_vendor"] = 3;

    {
        std::ofstream out( path, std::ios::binary | std::ios::trunc );
        out << json.dump( 2 );
    }

    {
        std::unique_ptr<DATABASE_LIB_SETTINGS> settings =
                LoadDatabaseLibSettingsReadOnly( wxString::FromUTF8( path.string() ) );

        settings->m_Tables[0].key_col = "MPN";
        settings->SetReadOnly( false );
        BOOST_REQUIRE( settings->SaveToFile() );
    }

    nlohmann::json saved = readJson();

    BOOST_CHECK_EQUAL( saved["libraries"][0]["key"].get<std::string>(), "MPN" );
    BOOST_CHECK_EQUAL( saved["libraries"][0]["x_vendor"].get<int>(), 1 );
    BOOST_CHECK_EQUAL( saved["libraries"][0]["fields"][0]["x_vendor"].get<int>(), 2 );
    BOOST_CHECK_EQUAL( saved["libraries"][0]["properties"]["x_vendor"].get<int>(), 3 );

    fs::remove_all( dir );
}


BOOST_AUTO_TEST_SUITE_END()
