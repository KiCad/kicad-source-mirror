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
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24606
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <sch_field.h>


BOOST_AUTO_TEST_SUITE( DerivedSymbolUserFields )


/**
 * The symbol properties dialog decides whether a grid row is an unchanged inherited field
 * by comparing the row (a copy of the parent field reparented to the edited symbol with a
 * reassigned ordinal) against the parent field.  operator== is owner-sensitive, so it never
 * matched and every inherited field was written into the derived symbol.
 */
BOOST_AUTO_TEST_CASE( UnmodifiedInheritedFieldDetection )
{
    LIB_SYMBOL base( wxS( "Base" ) );
    LIB_SYMBOL derived( wxS( "Derived" ) );

    derived.SetParent( &base );

    SCH_FIELD* parentField = new SCH_FIELD( &base, FIELD_T::USER, wxS( "Tolerance" ) );
    parentField->SetText( wxS( "1%" ) );
    base.AddField( parentField );

    SCH_FIELD row( *parentField );
    row.SetParent( &derived );
    row.SetOrdinal( 42 );

    BOOST_CHECK( !( row == *parentField ) );
    BOOST_CHECK( row.HasSameContent( *parentField ) );

    // Any user modification must break the match so the override is saved.
    SCH_FIELD changedText( row );
    changedText.SetText( wxS( "5%" ) );
    BOOST_CHECK( !changedText.HasSameContent( *parentField ) );

    SCH_FIELD changedVisibility( row );
    changedVisibility.SetVisible( !changedVisibility.IsVisible() );
    BOOST_CHECK( !changedVisibility.HasSameContent( *parentField ) );

    SCH_FIELD changedPosition( row );
    changedPosition.SetPosition( changedPosition.GetPosition() + VECTOR2I( 100, 100 ) );
    BOOST_CHECK( !changedPosition.HasSameContent( *parentField ) );

    SCH_FIELD changedNameShown( row );
    changedNameShown.SetNameShown( !changedNameShown.IsNameShown() );
    BOOST_CHECK( !changedNameShown.HasSameContent( *parentField ) );

    SCH_FIELD changedName( row );
    changedName.SetName( wxS( "Tol" ) );
    BOOST_CHECK( !changedName.HasSameContent( *parentField ) );
}


BOOST_AUTO_TEST_SUITE_END()
