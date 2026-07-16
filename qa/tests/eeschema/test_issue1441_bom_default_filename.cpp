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

// Regression tests for https://gitlab.com/kicad/code/kicad/-/issues/1441
//
// A blank BOM output-file path used to be a hard error. The exporter now defaults to
// <schematic>.csv beside the schematic, matching the other KiCad exporters. An unsaved
// schematic (no file name) still yields no destination so callers keep the error.

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialog_symbol_fields_table.h>


BOOST_AUTO_TEST_SUITE( Issue1441BomDefaultFileName )


BOOST_AUTO_TEST_CASE( SwapsExtensionToCsv )
{
    wxFileName expected( wxS( "/tmp/project/design.csv" ) );

    BOOST_CHECK_EQUAL( DIALOG_SYMBOL_FIELDS_TABLE::GetDefaultBomFileName( wxS( "/tmp/project/design.kicad_sch" ) ),
                       expected.GetFullPath() );
}


BOOST_AUTO_TEST_CASE( PreservesDirectory )
{
    // The default lands next to the schematic, not in the current working directory.
    wxFileName result( DIALOG_SYMBOL_FIELDS_TABLE::GetDefaultBomFileName( wxS( "/tmp/project/design.kicad_sch" ) ) );

    BOOST_CHECK_EQUAL( result.GetPath(), wxS( "/tmp/project" ) );
    BOOST_CHECK_EQUAL( result.GetExt(), wxS( "csv" ) );
}


BOOST_AUTO_TEST_CASE( EmptyForUnsavedSchematic )
{
    BOOST_CHECK( DIALOG_SYMBOL_FIELDS_TABLE::GetDefaultBomFileName( wxEmptyString ).IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
