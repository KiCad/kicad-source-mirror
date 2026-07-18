/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

/**
 * @file test_project_file.cpp
 * Tests for PROJECT_FILE class.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>

#include <json_common.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;


class PROJECT_FILE_TEST_FIXTURE
{
public:
    PROJECT_FILE_TEST_FIXTURE()
    {
        m_tempDir = fs::temp_directory_path() / "kicad_project_file_test";
        fs::remove_all( m_tempDir );
        fs::create_directories( m_tempDir );
    }

    ~PROJECT_FILE_TEST_FIXTURE()
    {
        fs::remove_all( m_tempDir );
    }

    fs::path m_tempDir;
};


BOOST_FIXTURE_TEST_SUITE( ProjectFile, PROJECT_FILE_TEST_FIXTURE )


/**
 * Test that SaveAs updates top-level sheet names when they match the old project name.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/22853
 */
BOOST_AUTO_TEST_CASE( SaveAsUpdatesTopLevelSheetNames )
{
    fs::path oldProjectDir = m_tempDir / "old_project";
    fs::path newProjectDir = m_tempDir / "new_project";
    fs::create_directories( oldProjectDir );
    fs::create_directories( newProjectDir );

    wxString oldProjectPath = wxString( oldProjectDir.string() ) + wxFileName::GetPathSeparator()
                              + wxS( "old_project." ) + FILEEXT::ProjectFileExtension;

    PROJECT_FILE projectFile( oldProjectPath );

    // Add a top-level sheet with name matching the project name
    TOP_LEVEL_SHEET_INFO sheetInfo;
    sheetInfo.uuid = KIID();
    sheetInfo.name = wxS( "old_project" );
    sheetInfo.filename = wxS( "old_project.kicad_sch" );

    projectFile.GetTopLevelSheets().push_back( sheetInfo );

    // Add another sheet with a custom name that should NOT be changed
    TOP_LEVEL_SHEET_INFO customSheet;
    customSheet.uuid = KIID();
    customSheet.name = wxS( "CustomSheet" );
    customSheet.filename = wxS( "custom_sheet.kicad_sch" );

    projectFile.GetTopLevelSheets().push_back( customSheet );

    // Perform SaveAs to new project
    projectFile.SaveAs( wxString( newProjectDir.string() ), wxS( "new_project" ) );

    // Verify the sheet name was updated
    const std::vector<TOP_LEVEL_SHEET_INFO>& sheets = projectFile.GetTopLevelSheets();

    BOOST_REQUIRE_EQUAL( sheets.size(), 2 );

    // First sheet's name should be updated to match the new project name
    BOOST_CHECK_EQUAL( sheets[0].name, wxS( "new_project" ) );

    // First sheet's filename should also be updated
    BOOST_CHECK_EQUAL( sheets[0].filename, wxS( "new_project.kicad_sch" ) );

    // Second sheet's custom name should remain unchanged
    BOOST_CHECK_EQUAL( sheets[1].name, wxS( "CustomSheet" ) );

    // Second sheet's filename should remain unchanged
    BOOST_CHECK_EQUAL( sheets[1].filename, wxS( "custom_sheet.kicad_sch" ) );
}


/**
 * Test that LoadFromFile fixes stale top_level_sheets references after template copy.
 *
 * When a project is created from a template, files are renamed to match the new project name,
 * but the .kicad_pro content still references the template's filenames. LoadFromFile should
 * detect and correct these stale references.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/22951
 */
