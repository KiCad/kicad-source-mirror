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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <widgets/panel_symbol_chooser.h>


BOOST_AUTO_TEST_SUITE( MdwWorkspaceRowItem )


// A part without a description shows its name rather than only its part number
BOOST_AUTO_TEST_CASE( PartDescriptionFallsBackToName )
{
    WORKSPACE_PARTS_SEARCH::ROW row;
    row.kind = WORKSPACE_PARTS_SEARCH::ROW_KIND::PART;
    row.part.part_number = wxS( "R-0207-1K5" );
    row.part.name = wxS( "RES 1.5K 1/4W 1% AXIAL" );

    WORKSPACE_ROW_ITEM item( row );

    BOOST_CHECK_EQUAL( item.GetDesc(), wxS( "RES 1.5K 1/4W 1% AXIAL" ) );

    row.part.description = wxS( "Axial resistor" );

    BOOST_CHECK_EQUAL( item.GetDesc(), wxS( "Axial resistor" ) );
}


BOOST_AUTO_TEST_SUITE_END()
