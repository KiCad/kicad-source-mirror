/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <sch_sheet_path.h>
#include <wildcards_and_files_ext.h>

class TEST_SCH_SHEET_LIST_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    wxFileName getSchematicFile( const wxString& aRelativePath ) override;
};


wxFileName TEST_SCH_SHEET_LIST_FIXTURE::getSchematicFile( const wxString& aRelativePath )
{
    wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
    fn.AppendDir( "netlists" );

    wxString path = fn.GetFullPath();
    path += aRelativePath + wxT( "." ) + KiCadSchematicFileExtension;

    return wxFileName( path );
}


BOOST_FIXTURE_TEST_SUITE( SchSheetList, TEST_SCH_SHEET_LIST_FIXTURE )


BOOST_AUTO_TEST_CASE( TestSheetListPageProperties )
{
    loadSchematic( "complex_hierarchy/complex_hierarchy" );

    SCH_SHEET_LIST sheets = m_schematic.GetSheets();

    BOOST_CHECK( sheets.AllSheetPageNumbersEmpty() );

    sheets.SetInitialPageNumbers();

    // The root sheet should now be page 1.
    BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "1" );
    BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "2" );
    BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "3" );
}


BOOST_AUTO_TEST_CASE( TestEditPageNumbersInSharedDesign )
{
    BOOST_TEST_CONTEXT( "Read Sub-Sheet, prior to modification" )
    {
        // Check the Sub Sheet has the expected page numbers
        loadSchematic( "complex_hierarchy_shared/ampli_ht/ampli_ht" );

        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 2 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "i" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "ii" );
    }

    BOOST_TEST_CONTEXT( "Read Root Sheet, prior to modification" )
    {
        // Check the parent sheet has the expected page numbers
        loadSchematic( "complex_hierarchy_shared/complex_hierarchy" );

        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 5 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "1" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "2" );
        BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "3" );
        BOOST_CHECK_EQUAL( sheets.at( 3 ).GetPageNumber(), "4" );
        BOOST_CHECK_EQUAL( sheets.at( 4 ).GetPageNumber(), "5" );
    }

    BOOST_TEST_CONTEXT( "Modify page numbers in root sheet" )
    {
        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        // Amend Page numbers
        sheets.at( 0 ).SetPageNumber( "A" );
        sheets.at( 1 ).SetPageNumber( "B" );
        sheets.at( 2 ).SetPageNumber( "C" );
        sheets.at( 3 ).SetPageNumber( "D" );
        sheets.at( 4 ).SetPageNumber( "E" );

        // Save and reload
        wxString   tempName = "complex_hierarchy_shared/complex_hierarchy_modified";
        wxFileName tempFn = getSchematicFile( tempName );
        m_pi->Save( tempFn.GetFullPath(), &m_schematic.Root(), &m_schematic );
        loadSchematic( tempName );

        sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 5 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "A" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "B" );
        BOOST_CHECK_EQUAL( sheets.at( 2 ).GetPageNumber(), "C" );
        BOOST_CHECK_EQUAL( sheets.at( 3 ).GetPageNumber(), "D" );
        BOOST_CHECK_EQUAL( sheets.at( 4 ).GetPageNumber(), "E" );

        // Cleanup
        wxRemoveFile( tempFn.GetFullPath() );
    }

    BOOST_TEST_CONTEXT( "Read Sub-Sheet, after modification" )
    {
        // Check the Sub Sheet has the expected page numbers
        // (This should not have been modified after editing the root sheet)
        loadSchematic( "complex_hierarchy_shared/ampli_ht/ampli_ht" );

        SCH_SHEET_LIST sheets = m_schematic.GetSheets();

        BOOST_CHECK_EQUAL( sheets.size(), 2 );
        BOOST_CHECK_EQUAL( sheets.at( 0 ).GetPageNumber(), "i" );
        BOOST_CHECK_EQUAL( sheets.at( 1 ).GetPageNumber(), "ii" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
