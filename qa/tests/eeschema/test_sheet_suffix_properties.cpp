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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <fields_grid_table.h>
#include <sch_sheet.h>


BOOST_AUTO_TEST_SUITE( SheetSuffixProperties )


BOOST_AUTO_TEST_CASE( MissingSuffixIsEditableWithoutChangingTheSheet )
{
    SCH_SHEET        sheet;
    const SCH_SHEET& original = sheet;
    const size_t     originalCount = original.GetFields().size();

    BOOST_REQUIRE( !original.GetField( FIELD_T::SHEET_REF_SUFFIX ) );

    std::vector<SCH_FIELD> fields = GetSheetFieldsForEditing( sheet );

    BOOST_REQUIRE_EQUAL( fields.size(), originalCount + 1 );
    BOOST_REQUIRE( fields[2].GetId() == FIELD_T::SHEET_REF_SUFFIX );
    BOOST_CHECK_EQUAL( fields[2].GetUntranslatedName(), wxS( "Reference Suffix" ) );
    BOOST_CHECK( fields[2].GetText().IsEmpty() );
    BOOST_CHECK( !fields[2].IsVisible() );

    fields[2].SetText( wxS( "_M1" ) );
    fields.clear();

    BOOST_CHECK_EQUAL( original.GetFields().size(), originalCount );
    BOOST_CHECK( !original.GetField( FIELD_T::SHEET_REF_SUFFIX ) );
}


BOOST_AUTO_TEST_CASE( ExistingSuffixRetainsItsValueAndAppearance )
{
    SCH_SHEET sheet;
    SCH_FIELD custom( &sheet, FIELD_T::SHEET_USER, wxS( "Description" ) );
    custom.SetOrdinal( 3, FIELD_T::SHEET_USER );
    sheet.AddField( custom );

    SCH_FIELD suffix( &sheet, FIELD_T::SHEET_REF_SUFFIX, wxS( "Reference Suffix" ) );
    suffix.SetOrdinal( 42, FIELD_T::SHEET_REF_SUFFIX );
    suffix.SetText( wxS( "_${CH}" ) );
    suffix.SetVisible( true );
    suffix.SetBold( true );
    suffix.SetPosition( VECTOR2I( 1500, 2000 ) );
    sheet.AddField( suffix );

    std::vector<SCH_FIELD> fields = GetSheetFieldsForEditing( sheet );

    BOOST_REQUIRE_EQUAL( fields.size(), sheet.GetFields().size() );
    BOOST_REQUIRE( fields[2].GetId() == FIELD_T::SHEET_REF_SUFFIX );
    BOOST_CHECK_EQUAL( fields[2].GetText(), wxS( "_${CH}" ) );
    BOOST_CHECK( fields[2].IsVisible() );
    BOOST_CHECK( fields[2].IsBold() );
    BOOST_CHECK( fields[2].GetPosition() == VECTOR2I( 1500, 2000 ) );
    BOOST_CHECK_EQUAL( fields[3].GetName(), wxS( "Description" ) );

    fields[2].SetText( wxS( "_M2" ) );

    const SCH_SHEET& original = sheet;

    BOOST_CHECK_EQUAL( original.GetField( FIELD_T::SHEET_REF_SUFFIX )->GetText(), wxS( "_${CH}" ) );
}


BOOST_AUTO_TEST_SUITE_END()