BOOST_AUTO_TEST_CASE( LoadFixesStaleTopLevelSheetReferences )
{
    fs::path projectDir = m_tempDir / "my_project";
    fs::create_directories( projectDir );

    // Write a .kicad_pro that references "default.kicad_sch" (as if copied from a template)
    std::string proContent = R"({
        "meta": {
            "filename": "my_project.kicad_pro",
            "version": 3
        },
        "schematic": {
            "top_level_sheets": [
                {
                    "uuid": "00000000-0000-0000-0000-000000000000",
                    "name": "default",
                    "filename": "default.kicad_sch"
                }
            ]
        }
    })";

    fs::path proPath = projectDir / "my_project.kicad_pro";
    std::ofstream proFile( proPath );
    proFile << proContent;
    proFile.close();

    // Create the renamed schematic file (as if the template copy renamed it)
    fs::path schPath = projectDir / "my_project.kicad_sch";
    std::ofstream schFile( schPath );
    schFile << "(kicad_sch (version 20231120) (generator \"eeschema\") (generator_version \"9.99\")";
    schFile << " (uuid \"12345678-1234-1234-1234-123456789abc\")";
    schFile << " (paper \"A4\"))";
    schFile.close();

    // Load the project using SETTINGS_MANAGER
    SETTINGS_MANAGER settingsManager;
    settingsManager.LoadProject( wxString( proPath.string() ) );

    PROJECT& project = settingsManager.Prj();
    PROJECT_FILE& projectFile = project.GetProjectFile();

    const std::vector<TOP_LEVEL_SHEET_INFO>& sheets = projectFile.GetTopLevelSheets();

    BOOST_REQUIRE_EQUAL( sheets.size(), 1 );

    // The filename should have been corrected from "default.kicad_sch" to "my_project.kicad_sch"
    BOOST_CHECK_EQUAL( sheets[0].filename, wxS( "my_project.kicad_sch" ) );

    // The name should also be updated
    BOOST_CHECK_EQUAL( sheets[0].name, wxS( "my_project" ) );
}


/**
 * Test that LoadFromFile does NOT modify top_level_sheets when references are already valid.
 */
BOOST_AUTO_TEST_CASE( LoadPreservesValidTopLevelSheetReferences )
{
    fs::path projectDir = m_tempDir / "valid_project";
    fs::create_directories( projectDir );

    // Write a .kicad_pro with valid references
    std::string proContent = R"({
        "meta": {
            "filename": "valid_project.kicad_pro",
            "version": 3
        },
        "schematic": {
            "top_level_sheets": [
                {
                    "uuid": "00000000-0000-0000-0000-000000000000",
                    "name": "valid_project",
                    "filename": "valid_project.kicad_sch"
                }
            ]
        }
    })";

    fs::path proPath = projectDir / "valid_project.kicad_pro";
    std::ofstream proFile( proPath );
    proFile << proContent;
    proFile.close();

    // Create the schematic file that matches the reference
    fs::path schPath = projectDir / "valid_project.kicad_sch";
    std::ofstream schFile( schPath );
    schFile << "(kicad_sch (version 20231120) (generator \"eeschema\") (generator_version \"9.99\")";
    schFile << " (uuid \"12345678-1234-1234-1234-123456789abc\")";
    schFile << " (paper \"A4\"))";
    schFile.close();

    SETTINGS_MANAGER settingsManager;
    settingsManager.LoadProject( wxString( proPath.string() ) );

    PROJECT& project = settingsManager.Prj();
    PROJECT_FILE& projectFile = project.GetProjectFile();

    const std::vector<TOP_LEVEL_SHEET_INFO>& sheets = projectFile.GetTopLevelSheets();

    BOOST_REQUIRE_EQUAL( sheets.size(), 1 );

    // References should be unchanged
    BOOST_CHECK_EQUAL( sheets[0].filename, wxS( "valid_project.kicad_sch" ) );
    BOOST_CHECK_EQUAL( sheets[0].name, wxS( "valid_project" ) );
}


/**
 * Test that loading a project by an absolute path is idempotent and does not evict (and free) the
 * already-loaded project when the same project is loaded again by its absolute path.
 *
 * This guards the invariant relied upon by `kicad-cli jobset run`. The jobset runner loads the
 * project once and holds the resulting PROJECT* across many kiface job calls. Each kiface board
 * loader looks the project up by its absolute path and loads it if not already present. If the
 * jobset runner had loaded the project by a relative path, the two paths would key different map
 * entries, the second (active) load would evict and free the first, and the held PROJECT* would
 * dangle, crashing in JOBS_OUTPUT_ARCHIVE::HandleOutputs.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24474
 */
