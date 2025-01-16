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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Test suite for SCH_SCREEN
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <schematic.h>
#include <wildcards_and_files_ext.h>

// Code under test
#include <sch_screen.h>

#include <qa_utils/uuid_test_utils.h>
#include <qa_utils/wx_utils/wx_assert.h>

class TEST_SCH_SCREEN_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    wxFileName SchematicQAPath( const wxString& aRelativePath ) override;
};


wxFileName TEST_SCH_SCREEN_FIXTURE::SchematicQAPath( const wxString& aRelativePath )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );

    wxString path = fn.GetFullPath();
    path += aRelativePath + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension;

    return wxFileName( path );
}


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchScreen, TEST_SCH_SCREEN_FIXTURE )


/**
 * Test SCH_SCREEN::InProjectPath().
 */
BOOST_AUTO_TEST_CASE( TestInProjectPath )
{
    LoadSchematic( SchematicQAPath( "schematic_object_tests/not_shared_by_multiple_projects/"
                                    "not_shared_by_multiple_projects" ) );

    SCH_SCREEN testScreen( m_schematic.get() );
    wxFileName testFn( m_schematic->RootScreen()->GetFileName() );

    // File is in same folder as project.
    testFn.SetName( "test" );
    testScreen.SetFileName( testFn.GetFullPath() );
    BOOST_CHECK( testScreen.InProjectPath() );

    // File is in a sub-folder inside project.
    testFn.AppendDir( "sch" );
    testScreen.SetFileName( testFn.GetFullPath() );
    BOOST_CHECK( testScreen.InProjectPath() );

    // File is one folder below poject folder.
    testFn.RemoveLastDir();
    testFn.RemoveLastDir();
    testScreen.SetFileName( testFn.GetFullPath() );
    BOOST_CHECK( !testScreen.InProjectPath() );

    // File is in a completely different path with the same folder depth.
    testFn.SetPath( "/home/foo/kicad" );

    wxFileName projectFn( m_schematic->Project().GetProjectFullName() );

    // Just in case someone has a build path with no subfolders.
    BOOST_CHECK( testFn.GetDirCount() < projectFn.GetDirCount() );

    int subDirCount = 1;

    while( projectFn.GetDirCount() != testFn.GetDirCount() )
    {
        testFn.AppendDir( wxString::Format( wxS( "subdir%d" ), subDirCount ) );
        subDirCount += 1;
    }

    testScreen.SetFileName( testFn.GetFullPath() );
    BOOST_CHECK( !testScreen.InProjectPath() );
}


/**
 * Test SCH_SCREEN::HasInstanceDataFromOtherProjects().
 */
BOOST_AUTO_TEST_CASE( TestSharedByMultipleProjects )
{
    LoadSchematic( SchematicQAPath( "schematic_object_tests/not_shared_by_multiple_projects/"
                                    "not_shared_by_multiple_projects" ) );

    const SCH_SCREEN* rootScreen = m_schematic->RootScreen();
    BOOST_CHECK( !rootScreen->HasInstanceDataFromOtherProjects() );
    BOOST_CHECK( rootScreen->InProjectPath() );
}


BOOST_AUTO_TEST_SUITE_END()
