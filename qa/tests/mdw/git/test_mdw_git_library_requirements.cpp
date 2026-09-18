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

#include <libraries/library_manager.h>
#include <mdw/client/mdw_library_sources.h>

#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace fs = std::filesystem;


namespace
{

/// The library nicknames a design file's items use, read from its saved text
std::set<wxString> nicknamesIn( const std::string& aData, const std::string& aKeyword )
{
    std::ifstream      in( fs::path( KI_TEST::GetTestDataRootDir() ) / aData, std::ios::binary );
    std::ostringstream text;

    text << in.rdbuf();

    const std::string  content = text.str();
    const std::regex  item( "\\(" + aKeyword + " \"([^\":]+):" );
    std::set<wxString> found;

    for( std::sregex_iterator it( content.begin(), content.end(), item ), end; it != end; ++it )
        found.insert( wxString::FromUTF8( ( *it )[1].str() ) );

    return found;
}


void writeTable( const fs::path& aFile, const char* aTag,
                const std::vector<std::pair<std::string, std::string>>& aRows )
{
    std::ofstream out( aFile );

    out << "(" << aTag << "\n\t(version 7)\n";

    for( const auto& [nickname, options] : aRows )
    {
        out << "\t(lib (name \"" << nickname << "\") (type \"KiCad\") (uri \"${KIPRJMOD}/" << nickname
            << "\") (options \"" << options << "\") (descr \"\"))\n";
    }

    out << ")\n";
}

} // namespace


BOOST_AUTO_TEST_SUITE( MdwGitLibraryRequirements )


// The project records the sources its design uses and none it merely has rows for
BOOST_AUTO_TEST_CASE( RequirementsForUsedNicknames )
{
    KI_TEST::SCOPED_TEMP_DIR temp( wxS( "mdw-requirements" ) );

    MDW_LIBRARY_SOURCES sources( temp.ChildPathStr( wxS( "sources.json" ) ), temp.ChildPathStr( wxS( "site.json" ) ),
                                 temp.CreateChildDirStr( wxS( "copies" ) ) );

    MDW_LIBRARY_SOURCE_CONFIG acme;
    acme.backend_id = wxS( "org.kicad.mdw.git" );
    acme.locator = wxS( "https://git.example.com/acme-library.git" );
    acme.name = wxS( "acme-library" );
    BOOST_REQUIRE( sources.Add( acme ).IsOk() );

    MDW_LIBRARY_SOURCE_CONFIG legacy;
    legacy.backend_id = wxS( "org.kicad.mdw.git" );
    legacy.locator = wxS( "https://git.example.com/acme-legacy.git" );
    legacy.name = wxS( "acme-legacy" );
    BOOST_REQUIRE( sources.Add( legacy ).IsOk() );

    const std::set<wxString> symbols = nicknamesIn( "common/git/mdw_ctrl_board/ctrl-board.kicad_sch", "lib_id" );
    const std::set<wxString> footprints = nicknamesIn( "common/git/mdw_ctrl_board/ctrl-board.kicad_pcb", "footprint" );

    BOOST_REQUIRE( symbols.count( wxS( "acme-passives" ) ) );
    BOOST_REQUIRE( footprints.count( wxS( "acme-fp_capacitors" ) ) );

    const fs::path project = fs::path( temp.Path() ) / "project";
    fs::create_directories( project );

    const std::string acmeOptions = "mdw=" + acme.id.utf8_string();
    const std::string legacyOptions = "mdw=" + legacy.id.utf8_string();

    std::vector<std::pair<std::string, std::string>> symbolRows;
    std::vector<std::pair<std::string, std::string>> footprintRows;

    for( const wxString& nickname : symbols )
        symbolRows.emplace_back( nickname.utf8_string(), acmeOptions );

    for( const wxString& nickname : footprints )
        footprintRows.emplace_back( nickname.utf8_string(), acmeOptions );

    // Rows the design does not use, one from another source and one no team owns
    symbolRows.emplace_back( "legacy-valves", legacyOptions );
    symbolRows.emplace_back( "local-parts", "" );

    writeTable( project / "sym-lib-table", "sym_lib_table", symbolRows );
    writeTable( project / "fp-lib-table", "fp_lib_table", footprintRows );

    LIBRARY_MANAGER tables;
    tables.LoadProjectTables( wxString::FromUTF8( project.string() ),
                              { LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_TYPE::FOOTPRINT } );

    std::vector<MDW_LIBRARY_REQUIREMENT> needed =
            MDW_LIBRARY_SOURCES::RequirementsFor( symbols, tables, MDW_LIBRARY_TYPE::SYMBOL, sources );

    BOOST_REQUIRE_EQUAL( needed.size(), 1u );
    BOOST_CHECK_EQUAL( needed[0].id, acme.id );
    BOOST_CHECK_EQUAL( needed[0].locator, acme.locator );
    BOOST_CHECK_EQUAL( needed[0].name, acme.name );
    BOOST_CHECK( needed[0].pin.token.IsEmpty() );

    // The legacy source is named once a nickname of its rows is used
    std::set<wxString> withLegacy = symbols;
    withLegacy.insert( wxS( "legacy-valves" ) );
    withLegacy.insert( wxS( "local-parts" ) );
    withLegacy.insert( wxS( "no-such-row" ) );

    needed = MDW_LIBRARY_SOURCES::RequirementsFor( withLegacy, tables, MDW_LIBRARY_TYPE::SYMBOL, sources );
    BOOST_CHECK_EQUAL( needed.size(), 2u );

    // A symbol-only nickname set finds nothing in the footprint table
    needed = MDW_LIBRARY_SOURCES::RequirementsFor( { wxS( "legacy-valves" ) }, tables, MDW_LIBRARY_TYPE::FOOTPRINT,
                                                   sources );
    BOOST_CHECK( needed.empty() );

    needed = MDW_LIBRARY_SOURCES::RequirementsFor( footprints, tables, MDW_LIBRARY_TYPE::FOOTPRINT, sources );
    BOOST_REQUIRE_EQUAL( needed.size(), 1u );
    BOOST_CHECK_EQUAL( needed[0].id, acme.id );

    // Recording adds only what the project lacks
    std::vector<MDW_LIBRARY_REQUIREMENT> recorded;

    BOOST_CHECK( MDW_LIBRARY_SOURCES::Record( recorded, needed ) );
    BOOST_CHECK( !MDW_LIBRARY_SOURCES::Record( recorded, needed ) );
    BOOST_CHECK_EQUAL( recorded.size(), 1u );
}


BOOST_AUTO_TEST_SUITE_END()
