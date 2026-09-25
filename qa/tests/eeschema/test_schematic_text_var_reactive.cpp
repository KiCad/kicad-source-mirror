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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <algorithm>
#include <memory>

#include <eeschema_helpers.h>
#include <eeschema_test_utils.h>
#include <locale_io.h>
#include <schematic.h>
#include <schematic_text_var_adapter.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <sch_field.h>
#include <sch_sheet_pin.h>
#include <text_var_dependency.h>

#include "sch_table.h"


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
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "U1:Value" ) ) ), 1u );
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
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "SHEETNAME" ) ) ), 1u );
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

    SCH_SHEET sheet;
    SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( &sheet );
    sheet.AddPin( pin );
    pin->SetText( wxT( "${OLD}" ) );

    SCH_TABLE      table;
    SCH_TABLECELL* cell = new SCH_TABLECELL();
    table.AddCell( cell );
    cell->SetText( wxT( "${OLD}" ) );

    std::vector<SCH_ITEM*> items{ &text, &sheet, &table };
    std::vector<SCH_ITEM*> just_text{ &text };
    std::vector<SCH_ITEM*> just_sheet{ &sheet };
    std::vector<SCH_ITEM*> just_table{ &table };
    sch.GetTextVarAdapter()->OnSchItemsAdded( sch, items );

    const TEXT_VAR_DEPENDENCY_INDEX& index = sch.GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "OLD" ) ) ), 3u );

    // Edit the text and fire a change notification — the adapter must drop
    // the old edge and register the new one.
    text.SetText( wxT( "${NEW}" ) );
    sch.GetTextVarAdapter()->OnSchItemsChanged( sch, just_text );

    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "OLD" ) ) ), 2u );
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "NEW" ) ) ), 1u );

    // Edit the pin and fire a change notification — the adapter must drop
    // the old edge and register the new one.
    pin->SetText( wxT( "${NEW}" ) );
    sch.GetTextVarAdapter()->OnSchItemsChanged( sch, just_sheet );

    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "OLD" ) ) ), 1u );
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "NEW" ) ) ), 2u );

    // Edit the cell text and fire a change notification — the adapter must drop
    // the old edge and register the new one.
    cell->SetText( wxT( "${NEW}" ) );
    sch.GetTextVarAdapter()->OnSchItemsChanged( sch, just_table );

    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "OLD" ) ) ), 0u );
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "NEW" ) ) ), 3u );
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


BOOST_AUTO_TEST_CASE( LoadedSchematicIsIndexed )
{
    LOCALE_IO dummy;

    wxString path = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                    + wxS( "NoConnectOnLineWithGlobalLabel.kicad_sch" );
    std::unique_ptr<SCHEMATIC> sch( EESCHEMA_HELPERS::LoadSchematic( path, true, true ) );
    BOOST_REQUIRE( sch );

    // Both global labels carry ${INTERSHEET_REFS} in their intersheet references field
    const TEXT_VAR_DEPENDENCY_INDEX& index = sch->GetTextVarAdapter()->Tracker().Index();
    BOOST_CHECK_EQUAL( index.DependentCount( TEXT_VAR_REF_KEY::FromToken( wxT( "INTERSHEET_REFS" ) ) ), 2u );
}


BOOST_AUTO_TEST_CASE( ReplacedSheetScreenIsUnindexed )
{
    LOCALE_IO dummy;

    wxString path = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue13212.kicad_sch" );
    std::unique_ptr<SCHEMATIC> sch( EESCHEMA_HELPERS::LoadSchematic( path, true, true ) );
    BOOST_REQUIRE( sch );

    const TEXT_VAR_REF_KEY           key = TEXT_VAR_REF_KEY::FromToken( wxT( "INTERSHEET_REFS" ) );
    const TEXT_VAR_DEPENDENCY_INDEX& index = sch->GetTextVarAdapter()->Tracker().Index();

    // Root holds 3 labels, subsheet 1 holds 2, and the shared subsheet 2 holds 1
    BOOST_REQUIRE_EQUAL( index.DependentCount( key ), 6u );

    SCH_SHEET* subsheet = nullptr;

    for( SCH_ITEM* item : sch->RootScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        if( sheet->GetFileName().EndsWith( wxS( "issue13212_subsheet_1.kicad_sch" ) ) )
            subsheet = sheet;
    }

    BOOST_REQUIRE( subsheet );

    // Same sequence as pointing a sheet at another file outside undo; the old screen is freed
    subsheet->SetScreen( new SCH_SCREEN( sch.get() ) );

    std::vector<SCH_ITEM*> items{ subsheet };
    sch->OnItemsRemoved( items );
    sch->OnItemsAdded( items );
    sch->RefreshHierarchy();

    BOOST_CHECK_EQUAL( index.DependentCount( key ), 4u );
}


BOOST_AUTO_TEST_SUITE_END()
