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

#include <mdw/mdw_backend_registry.h>

#include <qa_utils/file_utils.h>

#include <json_common.h>

#include <filesystem>
#include <fstream>
#include <string>


BOOST_AUTO_TEST_SUITE( MdwBackendRegistry )


// A backend folder left with its manifest but no executable is not offered
BOOST_AUTO_TEST_CASE( OffersOnlyBackendsWithTheirExecutable )
{
    KI_TEST::SCOPED_TEMP_DIR    temp( wxS( "mdw-qa-registry" ) );
    const std::filesystem::path dir = temp.Path();

    auto writeManifest = []( const std::filesystem::path& aDir, const std::string& aId, const std::string& aExecutable )
    {
        std::filesystem::create_directories( aDir );
        std::ofstream( aDir / ( aId + ".kicad-mdw.json" ) )
                << nlohmann::json( { { "id", aId }, { "executable", aExecutable } } ).dump();
    };

    writeManifest( dir, "org.kicad.mdw.qa.realm", MDW_REALM_EXECUTABLE );
    writeManifest( dir / "org.kicad.mdw.qa.stale", "org.kicad.mdw.qa.stale", "kicad-mdw-qa-stale" );

    MDW_BACKEND_REGISTRY registry;
    registry.Discover( { temp.PathStr() } );

    BOOST_REQUIRE_EQUAL( registry.Manifests().size(), 1u );
    BOOST_CHECK( registry.Manifests().front().backend_id == wxS( "org.kicad.mdw.qa.realm" ) );
}


BOOST_AUTO_TEST_SUITE_END()
