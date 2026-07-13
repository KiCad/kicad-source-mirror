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

#include <mdw_runtime/mdw_file_io.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>

namespace fs = std::filesystem;


BOOST_AUTO_TEST_SUITE( MdwFileIo )


// A save made after the caller read the file must survive the replacement that read was for
BOOST_AUTO_TEST_CASE( ReplaceLeavesAFileSavedSinceItWasRead )
{
    const fs::path folder = fs::temp_directory_path() / ( "mdw-file-io-" + std::to_string( std::random_device{}() ) );
    const fs::path file = folder / "passives.kicad_sym";

    fs::create_directories( folder );
    std::ofstream( file, std::ios::binary ) << "read";

    std::ptrdiff_t written = 0;

    // Saved once the replacement is written, the last moment the check can still see it
    auto unsaved = [&]()
    {
        written = std::distance( fs::directory_iterator( folder ), fs::directory_iterator() );
        std::ofstream( file, std::ios::binary | std::ios::trunc ) << "saved";
        return MDW_ReadFile( file ) == std::string( "read" );
    };

    BOOST_CHECK( MDW_ReplaceFileIf( file, "replacement", unsaved ) == std::errc::operation_canceled );
    BOOST_CHECK_EQUAL( written, 2 );
    BOOST_CHECK( MDW_ReadFile( file ) == std::string( "saved" ) );
    BOOST_CHECK_EQUAL( std::distance( fs::directory_iterator( folder ), fs::directory_iterator() ), 1 );

    fs::remove_all( folder );
}


BOOST_AUTO_TEST_SUITE_END()
