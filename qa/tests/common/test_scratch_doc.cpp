/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <jobs/scratch_doc.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>
#include <wx/utils.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>


BOOST_AUTO_TEST_SUITE( ScratchDoc )


// A diff of a file the editor has open names the live project's own path. That path must get
// a scratch project of its own, and the live project must still be there afterwards.
BOOST_AUTO_TEST_CASE( LivePathGetsItsOwnScratchProject )
{
    std::error_code       ec;
    std::filesystem::path dir = std::filesystem::temp_directory_path( ec )
                                / ( "kicad_qa_scratch_doc_live_" + std::to_string( wxGetProcessId() ) );

    std::filesystem::remove_all( dir, ec );
    std::filesystem::create_directories( dir, ec );

    std::filesystem::path proPath = dir / "live.kicad_pro";
    std::filesystem::path schPath = dir / "live.kicad_sch";

    {
        std::ofstream proFile( proPath );
        proFile << R"({ "meta": { "filename": "live.kicad_pro", "version": 3 } })";
        std::ofstream schFile( schPath );
        schFile << "(kicad_sch)";
    }

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    // However the test ends, later tests must not inherit this project as the active one.
    struct ACTIVE_PROJECT_RESET
    {
        SETTINGS_MANAGER&     mgr;
        std::filesystem::path dir;

        ~ACTIVE_PROJECT_RESET()
        {
            std::error_code ignored;
            mgr.LoadProject( "" );
            std::filesystem::remove_all( dir, ignored );
        }
    } reset{ mgr, dir };

    BOOST_REQUIRE( mgr.LoadProject( wxString( proPath.string() ) ) );

    PROJECT* live = &mgr.Prj();
    BOOST_REQUIRE_EQUAL( live, mgr.GetProject( wxString( proPath.string() ) ) );

    PROJECT* handed = nullptr;
    wxString scratchPath;

    {
        SCRATCH_DOC<int> doc = LoadScratchDoc<int>(
                mgr, wxString( schPath.string() ),
                [&handed, &scratchPath]( PROJECT* aProject )
                {
                    handed = aProject;
                    scratchPath = aProject->GetProjectFullName();
                    return std::make_unique<int>( 1 );
                },
                []( int* ) {} );

        BOOST_REQUIRE( doc.doc );
        BOOST_REQUIRE( handed );
        BOOST_CHECK_MESSAGE( handed != live, "the live project was handed to the loader" );
        BOOST_CHECK( doc.ownsProject );
        BOOST_CHECK( scratchPath.StartsWith( wxFileName::GetTempDir() ) );
    }

    // The scratch project is gone, the live one is untouched.
    BOOST_CHECK( mgr.GetProject( scratchPath ) == nullptr );
    BOOST_CHECK_EQUAL( &mgr.Prj(), live );
    BOOST_CHECK_EQUAL( mgr.GetProject( wxString( proPath.string() ) ), live );
}


// A path outside the live project keeps the existing behaviour: its own project, loaded and
// unloaded by the wrapper.
BOOST_AUTO_TEST_CASE( OtherPathLoadsAndUnloadsItsProject )
{
    std::error_code       ec;
    std::filesystem::path dir = std::filesystem::temp_directory_path( ec )
                                / ( "kicad_qa_scratch_doc_other_" + std::to_string( wxGetProcessId() ) );

    std::filesystem::remove_all( dir, ec );
    std::filesystem::create_directories( dir, ec );

    std::filesystem::path schPath = dir / "other.kicad_sch";

    {
        std::ofstream schFile( schPath );
        schFile << "(kicad_sch)";
    }

    // With no project open the active one is the empty project, which no path can match.
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    mgr.LoadProject( "" );

    wxString proPath = wxString( ( dir / "other.kicad_pro" ).string() );
    PROJECT* handed = nullptr;

    {
        SCRATCH_DOC<int> doc = LoadScratchDoc<int>(
                mgr, wxString( schPath.string() ),
                [&handed]( PROJECT* aProject )
                {
                    handed = aProject;
                    return std::make_unique<int>( 1 );
                },
                []( int* ) {} );

        BOOST_REQUIRE( handed );
        BOOST_CHECK_EQUAL( handed, mgr.GetProject( proPath ) );
        BOOST_CHECK( handed != &mgr.Prj() );
    }

    BOOST_CHECK( mgr.GetProject( proPath ) == nullptr );
    std::filesystem::remove_all( dir, ec );
}


BOOST_AUTO_TEST_SUITE_END()
