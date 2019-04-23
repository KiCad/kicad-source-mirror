/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Test suite for LIB_PART
 */

#include <unit_test_utils/unit_test_utils.h>

// Code under test
#include <class_libentry.h>

#include "lib_field_test_utils.h"

class TEST_LIB_PART_FIXTURE
{
public:
    TEST_LIB_PART_FIXTURE() : m_part_no_data( "part_name", nullptr )
    {
    }

    ///> Part with no extra data set
    LIB_PART m_part_no_data;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( LibPart, TEST_LIB_PART_FIXTURE )


/**
 * Check that we can get the basic properties out as expected
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
    BOOST_CHECK_EQUAL( m_part_no_data.GetName(), "part_name" );

    // Didn't set a library, so this is empty
    BOOST_CHECK_EQUAL( m_part_no_data.GetLibraryName(), "" );
    BOOST_CHECK_EQUAL( m_part_no_data.GetLib(), nullptr );

    // only get the root
    BOOST_CHECK_EQUAL( m_part_no_data.GetAliasCount(), 1 );

    // no sub units
    BOOST_CHECK_EQUAL( m_part_no_data.GetUnitCount(), 1 );
    BOOST_CHECK_EQUAL( m_part_no_data.IsMulti(), false );

    // no conversion
    BOOST_CHECK_EQUAL( m_part_no_data.HasConversion(), false );
}


/**
 * Check the drawings on a "blank" LIB_PART
 */
BOOST_AUTO_TEST_CASE( DefaultDrawings )
{
    // default drawings exist
    BOOST_CHECK_EQUAL( m_part_no_data.GetDrawItems().size(), 4 );
}


/**
 * Check the default fields are present as expected
 */
BOOST_AUTO_TEST_CASE( DefaultFields )
{
    LIB_FIELDS fields;
    m_part_no_data.GetFields( fields );

    // Should get the 4 default fields
    BOOST_CHECK_PREDICATE( KI_TEST::AreDefaultFieldsCorrect, ( fields ) );

    // but no more (we didn't set them)
    BOOST_CHECK_EQUAL( fields.size(), NumFieldType::MANDATORY_FIELDS );

    // also check the default field accessors
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetReferenceField() )( "Reference" )( NumFieldType::REFERENCE ) );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetValueField() )( "Value" )( NumFieldType::VALUE ) );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetFootprintField() )( "Footprint" )( NumFieldType::FOOTPRINT ) );
}


/**
 * Test adding fields to a LIB_PART
 */
BOOST_AUTO_TEST_CASE( AddedFields )
{
    LIB_FIELDS fields;
    m_part_no_data.GetFields( fields );

    // Ctor takes non-const ref (?!)
    const std::string newFieldName = "new_field";
    wxString          nonConstNewFieldName = newFieldName;
    fields.push_back( LIB_FIELD( 42, nonConstNewFieldName ) );

    // fairly roundabout way to add a field, but it is what it is
    m_part_no_data.SetFields( fields );

    // Should get the 4 default fields
    BOOST_CHECK_PREDICATE( KI_TEST::AreDefaultFieldsCorrect, ( fields ) );

    // and our new one
    BOOST_REQUIRE_EQUAL( fields.size(), NumFieldType::MANDATORY_FIELDS + 1 );

    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( fields[NumFieldType::MANDATORY_FIELDS] )( newFieldName )( 42 ) );

    // Check by-id lookup

    LIB_FIELD* gotNewField = m_part_no_data.GetField( 42 );

    BOOST_REQUIRE_NE( gotNewField, nullptr );

    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches, ( *gotNewField )( newFieldName )( 42 ) );

    // Check by-name lookup

    gotNewField = m_part_no_data.FindField( newFieldName );

    BOOST_REQUIRE_NE( gotNewField, nullptr );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches, ( *gotNewField )( newFieldName )( 42 ) );
}


struct TEST_LIB_PART_SUBREF_CASE
{
    int         m_index;
    bool        m_addSep;
    std::string m_expSubRef;
};


/**
 * Test the subreference indexing
 */
BOOST_AUTO_TEST_CASE( SubReference )
{
    const std::vector<TEST_LIB_PART_SUBREF_CASE> cases = {
        {
            1,
            false,
            "A",
        },
        {
            2,
            false,
            "B",
        },
        {
            26,
            false,
            "Z",
        },
        {
            27,
            false,
            "AA",
        },
        { // haven't configured a separator, so should be nothing
            1,
            true,
            "A",
        },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT(
                "Subref: " << c.m_index << ", " << c.m_addSep << " -> '" << c.m_expSubRef << "'" )
        {
            const auto subref = m_part_no_data.SubReference( c.m_index, c.m_addSep );
            BOOST_CHECK_EQUAL( subref, c.m_expSubRef );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()