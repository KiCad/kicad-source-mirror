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
 * @file test_issue24663_sheet_dnp_fields.cpp
 * Test for issue #24663: DNP set on a hierarchical sheet must read as effective DNP
 * in the symbol fields table, without stamping the attribute onto the child symbols.
 *
 * The fixture has a root sheet whose only subsheet is marked Do Not Populate. The one
 * symbol inside it (R1) has its own DNP attribute OFF. Setting DNP on a sheet is an
 * effective rollup computed at netlist, BOM and plot time, it is never written onto the
 * child symbols. The symbol fields table must still show that effective state, otherwise
 * the column contradicts the netlist, the BOM and the dialog's own "Exclude DNP" filter.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <fields_data_model.h>
#include <sch_reference_list.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>

#include <wx/filename.h>


struct ISSUE24663_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
};


BOOST_FIXTURE_TEST_CASE( SheetDNPEffectiveInSymbolFields, ISSUE24663_FIXTURE )
{
    wxFileName fn;
    fn.SetPath( KI_TEST::GetEeschemaTestDataDir() );
    fn.SetName( wxS( "test_issue24663_sheet_dnp" ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    LoadSchematic( fn.GetFullPath() );

    SCH_SHEET_LIST     sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    SCH_REFERENCE_LIST refs;
    sheets.GetSymbols( refs, SYMBOL_FILTER_ALL );

    BOOST_REQUIRE_EQUAL( refs.GetCount(), 1 );

    SCH_REFERENCE& r1 = refs[0];

    // Safety: setting DNP on the sheet must not stamp the attribute onto the symbol.
    BOOST_CHECK( !r1.GetSymbolDNP() );

    // The ancestor sheet forces DNP on, so the effective state is DNP.
    BOOST_CHECK( r1.GetSheetPath().GetDNP() );

    // The fix: the symbol fields data model reports that effective state. The same code
    // path serves the on screen checkbox and the BOM export. The exclude_from_board,
    // exclude_from_bom and exclude_from_sim attributes share it.
    FIELDS_EDITOR_GRID_DATA_MODEL model( refs, nullptr );
    model.AddColumn( wxS( "${DNP}" ), wxS( "DNP" ), false );

    int dnpCol = model.GetFieldNameCol( wxS( "${DNP}" ) );
    BOOST_REQUIRE( dnpCol >= 0 );

    // GetValue with resolveVars off is the path that feeds the on screen checkbox: a
    // value of "1" draws it checked. The stored attribute is still the symbol's own "0".
    DATA_MODEL_ROW group( r1, GROUP_SINGLETON );
    BOOST_CHECK_EQUAL( model.GetValue( group, dnpCol ), wxS( "1" ) );
}
