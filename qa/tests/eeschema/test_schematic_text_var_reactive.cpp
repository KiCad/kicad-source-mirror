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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <algorithm>
#include <memory>

#include <eeschema_test_utils.h>
#include <schematic.h>
#include <schematic_text_var_adapter.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <sch_field.h>
#include <text_var_dependency.h>


/**
 * SCHEMATIC_TEXT_VAR_ADAPTER exercised at the listener interface. Drives the
 * adapter via direct OnSchItems* calls — this bypasses SCH_COMMIT (which
 * requires TOOL_MANAGER scaffolding) and keeps the tests focused on the
 * adapter's own logic. The commit-driven path is exercised by the
 * pcbnew/BOARD equivalent, which uses the same TEXT_VAR_TRACKER underneath.
 */
BOOST_AUTO_TEST_SUITE( SchematicTextVarReactive )


BOOST_AUTO_TEST_CASE( AdapterInstalledOnSchematicConstruction )
{
    SCHEMATIC sch( nullptr );
    BOOST_REQUIRE( sch.GetTextVarAdapter() );
    BOOST_CHECK_EQUAL( sch.GetTextVarAdapter()->Tracker().Index().ItemCount(), 0u );
}


BOOST_AUTO_TEST_CASE( SchTextItemRegistersOnAdded )
{
    SCHEMATIC sch( nullptr );
    SCH_TEXT  text;
    text.SetText( wxT( "${U1:Value}" ) );

    std::vector<SCH_ITEM*> added{ &text };
    sch.GetTextVarAdapter()->OnSchItemsAdded( sch, added );

    const TEXT_VAR_DEPENDENCY_INDEX& index = sch.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL(
            index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "U1:Value" ) ) ), 1u );
}


BOOST_AUTO_TEST_CASE( SchSymbolFieldsRegisterAsDependents )
{
    SCHEMATIC sch( nullptr );
    SCH_SYMBOL sym;

    // The symbol's own VALUE field has a text-var reference.
    sym.GetField( FIELD_T::VALUE )->SetText( wxT( "${SHEETNAME}" ) );

    std::vector<SCH_ITEM*> added{ &sym };
    sch.GetTextVarAdapter()->OnSchItemsAdded( sch, added );

    const TEXT_VAR_DEPENDENCY_INDEX& index = sch.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL(
            index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "SHEETNAME" ) ) ), 1u );
}


BOOST_AUTO_TEST_CASE( RemovedItemsAreUnregistered )
{
    SCHEMATIC sch( nullptr );
    SCH_TEXT  text;
    text.SetText( wxT( "${X}" ) );

    std::vector<SCH_ITEM*> items{ &text };
    sch.GetTextVarAdapter()->OnSchItemsAdded( sch, items );

    const TEXT_VAR_DEPENDENCY_INDEX& index = sch.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) ), 1u );

    sch.GetTextVarAdapter()->OnSchItemsRemoved( sch, items );
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "X" ) ) ), 0u );
}


BOOST_AUTO_TEST_CASE( RetextReRegisters )
{
    SCHEMATIC sch( nullptr );
    SCH_TEXT  text;
    text.SetText( wxT( "${OLD}" ) );

    std::vector<SCH_ITEM*> items{ &text };
    sch.GetTextVarAdapter()->OnSchItemsAdded( sch, items );

    const TEXT_VAR_DEPENDENCY_INDEX& index = sch.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "OLD" ) ) ), 1u );

    // Edit the text and fire a change notification — the adapter must drop
    // the old edge and register the new one.
    text.SetText( wxT( "${NEW}" ) );
    sch.GetTextVarAdapter()->OnSchItemsChanged( sch, items );

    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "OLD" ) ) ), 0u );
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "NEW" ) ) ), 1u );
}


BOOST_AUTO_TEST_CASE( SpiceOPTokensAreNotRegistered )
{
    SCHEMATIC sch( nullptr );
    SCH_TEXT  text;
    text.SetText( wxT( "${OP:port}" ) );

    std::vector<SCH_ITEM*> items{ &text };
    sch.GetTextVarAdapter()->OnSchItemsAdded( sch, items );

    BOOST_CHECK_EQUAL( sch.GetTextVarAdapter()->Tracker().Index().ItemCount(), 0u );
}


BOOST_AUTO_TEST_SUITE_END()
