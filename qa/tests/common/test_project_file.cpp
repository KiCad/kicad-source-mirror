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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_project_file.cpp
 * Tests for PROJECT_FILE class.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <filesystem>
#include <fstream>

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


BOOST_AUTO_TEST_SUITE_END()
