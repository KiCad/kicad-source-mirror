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
 * Test suite for #SCH_SHEET_PATH and #SCH_SHEET_LIST
 */

#include <qa_utils/uuid_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

// Code under test
#include <sch_sheet_path.h>

#include <wildcards_and_files_ext.h>
#include <eeschema_helpers.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>

#include <sstream>

class TEST_SCH_SHEET_PATH_FIXTURE
{
public:
    TEST_SCH_SHEET_PATH_FIXTURE() : m_schematic( nullptr )
    {
        for( unsigned i = 0; i < 4; ++i )
        {
            m_sheets.emplace_back( nullptr, VECTOR2I( i, i ) );

            std::ostringstream ss;
            ss << "Sheet" << i;
            m_sheets[i].GetField( FIELD_T::SHEET_NAME )->SetText( ss.str() );
            m_sheets[i].SetParent( &m_schematic );
        }

        // 0->1->2
        m_linear.push_back( &m_sheets[0] );
        m_linear.push_back( &m_sheets[1] );
        m_linear.push_back( &m_sheets[2] );
    }

    SCHEMATIC      m_schematic;
    SCH_SHEET_PATH m_empty_path;

    /**
     * We look at sheet 2 in the hierarchy:
     * Sheets: 0 -> 1 -> 2
     */
    SCH_SHEET_PATH m_linear;

    /// handy store of SCH_SHEET objects
    std::vector<SCH_SHEET> m_sheets;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchSheetPath, TEST_SCH_SHEET_PATH_FIXTURE )


/**
 * Check properties of an empty SCH_SHEET_PATH
 */
BOOST_AUTO_TEST_CASE( Empty )
{
    BOOST_CHECK_EQUAL( m_empty_path.size(), 0 );

    BOOST_CHECK_THROW( m_empty_path.at( 0 ), std::out_of_range );

    // Sheet paths with no SCH_SCHEET object are illegal.
    // CHECK_WX_ASSERT( m_empty_path.GetPageNumber() );

    // These accessors return nullptr when empty (i.e. they don't crash)
    BOOST_CHECK_EQUAL( m_empty_path.Last(), nullptr );
    BOOST_CHECK_EQUAL( m_empty_path.LastScreen(), nullptr );

    BOOST_CHECK_EQUAL( m_empty_path.PathAsString(), "/" );
    BOOST_CHECK_EQUAL( m_empty_path.PathHumanReadable(), "/" );
}


/**
 * Check properties of a non-empty SCH_SHEET_PATH
 */
BOOST_AUTO_TEST_CASE( NonEmpty )
{
    BOOST_CHECK_EQUAL( m_linear.size(), 3 );

    BOOST_CHECK_EQUAL( m_linear.at( 0 ), &m_sheets[0] );
    BOOST_CHECK_EQUAL( m_linear.at( 1 ), &m_sheets[1] );
    BOOST_CHECK_EQUAL( m_linear.at( 2 ), &m_sheets[2] );

    BOOST_CHECK_EQUAL( m_linear.Last(), &m_sheets[2] );
    BOOST_CHECK_EQUAL( m_linear.LastScreen(), nullptr );

    // don't know what the uuids will be, but we know the format: /<8-4-4-4-12>/<8-4-4-4-12>/
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsUUIDPathWithLevels, ( m_linear.PathAsString().ToStdString() )( 2 ) );

    // Sheet0 is the root sheet and isn't in the path
    BOOST_CHECK_EQUAL( m_linear.PathHumanReadable(), "/Sheet1/Sheet2/" );
}


BOOST_AUTO_TEST_CASE( Compare )
{
    SCH_SHEET_PATH otherEmpty;

    BOOST_CHECK( m_empty_path == otherEmpty );

    BOOST_CHECK( m_empty_path != m_linear );
}


BOOST_AUTO_TEST_CASE( SheetListGetOrdinalPath )
{
    // The "complex_hierarchy" test project has a root sheet with two sheets that reference the
    // same file.
    std::unique_ptr<SCHEMATIC> schematic;
    wxFileName fn( wxString::Format( wxS( "%snetlists/complex_hierarchy" ),
                                     KI_TEST::GetEeschemaTestDataDir() ),
                   wxS( "complex_hierarchy" ), FILEEXT::ProjectFileExtension );

    schematic.reset( EESCHEMA_HELPERS::LoadSchematic( fn.GetFullPath(), false, false, nullptr ) );

    SCH_SHEET_LIST hierarchy = schematic->Hierarchy();
    BOOST_CHECK_EQUAL( hierarchy.size(), 3 );

    // A null pointer should always result in an empty return value.
    BOOST_CHECK( !hierarchy.GetOrdinalPath( nullptr ) );

    // The root sheet is a single instance.  It's always ordinal.
    BOOST_CHECK( hierarchy.GetOrdinalPath( schematic->RootScreen() ).value() == hierarchy.at( 0 ) );

    // The shared schematic with the lowest page number is the ordinal sheet path.
    SCH_SHEET* sheet = hierarchy.at( 1 ).Last();
    BOOST_CHECK( hierarchy.GetOrdinalPath( sheet->GetScreen() ).value() == hierarchy.at( 1 ) );

    // The shared sheet with a higher page number is not the ordinal sheet path.
    sheet = hierarchy.at( 2 ).Last();
    BOOST_CHECK( hierarchy.GetOrdinalPath( sheet->GetScreen() ).value() == hierarchy.at( 1 ) );
}


/**
 * Test sheet path page number properties.
 */
BOOST_AUTO_TEST_CASE( SheetPathPageProperties )
{
    // BOOST_CHECK_EQUAL( m_linear.GetPageNumber(), wxEmptyString );

    // Add new instance to sheet object.
    // BOOST_CHECK( m_linear.Last()->AddInstance( m_linear.Path() ) );
    // m_linear.SetPageNumber( "1" );
    // BOOST_CHECK_EQUAL( m_linear.GetPageNumber(), "1" );
    // m_linear.SetPageNumber( "i" );
    // BOOST_CHECK_EQUAL( m_linear.GetPageNumber(), "i" );
}


BOOST_AUTO_TEST_SUITE_END()