BOOST_AUTO_TEST_CASE( LoadProjectByAbsolutePathIsStable )
{
    fs::path projectDir = m_tempDir / "jobset_project";
    fs::create_directories( projectDir );

    std::string proContent = R"({
        "meta": {
            "filename": "jobset_project.kicad_pro",
            "version": 3
        }
    })";

    fs::path proPath = projectDir / "jobset_project.kicad_pro";
    std::ofstream proFile( proPath );
    proFile << proContent;
    proFile.close();

    wxFileName absFn( wxString( proPath.string() ) );
    absFn.MakeAbsolute();
    wxString absPath = absFn.GetFullPath();

    SETTINGS_MANAGER settingsManager;

    // Load as the fixed jobset runner does: by absolute path.
    BOOST_REQUIRE( settingsManager.LoadProject( absPath ) );

    PROJECT* heldProject = settingsManager.GetProject( absPath );
    BOOST_REQUIRE( heldProject != nullptr );

    // Simulate the kiface board loader resolving and (re)loading the project by its absolute path.
    BOOST_REQUIRE( settingsManager.LoadProject( absPath, true ) );

    // The second load must be a no-op for the held pointer; the project must not have been evicted.
    PROJECT* afterReload = settingsManager.GetProject( absPath );
    BOOST_CHECK( afterReload == heldProject );

    // The held pointer must still resolve to the same project name (i.e. it was not freed).
    BOOST_CHECK_EQUAL( heldProject->GetProjectFullName(), absPath );
}


/**
 * Unloading a non-active project must save it to its own directory, not the active one's.
 *
 * SETTINGS_MANAGER once resolved the save path through Prj() (the active project) rather
 * than the owning project, so unloading a same-named resident project clobbered the active
 * project's .kicad_pro with the unloaded project's data.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24607
 */
BOOST_AUTO_TEST_CASE( UnloadProjectSavesToOwnDirectory )
{
    fs::path projADir = m_tempDir / "proj_a";
    fs::path projBDir = m_tempDir / "proj_b";
    fs::create_directories( projADir );
    fs::create_directories( projBDir );

    // Shared basename in different directories reproduces the cross-project clobber.  The
    // matching schematic keeps LoadFromFile from flagging the project as migrated, which
    // would otherwise suppress the auto-save under test.
    const std::string projectName = "shared_name";

    auto writeProject = [&]( const fs::path& aDir )
    {
        std::string content = "{\n"
                              "    \"meta\": {\n"
                              "        \"filename\": \"" + projectName + ".kicad_pro\",\n"
                              "        \"version\": 3\n"
                              "    },\n"
                              "    \"schematic\": {\n"
                              "        \"top_level_sheets\": [\n"
                              "            {\n"
                              "                \"uuid\": \"00000000-0000-0000-0000-000000000000\",\n"
                              "                \"name\": \"" + projectName + "\",\n"
                              "                \"filename\": \"" + projectName + ".kicad_sch\"\n"
                              "            }\n"
                              "        ]\n"
                              "    }\n"
                              "}\n";
        std::ofstream out( aDir / ( projectName + ".kicad_pro" ) );
        out << content;
        out.close();

        std::ofstream sch( aDir / ( projectName + ".kicad_sch" ) );
        sch << "(kicad_sch (version 20231120) (generator \"eeschema\") (generator_version \"9.99\")";
        sch << " (uuid \"12345678-1234-1234-1234-123456789abc\") (paper \"A4\"))";
        sch.close();
    };

    fs::path proAPath = projADir / ( projectName + ".kicad_pro" );
    fs::path proBPath = projBDir / ( projectName + ".kicad_pro" );
    writeProject( projADir );
    writeProject( projBDir );

    SETTINGS_MANAGER mgr;

    // A becomes the active project; B is loaded but left non-active so both are resident.
    BOOST_REQUIRE( mgr.LoadProject( wxString( proAPath.string() ), true ) );
    BOOST_REQUIRE( mgr.LoadProject( wxString( proBPath.string() ), false ) );

    PROJECT* projB = mgr.GetProject( wxString( proBPath.string() ) );
    BOOST_REQUIRE( projB != nullptr );

    // Prj() must be A so that a Prj()-based path resolution would target the wrong directory.
    BOOST_REQUIRE_EQUAL( mgr.Prj().GetProjectFullName(), wxString( proAPath.string() ) );

    // Mark B's project file so the save has something distinctive to persist.
    projB->GetProjectFile().m_TextVars[wxS( "OWNER" )] = wxS( "proj_b" );

    BOOST_REQUIRE( mgr.UnloadProject( projB, true ) );

    auto readFile = []( const fs::path& aPath )
    {
        std::ifstream in( aPath );
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    };

    // B's own file must have received B's marker.
    std::string savedB = readFile( proBPath );
    BOOST_CHECK_MESSAGE( savedB.find( "OWNER" ) != std::string::npos,
                         "unloaded project must be saved to its own directory" );

    // A's identically named file must not have been clobbered with B's data.
    std::string savedA = readFile( proAPath );
    BOOST_CHECK_MESSAGE( savedA.find( "OWNER" ) == std::string::npos,
                         "active project's file must not receive the unloaded project's data" );
}


