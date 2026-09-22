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
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25606
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <sch_field.h>


BOOST_AUTO_TEST_SUITE( DerivedSymbolFootprint )


/**
 * The symbol chooser reported "No default footprint" for a derived symbol that had not
 * overridden the field, while the same symbol placed on the schematic took the parent's
 * footprint.  GetFootprint() read the symbol's own field, unlike every sibling accessor.
 */
BOOST_AUTO_TEST_CASE( InheritedFootprintResolution )
{
    LIB_SYMBOL base( wxS( "Base" ) );
    LIB_SYMBOL derived( wxS( "Derived" ) );

    base.GetFootprintField().SetText( wxS( "Package_DFN_QFN:DHWQFN-14" ) );
    derived.SetParent( &base );

    BOOST_CHECK( derived.IsDerived() );
    BOOST_CHECK( derived.GetFootprintField().GetText().IsEmpty() );
    BOOST_CHECK_EQUAL( derived.GetFootprint(), wxS( "Package_DFN_QFN:DHWQFN-14" ) );

    // An override on the derived symbol wins over the parent
    derived.GetFootprintField().SetText( wxS( "Package_SO:SOIC-14" ) );
    BOOST_CHECK_EQUAL( derived.GetFootprint(), wxS( "Package_SO:SOIC-14" ) );

    // A root symbol with no footprint still reports none
    LIB_SYMBOL orphan( wxS( "Orphan" ) );
    BOOST_CHECK( orphan.GetFootprint().IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
