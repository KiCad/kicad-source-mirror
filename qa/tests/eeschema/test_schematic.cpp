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
 * @file ./qa/tests/eeschema/test_schematic.cpp
 * @brief This file contains unit tests for the #SCHEMATIC object.
 */
#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <schematic.h>
#include <wildcards_and_files_ext.h>


class TEST_SCHEMATIC_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    wxFileName SchematicQAPath( const wxString& aRelativePath ) override;
};


wxFileName TEST_SCHEMATIC_FIXTURE::SchematicQAPath( const wxString& aRelativePath )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );

    wxString path = fn.GetFullPath();
    path += aRelativePath + wxT( "." ) + FILEEXT::KiCadSchematicFileExtension;

    return wxFileName( path );
}


BOOST_FIXTURE_TEST_SUITE( Schematic, TEST_SCHEMATIC_FIXTURE )


BOOST_AUTO_TEST_CASE( TestSchematicSharedByMultipleProjects )
{
    LoadSchematic( SchematicQAPath( "schematic_object_tests/not_shared_by_multiple_projects/"
                                     "not_shared_by_multiple_projects" ) );

    std::set<const SCH_SCREEN*> sharedScreens = m_schematic->GetSchematicsSharedByMultipleProjects();

    BOOST_CHECK( sharedScreens.empty() );

    LoadSchematic( SchematicQAPath( "schematic_object_tests/shared_by_multiple_projects/"
                                     "project_a/project_a" ) );

    sharedScreens = m_schematic->GetSchematicsSharedByMultipleProjects();

    BOOST_CHECK( !sharedScreens.empty() );
}


BOOST_AUTO_TEST_CASE( TestSchematicIsComplexHierarchy )
{
    LoadSchematic( SchematicQAPath( "netlists/group_bus_matching/group_bus_matching" ) );

    BOOST_CHECK( !m_schematic->IsComplexHierarchy() );

    LoadSchematic( SchematicQAPath( "netlists/complex_hierarchy/complex_hierarchy" ) );

    BOOST_CHECK( m_schematic->IsComplexHierarchy() );
}


BOOST_AUTO_TEST_SUITE_END()