/**
 * Opening a project and saving it without any user change must not rewrite the .kicad_pro.
 *
 * A file written by an older build omits parameters added since, so on load those parameters
 * hold their defaults while being absent from the file. Store() once counted every such absent
 * parameter as modified, which resurrected the missing keys and rewrote an otherwise-unchanged
 * project (and its .kicad_prl), spuriously touching version control and file timestamps.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24402
 */
BOOST_AUTO_TEST_CASE( NoRewriteWhenUnchanged )
{
    fs::path projectDir = m_tempDir / "unchanged_project";
    fs::create_directories( projectDir );

    fs::path proPath = projectDir / "unchanged_project.kicad_pro";

    // Produce a canonical, fully-populated current-version file with KiCad's own writer so the
    // reload round-trip is otherwise clean.
    {
        std::ofstream seed( proPath );
        seed << R"({"meta":{"version":3}})";
        seed.close();

        SETTINGS_MANAGER mgr;
        BOOST_REQUIRE( mgr.LoadProject( wxString( proPath.string() ), true ) );
        BOOST_REQUIRE( mgr.SaveProject() );
        mgr.UnloadProject( &mgr.Prj(), false );
    }

    auto readFile = []( const fs::path& aPath )
    {
        std::ifstream in( aPath );
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    };

    // Drop keys the file would omit if saved before those parameters existed. The scalar
    // board.ipc2581 block covers the plain PARAM path; schematic.bus_aliases (default null in
    // memory but serialized as {}) covers the PARAM_LAMBDA path. Both hold their defaults, so a
    // no-op load must not resurrect them.
    {
        nlohmann::json js = nlohmann::json::parse( readFile( proPath ) );
        js["board"].erase( "ipc2581" );
        js["schematic"].erase( "bus_aliases" );

        std::ofstream out( proPath );
        out << std::setw( 2 ) << js << std::endl;
        out.close();
    }

    std::string before = readFile( proPath );
    BOOST_REQUIRE( before.find( "ipc2581" ) == std::string::npos );
    BOOST_REQUIRE( before.find( "bus_aliases" ) == std::string::npos );

    SETTINGS_MANAGER mgr;
    BOOST_REQUIRE( mgr.LoadProject( wxString( proPath.string() ), true ) );

    PROJECT_FILE& projectFile = mgr.Prj().GetProjectFile();

    // The auto-save path must decline to write when nothing changed.
    BOOST_CHECK( !projectFile.SaveToFile( wxString( projectDir.string() ) ) );

    // And the on-disk file must be byte-for-byte unchanged.
    BOOST_CHECK_EQUAL( before, readFile( proPath ) );
}


BOOST_AUTO_TEST_SUITE_END()
