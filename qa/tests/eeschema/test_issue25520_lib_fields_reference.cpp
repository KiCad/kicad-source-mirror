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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue25520_lib_fields_reference.cpp
 * Test for issue #25520: the symbol editor's bulk fields table accepted an edit of the
 * Reference field but silently dropped it on apply.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/brush.h>
#include <wx/settings.h>

#include <lib_fields_data_model.h>
#include <lib_symbol.h>
#include <sch_field.h>
#include <template_fieldnames.h>


namespace
{

int FindRow( LIB_FIELDS_EDITOR_GRID_DATA_MODEL& aModel, const wxString& aSymbolName )
{
    for( int row = 0; row < aModel.GetNumberRows(); ++row )
    {
        if( aModel.GetSymbolForRow( row )->GetName() == aSymbolName )
            return row;
    }

    return -1;
}

} // namespace


BOOST_AUTO_TEST_SUITE( LibFieldsTableReference )


BOOST_AUTO_TEST_CASE( ReferenceEditApplies )
{
    LIB_SYMBOL symbol( wxS( "Connector" ) );
    symbol.GetReferenceField().SetText( wxS( "X" ) );

    LIB_FIELDS_EDITOR_GRID_DATA_MODEL model;
    model.SetSymbols( { &symbol } );
    model.AddColumn( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, wxS( "Symbol Name" ), false, false );
    model.AddColumn( GetCanonicalFieldName( FIELD_T::REFERENCE ), wxS( "Reference" ), false, false );
    model.RebuildRows();

    int row = FindRow( model, wxS( "Connector" ) );
    int col = model.GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );

    BOOST_REQUIRE( row != -1 );
    BOOST_REQUIRE( col != -1 );

    model.SetValue( row, col, wxS( "J" ) );
    model.ApplyData(
            []( LIB_SYMBOL* )
            {
            },
            nullptr );

    BOOST_CHECK_EQUAL( symbol.GetReferenceField().GetText(), wxS( "J" ) );
}


BOOST_AUTO_TEST_CASE( InvalidReferenceNotApplied )
{
    LIB_SYMBOL symbol( wxS( "Connector" ) );
    symbol.GetReferenceField().SetText( wxS( "X" ) );

    LIB_FIELDS_EDITOR_GRID_DATA_MODEL model;
    model.SetSymbols( { &symbol } );
    model.AddColumn( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, wxS( "Symbol Name" ), false, false );
    model.AddColumn( GetCanonicalFieldName( FIELD_T::REFERENCE ), wxS( "Reference" ), false, false );
    model.RebuildRows();

    int row = FindRow( model, wxS( "Connector" ) );
    int col = model.GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );

    BOOST_REQUIRE( row != -1 );
    BOOST_REQUIRE( col != -1 );

    // A reference designator cannot contain a space.
    model.SetValue( row, col, wxS( "J 1" ) );
    model.ApplyData(
            []( LIB_SYMBOL* )
            {
            },
            nullptr );

    BOOST_CHECK_EQUAL( symbol.GetReferenceField().GetText(), wxS( "X" ) );
}


BOOST_AUTO_TEST_CASE( EmptyReferenceInheritsOnlyOnDerived )
{
    LIB_SYMBOL root( wxS( "Root" ) );
    root.GetReferenceField().SetText( wxS( "X" ) );

    LIB_SYMBOL derived( wxS( "Derived" ) );
    derived.SetParent( &root );
    derived.GetReferenceField().SetText( wxS( "X" ) );

    LIB_FIELDS_EDITOR_GRID_DATA_MODEL model;
    model.SetSymbols( { &root, &derived } );
    model.AddColumn( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, wxS( "Symbol Name" ), false, false );
    model.AddColumn( GetCanonicalFieldName( FIELD_T::REFERENCE ), wxS( "Reference" ), false, false );
    model.RebuildRows();

    int rootRow = FindRow( model, wxS( "Root" ) );
    int derivedRow = FindRow( model, wxS( "Derived" ) );
    int col = model.GetFieldNameCol( GetCanonicalFieldName( FIELD_T::REFERENCE ) );

    BOOST_REQUIRE( rootRow != -1 );
    BOOST_REQUIRE( derivedRow != -1 );
    BOOST_REQUIRE( col != -1 );

    model.SetValue( rootRow, col, wxEmptyString );
    model.SetValue( derivedRow, col, wxEmptyString );
    model.ApplyData(
            []( LIB_SYMBOL* )
            {
            },
            nullptr );

    BOOST_CHECK_EQUAL( root.GetReferenceField().GetText(), wxS( "X" ) );
    BOOST_CHECK_EQUAL( derived.GetReferenceField().GetText(), wxString() );
}


BOOST_AUTO_TEST_SUITE_END()
