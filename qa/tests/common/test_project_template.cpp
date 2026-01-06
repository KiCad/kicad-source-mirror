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
 * @file
 * Tests for PROJECT_TEMPLATE class.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <project_template.h>

#include <wx/dir.h>
#include <wx/filename.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;


class PROJECT_TEMPLATE_TEST_FIXTURE
{
public:
    PROJECT_TEMPLATE_TEST_FIXTURE()
    {
        m_tempDir = fs::temp_directory_path() / "kicad_template_test";
        fs::remove_all( m_tempDir );
        fs::create_directories( m_tempDir );
    }

    ~PROJECT_TEMPLATE_TEST_FIXTURE()
    {
        fs::remove_all( m_tempDir );
    }

    void CreateTemplateStructure( const std::string& templateName,
                                  const std::vector<std::string>& subdirs,
                                  const std::vector<std::string>& files )
    {
        fs::path templatePath = m_tempDir / templateName;
        fs::create_directories( templatePath );

        fs::path metaPath = templatePath / "meta";
        fs::create_directories( metaPath );

        std::ofstream infoFile( ( metaPath / "info.html" ).string() );
        infoFile << "<html><head><title>Test Template</title></head><body></body></html>";
        infoFile.close();

        for( const auto& subdir : subdirs )
        {
            fs::create_directories( templatePath / subdir );
        }

        for( const auto& file : files )
        {
            fs::path filePath = templatePath / file;
            fs::create_directories( filePath.parent_path() );
            std::ofstream f( filePath.string() );
            f << "test content";
            f.close();
        }
    }

    fs::path m_tempDir;
};


BOOST_FIXTURE_TEST_SUITE( ProjectTemplate, PROJECT_TEMPLATE_TEST_FIXTURE )


BOOST_AUTO_TEST_CASE( DirectoriesRenamedCorrectly )
{
    // Create a template with subdirectories that should be renamed
    CreateTemplateStructure(
            "issue22289",
            { "issue22289-dir", "issue22289-backups", "other-dir" },
            { "issue22289.kicad_pro", "issue22289.kicad_sch", "issue22289-dir/test.kicad_sym" } );

    fs::path templatePath = m_tempDir / "issue22289";
    fs::path destPath = m_tempDir / "myproject";
    fs::create_directories( destPath );

    PROJECT_TEMPLATE tmpl( wxString::FromUTF8( templatePath.string() ) );

    // GetDestinationFiles expects a wxFileName with the project file path
    wxFileName newProjectPath;
    newProjectPath.SetPath( wxString::FromUTF8( destPath.string() ) );
    newProjectPath.SetName( wxS( "myproject" ) );
    newProjectPath.SetExt( wxS( "kicad_pro" ) );

    std::vector<wxFileName> destFiles;
    tmpl.GetDestinationFiles( newProjectPath, destFiles );

    bool foundRenamedDir = false;
    bool foundRenamedFile = false;
    bool foundOtherDir = false;

    for( const wxFileName& destFile : destFiles )
    {
        wxString fullPath = destFile.GetFullPath();

        if( fullPath.Contains( wxS( "myproject-dir" ) ) )
            foundRenamedDir = true;

        if( fullPath.Contains( wxS( "issue22289-dir" ) ) )
            BOOST_FAIL( "Directory should have been renamed from issue22289-dir to myproject-dir" );

        if( fullPath.Contains( wxS( "other-dir" ) ) )
            foundOtherDir = true;

        if( destFile.GetName() == wxS( "myproject" ) && destFile.GetExt() == wxS( "kicad_pro" ) )
            foundRenamedFile = true;
    }

    BOOST_CHECK_MESSAGE( foundRenamedDir, "Should find myproject-dir in destination files" );
    BOOST_CHECK_MESSAGE( foundRenamedFile, "Should find myproject.kicad_pro in destination files" );
    BOOST_CHECK_MESSAGE( foundOtherDir, "Should preserve other-dir (not matching template name)" );
}


BOOST_AUTO_TEST_CASE( CreateProjectRenamesDirectories )
{
    CreateTemplateStructure(
            "testtemplate",
            { "testtemplate-lib", "testtemplate" },
            { "testtemplate.kicad_pro", "testtemplate-lib/component.kicad_sym",
              "testtemplate/nested.txt" } );

    fs::path templatePath = m_tempDir / "testtemplate";
    fs::path destPath = m_tempDir / "newproject";
    fs::create_directories( destPath );

    PROJECT_TEMPLATE tmpl( wxString::FromUTF8( templatePath.string() ) );

    // CreateProject expects a wxFileName with the project file path (including extension)
    wxFileName newProjectPath;
    newProjectPath.SetPath( wxString::FromUTF8( destPath.string() ) );
    newProjectPath.SetName( wxS( "newproject" ) );
    newProjectPath.SetExt( wxS( "kicad_pro" ) );

    wxString errorMsg;
    bool     result = tmpl.CreateProject( newProjectPath, &errorMsg );

    BOOST_CHECK_MESSAGE( result, "CreateProject should succeed: " + errorMsg.ToStdString() );

    BOOST_CHECK( fs::exists( destPath / "newproject.kicad_pro" ) );
    BOOST_CHECK( fs::exists( destPath / "newproject-lib" ) );
    BOOST_CHECK( fs::exists( destPath / "newproject-lib" / "component.kicad_sym" ) );

    BOOST_CHECK_MESSAGE( !fs::exists( destPath / "testtemplate-lib" ),
                         "Old directory name should not exist" );
}


BOOST_AUTO_TEST_CASE( ExactMatchDirectoryRenamed )
{
    // Test that a directory exactly matching the template name is renamed
    CreateTemplateStructure( "mytemplate", { "mytemplate" },
                             { "mytemplate.kicad_pro", "mytemplate/subfile.txt" } );

    fs::path templatePath = m_tempDir / "mytemplate";
    fs::path destPath = m_tempDir / "finalproject";
    fs::create_directories( destPath );

    PROJECT_TEMPLATE tmpl( wxString::FromUTF8( templatePath.string() ) );

    // GetDestinationFiles expects a wxFileName with the project file path
    wxFileName newProjectPath;
    newProjectPath.SetPath( wxString::FromUTF8( destPath.string() ) );
    newProjectPath.SetName( wxS( "finalproject" ) );
    newProjectPath.SetExt( wxS( "kicad_pro" ) );

    std::vector<wxFileName> destFiles;
    tmpl.GetDestinationFiles( newProjectPath, destFiles );

    bool foundExactRenamedDir = false;

    for( const wxFileName& destFile : destFiles )
    {
        wxString fullPath = destFile.GetFullPath();

        if( fullPath.Contains( wxS( "/finalproject/finalproject/" ) )
            || fullPath.Contains( wxS( "\\finalproject\\finalproject\\" ) ) )
        {
            foundExactRenamedDir = true;
        }

        if( fullPath.Contains( wxS( "/finalproject/mytemplate/" ) )
            || fullPath.Contains( wxS( "\\finalproject\\mytemplate\\" ) ) )
        {
            BOOST_FAIL( "Exact match directory should be renamed from mytemplate to finalproject" );
        }
    }

    BOOST_CHECK_MESSAGE( foundExactRenamedDir, "Should find renamed subdirectory finalproject" );
}


BOOST_AUTO_TEST_SUITE_END()
