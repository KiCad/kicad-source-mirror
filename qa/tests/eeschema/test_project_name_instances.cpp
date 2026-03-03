/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file test_project_name_instances.cpp
 * Test that symbol and sheet instances are saved with the correct project name,
 * not an empty string. Regression test for the virtual root project name bug.
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <kiid.h>
#include <locale_io.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>


struct PROJECT_NAME_FIXTURE
{
    PROJECT_NAME_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "test_project.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~PROJECT_NAME_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    wxString GetTempSchFile( const wxString& aPrefix )
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString fileName = wxFileName::CreateTempFileName( tempDir + wxFileName::GetPathSeparator() + aPrefix );
        fileName += wxT( ".kicad_sch" );
        m_tempFiles.push_back( fileName );
        return fileName;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT*                   m_project;
    std::vector<wxString>      m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( ProjectNameInstances, PROJECT_NAME_FIXTURE )


/**
 * Test that symbol instances are saved with the current project name,
 * not an empty string, when using the virtual root pattern.
 */
BOOST_AUTO_TEST_CASE( SymbolInstanceProjectName )
{
    LOCALE_IO dummy;

    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* screen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Create a symbol
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetLibId( LIB_ID( wxT( "Device" ), wxT( "R" ) ) );
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    symbol->GetField( FIELD_T::REFERENCE )->SetText( wxT( "R1" ) );
    symbol->SetPrefix( wxT( "R" ) );
    screen->Append( symbol );

    // Build hierarchy and set the reference (this creates an instance
    // with empty m_ProjectName, simulating the bug condition)
    m_schematic->RefreshHierarchy();
    SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();
    BOOST_REQUIRE( !hierarchy.empty() );

    SCH_SHEET_PATH& path = hierarchy[0];
    symbol->SetRef( &path, wxT( "R1" ) );

    // Verify the instance exists but has empty project name (the bug)
    SCH_SYMBOL_INSTANCE inst;
    BOOST_REQUIRE( symbol->GetInstance( inst, path.Path() ) );
    BOOST_CHECK( inst.m_ProjectName.IsEmpty() );

    // Save the schematic
    wxString fileName = GetTempSchFile( wxT( "test_projname" ) );

    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( fileName, topSheets[0], m_schematic.get() );
    BOOST_REQUIRE( wxFileExists( fileName ) );

    BOOST_TEST_MESSAGE( "Saved file: " + fileName.ToStdString() );
    BOOST_TEST_MESSAGE( "Project name: " + m_project->GetProjectName().ToStdString() );

    // Read the saved file as text and check for project name
    wxTextFile textFile;
    BOOST_REQUIRE( textFile.Open( fileName ) );

    bool foundProjectClause = false;
    bool foundEmptyProject = false;

    for( size_t i = 0; i < textFile.GetLineCount(); i++ )
    {
        wxString line = textFile.GetLine( i );

        if( line.Contains( wxT( "(project \"test_project\"" ) ) )
            foundProjectClause = true;

        if( line.Contains( wxT( "(project \"\"" ) ) )
            foundEmptyProject = true;
    }

    textFile.Close();

    BOOST_CHECK_MESSAGE( foundProjectClause, "Saved file must contain (project \"test_project\")" );
    BOOST_CHECK_MESSAGE( !foundEmptyProject, "Saved file must NOT contain (project \"\")" );
}


BOOST_AUTO_TEST_SUITE_END()